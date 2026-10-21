---
id: mip-chain-quality-filter
title: A quality filter for the texture mip chain (Kaiser class, sRGB filtered in linear)
status: open
priority: unranked
scope: Graphics/TextureCompressor (generateMip), base PixelFactory::Processor
opened: 2026-10-21
tags: [textures, mipmaps, quality, texture-packing]
---

# A quality filter for the texture mip chain

## Why

Every mip level is an exact area-weighted box filter (`Processor::downsample()`) of the previous one, in the bytes'
space. Since D5 (graphics § 39 § GPU footprint per kind) a reduced texture IS one of those levels, so the chain's
filter decides both the look at distance and the look of a reduced texture. NVIDIA Texture Tools offers box, tent and
Kaiser-windowed sinc; the box is the softest. An sRGB colour averaged in its encoded space darkens the contrasts.

## What remains

- A Kaiser-class downsample (or Lanczos) in base `Processor`, tested (a constant stays constant, odd sizes, the mean
  kept within a bound), and `TextureCompressor` building the chain with it; an sRGB texture filtered in linear.
- Keep what the chain does per kind: BC5 renormalized per level, the coverage of an alpha mask, the colour dilation.
- A/B at a pinned pose (the near field of `relief` with every kind at 1, a distant view), `TextureCache` version bump.

## References

- `docs/subsystems/graphics/39-material-texture-packing.md` § GPU footprint per kind; NVIDIA Texture Tools mip filters.
