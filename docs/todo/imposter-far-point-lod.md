---
id: imposter-far-point-lod
title: A last level beyond the octahedral imposters — a single coloured point
status: open
priority: unranked
scope: Graphics/Renderable (imposters), Scenes (vegetation)
opened: 2026-09-25
tags: [vegetation, lod, imposter]
---

# A last level beyond the octahedral imposters — a single coloured point

## Why

Owner, 2026-09-25 (`terrain`): "past a large distance, could the octahedral imposter become a simple coloured
point?". A 20 m tree is ~9 px tall at 3 km and ~3.5 px at 8 km (2880 × 1620, 60°): an octahedral atlas lookup on
two triangles is then more than the pixel needs.

`terrain` now simply stops drawing its forest cells past `TreeDrawDistance` (6 km, the same night: "stop trying to
render the trees past some distance"). A point level would let the woods stay visible to the horizon for almost
nothing, instead of ending at a line.

## What remains

- Decide the representation (a point list with the imposter's mean colour per view direction, or the imposter's
  lowest atlas mip) and the switch distance (projected size ~2-3 px).
- Measure first: on `terrain` the frame is GPU-bound at 26-29 ms after the octree culling, SSGI (9.3 ms) and the
  scene pass (~10-13 ms) lead; the imposters' share of the scene pass is not measured yet.
