---
id: tbn-display-retried-every-frame-without-tangents
title: The TBN space display is retried every frame on a geometry without a tangent space — tens of thousands of errors
status: open
priority: high
scope: src/Graphics/RenderableInstance/Abstract.cpp (getReadyForTBNSpace, getReadyForRender), src/Scenes/Scene.rendering.cpp (TBN loops), src/Scenes/Manager.console.cpp (enableTBN)
opened: 2026-10-12
tags: [defect, debug-display, log-flood]
---

# The TBN space display is retried every frame on a geometry without a tangent space

## Why

Measured on Linux (RTX 3070 Ti, Release, validation ON, 2026-10-12): `--load-demo coordinates-debug`, then
`Core.SceneManagerService.enableTBN(true)` ("enabled on 13 renderable instance(s)"), about 20 s of run — **24 805**
"Unable to generate the TBN space program !" and about 99 000 `[Error]` lines in all. 0 VUID; the display itself works
on the geometries that carry a tangent space (the cube's axes are drawn).

- `enableTBN` sets the flag on EVERY renderable instance of the scene, including those whose geometry has no tangent
  space in its vertex buffer: `+BoundaryPlane*`, `SkyBoxGeometry`, `+RenderBoundingBoxGeometry`, `+GroundZeroPlane`,
  `+Cuboid(…)`, `+Axis(…)`. `VertexBufferFormatManager` refuses each one ("needs tangent space, but the geometry … do not
  provide it").
- `Scene.rendering.cpp` (~1129, ~1164, ~1205) calls `getReadyForTBNSpace()` for every flagged instance EVERY frame.
  A failure only logs and returns false (`RenderableInstance/Abstract.cpp` ~1110). Nothing records it, so the next frame
  tries again; `getReadyForRender()` does the same (~1084).

This is not the geometry-stage gate of 2026-10-12 (that one only applies to devices without `geometryShader`): on NVIDIA
the behaviour was the same before it.

## What remains

- Decide the refusal (an intent, the owner's):
  - (a) `enableTBN` / `enableDisplayTBNSpace()` refuse an instance whose geometry has no vertex-buffer tangent space
    (`Geometry::Interface::tangentSpaceEnabled()`, NOT `surfaceProvidesTangentSpace()`), and the command reports
    "enabled on N, skipped M";
  - (b) and/or a generation failure clears the instance's flag with ONE error naming the renderable, so it is never
    retried.
  The geometry may still be loading when the flag is set, so (a) alone cannot see every case.
- Proof: the same run, 0 "Unable to generate the TBN space program", the axes still drawn on the cube, 0 VUID.

## References

- Found while validating the macOS peer's geometry-stage gate (engine caution-points § The TBN space display is gated on
  the device's geometry stage).
