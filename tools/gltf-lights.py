#!/usr/bin/env python3
"""
gltf-lights.py — list or set the KHR_lights_punctual lights of a glTF/GLB asset.

Why this exists: exporters ship lights without energy. Every one of the 24 lights of Intel's
Sponza 2022 left 3ds Max (babylon.js exporter) with `intensity: 0` — positions, colors and the
sun's direction were right, the photometry was gone, and no other format of the package (USD:
no lights at all; FBX: `Null` nodes) carried it. The engine's SceneDataConsumer refuses to
instantiate a light without energy, so the values have to be written INTO the asset: the asset
is the lighting authority, a demo must not patch it behind its back.

Units are the KHR_lights_punctual ones, which are also the engine's: lux for a directional
light, candela for a point or a spot.

Usage:
    gltf-lights.py FILE                                  # list the lights (by carrying node name)
    gltf-lights.py FILE --set SUN=100000 --set 'lamp_light_*=100' [--out OUT]

`--set NAME=VALUE` matches the NAME of the node carrying the light (shell-style glob), and
sets the intensity of the light it references. Without --out the file is rewritten IN PLACE
(the GLB binary chunk is streamed, never loaded in memory). A light referenced by no node
cannot be addressed by name; it is listed as `(unreferenced)`.
"""

import argparse
import fnmatch
import json
import os
import shutil
import struct
import sys
import tempfile

GLB_MAGIC = 0x46546C67
CHUNK_JSON = 0x4E4F534A
CHUNK_BIN = 0x004E4942
EXTENSION = "KHR_lights_punctual"


def read_glb_header(handle):
	magic, version, length = struct.unpack("<III", handle.read(12))

	if magic != GLB_MAGIC:
		raise ValueError("not a GLB file (bad magic)")

	if version != 2:
		raise ValueError(f"unsupported GLB version {version}")

	return length


def load(path):
	"""Returns (json_dict, is_glb, bin_chunk_offset, bin_chunk_length_with_header)."""
	with open(path, "rb") as handle:
		head = handle.read(4)
		handle.seek(0)

		if struct.unpack("<I", head)[0] != GLB_MAGIC:
			return json.load(open(path, "r", encoding="utf-8")), False, 0, 0

		read_glb_header(handle)
		json_length, json_type = struct.unpack("<II", handle.read(8))

		if json_type != CHUNK_JSON:
			raise ValueError("first GLB chunk is not JSON")

		document = json.loads(handle.read(json_length).decode("utf-8"))
		bin_offset = 12 + 8 + json_length
		handle.seek(0, os.SEEK_END)
		bin_length = handle.tell() - bin_offset

		return document, True, bin_offset, bin_length


def light_table(document):
	lights = document.get("extensions", {}).get(EXTENSION, {}).get("lights", [])
	carriers = {}

	for node_index, node in enumerate(document.get("nodes", [])):
		reference = node.get("extensions", {}).get(EXTENSION, {}).get("light")

		if reference is not None:
			carriers.setdefault(reference, []).append((node_index, node.get("name", f"node#{node_index}")))

	return lights, carriers


def describe(lights, carriers):
	unit = {"directional": "lux", "point": "cd", "spot": "cd"}

	for index, light in enumerate(lights):
		names = ", ".join(name for _, name in carriers.get(index, [])) or "(unreferenced)"
		color = light.get("color", [1.0, 1.0, 1.0])
		extra = ""

		if "range" in light:
			extra += f", range {light['range']}"

		if light.get("type") == "spot":
			spot = light.get("spot", {})
			extra += f", cone {spot.get('innerConeAngle', 0.0):.4f}/{spot.get('outerConeAngle', 0.7854):.4f} rad"

		print(f"  [{index:2d}] {light.get('type', '?'):11s} {light.get('intensity', 1.0):>12g} {unit.get(light.get('type'), ''):3s}"
			f"  color ({color[0]:.3f}, {color[1]:.3f}, {color[2]:.3f}){extra}  <- {names}")


def apply_sets(lights, carriers, assignments):
	changed = 0

	for pattern, value in assignments:
		hits = 0

		for index, names in carriers.items():
			if any(fnmatch.fnmatchcase(name, pattern) for _, name in names):
				lights[index]["intensity"] = value
				hits += 1

		if hits == 0:
			print(f"warning: --set '{pattern}' matched no light-carrying node", file=sys.stderr)

		changed += hits

	return changed


def write_gltf(document, path):
	with open(path, "w", encoding="utf-8") as handle:
		json.dump(document, handle, indent="\t")
		handle.write("\n")


def write_glb(document, source_path, bin_offset, bin_length, out_path):
	payload = json.dumps(document, separators=(",", ":")).encode("utf-8")
	padding = (4 - len(payload) % 4) % 4
	payload += b" " * padding
	total = 12 + 8 + len(payload) + bin_length

	directory = os.path.dirname(os.path.abspath(out_path)) or "."
	descriptor, temporary = tempfile.mkstemp(prefix=".gltf-lights-", dir=directory)

	with os.fdopen(descriptor, "wb") as out, open(source_path, "rb") as source:
		out.write(struct.pack("<III", GLB_MAGIC, 2, total))
		out.write(struct.pack("<II", len(payload), CHUNK_JSON))
		out.write(payload)
		source.seek(bin_offset)
		shutil.copyfileobj(source, out, 16 * 1024 * 1024)

	os.replace(temporary, out_path)


def parse_assignment(text):
	name, separator, value = text.partition("=")

	if not separator or not name:
		raise argparse.ArgumentTypeError(f"expected NAME=VALUE, got '{text}'")

	return name, float(value)


def main():
	parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
	parser.add_argument("file", help="a .gltf or .glb asset")
	parser.add_argument("--set", dest="assignments", action="append", type=parse_assignment, default=[], metavar="NAME=VALUE", help="set the intensity of the light carried by the node(s) matching NAME (glob)")
	parser.add_argument("--out", help="output path (default: rewrite the input in place)")
	arguments = parser.parse_args()

	document, is_glb, bin_offset, bin_length = load(arguments.file)
	lights, carriers = light_table(document)

	if not lights:
		print(f"{arguments.file}: no {EXTENSION} light.")
		return 0

	if not arguments.assignments:
		print(f"{arguments.file}: {len(lights)} {EXTENSION} light(s)")
		describe(lights, carriers)
		return 0

	changed = apply_sets(lights, carriers, arguments.assignments)

	if changed == 0:
		print("nothing changed.", file=sys.stderr)
		return 1

	out_path = arguments.out or arguments.file

	if is_glb:
		write_glb(document, arguments.file, bin_offset, bin_length, out_path)
	else:
		write_gltf(document, out_path)

	print(f"{out_path}: {changed} light(s) updated")
	describe(lights, carriers)
	return 0


if __name__ == "__main__":
	sys.exit(main())
