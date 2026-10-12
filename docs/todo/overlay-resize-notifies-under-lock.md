---
id: overlay-resize-notifies-under-lock
title: Overlay::Manager::onWindowResized() notifies its observers with the representation lock held
status: open
priority: high
scope: src/Overlay/Manager.cpp (onWindowResized), its observers (the CEF web views, the notifier, the editor)
opened: 2026-10-12
tags: [concurrency, overlay, defect, ave-robustus-ii]
---

# Overlay::Manager::onWindowResized() notifies its observers with the representation lock held

## Why
Found while fixing the overlay lock-order inversion that ThreadSanitizer reported (2026-10-12, the TrackMixer TSan
stress). `onWindowResized()` holds `m_physicalRepresentationUpdateMutex` for its whole body, including the
`OverlayResized` and `OverlayScaleChanged` notifications. An observer that calls back into the overlay manager (a screen
lookup, a surface update, a render request that ends in `render()` or `updateVideoMemory()` on the same thread) takes
that lock again: a self-deadlock on a non-recursive mutex, or a wait on the render thread. Cascade rule: never emit
while holding a mutex a handler may take. Not observed yet; the code reading is enough to call it a defect.

## What remains
- Release the lock before notifying: compute the new properties and update the screens under it, then notify after
  (the payload is already a copy of the window state).
- Check every observer of `OverlayResized` / `OverlayScaleChanged` for what it calls back.
- Prove it under the TSan build (a resize driven while frames render), as the TrackMixer stress does
  (`tools/stress/`).

## References
- `docs/caution-points.md` § "Overlay lock order: m_physicalRepresentationUpdateMutex THEN m_screensAccess".
- projet-alpha `docs/plans/ave-robustus-ii.md` (CP.31/32: never emit under a lock).
