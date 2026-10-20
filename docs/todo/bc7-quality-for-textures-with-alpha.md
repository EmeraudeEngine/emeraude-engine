---
id: bc7-quality-for-textures-with-alpha
title: A finer BC7 for the textures with alpha (the packed albedo + opacity)
status: open
priority: unranked
scope: Graphics/TextureCompressor
opened: 2026-10-20
tags: [textures, compression, texture-packing]
---

# A finer BC7 for the textures with alpha

## Why

Packing the opacity into the albedo's A (graphics § 39) makes the colour and the mask share BC7 blocks: measured on
`Metals/Grid003` (relief, pinned exposure, packed vs not) at 0.66 against a 0.21 noise on macOS, 1.5 against 0.7 on
Windows, 1.42 against ~1.1 on Linux — all of it on the masked panels, no brightness bias. The owner KEPT the packing
(2026-10-20) and asked for the encoder to be refined later. `TextureCompressor` encodes with bc7enc_rdo at uber level 0
and 16 partitions (the "fast" preset, `TextureCompressor.cpp`), the same for every texture.

## What remains

- Measure a finer preset (higher uber level, 64 partitions) on the textures that HAVE an alpha only, against the
  same Grid003 capture: the image difference and the encode time (cold cache), the cache making it a one-time cost.
- The preset enters the `TextureCache` key (D7: the encoder settings are part of the package descriptor).

## References

- `docs/subsystems/graphics/39-material-texture-packing.md` § The repack, `30-compressed-image-path.md`.
