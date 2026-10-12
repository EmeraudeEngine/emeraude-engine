---
id: overlay-redraw-requested-under-lock
title: Overlay::Manager still emits RedrawRequested and runs surface hooks under the representation lock
status: open
priority: high
scope: src/Overlay/Manager.cpp (onWindowResized, updateVideoMemory, requestRedraw), src/Overlay/Surface.* (notifyRedrawRequired, processUpdates hooks)
opened: 2026-10-12
tags: [concurrency, overlay, defect, ave-robustus-ii]
---

# Overlay::Manager still emits RedrawRequested and runs surface hooks under the representation lock

## Why
`overlay-resize-notifies-under-lock` moved `OverlayResized` / `OverlayScaleChanged` out of
`m_physicalRepresentationUpdateMutex` (2026-10-12). Two paths still call out under it:
- `Surface::invalidate()` and `processUpdates()` fire the surface's redraw requester, which is
  `Manager::requestRedraw()` → `notify(RedrawRequested)` to EVERY overlay observer — from `onWindowResized()` (main
  thread) and from `updateVideoMemory()` (render thread), both under the representation lock.
- `processUpdates()` runs the surface's virtual hooks (`onActiveBufferReady()`…), overridden by subclasses outside
  the overlay (projet-alpha's web views), under the same lock.
Today's observers are safe by reading (Core only wakes the render thread under its own budget mutex; UI::Manager and
the Notifier ignore the code), so nothing deadlocks now. Cascade rule (CP.31/32): never call out under a lock a
handler may take — the next observer that touches the overlay deadlocks.

## What remains
- Owner decision: (a) `requestRedraw()` sets an atomic "redraw wanted" flag while a representation pass runs, emitted
  once after the lock is released (one notification per pass instead of one per surface); (b) the redraw requester
  becomes a direct, documented lock-free call into Core (no Observer); (c) keep it, and document the contract
  "a RedrawRequested handler must not call into the overlay manager", asserted in Debug.
- Audit the surface hooks run under the lock (who overrides them, what they call).
- Proof: `tools/stress/OverlayResizeStress.cpp` with its observer calling back on `RedrawRequested`.

## References
- `docs/caution-points.md` § "Overlay lock order".
- projet-alpha `docs/plans/ave-robustus-ii.md` (CP.31/32).
