---
id: gltf-ext-texture-webp-not-in-parser-mask
title: EXT_texture_webp is not in the glTF parser mask, so such an asset does not load at all
status: open
priority: unranked
scope: Scenes/Loaders/GLTFLoader, Graphics/ImageResource
opened: 2026-09-18
blocked-by: []
tags: [gltf, loader, measured]
---

# EXT_texture_webp is not in the glTF parser mask, so such an asset does not load at all

## Why

Found 2026-09-18 while sweeping the Khronos corpus for `KHR_draco_mesh_compression`.
`SunglassesKhronos/glTF-Draco` was the only one of the seventeen Draco variants that produced no
geometry — **not because of Draco**, which it uses on all eight of its meshes, but because it also
lists `EXT_texture_webp` in `extensionsRequired`.

fastgltf validates `extensionsRequired` against the mask given to its `Parser` and rejects the
**whole file** with `Error::MissingExtensions` when one entry is missing. The observed log line is:

```
[Error][GLTFLoader] Failed to parse '…/SunglassesKhronos.gltf' : One or more extensions are
required by the glTF but not enabled in the Parser.
```

So the failure mode is **absence, not degradation**: zero nodes, one error line. It is the same
all-or-nothing rule already documented for the compressed-glTF trio.

**It blocks the owner's own content, not just a conformance asset** (reported 2026-09-18):
`projet-alpha.data/data-stores/glTF/SheenWoodLeatherSofa.glb` requires
`['KHR_texture_transform', 'EXT_texture_webp']` and cannot be opened at all. `KHR_texture_transform`
is already declared, so `EXT_texture_webp` is the single blocker on that file.

**The diagnostic now names it** (`GLTFLoader::reportMissingExtensions`, 2026-09-18). fastgltf's own
message never said *which* extension was missing, and the asset is refused whole so nothing is left
to inspect; the loader now re-parses with every extension fastgltf knows, purely to read
`extensionsRequired`, and logs the difference against its own mask:

```
[Error][GLTFLoader] Required extension(s) this loader does not support : EXT_texture_webp.
```

That fixed the *diagnosis*, not the gap — the file still does not load.

## What remains

1. Declare `fastgltf::Extensions::EXT_texture_webp` in the parser mask of `GLTFLoader::load()`.
2. Wire the decode. The extension hangs the image off the texture the same way
   `KHR_texture_basisu` does (a separate image index), so `resolveTexture()` needs a third fallback
   leg, next to `imageIndex` and `basisuImageIndex`.
3. Decode WebP into a `Pixmap< uint8_t >`. **`libwebp` is already built by ext-deps-generator**
   (`libraries/libwebp.yaml`) — this is a decoder-wiring job, not a dependency one. A `SetupWebP`
   module in emeraude-base would follow `SetupPNG`/`SetupJPEG`.
4. A WebP image goes down the ordinary `ImageResource` path (CPU BC7 + `TextureCache`), unlike
   KTX2 which stays block-compressed end to end.

## ⚠️ Traps

- **`KHR_texture_basisu` and `EXT_texture_webp` are alternatives on the SAME texture**, both
  meaning "the plain `imageIndex` is absent". A texture reader that stops at the first of the two
  it knows leaves every material of the other kind untextured, with no error — the trap already
  recorded for basisu in `docs/caution-points.md`.
- Adding the extension to the mask **without** wiring the decode would be worse than today: the
  file would parse and the materials would come out silently untextured, instead of failing loudly.
  Do both, in one change.
- The corpus variant to verify against is `Models/SunglassesKhronos/glTF-Draco` (WebP **and**
  Draco), so it also exercises the two families together.

## References

- `src/Scenes/Loaders/GLTFLoader.cpp` — the parser mask, and `resolveTexture()`.
- `src/Scenes/Loaders/AGENTS.md` § *Known gaps (glTF 2.0)*.
- https://github.com/KhronosGroup/glTF/tree/main/extensions/2.0/Vendor/EXT_texture_webp
