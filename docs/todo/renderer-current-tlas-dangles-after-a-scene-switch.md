---
id: renderer-current-tlas-dangles-after-a-scene-switch
title: Renderer::m_currentTLAS outlives its scene — isRayTracingReady() reads a destroyed acceleration structure at the first frame after a scene switch
status: open
priority: high
scope: Graphics/Renderer (m_currentTLAS, isRayTracingReady(), currentTLAS(), rtDescriptorSet()), Scenes/SceneMetaData (the TLAS owner), Graphics/PostProcessStack (canOccupantRun)
opened: 2026-11-15
tags: [tsan, lifetime, defect, measured, use-after-free]
---

# Renderer::m_currentTLAS outlives its scene — isRayTracingReady() reads a destroyed acceleration structure at the first frame after a scene switch

## Why

`Renderer::m_currentTLAS` is a RAW pointer (`const Vulkan::AccelerationStructure *`) to the acceleration structure the
scene's `SceneMetaData` owns (`std::unique_ptr`). The renderer stores it in the MIDDLE of a frame
(`Renderer.cpp`, the two "Set the current TLAS" sites, after the frame's build) and clears it only on the branch where
the frame has no scene at that point. It is read EARLIER in the next frame:
`Renderer::renderFrame()` → `PostProcessStack::syncSlotSelection()` → `canOccupantRun()` →
`Renderer::isRayTracingReady()` → `m_currentTLAS->isCreated()`.

In steady state the pointer is valid (the scene keeps that structure until its next build, and retires the old one
through the deferred destructor). When the scene changes, the previous scene's structure is retired and destroyed a few
frames later while the renderer still holds its address: the first frame of the next scene asks
`isRayTracingReady()` through a dead object.

**Measured** (ThreadSanitizer, 2026-11-15, Debug + TSan on llvmpipe, the testbed's active scene switched through the
console, one run of three): `heap-use-after-free`, both stacks on the RENDERING thread — the read in
`Vulkan::AbstractObject::isCreated()` ← `Renderer::isRayTracingReady()` ← `PostProcessStack::canOccupantRun()` ←
`syncSlotSelection()` ← `renderFrame()`; the block freed (and since reused by a temporary of
`Effects::Camera::ToneMapping::createPipelines()`) earlier in the same `renderFrame()`.

What it does on a real device, by reading the code (not observed): one byte of freed memory decides, for that frame,
whether every ray-traced effect "can run" — a slot switched to its fallback or kept on a traced occupant for nothing —
and a crash if the page was returned to the system. `currentTLAS()` hands the same pointer to the traced effects and
`rtDescriptorSet()` tests it against null: both are refreshed by the frame before the effects run, so the stale value
is only seen by the readers that come BEFORE the refresh.

## What remains

- **Decide the contract (owner)**:
  1. the frame takes the structure from the scene it renders, at its START (`renderFrame()`, before
     `syncSlotSelection()`): the previous build of THAT scene, alive for the whole frame; no value survives a frame
     without its scene (cleared when there is none);
  2. `isRayTracingReady()` answers for a scene given to it and no longer reads a stored pointer; the stored pointer
     stays for the effects, set and cleared inside the frame only;
  3. shared ownership (the scene holds its structure by `std::shared_ptr`, the renderer too): no dangling possible, at
     the price of a reference count on a per-frame object the deferred destructor already orders.
- List every reader of `m_currentTLAS` that runs before the frame's refresh (`canOccupantRun()`,
  `Renderer.cpp` the irradiance-probe test, `RTGI::providesIndirectDiffuse()`, `PostProcessor`'s executor) and say for
  each which frame's structure it means.
- Proof: the same console scenario under TSan, 0 `heap-use-after-free` in at least 10 switches; a ray-traced scene on
  the hardware device keeps its traced effects across a switch (the status of `PostProcess.getStatus`).

## ⚠️ Traps

- TSan reports it as `heap-use-after-free`, not as a data race: a count of "data race" lines misses it.
- One run of three: the freed block must not have been handed back yet. More switches per run raise the odds.

## References

- Scenes doc 12 § *Plain state shared by the logic and rendering threads* (the logic thread no longer asks this
  question itself: it reads the answer the rendering thread publishes).
- `docs/caution-points.md` § *TSan on a machine whose display GPU runs the NVIDIA driver* (the recipe).
