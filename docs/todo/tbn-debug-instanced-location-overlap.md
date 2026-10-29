---
id: tbn-debug-instanced-location-overlap
title: The TBN debug view fails to compile for every instanced renderable (overlapping location 1)
status: open
priority: high
scope: Saphir/Generator/TBNSpaceRendering.cpp
opened: 2026-10-29
tags: [shaders, debug-view, defect, measured]
---

# The TBN debug view fails to compile for every instanced renderable (overlapping location 1)

## Why

Windows-PA (2026-10-29, engine 3fd1d465, projet-alpha `city 32`, `SceneManager enableTBN(1)`): every INSTANCED
renderable's TBN program fails — glslang "ERROR: 0:9: 'location' : overlapping use of location 1": 426 compile failures,
2130 `[Error]` lines ("TBN space display ... refused"), the Shader Debugger overlay opened over the view; the trees and
their imposters, and the instanced street props, show no TBN. Non-instanced meshes (the TileWorld regions, the car) draw
theirs.

`TBNSpaceRendering.cpp:144` declares the instanced `svModelViewProjectionMatrix` (a `mat4`, four locations) with
`getNextShaderVariableLocation()` — the default increment of 1 — so `svTangentWorldSpace` lands on location 1, inside the
matrix.

## What remains

- [ ] `getNextShaderVariableLocation(4)` for the matrix (check every other generator declaring a `mat4` stage output
  the same way).
- [ ] Prove: `city 32` with `enableTBN(1)`, 0 compile error, the trees' and props' TBN drawn.

## References

- `src/Saphir/Generator/TBNSpaceRendering.cpp:144`, `src/Saphir/Generator/Abstract.cpp:488`.
