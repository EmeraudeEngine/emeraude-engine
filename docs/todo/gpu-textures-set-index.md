---
id: gpu-textures-set-index
title: An index of the GPUTextures store by set name
status: open
priority: unranked
scope: Graphics/GPUTextureSet, Resources
opened: 2026-10-22
tags: [textures, performance]
---

# An index of the GPUTextures store by set name

## Why

`GPUTextureSet::find()` (graphics § 40, L2) scans the store's names once per material (`availableResourceNames()`, a
copy). Cheap with a few hundred entries; a store of thousands (a finished product, T15) makes every material load pay a
full scan.

## What remains

- Build an index set name → entries once per store scan, and measure a material load before / after on a large store.

## References

- `docs/subsystems/graphics/40-gpu-textures-store.md` § Implementation (L2).
