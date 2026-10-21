---
id: gpu-data-texture-single-fetch
title: One fetch for a GPU data texture read by several components
status: open
priority: unranked
scope: Graphics/Material/StandardResource
opened: 2026-10-22
tags: [textures, performance]
---

# One fetch for a GPU data texture read by several components

## Why

Graphics § 39 D10: a `Packed/` data texture (AO, roughness, metalness, height) is fetched ONCE and its channels shared,
its readers sharing their UVs by construction. A `GPUTextures` data entry (L2, graphics § 40) is fetched once per
reader instead, because nothing yet proves its readers' UV transforms are equal.

## What remains

- Extend the single fetch to a `GPUTextures/` texture when every reader of it has the same UV set, scale and offset.

## References

- `docs/subsystems/graphics/40-gpu-textures-store.md` § Implementation (L2), `39-material-texture-packing.md` (D10).
