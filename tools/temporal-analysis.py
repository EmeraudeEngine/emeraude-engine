#!/usr/bin/env python3
"""
Emeraude Engine - Temporal capture analysis: WHERE does a still image move?

Reads a temporal capture written by `Core.RendererService.temporalCapture(N)` -- N CONSECUTIVE
presented frames `<stem>-<n>.png` plus `<stem>.json` -- and turns "the ground shimmers in the
distance" into numbers and a map. It is the protocol of projet-alpha's
`docs/temporal-stability-measurement.md`, applied to consecutive frames instead of screenshots
taken a second apart:

  1. flatness first: the per-frame mean luma must be FLAT, or a feedback loop (auto-exposure,
     autofocus, accumulation) is still moving the whole image and every other number is polluted;
  2. the per-pixel temporal PEAK-TO-PEAK of the luma (max - min over the frames): mean, p99, p99.9,
     max and the share of pixels above 1/2/4/8/16 (8-bit units) -- the tails are where a visible
     artefact lives, never the mean;
  3. where: horizontal bands (a ground seen at grazing angle puts DISTANCE on the rows) and the
     worst tiles, each with its box;
  4. what: the signature of the residual against the mean image -- correlation with the gradient
     magnitude (a sub-pixel DISPLACEMENT) and with the Laplacian (a varying BLUR), plus the standardised
     two-term regression that separates them (they are themselves correlated);
  5. the maps: `<stem>-ptp.png` (peak-to-peak x16, grey) and `<stem>-heat.png` (the worst areas in
     red over the darkened mean image).

⚠️ Park the camera and let the scene converge before capturing. The JSON says whether the camera
moved during the capture (view matrices) and whether the exposure is automatic: the tool warns on both.
⚠️ The TAA jitter is a Halton (2,3) cycle of 8 frames: capture at least 8 frames to see a whole
cycle (the command's default of 5 does not).
⚠️ These are 8-bit PNG: a mean around 0.02-0.03 is the quantisation floor, it ranks nothing.

Usage:
    # In the running engine (remote console):
    #   Core.RendererService.temporalCapture(8)
    python3 temporal-analysis.py ~/.local/share/LNIsle/projet-alpha/captures/1790124178.json
    python3 temporal-analysis.py 1790124178 --crop 0,300,2880,300 --bands 12 --tiles 64
    # A/B: the same crop of two captures, side by side in the report
    python3 temporal-analysis.py A.json --compare B.json --crop 0,300,2880,300

Requires numpy and Pillow.
"""

import argparse
import json
import os
import sys

import numpy as np
from PIL import Image

DEFAULT_CAPTURES = os.path.expanduser("~/.local/share/LNIsle/projet-alpha/captures")
THRESHOLDS = (1, 2, 4, 8, 16)


def resolve_capture(argument):
	"""Returns (json path, capture directory, stem) from a JSON path, a stem, or a PNG of the series."""
	if os.path.isfile(argument) and argument.endswith(".json"):
		path = argument
	elif os.path.isfile(argument) and argument.endswith(".png"):
		stem = os.path.basename(argument).split("-")[0]
		path = os.path.join(os.path.dirname(argument), stem + ".json")
	else:
		path = os.path.join(DEFAULT_CAPTURES, argument + ".json")

	if not os.path.isfile(path):
		sys.exit(f"No temporal capture metadata at {path}")

	return path, os.path.dirname(os.path.abspath(path)), os.path.basename(path)[:-5]


def load_capture(argument, crop):
	"""Loads the luma stack (frames, height, width) as float, the metadata, and the directory."""
	path, directory, stem = resolve_capture(argument)

	with open(path, "r", encoding="utf-8") as handle:
		metadata = json.load(handle)

	frames = []

	for frame in metadata["frames"]:
		image = np.asarray(Image.open(os.path.join(directory, frame["file"])).convert("RGB")).astype(np.float64)

		if crop is not None:
			x, y, w, h = crop
			image = image[y:y + h, x:x + w]

		# Rec. 709 luma of the displayed (gamma-encoded) values: what the eye compares frame to frame.
		frames.append(0.2126 * image[..., 0] + 0.7152 * image[..., 1] + 0.0722 * image[..., 2])

	return np.stack(frames), metadata, directory, stem


def warnings_from_metadata(metadata):
	"""Returns the reasons the capture may not be a still image."""
	warnings = []
	frames = metadata["frames"]
	views = np.array([frame["viewMatrix"] for frame in frames])

	if np.abs(views - views[0]).max() > 1e-5:
		warnings.append("the CAMERA MOVED during the capture (view matrices differ): the residual includes that motion")

	if any(frame.get("exposure") and frame["exposure"].get("auto") for frame in frames):
		warnings.append("AUTO EXPOSURE is on: check the flatness line below, or pin it (Act.setExposure) for an A/B")

	if len(frames) < 8:
		warnings.append(f"only {len(frames)} frames: the TAA jitter cycle is 8 frames, a shorter capture does not cover it")

	deltas = [frame["deltaMS"] for frame in frames[1:]]

	if deltas and max(deltas) > 2.5 * max(np.median(deltas), 1e-3):
		warnings.append(f"a frame took {max(deltas):.1f} ms against a median of {np.median(deltas):.1f} ms: a hitch inside the capture")

	return warnings


def statistics(ptp):
	"""The protocol's numbers for a peak-to-peak map."""
	values = ptp.ravel()
	result = {
		"mean": float(values.mean()),
		"p99": float(np.percentile(values, 99)),
		"p99.9": float(np.percentile(values, 99.9)),
		"max": float(values.max()),
	}

	for threshold in THRESHOLDS:
		result[f">{threshold}"] = float((values > threshold).mean() * 100.0)

	return result


def format_statistics(stats):
	shares = "  ".join(f">{t} {stats[f'>{t}']:.2f}%" for t in THRESHOLDS)

	return f"mean {stats['mean']:.3f}  p99 {stats['p99']:.2f}  p99.9 {stats['p99.9']:.2f}  max {stats['max']:.0f}  |  {shares}"


def gradient_and_laplacian(image):
	"""|grad I| and |laplacian I| of the mean image (central differences, borders cropped by one pixel)."""
	gx = (image[1:-1, 2:] - image[1:-1, :-2]) * 0.5
	gy = (image[2:, 1:-1] - image[:-2, 1:-1]) * 0.5
	laplacian = image[1:-1, 2:] + image[1:-1, :-2] + image[2:, 1:-1] + image[:-2, 1:-1] - 4.0 * image[1:-1, 1:-1]

	return np.hypot(gx, gy), np.abs(laplacian)


def signature(ptp, mean_image):
	"""Correlations and the standardised two-term regression of the residual on |grad| and |laplacian|."""
	gradient, laplacian = gradient_and_laplacian(mean_image)
	residual = ptp[1:-1, 1:-1].ravel()
	g = gradient.ravel()
	l = laplacian.ravel()

	def standardise(values):
		deviation = values.std()

		return (values - values.mean()) / deviation if deviation > 0 else values * 0.0

	rs, gs, ls = standardise(residual), standardise(g), standardise(l)

	if not rs.any():
		return None

	design = np.stack([gs, ls], axis=1)
	betas, *_ = np.linalg.lstsq(design, rs, rcond=None)

	return {
		"corr_gradient": float(np.mean(rs * gs)),
		"corr_laplacian": float(np.mean(rs * ls)),
		"corr_gradient_laplacian": float(np.mean(gs * ls)),
		"beta_gradient": float(betas[0]),
		"beta_laplacian": float(betas[1]),
	}


def bands(ptp, count, origin_y):
	"""Per horizontal band: rows, mean and the > 8 share."""
	rows = []
	edges = np.linspace(0, ptp.shape[0], count + 1).astype(int)

	for top, bottom in zip(edges[:-1], edges[1:]):
		band = ptp[top:bottom]
		rows.append((origin_y + top, origin_y + bottom, float(band.mean()), float((band > 8).mean() * 100.0)))

	return rows


def worst_tiles(ptp, tile, count, origin):
	"""The tiles with the highest mean peak-to-peak."""
	height, width = ptp.shape
	tiles = []

	for y in range(0, height - tile + 1, tile):
		for x in range(0, width - tile + 1, tile):
			block = ptp[y:y + tile, x:x + tile]
			tiles.append((float(block.mean()), float((block > 8).mean() * 100.0), origin[0] + x, origin[1] + y))

	tiles.sort(reverse=True)

	return tiles[:count]


def write_maps(ptp, mean_image, directory, stem, suffix):
	"""The amplified peak-to-peak map and the heat overlay."""
	ptp_path = os.path.join(directory, f"{stem}{suffix}-ptp.png")
	Image.fromarray(np.clip(ptp * 16.0, 0, 255).astype(np.uint8)).save(ptp_path)

	# Heat: the darkened mean image, red where the residual exceeds 2, saturating at 16.
	base = np.clip(mean_image * 0.45, 0, 255)
	heat = np.clip((ptp - 2.0) / 14.0, 0.0, 1.0)
	rgb = np.stack([base + (255 - base) * heat, base * (1 - heat), base * (1 - heat)], axis=-1)
	heat_path = os.path.join(directory, f"{stem}{suffix}-heat.png")
	Image.fromarray(np.clip(rgb, 0, 255).astype(np.uint8)).save(heat_path)

	return ptp_path, heat_path


def analyse(argument, crop, band_count, tile, tile_count, write, suffix=""):
	stack, metadata, directory, stem = load_capture(argument, crop)
	origin = (crop[0], crop[1]) if crop else (0, 0)

	print(f"== {stem}: {metadata['frameCount']} frames {metadata['width']}x{metadata['height']}" + (f", crop {crop}" if crop else ""))

	frames = metadata["frames"]
	serials = [frame["rendererFrame"] for frame in frames]

	if serials != list(range(serials[0], serials[0] + len(serials))):
		print(f"   ⚠️ renderer frames are NOT consecutive: {serials}")

	print(f"   renderer frames {serials[0]}..{serials[-1]}, deltas " + ", ".join(f"{frame['deltaMS']:.1f}" for frame in frames[1:]) + " ms")
	print("   jitter (px): " + "  ".join(f"({frame['jitterPixels'][0]:+.2f},{frame['jitterPixels'][1]:+.2f})" for frame in frames))

	for warning in warnings_from_metadata(metadata):
		print(f"   ⚠️ {warning}")

	means = stack.mean(axis=(1, 2))
	drift = float(means.max() - means.min())
	print("   flatness: mean luma per frame " + " ".join(f"{value:.2f}" for value in means) + f"  (drift {drift:.2f})" + ("  ⚠️ NOT FLAT" if drift > 0.5 else ""))

	ptp = stack.max(axis=0) - stack.min(axis=0)
	frame_to_frame = np.abs(np.diff(stack, axis=0)).mean()
	stats = statistics(ptp)
	print(f"   peak-to-peak: {format_statistics(stats)}")
	print(f"   mean |frame(n+1) - frame(n)|: {frame_to_frame:.3f}")

	mean_image = stack.mean(axis=0)
	sig = signature(ptp, mean_image)

	if sig is not None:
		print(f"   signature: corr(ptp, |grad|) {sig['corr_gradient']:.2f}  corr(ptp, |lap|) {sig['corr_laplacian']:.2f}  "
			f"(grad~lap {sig['corr_gradient_laplacian']:.2f})  beta grad {sig['beta_gradient']:.2f} / lap {sig['beta_laplacian']:.2f}")

	print("   bands (rows: mean, >8):")

	for top, bottom, mean, share in bands(ptp, band_count, origin[1]):
		bar = "#" * int(min(mean, 8.0) * 5)
		print(f"     y {top:5d}-{bottom:5d}  {mean:6.3f}  {share:6.2f}%  {bar}")

	print(f"   worst {tile}x{tile} tiles (mean, >8, x, y):")

	for mean, share, x, y in worst_tiles(ptp, tile, tile_count, origin):
		print(f"     {mean:6.2f}  {share:6.2f}%  at ({x}, {y})")

	if write:
		ptp_path, heat_path = write_maps(ptp, mean_image, directory, stem, suffix)
		print(f"   maps: {ptp_path}\n         {heat_path}")

	return stats


def main():
	parser = argparse.ArgumentParser(description="Locate and quantify temporal instability in a temporal capture.")
	parser.add_argument("capture", help="the capture's JSON, one of its PNG, or its stem (looked up in the default captures directory)")
	parser.add_argument("--compare", help="a second capture, analysed with the same crop (A/B)")
	parser.add_argument("--crop", help="x,y,w,h in pixels")
	parser.add_argument("--bands", type=int, default=10, help="horizontal bands (default 10)")
	parser.add_argument("--tile", type=int, default=64, help="tile size for the worst-tile list (default 64)")
	parser.add_argument("--top", type=int, default=8, help="worst tiles listed (default 8)")
	parser.add_argument("--no-maps", action="store_true", help="do not write the ptp and heat maps")
	arguments = parser.parse_args()

	crop = tuple(int(value) for value in arguments.crop.split(",")) if arguments.crop else None

	if crop is not None and len(crop) != 4:
		sys.exit("--crop takes x,y,w,h")

	stats_a = analyse(arguments.capture, crop, arguments.bands, arguments.tile, arguments.top, not arguments.no_maps)

	if arguments.compare:
		print()
		stats_b = analyse(arguments.compare, crop, arguments.bands, arguments.tile, arguments.top, not arguments.no_maps)
		print()
		ratio = stats_b["mean"] / stats_a["mean"] if stats_a["mean"] > 0 else float("inf")
		print(f"== B / A: mean x{ratio:.2f}, p99.9 {stats_a['p99.9']:.2f} -> {stats_b['p99.9']:.2f}, >8 {stats_a['>8']:.2f}% -> {stats_b['>8']:.2f}%")
		print("   ⚠️ run each configuration TWICE: two identical runs have differed by x1.85 (docs/temporal-stability-measurement.md)")


if __name__ == "__main__":
	main()
