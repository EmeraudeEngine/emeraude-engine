#!/usr/bin/env python3
"""
sky-manifest.py — measure an equirectangular HDR sky and print its two store manifests.

The engine's Background manifest (store `Backgrounds`) is the FULL photometric description of a
sky: an absolute `Luminance` in nits, an `AverageColor`, an `AmbientIlluminance` in lux and the
celestial bodies (`Stars`) to derive analytic directional lights from. An HDRI carries none of
these as numbers — it holds RELATIVE radiances — and the engine's loader calibrates it so that a
scale of 1 behaves like a UNIFORM DOME of luminance 1 (E = pi lux on the ground, owner decision
D6). Every number below is therefore MEASURED on the texels and anchored on ONE declared value:
the illuminance of the sun found in the picture (`--sun-illuminance`, lux on a surface facing it).

What is measured:
  - the sun: the brightest texel, refined as the radiance-weighted centroid of the texels above
    5 % of the peak; its direction in the ENGINE frame (the loader's equirect convention:
    u = atan2(z, x) / 2pi + 0.5, v = 0.5 - asin(y) / pi, Y up) and its elevation;
  - the energy profile around it (how much of the excess flux sits within 0.25/0.5/1/2 deg),
    which sizes the IBL mask: the engine masks ONE angular diameter around the centre;
  - the sky-only horizontal illuminance (sun excluded) and the total one, in relative units;
  - the sky-only average color of the upper hemisphere (solid-angle weighted).

The anchor gives the scale k (lux per relative unit) = sun-illuminance / E_sun-normal(relative).
    Luminance         = k * E_h,total / pi     (what makes the engine's dome-of-1 calibration
                                                reproduce the picture's absolute level)
    AmbientIlluminance = k * E_h,sky-only     (what the dome pours on the ground WITHOUT the
                                                sun, which the analytic star carries)
    Stars[0]          = the sun, InTexture, at the measured direction with the declared lux.

Usage:
    sky-manifest.py kloppenheim_05_4k.hdr --name Kloppenheim05 --sun-illuminance 100000 \
        [--temperature 5500] [--face-size 1024] [--copyright 'Poly Haven - CC0 - polyhaven.com']

Requires numpy (the measurement runs over 8 M texels).
"""

import argparse
import json
import math
import sys

try:
	import numpy
except ImportError:  # pragma: no cover
	sys.exit("numpy is required: pip install numpy")


def read_radiance_hdr(path):
	"""Radiance RGBE (.hdr) reader, new-style RLE scanlines. Returns a float32 (H, W, 3) array."""
	with open(path, "rb") as handle:
		if handle.readline().strip() not in (b"#?RADIANCE", b"#?RGBE"):
			raise ValueError("not a Radiance HDR file")

		while True:
			line = handle.readline()

			if not line:
				raise ValueError("truncated header")

			if line.strip() == b"":
				break

			if line.startswith(b"FORMAT=") and b"32-bit_rle_rgbe" not in line:
				raise ValueError("only 32-bit_rle_rgbe is supported")

		resolution = handle.readline().split()

		if resolution[0] != b"-Y" or resolution[2] != b"+X":
			raise ValueError("only the standard -Y +X orientation is supported")

		height, width = int(resolution[1]), int(resolution[3])
		data = numpy.frombuffer(handle.read(), dtype=numpy.uint8)

	rgbe = numpy.empty((height, width, 4), dtype=numpy.uint8)
	position = 0

	for row in range(height):
		if data[position] != 2 or data[position + 1] != 2:
			raise ValueError("old-style RLE scanline: unsupported")

		line_width = (int(data[position + 2]) << 8) | int(data[position + 3])

		if line_width != width:
			raise ValueError("scanline width mismatch")

		position += 4

		for channel in range(4):
			column = 0

			while column < width:
				count = int(data[position])
				position += 1

				if count > 128:
					count -= 128
					rgbe[row, column:column + count, channel] = data[position]
					position += 1
				else:
					rgbe[row, column:column + count, channel] = data[position:position + count]
					position += count

				column += count

	exponent = rgbe[..., 3].astype(numpy.int32)
	scale = numpy.where(exponent == 0, 0.0, numpy.ldexp(1.0, exponent - 136)).astype(numpy.float32)

	return rgbe[..., :3].astype(numpy.float32) * scale[..., None]


def main():
	parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
	parser.add_argument("file", help="an equirectangular Radiance .hdr sky")
	parser.add_argument("--name", required=True, help="resource name (Cubemaps/<name>.Equirectangular.hdr, Backgrounds/<name>.json)")
	parser.add_argument("--sun-illuminance", type=float, required=True, help="illuminance of the in-picture sun on a surface facing it, in lux (the anchor)")
	parser.add_argument("--temperature", type=float, default=5500.0, help="sun color temperature written in the manifest, kelvins (default 5500)")
	parser.add_argument("--face-size", type=int, default=1024, help="cube face size written in the Cubemaps manifest (default 1024)")
	parser.add_argument("--copyright", default="", help="copyright line written in the Cubemaps manifest")
	arguments = parser.parse_args()

	radiance = read_radiance_hdr(arguments.file)
	height, width = radiance.shape[:2]
	luma = radiance @ numpy.array([0.2126, 0.7152, 0.0722], dtype=numpy.float32)

	rows = numpy.arange(height, dtype=numpy.float64)
	elevation = math.pi * (0.5 - (rows + 0.5) / height)  # per row, radians
	solid_angle = (2.0 * math.pi / width) * (math.pi / height) * numpy.cos(elevation)  # per texel of the row
	ground_cosine = numpy.sin(elevation)

	# --- the sun: brightest texel, refined as a radiance-weighted centroid above 5 % of the peak.
	peak = float(luma.max())
	sun_mask = luma > 0.05 * peak
	weights = luma * sun_mask * solid_angle[:, None]
	ys, xs = numpy.nonzero(sun_mask)
	sun_x = float((weights[ys, xs] * (xs + 0.5)).sum() / weights[ys, xs].sum())
	sun_y = float((weights[ys, xs] * (ys + 0.5)).sum() / weights[ys, xs].sum())
	u, v = sun_x / width, sun_y / height
	theta = (u - 0.5) * 2.0 * math.pi
	phi = (0.5 - v) * math.pi
	sun_direction = (math.cos(phi) * math.cos(theta), math.sin(phi), math.cos(phi) * math.sin(theta))

	# --- angular distance of every texel to the sun, energy profile.
	columns = numpy.arange(width, dtype=numpy.float64)
	texel_theta = ((columns + 0.5) / width - 0.5) * 2.0 * math.pi
	cos_elevation = numpy.cos(elevation)
	direction_x = cos_elevation[:, None] * numpy.cos(texel_theta)[None, :]
	direction_y = numpy.sin(elevation)[:, None] * numpy.ones((1, width))
	direction_z = cos_elevation[:, None] * numpy.sin(texel_theta)[None, :]
	cos_to_sun = numpy.clip(direction_x * sun_direction[0] + direction_y * sun_direction[1] + direction_z * sun_direction[2], -1.0, 1.0)
	angle_to_sun = numpy.degrees(numpy.arccos(cos_to_sun))

	flux = luma.astype(numpy.float64) * solid_angle[:, None]
	ring = (angle_to_sun >= 12.0) & (angle_to_sun <= 16.0)
	ring_luminance = float(flux[ring].sum() / solid_angle[:, None].repeat(width, axis=1)[ring].sum())

	profile = []

	for radius in (0.25, 0.5, 1.0, 2.0, 5.0):
		inside = angle_to_sun <= radius
		omega = 2.0 * math.pi * (1.0 - math.cos(math.radians(radius)))
		profile.append((radius, float(flux[inside].sum()) - ring_luminance * omega))

	sun_excess = profile[-1][1]  # the excess within 5 deg is what the analytic star replaces
	# The sun's illuminance on a surface FACING it, relative units: its excess radiance x solid angle.
	sun_normal_relative = sun_excess

	# --- horizontal illuminances (upper hemisphere), relative units.
	upper = ground_cosine > 0.0
	weight_h = (solid_angle * numpy.where(upper, ground_cosine, 0.0))[:, None]
	e_h_total = float((luma.astype(numpy.float64) * weight_h).sum())
	sun_cone = angle_to_sun <= 5.0
	e_h_sun = float((luma.astype(numpy.float64) * weight_h * sun_cone).sum()) - ring_luminance * float((weight_h * sun_cone).sum())
	e_h_sky = e_h_total - e_h_sun

	# --- sky-only average color of the upper hemisphere, solid-angle weighted, normalized to max 1.
	sky_weight = (solid_angle[:, None] * numpy.where(upper, 1.0, 0.0)[:, None]) * (~sun_cone)
	mean_rgb = (radiance.astype(numpy.float64) * sky_weight[..., None]).sum(axis=(0, 1)) / sky_weight.sum()
	average_color = mean_rgb / mean_rgb.max()

	# --- anchor.
	k = arguments.sun_illuminance / sun_normal_relative
	luminance = k * e_h_total / math.pi
	ambient_illuminance = k * e_h_sky

	print(f"# {arguments.file}: {width}x{height}")
	print(f"# sun: texel ({sun_x:.1f}, {sun_y:.1f}), elevation {math.degrees(phi):.2f} deg, direction (engine frame, toward) "
		f"({sun_direction[0]:.5f}, {sun_direction[1]:.5f}, {sun_direction[2]:.5f}); peak {peak:.1f}, sky at 12-16 deg {ring_luminance:.3f}")

	for radius, excess in profile:
		print(f"#   excess flux within {radius:4.2f} deg: {excess:8.3f}  ({100.0 * excess / sun_excess:5.1f} % of the 5 deg total)")

	print(f"# horizontal illuminance (relative): total {e_h_total:.3f}, sun {e_h_sun:.3f}, sky-only {e_h_sky:.3f}  (sun/sky {e_h_sun / e_h_sky:.2f})")
	print(f"# anchor: {arguments.sun_illuminance:.0f} lux facing the sun -> k = {k:.1f} lux per relative unit; "
		f"in-picture sun peak {peak * k:.3g} cd/m2, sky-only mean {ambient_illuminance / math.pi:.0f} cd/m2")
	print()
	print(f"# data-stores/Cubemaps/{arguments.name}.json")
	cubemap = {"Equirectangular": True, "FileFormat": "hdr", "Size": arguments.face_size}

	if arguments.copyright:
		cubemap["Copyright"] = arguments.copyright

	print(json.dumps(cubemap, indent="\t"))
	print()
	print(f"# data-stores/Backgrounds/{arguments.name}.json")
	background = {
		"Cubemap": arguments.name,
		"Luminance": round(luminance, -1),
		"AverageColor": [round(float(c), 3) for c in average_color],
		"AmbientIlluminance": round(ambient_illuminance, -1),
		"Stars": [
			{
				"Type": "Sun",
				"Direction": [round(c, 4) for c in sun_direction],
				"Illuminance": arguments.sun_illuminance,
				"Temperature": arguments.temperature,
				"AngularDiameter": 0.53,
				"InTexture": True,
			}
		],
	}
	print(json.dumps(background, indent="\t"))
	return 0


if __name__ == "__main__":
	sys.exit(main())
