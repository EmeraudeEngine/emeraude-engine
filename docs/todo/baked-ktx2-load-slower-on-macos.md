---
id: baked-ktx2-load-slower-on-macos
title: A baked GLB (raw BC KTX2) loads ~1.5 s slower than its UASTC version on macOS, faster elsewhere
status: open
priority: unranked
scope: Graphics/KTX2Decoder, CompressedImageResource, Scenes/Loaders/GLTFLoader (the KTX2 load path), macOS
opened: 2026-10-21
tags: [textures, loading, macos, texture-packing]
---

# A baked GLB loads slower than its UASTC version on macOS

## Why

The baked assets (graphics § 39 § Baked assets) skip the Basis transcode: the KTX2 blocks are uploaded as stored. On
Linux and Windows `Sponza.baked.glb` loads as fast or faster than `Sponza.ktx2.glb` (Windows 10.1–10.8 s vs 11.3–11.4 s
to the last resource); on the macOS peer (M6, MoltenVK 1.4.2, warm cache, 2026-10-21) it ends ~1.5 s LATER (5.7 vs
4.2 s to the last resource, the scene itself 4.9 vs 1.9 s). The two files differ by their size (1.44 GB vs 1.19 GB) and by
their payload: zstd-supercompressed raw BC (poorly compressible) against zstd-supercompressed UASTC (RDO, compressible).

## What remains

- Profile the KTX2 path on macOS: the file read, the zstd inflate (libktx, per texture, on which thread), the upload;
  compare with the UASTC path, whose transcode is cheaper than expected there.
- Try a lower zstd level or none for raw BC (the blocks compress poorly: ~20 % saved), measured on the three OS.

## References

- `docs/subsystems/graphics/39-material-texture-packing.md` § Baked assets (the 3-OS measurements).
