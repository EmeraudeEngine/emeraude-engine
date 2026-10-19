---
id: animated-texture-frames-uncompressed
title: AnimatedTexture2D uploads every frame uncompressed — the ocean normal map holds 640 MiB
status: open
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

- Give the animated textures the block-compression path of `Texture2D` (per layer, through the `TextureCache`), with
  the format chosen by the texture kind (D8: BC5 for a normal map, BC7 for colour).
- Decide with the owner whether 120 frames at 1024² is the intended budget for a looping normal map (fewer frames,
  a lower resolution, or a ratio from `texture-footprint-ratios-per-kind`).
- Proof: `getTextureMemory` before / after, the water surface at a pinned pose.

## References

- `docs/subsystems/graphics/39-material-texture-packing.md`, `12-11-animated-texture-cubemap-system.md`.
