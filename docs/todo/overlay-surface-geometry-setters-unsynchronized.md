---
id: overlay-surface-geometry-setters-unsynchronized
title: Overlay::Surface geometry setters race the render thread (m_rectangle, lost invalidation, manual try_lock)
status: open
priority: high
scope: src/Overlay/Surface.hpp (setGeometry, setSize, setPosition, move…), src/Overlay/Surface.cpp (processUpdates)
opened: 2026-10-12
tags: [concurrency, overlay, defect, ave-robustus-ii]
---

# Overlay::Surface geometry setters race the render thread (m_rectangle, lost invalidation, manual try_lock)

## Why
Found while closing `overlay-resize-notifies-under-lock` (2026-10-12), by code reading; the overlay resize TSan stress
does not exercise it (it never moves a surface).
- `setGeometry()` / `setSize()` / `setPosition()` write `m_rectangle` with no lock, from whatever thread calls them
  (application code, a web view), while the render thread reads it in `processUpdates()` (`updateModelMatrix()`,
  `updatePhysicalRepresentation()`) and at draw time: a data race on a non-atomic structure.
- Lost invalidation: the render thread sees `m_videoMemorySizeValid == false`, rebuilds the buffer from the OLD
  rectangle, a setter writes the new one and invalidates, then the render thread stores `true` — the surface keeps the
  old size until the next invalidation. The flags became atomics on 2026-10-12 (no more UB), which does not fix this.
- `processUpdates()` takes `m_framebufferAccess` by a manual `try_lock()` / `unlock()` pair (CP.20: guards only).

## What remains
- Owner decision on the protocol: (a) the setters take `m_framebufferAccess` (the buffer lock the render thread
  already holds in `processUpdates()`; check every caller that already holds it — the mutex is not recursive); (b) a
  generation counter (the setter bumps it, the render thread re-checks it before storing `true`) plus a locked or
  double-buffered rectangle; (c) a main-thread-only contract for the setters, asserted in Debug, and the render thread
  reading a rectangle snapshot published under the representation lock.
- `processUpdates()` through `std::unique_lock{m_framebufferAccess, std::try_to_lock}`.
- Proof: extend `tools/stress/OverlayResizeStress.cpp` with a thread that moves / resizes a surface.

## References
- `docs/caution-points.md` § "Overlay::Surface visibility is atomic".
- projet-alpha `docs/plans/ave-robustus-ii.md` (CP.20, data races).
