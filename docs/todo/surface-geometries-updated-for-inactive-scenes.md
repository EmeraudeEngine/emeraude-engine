---
id: surface-geometries-updated-for-inactive-scenes
title: Every registered surface geometry (ocean FFT, terrain clipmap) is updated every frame, even for inactive scenes
status: open
priority: unranked
scope: Graphics::Renderer (registerSurfaceGeometry / updateSurfaceGeometries), Geometry::OceanSurfaceResource, Geometry::CDLODTerrainResource
opened: 2026-09-30
tags: [performance, gpu, scene, lifetime]
---

# Every registered surface geometry is updated every frame, even for inactive scenes

## Why

`Renderer::updateSurfaceGeometries()` walks an ENGINE-WIDE list of weak pointers (`registerSurfaceGeometry()`, called
by the geometry itself when it is created) and submits each surface's work every frame: the FFT ocean's compute, the
CDLOD terrain's clipmap update. It does not know which scene draws a surface. projet-alpha keeps inactive acts loaded
(their scenes too), so every loaded demo with a sea or a terrain costs its FFT / clipmap work in every frame of another
scene. Found 2026-09-30 while fixing the destruction of those surfaces (they were in flight in the active scene's
frames: engine caution-points § "A surface geometry must RETIRE its GPU objects").

## What remains (architecture: owner decision)

Options:
1. Update only the surfaces the frame draws: the active scene's render lists know their renderables; a surface
   marked "used" by the scene's render list preparation is updated. The first frame after an enable must update
   before it draws (order inside the frame).
2. Scope the registration to a scene: the scene registers the surfaces of its renderables (a resource can be shared
   by several scenes: a per-scene set of weak pointers, the renderer walks the ACTIVE scene's set).
3. Keep it as is (a loaded-but-inactive scene keeps its sea animated, so time continuity on re-enable is free).

⚠️ Traps: the ocean integrates time (`m_previousTime`): skipping frames must not make it jump on re-enable; a surface
used by two scenes must be updated once per frame.

## References

- `src/Graphics/Renderer.cpp` `updateSurfaceGeometries()`, `src/Graphics/Geometry/OceanSurfaceResource.cpp`,
  `src/Graphics/Geometry/CDLODTerrainResource.cpp`.
