---
id: adaptive-terrain-blas-goes-stale-on-subgrid-slide
title: The adaptive terrain BLAS is never refreshed when the sub-grid slides
status: open
priority: unranked
scope: Graphics/Geometry
opened: 2026-09-22
tags: [ray-tracing, terrain, streaming]
---

# The adaptive terrain BLAS is never refreshed when the sub-grid slides

## Why

`Geometry::AdaptiveVertexGridResource` now feeds a BLAS (`generateTriangleListIndicesForRT()`,
2026-09-22), so a terrain floor finally occludes and bounces light in the traced lane. That BLAS is
built **once**, from `Interface::onDependenciesLoaded()`.

But the geometry is a streaming window, not a fixed mesh: `TerrainResource::updateVisibility()` is
called every frame with the camera position (`Scene.cpp`), and past `m_visibleSize / 3` of travel it
extracts a new sub-grid and hands it to `AdaptiveVertexGridResource::updateData()`, which **replaces
the whole VBO** with vertices at different world positions. The BLAS still holds the surface of the
sub-grid that was current at load: from that moment the traced lane occludes against a terrain that
is no longer where it is drawn.

⚠️ **It does not reproduce on today's demos, which is exactly why it must be written down.**
`m_visibleSize` defaults to 4096 m while `Grid::subGrid()` clamps the requested cell count to the
grid's own, so on `forest` (250 m) and `terrain` (5 km, 4096 divisions of 1.22 m) the sub-grid IS the
whole grid and the update threshold (1365 m) is never crossed on `forest`. A demo that sets
`GridVisibleSize` below the grid size, or a 5 km walk on `terrain`, exposes it immediately.

## What remains

- [ ] Decide what an update owes the BLAS: rebuild it, refit it (the topology and the vertex count
  are invariant across an update — only the positions move, which is exactly the refit case), or
  drop the geometry out of the TLAS while it streams.
- [ ] Whatever the answer, it cannot be done where `updateData()` runs: `updateVisibility()` starts
  it on a **detached `std::thread`** (`TerrainResource.cpp`), which is both a Vulkan-submission
  hazard and a `-fno-exceptions` violation of its own (a throwing `std::thread` constructor —
  see the coding rules). The refresh belongs on the main thread, e.g. an `m_BLASDirty` flag that
  `SceneMetaData` consumes where it already rebuilds a missing BLAS.

## ⚠️ Traps

- The proxy is regenerated from `m_localData`, which `updateData()` overwrites — so the indices
  stay correct by construction; it is the **baked vertex positions inside the BLAS** that go stale,
  not the index buffer.
- A stale BLAS gives no error and no validation message. The symptom is an occlusion that does not
  match the picture, which reads as "the RT lane is wrong" rather than "the terrain moved".

## References

- `src/Graphics/Geometry/AdaptiveVertexGridResource.cpp` — `generateTriangleListIndicesForRT()`,
  `updateData()`.
- `src/Graphics/Renderable/TerrainResource.cpp` — `updateVisibility()`.
- `src/Scenes/AGENTS.md` § BLAS Building.
