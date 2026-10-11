---
id: imposter-bake-wind-frozen
title: Octahedral imposters — freeze the wind during the bake (a strong wind smears the views)
status: open
priority: medium
scope: Graphics / Scenes (Toolkit::bakeTreeImposter, vegetation wind)
opened: 2026-09-21
tags: [vegetation, lod, imposter]
---

# Octahedral imposters — freeze the wind during the bake

## Why

The 64 copies of the tree sway with the scene's wind while they are baked. The 1.15 rig margin absorbs `terrain`'s
wind; a stronger wind smears the views (graphics § 15e). The implementation plan already called for the wind frozen
during the bake.

## What remains

- Bake with the vegetation wind amplitude at 0 for the bake subject only (the scene keeps its wind), or a bake-only
  wind override.
- Proof: the same species baked under `terrain`'s wind and under a strong wind give the same atlas (per-pixel diff).

## References

- Graphics § 15e (`docs/subsystems/graphics/24-15e-octahedral-imposters-the-last-rung-of-a-tree.md`).
