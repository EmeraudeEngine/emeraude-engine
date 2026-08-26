---
id: hardcoded-near-plane
title: The camera near plane is hard-coded — millimetre-scale scenes clip
status: open
priority: medium
scope: Scenes/Camera
opened: 2026-08-25
tags: [measured, gltf]
---

# The camera near plane is hard-coded — millimetre-scale scenes clip

## Why

`ViewMatrices2DUBO.cpp:212` computes `0.1 / sqrt(1 + tan²(fov/2)(aspect² + 1))` ≈ **0.089 m
whatever the scene**. `MetalRoughSpheresNoTextures` (radius 0.00035 m, authentically millimetric)
caps at 5.5 % of frame height and cannot be judged.

This was the real cause of the "unjudgeable" verdicts in the v1 glTF bench, which had been
misattributed to framing.

## What remains

- [ ] Derive the near plane from the scene/subject scale (the viewers already compute a framing
  distance) instead of a constant, or expose it so a viewer can set it.
