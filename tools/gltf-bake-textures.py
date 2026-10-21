#!/usr/bin/env python3
"""
gltf-bake-textures.py — replace the textures of a GLB with KTX2 containers baked for Emeraude (graphics § 39, the baked
assets, owner decision 2026-10-21).

Why this exists: a texture the engine loads from PNG / JPEG is repacked into the canonical layout and block-compressed
at load time (cached afterwards); one shipped as Basis Universal is transcoded at every load and cannot be repacked.
A baked texture is the result of the runtime's own pipeline written to disk once — EmeraudeTextureBake runs
PackRecipe::fillChannel(), TextureCompressor (BC7 / BC5 / BC4, full mip chain) and KTX2Writer — so the engine uploads
it as stored: no decode, no transcode, no encode.

⚠️ The output is meant for Emeraude only: its KTX2 images carry raw BC blocks behind KHR_texture_basisu, which requires
Basis Universal payloads. Keep the source asset for anything else.

Usage:
    gltf-bake-textures.py IN.glb MANIFEST.json --sources DIR --baker PATH --out OUT.glb [--cache DIR] [--jobs N] [--zstd N]

The manifest names, per material, what each texture of the canonical layout is made of (paths relative to --sources):

    {"materials": {
        "brickwall_01": {
            "baseColor":         {"kind": "colour", "r": "textures/brick_BaseColor.png:r", "g": "...:g", "b": "...:b", "a": "...:a"},
            "normal":            {"kind": "normal", "flipNormalY": true, "r": "textures/brick_Normal.png:r", "g": "...:g", "b": "...:b"},
            "metallicRoughness": {"kind": "data", "r": "#255", "g": "textures/brick_MR.png:g", "b": "textures/brick_MR.png:b"}
        }}}

Roles: baseColor, metallicRoughness, normal, occlusion, emissive (the glTF texture slots). A role absent from a material
keeps nothing: every textured slot of the GLB must be covered, or the tool refuses (nothing is silently left behind).
A channel not given is 255. The occlusion of the canonical layout is the R of the metallicRoughness texture: give the same
recipe to both roles to share one image (identical recipes are baked once and stored once).

The GLB binary chunk is rebuilt: the old images are dropped, the kept views (EXT_meshopt_compression ranges included)
are copied, the KTX2 containers appended. --cache keeps the baked containers (named by their recipe, the baker's hash and
the zstd level) so a rerun only bakes what changed.
"""

import argparse
import concurrent.futures
import hashlib
import json
import os
import struct
import subprocess
import sys

GLB_MAGIC = 0x46546C67
CHUNK_JSON = 0x4E4F534A
CHUNK_BIN = 0x004E4942
ALIGNMENT = 16

ROLES = {
	"baseColor": ("pbrMetallicRoughness", "baseColorTexture"),
	"metallicRoughness": ("pbrMetallicRoughness", "metallicRoughnessTexture"),
	"normal": (None, "normalTexture"),
	"occlusion": (None, "occlusionTexture"),
	"emissive": (None, "emissiveTexture"),
}

KINDS = ("colour", "normal", "data")


def fail(message):
	print(f"error: {message}", file=sys.stderr)
	sys.exit(1)


def read_glb(path):
	"""Returns (json document, offset of the BIN chunk data in the file, BIN length)."""
	with open(path, "rb") as handle:
		magic, version, length = struct.unpack("<III", handle.read(12))

		if magic != GLB_MAGIC or version != 2:
			fail(f"'{path}' is not a GLB 2.0 file")

		json_length, json_type = struct.unpack("<II", handle.read(8))

		if json_type != CHUNK_JSON:
			fail(f"'{path}': the first chunk is not JSON")

		document = json.loads(handle.read(json_length))
		bin_offset = 0
		bin_length = 0

		if handle.tell() + 8 <= length:
			chunk_length, chunk_type = struct.unpack("<II", handle.read(8))

			if chunk_type != CHUNK_BIN:
				fail(f"'{path}': the second chunk is not BIN")

			bin_offset = handle.tell()
			bin_length = chunk_length

	return document, bin_offset, bin_length


def texture_slot(material, role):
	"""Returns the texture-info dict of a role, or None."""
	parent, key = ROLES[role]
	holder = material.get(parent, {}) if parent else material

	return holder.get(key)


def set_texture_slot(material, role, info):
	parent, key = ROLES[role]

	if parent:
		material.setdefault(parent, {})[key] = info
	else:
		material[key] = info


def recipe_arguments(recipe, sources):
	"""Builds the EmeraudeTextureBake arguments of a recipe (paths resolved under --sources)."""
	kind = recipe.get("kind")

	if kind not in KINDS:
		fail(f"a recipe has kind '{kind}' (expected one of {KINDS})")

	arguments = ["--kind", kind]

	if recipe.get("flipNormalY", False):
		arguments.append("--flip-normal-y")

	for channel in ("r", "g", "b", "a"):
		value = recipe.get(channel)

		if value is None:
			continue

		if not value.startswith("#"):
			path, _, letter = value.rpartition(":")
			full = os.path.join(sources, path)

			if not os.path.isfile(full):
				fail(f"the source '{full}' does not exist")

			value = f"{full}:{letter}"

		arguments += [f"--{channel}", value]

	return arguments


def baker_identity(baker):
	"""The hash of the baker executable: a rebuilt baker (another encoder, another mip chain) bakes again."""
	digest = hashlib.sha256()

	with open(baker, "rb") as handle:
		for block in iter(lambda: handle.read(1 << 20), b""):
			digest.update(block)

	return digest.hexdigest()


def recipe_key(recipe, identity, zstd):
	payload = json.dumps({"recipe": recipe, "baker": identity, "zstd": zstd}, sort_keys=True)

	return hashlib.sha256(payload.encode()).hexdigest()[:20]


def bake_all(recipes, sources, baker, cache, jobs, zstd):
	"""Bakes every recipe not already in the cache; returns {key: path}."""
	os.makedirs(cache, exist_ok=True)
	outputs = {key: os.path.join(cache, f"{key}.ktx2") for key in recipes}
	pending = [key for key in recipes if not os.path.isfile(outputs[key])]

	def run(key):
		temporary = outputs[key] + ".part"
		command = [baker, "--output", temporary, "--zstd", str(zstd)] + recipe_arguments(recipes[key], sources)
		result = subprocess.run(command, capture_output=True, text=True)

		if result.returncode != 0:
			return key, result.stderr.strip() or result.stdout.strip()

		os.replace(temporary, outputs[key])

		return key, None

	print(f"{len(recipes)} distinct textures, {len(pending)} to bake ({jobs} at a time)")

	with concurrent.futures.ThreadPoolExecutor(max_workers=jobs) as pool:
		failures = [(key, error) for key, error in pool.map(run, pending) if error is not None]

	for key, error in failures:
		print(f"bake failed for {json.dumps(recipes[key])}:\n{error}", file=sys.stderr)

	if failures:
		fail(f"{len(failures)} texture(s) failed to bake")

	return outputs


def aligned(value):
	return (value + ALIGNMENT - 1) // ALIGNMENT * ALIGNMENT


def main():
	parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
	parser.add_argument("input")
	parser.add_argument("manifest")
	parser.add_argument("--sources", required=True, help="the directory the manifest's paths are relative to")
	parser.add_argument("--baker", required=True, help="the EmeraudeTextureBake executable")
	parser.add_argument("--out", required=True)
	parser.add_argument("--cache", help="where the baked containers are kept (default: OUT.bake-cache)")
	parser.add_argument("--jobs", type=int, default=os.cpu_count() or 1)
	parser.add_argument("--zstd", type=int, default=18, help="zstd supercompression level of the containers (0-22)")
	arguments = parser.parse_args()

	if os.path.abspath(arguments.input) == os.path.abspath(arguments.out):
		fail("--out must differ from the input (the input is read while the output is written)")

	document, bin_offset, bin_length = read_glb(arguments.input)

	with open(arguments.manifest, encoding="utf-8") as handle:
		manifest = json.load(handle).get("materials", {})

	materials = document.get("materials", [])
	by_name = {}

	for index, material in enumerate(materials):
		by_name.setdefault(material.get("name"), []).append(index)

	# Every textured slot must be covered, and only the glTF slots this tool knows may carry a texture.
	for index, material in enumerate(materials):
		name = material.get("name")

		for role in ROLES:
			if texture_slot(material, role) is not None and role not in manifest.get(name, {}):
				fail(f"material '{name}' (#{index}) has a {role} texture the manifest does not cover")

		for extension in material.get("extensions", {}).values():
			if "Texture" in json.dumps(extension):
				fail(f"material '{name}' (#{index}) has a texture inside an extension: not handled")

	unknown = [name for name in manifest if name not in by_name]

	if unknown:
		fail(f"the manifest names materials the GLB does not have: {unknown}")

	if not os.path.isfile(arguments.baker):
		fail(f"the baker '{arguments.baker}' does not exist")

	identity = baker_identity(arguments.baker)
	recipes = {}

	for name, roles in manifest.items():
		for role, recipe in roles.items():
			if role not in ROLES:
				fail(f"material '{name}': unknown role '{role}' (expected one of {list(ROLES)})")

			recipes[recipe_key(recipe, identity, arguments.zstd)] = recipe

	cache = arguments.cache or arguments.out + ".bake-cache"
	baked = bake_all(recipes, os.path.abspath(arguments.sources), arguments.baker, cache, max(1, arguments.jobs), arguments.zstd)

	# The views the old images used go; every other view is kept, in order.
	image_views = {image["bufferView"] for image in document.get("images", []) if "bufferView" in image}
	old_views = document.get("bufferViews", [])
	kept = [index for index in range(len(old_views)) if index not in image_views]
	view_map = {old: new for new, old in enumerate(kept)}

	for accessor in document.get("accessors", []):
		if "bufferView" in accessor:
			accessor["bufferView"] = view_map[accessor["bufferView"]]

		if "sparse" in accessor:
			fail("sparse accessors are not handled")

	# The byte ranges of buffer 0 that survive: plain views and meshopt-compressed ranges.
	ranges = []
	new_views = []

	for old in kept:
		view = dict(old_views[old])
		meshopt = view.get("extensions", {}).get("EXT_meshopt_compression")

		if view.get("buffer") == 0:
			ranges.append((view, "byteOffset", view.get("byteOffset", 0), view["byteLength"]))

		if meshopt is not None and meshopt.get("buffer") == 0:
			meshopt = dict(meshopt)
			view["extensions"] = dict(view["extensions"], EXT_meshopt_compression=meshopt)
			ranges.append((meshopt, "byteOffset", meshopt.get("byteOffset", 0), meshopt["byteLength"]))

		new_views.append(view)

	# The new images, one per distinct recipe actually used; the textures, one per (sampler, image).
	images = []
	image_of = {}
	textures = []
	texture_of = {}
	old_textures = document.get("textures", [])

	for name, roles in manifest.items():
		for material_index in by_name[name]:
			material = materials[material_index]

			for role, recipe in roles.items():
				key = recipe_key(recipe, identity, arguments.zstd)

				if key not in image_of:
					image_of[key] = len(images)
					images.append({"name": f"{name}.{role}", "mimeType": "image/ktx2", "bufferView": None, "_key": key})

				info = dict(texture_slot(material, role) or {})
				sampler = old_textures[info["index"]].get("sampler") if "index" in info and info["index"] < len(old_textures) else None
				pair = (sampler, image_of[key])

				if pair not in texture_of:
					texture_of[pair] = len(textures)
					texture = {"extensions": {"KHR_texture_basisu": {"source": image_of[key]}}}

					if sampler is not None:
						texture["sampler"] = sampler

					textures.append(texture)

				info["index"] = texture_of[pair]
				set_texture_slot(material, role, info)

	# Write: the kept ranges of buffer 0, then the containers, each aligned.
	position = 0
	layout = []

	for holder, field, offset, length in ranges:
		position = aligned(position)
		layout.append((position, offset, length))
		holder[field] = position
		position += length

	appended = []

	for image in images:
		path = baked[image.pop("_key")]
		size = os.path.getsize(path)
		position = aligned(position)
		image["bufferView"] = len(new_views)
		new_views.append({"buffer": 0, "byteOffset": position, "byteLength": size})
		appended.append((position, path))
		position += size

	total = aligned(position)
	document["bufferViews"] = new_views
	document["images"] = images
	document["textures"] = textures
	document["buffers"][0]["byteLength"] = total

	for extension_list in ("extensionsUsed", "extensionsRequired"):
		values = document.setdefault(extension_list, [])

		if "KHR_texture_basisu" not in values:
			values.append("KHR_texture_basisu")

	json_bytes = json.dumps(document, separators=(",", ":")).encode()
	json_bytes += b" " * ((4 - len(json_bytes) % 4) % 4)
	glb_length = 12 + 8 + len(json_bytes) + 8 + total

	if glb_length >= 2 ** 32:
		fail(f"the output would be {glb_length} bytes: past the 4 GiB a GLB can address")

	temporary = arguments.out + ".part"

	with open(arguments.input, "rb") as source, open(temporary, "wb") as output:
		output.write(struct.pack("<III", GLB_MAGIC, 2, glb_length))
		output.write(struct.pack("<II", len(json_bytes), CHUNK_JSON))
		output.write(json_bytes)
		output.write(struct.pack("<II", total, CHUNK_BIN))
		written = 0

		def pad_to(target):
			nonlocal written
			output.write(b"\0" * (target - written))
			written = target

		for new_offset, old_offset, length in layout:
			if old_offset + length > bin_length:
				fail("a view reaches past the input's BIN chunk")

			pad_to(new_offset)
			source.seek(bin_offset + old_offset)
			remaining = length

			while remaining > 0:
				block = source.read(min(remaining, 1 << 24))

				if not block:
					fail("the input's BIN chunk is truncated")

				output.write(block)
				remaining -= len(block)

			written += length

		for new_offset, path in appended:
			pad_to(new_offset)

			with open(path, "rb") as container:
				data = container.read()

			output.write(data)
			written += len(data)

		pad_to(total)

	os.replace(temporary, arguments.out)
	print(f"{arguments.out}: {len(images)} images, {len(textures)} textures, {glb_length / 1e6:.1f} MB")


if __name__ == "__main__":
	main()
