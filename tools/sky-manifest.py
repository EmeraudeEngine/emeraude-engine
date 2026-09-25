#!/usr/bin/env python3
"""
sky-manifest.py — measure an equirectangular HDR sky and print its two store manifests.

The engine's Background manifest (store `Backgrounds`) is the FULL photometric description of a
sky: an absolute `Luminance` in nits, an `AverageColor`, an `AmbientIlluminance` in lux and the
celestial bodies (`Stars`) to derive analytic directional lights from. An HDRI carries none of
these as numbers — it holds RELATIVE radiances — and the engine's loader calibrates it so that a
scale of 1 behaves like a UNIFORM DOME of luminance 1 (E = pi lux on the ground, owner decision
D6). Every number below is therefore MEASURED on the texels and anchored on ONE declared value:
the illuminance of the sun found in the picture (`--sun-illuminance`, lux on a surface facing it),
or, for a VEILED sun, the luminance of the sky (`--sky-luminance`, nits).

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

A veiled sun (haze, smog) carries a tiny share of the picture's light: anchoring on a plausible
sun then inflates the sky. IndustrialSunsetPureSky: the sun holds 0.4 % of the horizontal
illuminance, and 10 000 lx on it gave a 52 000-nit sky with 163 000 lx of ambient. Anchor such a
sky on its LUMINANCE instead (`--sky-luminance`, the July 2026 reviewed value for instance):
k = Luminance * pi / E_h,total, and the star's illuminance is DERIVED, k * E_sun-normal. Read the
`sun/sky` ratio the tool prints before choosing the anchor.

Usage:
    sky-manifest.py kloppenheim_05_4k.hdr --name Kloppenheim05 --sun-illuminance 100000 \
        [--temperature 5500] [--face-size 1024] [--copyright 'Poly Haven - CC0 - polyhaven.com']

Locate mode — WHERE is the body painted in a sky already in a store, whatever its picture (a packed
cube or an equirectangular, LDR png/jpg/tga or HDR), compared with what its Background manifest
declares:
    sky-manifest.py --locate StormyDays --store ../projet-alpha.data/data-stores [--preview out.png]

  An LDR sun is CLIPPED: its core is a flat plateau of white texels whose outline follows the clouds,
  and a lit cloud edge hanging off the disc drags a centroid (StormyDays: 22-24 deg, with the threshold, for a disc
  centred at 17.8 deg). The locate mode takes the centre of the LARGEST DISC INSCRIBED in the plateau instead (the
  plateau point farthest from its outline, which thin appendages cannot move) and draws every estimate
  on the preview: always LOOK at it. An HDR sun is not clipped: its radiance-weighted centroid is used.
  --around AZ,EL restricts the analysis to 15 deg around a hint read on the preview, and adds the centre of
  an UNCLIPPED disc (a moon: the centroid of the region above half its peak) and the centre of a halo half
  hidden by clouds (the peak of the luminance smoothed over 2 deg).

Requires numpy (the measurement runs over 8 M texels).
"""

import argparse
import json
import math
import os
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


def srgb_to_linear(values):
	"""Decodes the sRGB transfer function (values in [0, 1])."""
	return numpy.where(values <= 0.04045, values / 12.92, ((values + 0.055) / 1.055) ** 2.4)


def load_store_sky(store, name):
	"""
	Reads a store sky: data-stores/Cubemaps/<name>.json and the picture it names.

	:return: (kind, linear RGB float32 array, is_hdr), kind being "Packed" or "Equirectangular".
	"""
	with open(os.path.join(store, "Cubemaps", name + ".json")) as handle:
		manifest = json.load(handle)

	file_format = manifest.get("FileFormat", "png")

	if manifest.get("Packed"):
		kind = "Packed"
	elif manifest.get("Equirectangular"):
		kind = "Equirectangular"
	else:
		raise ValueError(f"{name}: only packed and equirectangular cubemaps are supported")

	path = os.path.join(store, "Cubemaps", f"{name}.{kind}.{file_format}")

	if file_format == "hdr":
		return kind, read_radiance_hdr(path), True

	try:
		from PIL import Image
	except ImportError:  # pragma: no cover
		sys.exit("Pillow is required to read an LDR sky: pip install pillow")

	picture = numpy.asarray(Image.open(path).convert("RGB"), dtype=numpy.float32) / 255.0

	return kind, srgb_to_linear(picture).astype(numpy.float32), False


def equirect_directions(width, height):
	"""Unit directions of an equirectangular grid in the ENGINE frame (the loader's convention, Y up)."""
	elevation = math.pi * (0.5 - (numpy.arange(height) + 0.5) / height)
	theta = ((numpy.arange(width) + 0.5) / width - 0.5) * 2.0 * math.pi
	cos_elevation = numpy.cos(elevation)[:, None]
	x = cos_elevation * numpy.cos(theta)[None, :]
	y = numpy.sin(elevation)[:, None] * numpy.ones((1, width))
	z = cos_elevation * numpy.sin(theta)[None, :]

	return x, y, z, elevation


def sample_packed(picture, x, y, z):
	"""
	Samples a PACKED cube (nearest) along world directions.

	Layout of CubemapResource::load(Pixmap): column 0 = +X over -X, column 1 = +Y over -Y, column 2 = +Z over
	-Z; each face follows the Vulkan cube convention (sc, tc, major axis), the cube space being the world
	space since the Y-up flip.
	"""
	face = picture.shape[1] // 3
	ax, ay, az = numpy.abs(x), numpy.abs(y), numpy.abs(z)
	major_x = (ax >= ay) & (ax >= az)
	major_y = ~major_x & (ay >= az)
	major_z = ~major_x & ~major_y
	sc = numpy.zeros_like(x)
	tc = numpy.zeros_like(x)
	ma = numpy.ones_like(x)
	block_column = numpy.zeros(x.shape, dtype=numpy.int64)
	block_row = numpy.zeros(x.shape, dtype=numpy.int64)

	for mask, positive, major, s_value, t_value, column in (
		(major_x & (x > 0), 0, ax, -z, -y, 0),
		(major_x & (x <= 0), 1, ax, z, -y, 0),
		(major_y & (y > 0), 0, ay, x, z, 1),
		(major_y & (y <= 0), 1, ay, x, -z, 1),
		(major_z & (z > 0), 0, az, x, -y, 2),
		(major_z & (z <= 0), 1, az, -x, -y, 2),
	):
		sc[mask] = s_value[mask]
		tc[mask] = t_value[mask]
		ma[mask] = major[mask]
		block_column[mask] = column
		block_row[mask] = positive

	u = numpy.clip(((sc / ma + 1.0) * 0.5 * face).astype(numpy.int64), 0, face - 1)
	v = numpy.clip(((tc / ma + 1.0) * 0.5 * face).astype(numpy.int64), 0, face - 1)

	return picture[block_row * face + v, block_column * face + u]


def sample_equirect(picture, x, y, z):
	"""Samples an equirectangular picture (nearest) along world directions: u = atan2(z, x) / 2pi + 0.5, v = 0.5 - asin(y) / pi."""
	height, width = picture.shape[:2]
	u = numpy.arctan2(z, x) / (2.0 * math.pi) + 0.5
	v = 0.5 - numpy.arcsin(numpy.clip(y, -1.0, 1.0)) / math.pi
	columns = numpy.clip((u * width).astype(numpy.int64), 0, width - 1)
	rows = numpy.clip((v * height).astype(numpy.int64), 0, height - 1)

	return picture[rows, columns]


def locate_body(sample, luma, x, y, z, solid_angle, is_hdr):
	"""
	Locates the brightest painted body.

	:param sample: a function (x, y, z) -> linear RGB sampling the source picture along world directions.
	:return: dict of estimates, each a unit direction: "centroid" (radiance-weighted above 5 % of the peak for an
	HDR sky; the clipped plateau for an LDR one) and, for an LDR sky, "inscribed" (the centre of the largest disc
	inscribed in the plateau) with its angular radius.
	"""
	peak = float(luma.max())
	directions = numpy.stack([x, y, z], axis=-1)
	weights = solid_angle[:, None] * numpy.ones((1, luma.shape[1]))
	# LDR: the plateau is the SATURATED white (8-bit 255 on the three channels decodes to 1.0; a channel at 250
	# still reads 0.997) — a lower threshold takes in the whole glow around it.
	threshold = (0.05 if is_hdr else 0.995) * peak
	core = luma >= threshold
	core_weight = (luma * weights)[core] if is_hdr else weights[core]
	centroid = (directions[core] * core_weight[:, None]).sum(axis=0)
	centroid /= numpy.linalg.norm(centroid)
	estimates = {"centroid": centroid}

	if is_hdr:
		return estimates, None

	# An LDR sun is a flat plateau whose outline follows the clouds: a lit cloud edge hanging off the disc
	# drags any centroid (StormyDays: +7 deg). The point of the plateau FARTHEST from its outline is the
	# centre of its largest inscribed disc, which thin appendages cannot move. Measured on a gnomonic
	# (tangent-plane) grid around the centroid, 0.05 deg per cell at its centre.
	try:
		from scipy.ndimage import distance_transform_edt
	except ImportError:  # pragma: no cover
		sys.exit("scipy is required to locate the body of an LDR sky: pip install scipy")

	helper = numpy.array([0.0, 1.0, 0.0]) if abs(centroid[1]) < 0.9 else numpy.array([1.0, 0.0, 0.0])
	first = numpy.cross(helper, centroid)
	first /= numpy.linalg.norm(first)
	second = numpy.cross(centroid, first)
	half = math.tan(math.radians(40.0))
	cells = 1600
	grid = numpy.linspace(-half, half, cells)
	u, v = numpy.meshgrid(grid, grid)
	gx = centroid[0] + u * first[0] + v * second[0]
	gy = centroid[1] + u * first[1] + v * second[1]
	gz = centroid[2] + u * first[2] + v * second[2]
	norm = numpy.sqrt(gx * gx + gy * gy + gz * gz)
	gx, gy, gz = gx / norm, gy / norm, gz / norm
	plane_luma = sample(gx, gy, gz) @ numpy.array([0.2126, 0.7152, 0.0722], dtype=numpy.float32)
	distance = distance_transform_edt(plane_luma >= threshold)
	row, column = numpy.unravel_index(int(numpy.argmax(distance)), distance.shape)
	inscribed = numpy.array([gx[row, column], gy[row, column], gz[row, column]])
	estimates["inscribed"] = inscribed
	radius = math.degrees(math.atan(float(distance[row, column]) * (2.0 * half / (cells - 1))))

	return estimates, radius


def locate_around(sample, hint, half_degrees=15.0, cells=1200):
	"""
	Local analysis around a hint direction — when the brightest thing of the sky is not the body (a lit cloud
	top), or when the body is not clipped (a moon).

	:return: dict of estimates: "inscribed" (centre of the largest disc inscribed in the saturated plateau, when
	there is one), "disc" (luminance-weighted centroid of the connected region above half the local peak, around
	that peak — the centre of an unclipped disc) and "glow" (the maximum of the luminance smoothed over 2 deg —
	the centre of a halo half hidden by clouds).
	"""
	from scipy.ndimage import distance_transform_edt, gaussian_filter, label

	hint = hint / numpy.linalg.norm(hint)
	helper = numpy.array([0.0, 1.0, 0.0]) if abs(hint[1]) < 0.9 else numpy.array([1.0, 0.0, 0.0])
	first = numpy.cross(helper, hint)
	first /= numpy.linalg.norm(first)
	second = numpy.cross(hint, first)
	half = math.tan(math.radians(half_degrees))
	grid = numpy.linspace(-half, half, cells)
	u, v = numpy.meshgrid(grid, grid)
	gx = hint[0] + u * first[0] + v * second[0]
	gy = hint[1] + u * first[1] + v * second[1]
	gz = hint[2] + u * first[2] + v * second[2]
	norm = numpy.sqrt(gx * gx + gy * gy + gz * gz)
	gx, gy, gz = gx / norm, gy / norm, gz / norm
	luma = sample(gx, gy, gz) @ numpy.array([0.2126, 0.7152, 0.0722], dtype=numpy.float32)
	cell_degrees = math.degrees(2.0 * half / (cells - 1))
	peak = float(luma.max())
	estimates = {}

	def at(row, column):
		return numpy.array([gx[row, column], gy[row, column], gz[row, column]])

	plateau = luma >= 0.995

	if plateau.any():
		distance = distance_transform_edt(plateau)
		estimates["inscribed"] = at(*numpy.unravel_index(int(numpy.argmax(distance)), distance.shape))

	regions, _ = label(luma >= 0.5 * peak)
	peak_row, peak_column = numpy.unravel_index(int(numpy.argmax(luma)), luma.shape)
	region = regions == regions[peak_row, peak_column]
	weights = luma * region
	direction = numpy.array([(gx * weights).sum(), (gy * weights).sum(), (gz * weights).sum()])
	estimates["disc"] = direction / numpy.linalg.norm(direction)
	smoothed = gaussian_filter(luma, sigma=2.0 / cell_degrees)
	estimates["glow"] = at(*numpy.unravel_index(int(numpy.argmax(smoothed)), smoothed.shape))

	return estimates


def elevation_azimuth(direction):
	"""Elevation above the horizon and azimuth atan2(z, x), in degrees."""
	return math.degrees(math.asin(max(-1.0, min(1.0, float(direction[1]))))), math.degrees(math.atan2(float(direction[2]), float(direction[0])))


def angle_between(a, b):
	return math.degrees(math.acos(max(-1.0, min(1.0, float(numpy.dot(a, b) / (numpy.linalg.norm(a) * numpy.linalg.norm(b)))))))


def locate(arguments):
	"""The --locate mode: measure where a store sky's body is and compare with its Background manifest."""
	kind, picture, is_hdr = load_store_sky(arguments.store, arguments.locate)
	width, height = 1440, 720
	x, y, z, elevation = equirect_directions(width, height)

	if kind == "Packed":
		rgb = sample_packed(picture, x, y, z)
	else:
		rgb = sample_equirect(picture, x, y, z)

	luma = rgb @ numpy.array([0.2126, 0.7152, 0.0722], dtype=numpy.float32)
	solid_angle = (2.0 * math.pi / width) * (math.pi / height) * numpy.cos(elevation)

	def sample(dx, dy, dz):
		return sample_packed(picture, dx, dy, dz) if kind == "Packed" else sample_equirect(picture, dx, dy, dz)

	estimates, radius = locate_body(sample, luma, x, y, z, solid_angle, is_hdr)
	measured = estimates.get("inscribed", estimates["centroid"])

	if arguments.around:
		azimuth, elevation_hint = (math.radians(float(value)) for value in arguments.around.split(","))
		hint = numpy.array([math.cos(elevation_hint) * math.cos(azimuth), math.sin(elevation_hint), math.cos(elevation_hint) * math.sin(azimuth)])
		estimates = {key: value for key, value in estimates.items()}
		estimates.update({"around/" + key: value for key, value in locate_around(sample, hint).items()})
		measured = estimates.get("around/inscribed", estimates["around/disc"])

	print(f"# {arguments.locate}: {kind} {'HDR' if is_hdr else 'LDR'} {picture.shape[1]}x{picture.shape[0]}, peak {float(luma.max()):.3f}"
		+ (f", clipped plateau: largest inscribed disc {radius:.2f} deg in radius" if radius is not None else ""))

	for label, direction in estimates.items():
		el, az = elevation_azimuth(direction)
		print(f"#   {label:8s} ({direction[0]:+.4f}, {direction[1]:+.4f}, {direction[2]:+.4f})  elevation {el:+6.2f} deg, azimuth {az:+7.2f} deg")

	declared = []
	manifest_path = os.path.join(arguments.store, "Backgrounds", arguments.locate + ".json")

	if os.path.exists(manifest_path):
		with open(manifest_path) as handle:
			background = json.load(handle)

		for star in background.get("Stars", []):
			direction = numpy.array(star.get("Direction", [0.0, 0.0, 0.0]), dtype=numpy.float64)

			if not numpy.any(direction):
				continue

			declared.append(direction / numpy.linalg.norm(direction))
			el, az = elevation_azimuth(declared[-1])
			print(f"#   declared {star.get('Type', '?')} {'(InTexture)' if star.get('InTexture') else '(not in texture)'} "
				f"({direction[0]:+.4f}, {direction[1]:+.4f}, {direction[2]:+.4f})  elevation {el:+6.2f} deg, azimuth {az:+7.2f} deg, "
				f"{angle_between(declared[-1], measured):.1f} deg from the measurement")

	print(f"# measured direction for the manifest: [{measured[0]:.4f}, {measured[1]:.4f}, {measured[2]:.4f}]")

	if arguments.preview:
		from PIL import Image, ImageDraw

		shown = rgb / (1.0 + rgb) if is_hdr else rgb
		shown = numpy.clip(numpy.where(shown <= 0.0031308, shown * 12.92, 1.055 * numpy.power(numpy.maximum(shown, 0.0), 1.0 / 2.4) - 0.055), 0.0, 1.0)
		image = Image.fromarray((shown * 255.0).astype(numpy.uint8)).resize((width // 2, height // 2))
		draw = ImageDraw.Draw(image)
		horizon = height // 4
		draw.line([0, horizon, width // 2, horizon], fill=(255, 255, 0))

		def mark(direction, colour, size):
			el, az = elevation_azimuth(direction)
			column = (az / 360.0 + 0.5) * (width // 2)
			row = (0.5 - el / 180.0) * (height // 2)
			draw.ellipse([column - size, row - size, column + size, row + size], outline=colour, width=2)

		for direction in declared:
			mark(direction, (255, 0, 0), 9)
		mark(estimates["centroid"], (0, 128, 255), 6)

		if "inscribed" in estimates:
			mark(estimates["inscribed"], (0, 255, 0), 4)

		for key, colour in (("around/inscribed", (0, 255, 0)), ("around/disc", (255, 0, 255)), ("around/glow", (255, 255, 255))):
			if key in estimates:
				mark(estimates[key], colour, 3)

		image.save(arguments.preview)
		print(f"# preview: {arguments.preview} (yellow = horizon, red = declared, blue = centroid, green = inscribed-disc centre)")

	return 0


def main():
	parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
	parser.add_argument("file", nargs="?", help="an equirectangular Radiance .hdr sky")
	parser.add_argument("--name", help="resource name (Cubemaps/<name>.Equirectangular.hdr, Backgrounds/<name>.json)")
	parser.add_argument("--sun-illuminance", type=float, help="illuminance of the in-picture sun on a surface facing it, in lux (the anchor)")
	parser.add_argument("--sky-luminance", type=float, help="the sky's Luminance in nits, as the anchor instead of the sun (a veiled sun): the sun's illuminance is derived")
	parser.add_argument("--locate", metavar="NAME", help="locate mode: measure where the body of the store sky NAME is painted")
	parser.add_argument("--store", help="locate mode: the data-stores directory holding Cubemaps/ and Backgrounds/")
	parser.add_argument("--preview", help="locate mode: write an equirectangular preview PNG with the estimates drawn on it")
	parser.add_argument("--around", metavar="AZ,EL", help="locate mode: analyse within 15 deg of this azimuth,elevation (degrees; azimuth = atan2(z, x)) — "
		"for a body that is not the brightest thing of the picture, or not clipped")
	parser.add_argument("--temperature", type=float, default=5500.0, help="sun color temperature written in the manifest, kelvins (default 5500)")
	parser.add_argument("--face-size", type=int, default=1024, help="cube face size written in the Cubemaps manifest (default 1024)")
	parser.add_argument("--copyright", default="", help="copyright line written in the Cubemaps manifest")
	arguments = parser.parse_args()

	if arguments.locate:
		if not arguments.store:
			parser.error("--locate needs --store")

		return locate(arguments)

	if not arguments.file or not arguments.name or (arguments.sun_illuminance is None) == (arguments.sky_luminance is None):
		parser.error("measuring an HDR sky needs FILE, --name and ONE anchor: --sun-illuminance or --sky-luminance")

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
	if arguments.sky_luminance is not None:
		k = arguments.sky_luminance * math.pi / e_h_total
		sun_illuminance = round(k * sun_normal_relative, -1)
		anchor = f"{arguments.sky_luminance:.0f} nits of sky -> sun derived at {sun_illuminance:.0f} lux facing it"
	else:
		k = arguments.sun_illuminance / sun_normal_relative
		sun_illuminance = arguments.sun_illuminance
		anchor = f"{arguments.sun_illuminance:.0f} lux facing the sun"

	luminance = k * e_h_total / math.pi
	ambient_illuminance = k * e_h_sky

	print(f"# {arguments.file}: {width}x{height}")
	print(f"# sun: texel ({sun_x:.1f}, {sun_y:.1f}), elevation {math.degrees(phi):.2f} deg, direction (engine frame, toward) "
		f"({sun_direction[0]:.5f}, {sun_direction[1]:.5f}, {sun_direction[2]:.5f}); peak {peak:.1f}, sky at 12-16 deg {ring_luminance:.3f}")

	for radius, excess in profile:
		print(f"#   excess flux within {radius:4.2f} deg: {excess:8.3f}  ({100.0 * excess / sun_excess:5.1f} % of the 5 deg total)")

	print(f"# horizontal illuminance (relative): total {e_h_total:.3f}, sun {e_h_sun:.3f}, sky-only {e_h_sky:.3f}  (sun/sky {e_h_sun / e_h_sky:.2f})")
	print(f"# anchor: {anchor} -> k = {k:.1f} lux per relative unit; "
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
				"Illuminance": sun_illuminance,
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
