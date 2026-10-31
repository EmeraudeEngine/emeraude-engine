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

Linux (2026-10-31, engine e6f7f7a5 + the location fix below, `city 32,1,12,0`, `enableTBN(1)`): with the location fixed,
the same 426 programs still fail on a SECOND defect the first one hid: line 146 reads
`ViewUB(UniformBlock::Component::ViewProjectionMatrix)`, but the View block no longer has that member — the
view-projection matrix is pushed now (`pcMatrices.viewProjectionMatrix`, the push block this generator already
declares): glslang "'viewProjectionMatrix' : no such field in structure 'ubView'". Deeper: the generator spells its own
MVP for ONE instancing path only (the model matrix as a vertex attribute, `vaModelMatrix`); the
`AbstractVertexStage::prepareModelViewProjectionMatrix()` paths it ignores are the InstanceTransforms SSBO, MDI, the
advanced matrices (projection × pushed view) and CSM — a program built for one of those would read an attribute the
pipeline never binds.

## What remains

- [ ] `getNextShaderVariableLocation(4)` for the matrix — verified 2026-10-31 (it removes the overlap; no other
  generator declares a `mat4` stage output, the `mat3` ones already pass 3), left out of the tree: it alone fixes
  nothing visible; it goes with the fix below, with the file's IWYU backlog (+17 −1, gate 2026-10-31).
- [ ] Choose how the TBN program gets its matrices, then fix:
  - reuse the vertex stage's MVP synthesis (`prepareModelViewProjectionMatrix()`, every instancing path at once) and
    pass the result to the geometry stage;
  - or project the four line end points in the vertex stage (vertex, + T, + B, + N, through the same synthesis) so the
    geometry stage needs no matrix at all.
- [ ] Prove: `city 32` with `enableTBN(1)`, 0 compile error, the trees' and props' TBN drawn.

## References

- `src/Saphir/Generator/TBNSpaceRendering.cpp:144`, `src/Saphir/Generator/Abstract.cpp:488`.
