---
id: material-bake-parallel
title: Bake the materials of a store in parallel
status: open
priority: unranked
scope: tools/texture-bake/MaterialBake.cpp
opened: 2026-10-22
tags: [textures, tools]
---

# Bake the materials of a store in parallel

## Why

`EmeraudeTextureBake --materials` (graphics § 40, L3) bakes the definitions one after the other (Metals: 13 in 13.7 s).
A finished product's store (T15) holds hundreds; the glTF tool already bakes in parallel.

## What remains

- Bake the entries on a bounded pool (the encoder is already multi-threaded per texture: measure which level pays).

## References

- `docs/subsystems/graphics/40-gpu-textures-store.md` § Implementation (L3).
