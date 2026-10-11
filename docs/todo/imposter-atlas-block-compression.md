---
id: imposter-atlas-block-compression
title: Octahedral imposters — compress the atlases (BC7 / BC5), ~200 MB uncompressed today
status: open
priority: medium
scope: Graphics (ImposterAtlas), emeraude-base TextureCompressor
opened: 2026-09-21
tags: [vegetation, lod, imposter, memory]
---

# Octahedral imposters — compress the atlases

## Why

12 variants × (albedo RGBA8 + normal RGBA16F, 5 mips) ≈ 200 MB of VRAM uncompressed (graphics § 15e). The 8 GB VRAM
ceiling is a design constraint; BC7 (albedo + coverage) and BC5 (normal XY) would divide it.

## What remains

- Compress after the bake (the atlas is baked at load: the compression cost lands in the load hitch), or decide a GPU
  compressor.
- Check the coverage edge survives BC7 (premultiplied albedo), and the mip chain (emeraude-base `generateMip()` is the
  mip filter since the BC7 fix, caution-points § *The BC7 mip chain lost the mean*).
- Measure the VRAM before/after and the load time.

## References

- Graphics § 15e (`docs/subsystems/graphics/24-15e-octahedral-imposters-the-last-rung-of-a-tree.md`).
