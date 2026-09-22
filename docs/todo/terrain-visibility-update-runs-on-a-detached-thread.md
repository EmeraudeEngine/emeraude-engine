---
id: terrain-visibility-update-runs-on-a-detached-thread
title: TerrainResource streams its sub-grid from a detached std::thread
status: open
priority: unranked
scope: Graphics/Renderable
opened: 2026-09-22
tags: [threading, no-exceptions, terrain, streaming]
---

# TerrainResource streams its sub-grid from a detached std::thread

## Why

`TerrainResource::updateVisibility()` — called every frame from `Scene.cpp` with the camera
position — starts the sub-grid update like this:

```cpp
std::thread([geometry = m_geometry, subGrid = subGrid] () {
    geometry->updateData(subGrid);
}).detach();
```

Two things are wrong with it, independently of what the update does.

1. **`std::thread`'s constructor can throw**, and the cascade is built `-fno-exceptions`
   (MSVC: `/EHs- /EHc-` + `_HAS_EXCEPTIONS=0`). A throwing std call is forbidden project-wide, the
   same way `.at()` and `std::stoi` are. On a resource exhaustion this does not fail, it terminates.
2. **The work it starts is Vulkan work on an unsynchronised thread**: `updateData()` creates a VBO,
   transfers into it, and then `std::swap`s it into `m_vertexBufferObject` — a pointer the render
   thread reads while recording draw calls. `m_pendingDestructionVBO` keeps the previous buffer
   alive until the NEXT update, which is why it has not been seen failing, but that is a delay, not
   a synchronisation. `m_localData` is overwritten the same way while the render path reads it
   (`boundingBox()`, and since 2026-09-22 `generateTriangleListIndicesForRT()` for the BLAS).

⚠️ **The staleness of the BLAS is already handled and is NOT this item**: the geometry raises a flag
and `SceneMetaData::rebuild()` rebuilds on the frame path (`docs/caution-points.md` § Ray Tracing).
That design was chosen *because* the update thread cannot be trusted with the work — it does not
make the thread acceptable.

## What remains

- [ ] Move the update off a raw detached thread: the engine's thread pool, or a job consumed on the
  main thread. Whatever is chosen must not construct a `std::thread` inline.
- [ ] Decide what publishes the new VBO and the new `m_localData` to the render thread. Today it is
  a plain assignment under an `m_isUpdating` flag that the render path does not honour for reads.
- [ ] Re-examine `m_pendingDestructionVBO` once the above is settled: the engine has a
  `DeferredDestructor` for exactly this, and the hand-rolled one-update delay can go.

## ⚠️ Traps

- It does not reproduce on today's demos: `m_visibleSize` defaults to 4096 m while `Grid::subGrid()`
  clamps the requested cell count to the grid's own, so on every current scene the sub-grid IS the
  whole grid and no update is ever started. Forcing one means shrinking `DefaultVisibleSize` (a
  bench-only edit) or building a demo with `GridVisibleSize` in its JSON.
- `Forest` no longer uses `TerrainResource` at all (a 250 m area takes a `BasicGroundResource`), so
  the first scene to exercise this will be a genuinely kilometre-scale one.

## References

- `src/Graphics/Renderable/TerrainResource.cpp` — `updateVisibility()`.
- `src/Graphics/Geometry/AdaptiveVertexGridResource.cpp` — `updateData()`.
- `.claude/rules/coding-style.md` (projet-alpha) — no `try`/`catch`/`throw`, and no throwing std call.
