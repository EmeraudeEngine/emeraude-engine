---
id: forest-canopy-far-field-proxy
title: Far-field forest canopy — one coarse baked shell per wooded tile, beyond the imposters
status: parked
priority: unranked
scope: Scenes/Toolkit (vegetation), Graphics
opened: 2026-09-25
tags: [vegetation, lod, imposters, terrain, performance]
---

# Far-field forest canopy — one coarse baked shell per wooded tile

## Why

Owner idea, 2026-09-25, from Black & White (Lionhead, 2001): single trees up close, and a far forest
drawn as one big mesh with a forest texture on it. The same trick applied to `terrain`'s very distant
woods: generate, at load, a coarse mesh standing for each wooded area and bake a texture onto it. It
only has to "look like" the forest from far away, unlike the octahedral imposters.

It would also fix what the demo shows today: the forest stops dead at `TreeDrawDistance` (6 km) and
the land is bare beyond.

**Parked** by the owner: "it was an idea, note it". Not scheduled.

## The design discussed (not decided)

- **Shape**: a 2.5D canopy shell. Its height field is "ground + canopy height" over the woods,
  skirted down to the ground at the edges. From 3 km a forest reads as a bumpy blanket on the relief,
  and its outline on the ridges sells it.
- **How to build it**:
  - CPU, analytic: heights from the crown extents, colour from the species' mean tint plus noise,
    normals from the height field;
  - or GPU bake: an orthographic top view of the real trees per tile gives colour, normal and depth.
  - Either way, bake ALBEDO and NORMALS, never lit colour: `terrain`'s sun moves.
- **Granularity**: tiles of 500 m-1 km (at most ~290 draws for the 16.4 km map). The 62.5 m planting
  cells (12 776) are far too fine.
- **Alternative**: fold it into the CDLOD terrain beyond X km. The vertex stage adds a map-wide
  canopy height, the fragment stage blends to a forest albedo: zero extra draws, silhouette and
  shadows for free. But the terrain would then know about vegetation (an architecture decision), and
  its far vertex density limits the detail.
- **Transition**: a dithered cross-fade with the imposters at 2-3 km. The imposters could then stop
  there instead of 6 km, and the canopy reach the horizon.
- Recommendation given: one mesh per 1 km tile, CPU analytic first, GPU bake only if it does not look
  close enough.

## What remains (when unparked)

1. **Measure first**: what the 37 335 imposter visuals cost. A/B with `terrain` option 5 = 1 (no
   imposters) gives the ceiling of the gain. At the spawn pose, SSGI (7.5 ms) and the shadow cascades
   (2.9 ms) outweigh the scene pass (4-6 ms).
2. State of the art before coding: Unreal's HLOD (Hierarchical LOD: proxy mesh + baked material) and
   *Billboard Clouds for Extreme Model Simplification* (Décoret, Durand, Sillion, Dorsey, SIGGRAPH
   2003).

## References

- projet-alpha `src/Builtin/Terrain.cpp` (`plantForest`, `TreeDrawDistance`, `ImposterDistance`),
  `src/Builtin/AGENTS.md` § 6d.
