---
id: model-embedded-image-sources
title: Images from glTF / FBX / USD models get a source, so their CPU copy can be released and reloaded
status: in-progress
priority: unranked
scope: Graphics/ImageResource, Scenes/Loaders (GLTFLoader, FBXLoader, USDLoader)
opened: 2026-10-04
tags: [memory, resources, loaders]
---

# Images from glTF / FBX / USD models get a source, so their CPU copy can be released and reloaded

## Why

The CPU-copy release (`docs/subsystems/resources/11-cpu-copy-release.md`) never releases an image without a store
source; the images a scene loader creates from a model have none (`getOrCreateResource()` lambdas calling
`load(pixmap)`), so they stay resident: citadel 81 MiB (CarConcept.glb), liminal 228 MiB (DragonPigHigh.glb, Fox.glb,
the Paladin's base_model.fbx), the Fox + Paladin demos 36 MiB. Their texture is BC7 on every BC-capable GPU, so no GPU
readback can give the pixels back.

## Owner decisions (2026-10-04)

- **Coverage: files + byte ranges + FBX.** An `ImageResource` gets an ENCODED SOURCE set by the loader: a whole
  external file (glTF `uri`, FBX / USD external textures), a BYTE RANGE of a file (a glTF bufferView inside a `.glb`'s
  BIN chunk or an external `.bin`, a USDZ entry — stored uncompressed), or an FBX RE-PARSED with its geometry and
  animation ignored for its embedded content (ufbx gives no file offset). Stay resident: base64 `data:` URIs, the
  KTX2 fallback of a GPU without BC.
- **Storage: in `ImageResource`**, a small structure of its own (path, range, format, read options) set by the
  loader; `reloadLocalDataFromSource()` tries it after the store entry. `BaseInformation` stays a store entry.

## What remains

Implemented and verified on Linux 2026-10-04 (`docs/subsystems/resources/03` § CPU Copies, `11-cpu-copy-release.md`):
base `IO::fileGetRange()` (+2 tests, 2330 green Release + ASan/UBSan), `EncodedSource` in `ImageResource`, the
glTF / FBX / USD loaders; every kind reloaded with IDENTICAL pixels; census citadel 418, liminal 500, terrain 493 MiB,
0 VUID. **Left: the macOS / Windows validation**, then this file is deleted. Not exercised for lack of an asset: an
FBX with EXTERNAL textures (the same whole-file path as the glTF and USD ones, which were).

## References

- Loaders: `GLTFLoader::loadImages()` (GLTFLoader.cpp ~2065-2264), `FBXLoader::loadImages()` (FBXLoader.cpp ~374-494),
  `USDLoader::archiveTexture()` / `resolveTexture` (USDLoader.cpp ~1281-1500).
