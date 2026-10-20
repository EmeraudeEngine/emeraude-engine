---
id: animated-texture-frames-uncompressed
title: AnimatedTexture2D uploads every frame uncompressed — the ocean normal map holds 640 MiB
status: in-progress
priority: unranked
scope: Graphics/TextureResource (AnimatedTexture2D, AnimatedTextureCubemap), TextureCompressor
opened: 2026-10-20
tags: [textures, memory, compression, texture-packing]
---

# Animated textures are uploaded uncompressed

## Why

Measured 2026-10-20 (`getTextureMemory`, `water-world`, graphics § 39 § Reference measurement): the ocean's animated
normal map (`BuiltinOceanNormalMap`, a projet-alpha demo material built from the `WaterNormal` movie) is an
`AnimatedTexture2D` of 120 layers × 1024², 11 mips, `R8G8B8A8_UNORM` = **640.3 MiB**, 86 % of the scene's texture
memory. `AnimatedTexture2D.cpp:106` takes `Image::getFormat< uint8_t >(colorCount, sRGB)`: no block compression,
unlike `Texture2D` (BC7 on every BC-capable GPU). BC7 would divide it by 4 (160 MiB); BC5 fits a normal map
(`texture-block-format-per-kind`, D8).

## What remains

- DONE on Linux (2026-10-20, uncommitted): BC7 per frame through the texture cache, one BC7 array — the ocean 640.3 →
  160.3 MiB, water-world 734.1 → 242.2 MiB, 0 VUID (graphics § 39 § Animated textures in BC7). Owner decisions: BC7
  now, BC5 with D8 (`texture-block-format-per-kind`); 120 × 1024² kept.
- `AnimatedTextureCubemap` keeps its uncompressed upload (no measured need yet).
- Validation by the peers (macOS, Windows), then close.

## References

- `docs/subsystems/graphics/39-material-texture-packing.md`, `12-11-animated-texture-cubemap-system.md`.
