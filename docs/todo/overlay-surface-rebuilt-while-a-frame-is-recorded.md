---
id: overlay-surface-rebuilt-while-a-frame-is-recorded
title: An overlay surface rebuilt off the render thread frees an image a recorded, unsubmitted frame still uses
status: open
priority: high
scope: src/Overlay/Surface.cpp (updatePhysicalRepresentation), src/Overlay/Manager.cpp (onWindowResized, updateVideoMemory)
opened: 2026-10-13
tags: [overlay, vulkan, lifetime, concurrency, defect]
---

# An overlay surface rebuilt off the render thread frees an image a recorded, unsubmitted frame still uses

## Why

Found by the overlay TSan stress (2026-10-13, `tools/stress/OverlayResizeStress.cpp`, Linux, lavapipe): when its
observer called `Overlay::Manager::updateVideoMemory()` from the LOGIC thread, 2 runs of 3 died in lavapipe's queue
thread on a SEGV (a null read inside an executing command buffer) — the profile of an image freed while a submitted
frame uses it. Restricted to the main thread, the stress is clean.

The mechanism, by reading:
- the render thread records the overlay into the frame's command buffer in `Manager::render()` under
  `m_physicalRepresentationUpdateMutex`, releases the lock, then SUBMITS the frame later (`Renderer` after
  `endRenderPass()` and the following passes);
- `Surface::updatePhysicalRepresentation()` rebuilds a surface with `device()->waitIdle()` then `Framebuffer::destroy()`
  (`src/Overlay/Surface.cpp:950` and `:992`; the same in `recreateTransitionBufferToRequestedSize()`, `:835`): `waitIdle()` waits for SUBMITTED work only;
- any pass that runs on another thread between the recording and the submission destroys an image the recorded command
  buffer references. In the engine itself that thread is the MAIN thread: `onWindowResized()` →
  `UIScreen::processSurfaceUpdates(true)` → rebuild, on every window resize — the stress only made it frequent.

## What remains

- Decide with the owner (options, trade-offs, recommendation):
  - (a) retire the old framebuffer resources through `Renderer::deferredDestructor()` (the frames-in-flight rule of the
    cascade) instead of `waitIdle()` + immediate destroy — also removes a `waitIdle()` per resize;
  - (b) never rebuild off the render thread: `onWindowResized()` only invalidates, the render thread's
    `updateVideoMemory()` rebuilds before it records (the `OverlayResized` observers that draw into the new pixmap — the
    Notifier — then see the old size for one pass: a contract change).
- Proof: the stress with the observer calling back from the logic thread again (the 2026-10-13 variant), 0 SEGV over
  6 runs; Vulkan validation on a normal build, 0 `VUID-` during resize bursts.

## References

- engine `docs/caution-points.md` § "Overlay lock order" and § "Overlay::Surface visibility is atomic".
- The stress: `tools/stress/OverlayResizeStress.cpp` (its RedrawRequested callback names this item).
