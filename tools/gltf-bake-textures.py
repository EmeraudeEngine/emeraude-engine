#!/usr/bin/env python3
"""
gltf-bake-textures.py — replace the textures of a glTF (a GLB, or a .gltf and its files) with KTX2 containers baked for
Emeraude (graphics § 39, the baked assets, owner decision 2026-10-21; graphics § 40, L6, owner decision 2026-10-22).

Why this exists: a texture the engine loads from PNG / JPEG is repacked into the canonical layout and block-compressed
at load time (cached afterwards); one shipped as Basis Universal is transcoded at every load and cannot be repacked.
A baked texture is the result of the runtime's own pipeline written to disk once — EmeraudeTextureBake runs
PackRecipe::fillChannel(), TextureCompressor (BC7 / BC5 / BC4, full mip chain) and KTX2Writer — so the engine uploads
it as stored: no decode, no transcode, no encode.

⚠️ The output is meant for Emeraude only: its KTX2 images carry raw BC blocks behind KHR_texture_basisu, which requires
Basis Universal payloads. Keep the source asset for anything else.

Usage:
    gltf-bake-textures.py IN.glb MANIFEST.json --sources DIR --baker PATH --out OUT.glb [--cache DIR] [--jobs N] [--zstd N]
    gltf-bake-textures.py IN.gltf [MANIFEST.json --sources DIR | --flip-normal-y] --baker PATH --out OUT.gltf
                          [--textures DIR] [--cache DIR] [--jobs N] [--zstd N]

The manifest names, per material, what each texture of the canonical layout is made of (paths relative to --sources):

    {"materials": {
        "brickwall_01": {
            "baseColor":         {"kind": "colour", "r": "textures/brick_BaseColor.png:r", "g": "...:g", "b": "...:b", "a": "...:a"},
            "normal":            {"kind": "normal", "flipNormalY": true, "r": "textures/brick_Normal.png:r", "g": "...:g", "b": "...:b"},
            "metallicRoughness": {"kind": "data", "r": "#255", "g": "textures/brick_MR.png:g", "b": "textures/brick_MR.png:b"}
        }}}

Roles: baseColor, metallicRoughness, normal, occlusion, emissive (the glTF texture slots). A role absent from a material
keeps nothing: every textured slot of the glTF must be covered, or the tool refuses (nothing is silently left behind).
A channel not given is 255. The occlusion of the canonical layout is the R of the metallicRoughness texture: give the same
recipe to both roles to share one image (identical recipes are baked once and stored once).

Without a manifest (a .gltf only), the recipes are read from the glTF itself: every textured slot bakes its own image file
(its uri, relative to the .gltf) channel for channel, the metallicRoughness and occlusion slots sharing one image — R the
occlusion's R, G and B the metallicRoughness texture's G and B, 255 for a slot the material does not have.
--flip-normal-y converts Direct3D normal maps: the glTF convention is OpenGL's, give it only for an asset that breaks it
(Intel's Sponza packages do).

GLB: the binary chunk is rebuilt — the old images are dropped, the kept views (EXT_meshopt_compression ranges included)
are copied, the KTX2 containers appended.
.gltf: the buffers are not touched, their uris rewritten relative to the output, which must sit in a directory holding
them (the loader confines every uri to the asset's directory); the containers are written as files in --textures (default:
the output's name, '.textures' for its extension) and the images point to them. The views of embedded old images stay in
the buffers, unused. A container of that directory the output does not use is reported, never deleted.

--cache keeps the baked containers, named by their recipe, the content of their sources, the baker's hash and the zstd
level: a rerun only bakes what changed.
"""

import argparse
import concurrent.futures
import filecmp
import hashlib
import json
import os
import re
import shutil
import struct
import subprocess
import sys
import urllib.parse

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


def read_gltf(path):
	"""Returns the json document of a .gltf."""
	try:
		with open(path, encoding="utf-8") as handle:
			document = json.load(handle)
	except (OSError, ValueError) as error:
		fail(f"'{path}' is not a readable glTF: {error}")

	if not isinstance(document, dict):
		fail(f"'{path}' is not a glTF document")

	return document


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


def material_label(material, index):
	"""The name of a material, or its index when it has none."""
	name = material.get("name")

	return name if isinstance(name, str) and name else f"material{index}"


def is_inside(relative):
	"""Whether a relative path stays under its base (no '..' escape, not absolute)."""
	return not os.path.isabs(relative) and relative != os.pardir and not relative.startswith(os.pardir + os.sep)


def recipe_sources(recipe, sources):
	"""Yields the full path of every image file a recipe reads."""
	for channel in ("r", "g", "b", "a"):
		value = recipe.get(channel)

		if value is None:
			continue

		if not isinstance(value, str):
			fail(f"a recipe's channel '{channel}' is not a string: {json.dumps(recipe)}")

		if not value.startswith("#"):
			yield os.path.join(sources, value.rpartition(":")[0])


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


def file_hash(path):
	digest = hashlib.sha256()

	with open(path, "rb") as handle:
		for block in iter(lambda: handle.read(1 << 20), b""):
			digest.update(block)

	return digest.hexdigest()


def recipe_key(recipe, contents, identity, zstd):
	"""The cache name of a recipe: its text, the content of its sources, the baker's hash, the zstd level."""
	payload = json.dumps({"recipe": recipe, "sources": contents, "baker": identity, "zstd": zstd}, sort_keys=True)

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


def manifest_assignments(manifest, materials):
	"""Returns [(material index, role, recipe)] from a manifest, in its order."""
	by_name = {}

	for index, material in enumerate(materials):
		by_name.setdefault(material.get("name"), []).append(index)

	# Every textured slot must be covered.
	for index, material in enumerate(materials):
		name = material.get("name")

		for role in ROLES:
			if texture_slot(material, role) is not None and role not in manifest.get(name, {}):
				fail(f"material '{name}' (#{index}) has a {role} texture the manifest does not cover")

	unknown = [name for name in manifest if name not in by_name]

	if unknown:
		fail(f"the manifest names materials the glTF does not have: {unknown}")

	assignments = []

	for name, roles in manifest.items():
		for role in roles:
			if role not in ROLES:
				fail(f"material '{name}': unknown role '{role}' (expected one of {list(ROLES)})")

		for material_index in by_name[name]:
			assignments += [(material_index, role, recipe) for role, recipe in roles.items()]

	return assignments


def image_file(document, info, base, where):
	"""Returns the path, relative to base, of the image file a texture-info reads."""
	textures = document.get("textures", [])
	index = info.get("index")

	if not isinstance(index, int) or not 0 <= index < len(textures):
		fail(f"{where}: no texture #{index}")

	texture = textures[index]

	if texture.get("extensions"):
		fail(f"{where}: texture #{index} has an extension (an already compressed image?): give a manifest")

	images = document.get("images", [])
	source = texture.get("source")

	if not isinstance(source, int) or not 0 <= source < len(images):
		fail(f"{where}: texture #{index} has no image")

	uri = images[source].get("uri")

	if not isinstance(uri, str) or uri.startswith("data:"):
		fail(f"{where}: image #{source} is not a file (embedded): give a manifest")

	path = os.path.normpath(urllib.parse.unquote(uri))

	if not is_inside(path):
		fail(f"{where}: image #{source} ('{uri}') is outside the glTF's directory")

	if not os.path.isfile(os.path.join(base, path)):
		fail(f"{where}: image #{source} ('{uri}') does not exist")

	return path.replace(os.sep, "/")


def derived_assignments(document, materials, base, flip_normal_y):
	"""Returns [(material index, role, recipe)] read from the glTF's own image files (paths relative to base)."""
	assignments = []

	for index, material in enumerate(materials):
		where = f"material '{material_label(material, index)}' (#{index})"
		paths = {}

		for role in ROLES:
			info = texture_slot(material, role)

			if info is not None:
				paths[role] = image_file(document, info, base, f"{where} {role}")

		if "baseColor" in paths:
			path = paths["baseColor"]
			assignments.append((index, "baseColor", {"kind": "colour", **{channel: f"{path}:{channel}" for channel in "rgba"}}))

		if "emissive" in paths:
			path = paths["emissive"]
			assignments.append((index, "emissive", {"kind": "colour", **{channel: f"{path}:{channel}" for channel in "rgb"}}))

		if "normal" in paths:
			path = paths["normal"]
			recipe = {"kind": "normal", **{channel: f"{path}:{channel}" for channel in "rgb"}}

			if flip_normal_y:
				recipe["flipNormalY"] = True

			assignments.append((index, "normal", recipe))

		roughness = paths.get("metallicRoughness")
		occlusion = paths.get("occlusion")

		if roughness is not None and occlusion is not None:
			sets = {texture_slot(material, role).get("texCoord", 0) for role in ("metallicRoughness", "occlusion")}

			if len(sets) > 1:
				fail(f"{where}: its metallicRoughness and occlusion textures read different texCoord sets (one image cannot serve both): give a manifest")

		if roughness is not None or occlusion is not None:
			recipe = {
				"kind": "data",
				"r": f"{occlusion}:r" if occlusion is not None else "#255",
				"g": f"{roughness}:g" if roughness is not None else "#255",
				"b": f"{roughness}:b" if roughness is not None else "#255",
			}

			for role, path in (("metallicRoughness", roughness), ("occlusion", occlusion)):
				if path is not None:
					assignments.append((index, role, recipe))

	return assignments


def build_images(document, materials, assignments, key_of):
	"""Points every assigned slot to its new texture; returns the new images (one per distinct recipe) and textures (one
	per (sampler, image)). An image holds its recipe's key in '_key' for the writer."""
	images = []
	image_of = {}
	textures = []
	texture_of = {}
	old_textures = document.get("textures", [])

	for material_index, role, recipe in assignments:
		material = materials[material_index]
		key = key_of(recipe)

		if key not in image_of:
			image_of[key] = len(images)
			images.append({"name": f"{material_label(material, material_index)}.{role}", "mimeType": "image/ktx2", "_key": key})

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

	return images, textures


def write_glb(document, old_images, images, baked, input_path, bin_offset, bin_length, out):
	"""Rebuilds the BIN chunk (the views of the old images dropped, the containers appended) and writes the GLB."""
	if "KHR_draco_mesh_compression" in document.get("extensionsUsed", []):
		fail("KHR_draco_mesh_compression is not handled: its views would not be renumbered")

	# The views the old images used go; every other view is kept, in order.
	image_views = {image["bufferView"] for image in old_images if "bufferView" in image}
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
	document["buffers"][0]["byteLength"] = total

	json_bytes = json.dumps(document, separators=(",", ":")).encode()
	json_bytes += b" " * ((4 - len(json_bytes) % 4) % 4)
	glb_length = 12 + 8 + len(json_bytes) + 8 + total

	if glb_length >= 2 ** 32:
		fail(f"the output would be {glb_length} bytes: past the 4 GiB a GLB can address")

	temporary = out + ".part"

	with open(input_path, "rb") as source, open(temporary, "wb") as output:
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

	os.replace(temporary, out)

	return glb_length


def prepare_gltf(document, input_path, out, textures_directory):
	"""Rewrites the buffers' uris relative to the output and checks where the containers go, before anything is baked;
	returns (the textures directory, its uri from the output)."""
	input_directory = os.path.dirname(os.path.abspath(input_path))
	output_directory = os.path.dirname(os.path.abspath(out))

	def relative_to_output(full, what):
		try:
			relative = os.path.relpath(full, output_directory)
		except ValueError:
			relative = full

		if not is_inside(relative):
			fail(f"{what} is outside the output's directory '{output_directory}': the loader would refuse it")

		return relative.replace(os.sep, "/")

	for index, buffer in enumerate(document.get("buffers", [])):
		uri = buffer.get("uri")

		if not isinstance(uri, str):
			fail(f"buffer #{index} has no uri (a .gltf holds no BIN chunk)")

		if uri.startswith("data:"):
			continue

		full = os.path.normpath(os.path.join(input_directory, urllib.parse.unquote(uri)))
		buffer["uri"] = urllib.parse.quote(relative_to_output(full, f"buffer #{index} ('{uri}')"))

	directory = os.path.abspath(textures_directory)

	return directory, relative_to_output(directory, f"the textures directory '{directory}'")


def write_gltf(document, images, baked, out, directory, directory_uri):
	"""Writes the containers as sidecar files in directory, points the images to them, then writes the .gltf."""
	os.makedirs(directory, exist_ok=True)
	used = set()

	for image in images:
		key = image.pop("_key")
		stem = re.sub(r"[^A-Za-z0-9_.-]", "_", image["name"])

		# NOTE: Compared without case: two names differing only by it are one file on Windows and macOS.
		for file_name in (f"{stem}.ktx2", f"{stem}.{key[:8]}.ktx2", f"{stem}.{key}.ktx2"):
			if file_name.lower() not in used:
				break

		used.add(file_name.lower())
		target = os.path.join(directory, file_name)

		if not (os.path.isfile(target) and filecmp.cmp(target, baked[key], shallow=False)):
			shutil.copyfile(baked[key], target + ".part")
			os.replace(target + ".part", target)

		image["uri"] = urllib.parse.quote(f"{directory_uri}/{file_name}")

	for file_name in sorted(os.listdir(directory)):
		if file_name.endswith(".ktx2") and file_name.lower() not in used:
			print(f"warning: '{os.path.join(directory, file_name)}' is not used by the output (left in place)")

	temporary = out + ".part"

	with open(temporary, "w", encoding="utf-8") as handle:
		json.dump(document, handle, indent=1, ensure_ascii=False)
		handle.write("\n")

	os.replace(temporary, out)


def main():
	parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
	parser.add_argument("input")
	parser.add_argument("manifest", nargs="?", help="mandatory for a GLB; without it, a .gltf bakes its own images")
	parser.add_argument("--sources", help="the directory the manifest's paths are relative to")
	parser.add_argument("--baker", required=True, help="the EmeraudeTextureBake executable")
	parser.add_argument("--out", required=True)
	parser.add_argument("--textures", help="a .gltf: where the containers are written (default: OUT with '.textures' for its extension)")
	parser.add_argument("--flip-normal-y", action="store_true", help="without a manifest: the normal maps are Direct3D's")
	parser.add_argument("--cache", help="where the baked containers are kept (default: OUT.bake-cache)")
	parser.add_argument("--jobs", type=int, default=os.cpu_count() or 1)
	parser.add_argument("--zstd", type=int, default=18, help="zstd supercompression level of the containers (0-22)")
	arguments = parser.parse_args()

	extension = os.path.splitext(arguments.input)[1].lower()

	if extension not in (".glb", ".gltf"):
		fail("the input must be a .glb or a .gltf")

	if os.path.splitext(arguments.out)[1].lower() != extension:
		fail(f"--out must be a {extension}, as the input")

	if os.path.abspath(arguments.input) == os.path.abspath(arguments.out):
		fail("--out must differ from the input (the input is read while the output is written)")

	if (arguments.manifest is None) != (arguments.sources is None):
		fail("a manifest and --sources go together")

	if extension == ".glb" and arguments.manifest is None:
		fail("a GLB needs a manifest (its images are not files)")

	if arguments.manifest is not None and arguments.flip_normal_y:
		fail("--flip-normal-y is for a .gltf without manifest (a manifest's recipe says flipNormalY)")

	if extension == ".glb" and arguments.textures is not None:
		fail("--textures is for a .gltf (a GLB holds its containers)")

	if extension == ".glb":
		document, bin_offset, bin_length = read_glb(arguments.input)
	else:
		document = read_gltf(arguments.input)

	materials = document.get("materials", [])

	# Only the glTF slots this tool knows may carry a texture.
	for index, material in enumerate(materials):
		for extension_name, extension_value in material.get("extensions", {}).items():
			if "Texture" in json.dumps(extension_value):
				fail(f"material '{material_label(material, index)}' (#{index}) has a texture inside {extension_name}: not handled")

	if arguments.manifest is not None:
		with open(arguments.manifest, encoding="utf-8") as handle:
			manifest = json.load(handle).get("materials", {})

		sources = os.path.abspath(arguments.sources)
		assignments = manifest_assignments(manifest, materials)
	else:
		sources = os.path.dirname(os.path.abspath(arguments.input))
		assignments = derived_assignments(document, materials, sources, arguments.flip_normal_y)

	if not os.path.isfile(arguments.baker):
		fail(f"the baker '{arguments.baker}' does not exist")

	identity = file_hash(arguments.baker)
	hashes = {}

	def key_of(recipe):
		contents = []

		for path in recipe_sources(recipe, sources):
			if path not in hashes:
				if not os.path.isfile(path):
					fail(f"the source '{path}' does not exist")

				hashes[path] = file_hash(path)

			contents.append(hashes[path])

		return recipe_key(recipe, contents, identity, arguments.zstd)

	if extension == ".gltf":
		textures_directory = arguments.textures or os.path.splitext(arguments.out)[0] + ".textures"
		directory, directory_uri = prepare_gltf(document, arguments.input, arguments.out, textures_directory)

	recipes = {key_of(recipe): recipe for _, _, recipe in assignments}
	cache = arguments.cache or arguments.out + ".bake-cache"
	baked = bake_all(recipes, sources, arguments.baker, cache, max(1, arguments.jobs), arguments.zstd)
	images, textures = build_images(document, materials, assignments, key_of)
	old_images = document.get("images", [])
	document["images"] = images
	document["textures"] = textures

	for extension_list in ("extensionsUsed", "extensionsRequired"):
		values = document.setdefault(extension_list, [])

		if "KHR_texture_basisu" not in values:
			values.append("KHR_texture_basisu")

	if extension == ".glb":
		size = write_glb(document, old_images, images, baked, arguments.input, bin_offset, bin_length, arguments.out)
		print(f"{arguments.out}: {len(images)} images, {len(textures)} textures, {size / 1e6:.1f} MB")
	else:
		write_gltf(document, images, baked, arguments.out, directory, directory_uri)
		size = sum(os.path.getsize(baked[key]) for key in recipes)
		print(f"{arguments.out}: {len(images)} images ({size / 1e6:.1f} MB in '{textures_directory}'), {len(textures)} textures")


if __name__ == "__main__":
	main()
