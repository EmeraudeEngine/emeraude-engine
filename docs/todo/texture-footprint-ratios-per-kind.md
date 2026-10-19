---
id: texture-footprint-ratios-per-kind
title: GPU footprint policy — a resolution ratio per texture kind, per-kind mip chains
status: open
priority: unranked
scope: Graphics/TextureResource, TextureCompressor, settings
opened: 2026-10-20
blocked-by: [material-texture-canonical-repack]
tags: [textures, memory, settings, texture-packing]
---

# A resolution ratio per texture kind

## Why

Decision D5 (graphics § 39): after the repack, one footprint policy for every loader — a global setting per
canonical kind (albedo, normal, data: 1, 1/2, 1/4…). Halving the resolution divides the VRAM by four. Load time is
not a constraint (owner): high-quality downscale filters are welcome.

## What remains

- Settings per kind (e.g. `Core/Graphics/Texture/AlbedoRatio`, `NormalRatio`, `DataRatio`), read at the repack;
  the ratio enters the `TextureCache` package descriptor (D7).
- A quality downscale filter (Kaiser / Lanczos class) instead of the box filter.
- Per-kind mip chains: the normal is renormalized per level (not its height in A), the albedo's A keeps its
  coverage (Castaño, already in `TextureCompressor` for binary masks), sRGB colour filtered in linear space.
- Proof: VRAM per ratio, the image at a pinned pose (per-pixel difference), against the reference.

## References

- `docs/subsystems/graphics/39-material-texture-packing.md`, `31-alpha-coverage-a-mask-is-not-a-gradient.md`,
  `texture-streaming` (the larger footprint problem).
