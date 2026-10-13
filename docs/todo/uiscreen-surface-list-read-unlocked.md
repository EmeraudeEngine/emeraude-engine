---
id: uiscreen-surface-list-read-unlocked
title: UIScreen and Overlay::Manager read their surface and screen lists without their lock
status: open
priority: high
scope: src/Overlay/UIScreen.cpp, src/Overlay/Manager.cpp
opened: 2026-10-13
tags: [overlay, concurrency, defect]
---

# UIScreen and Overlay::Manager read their surface and screen lists without their lock

## Why

The overlay TSan stress (2026-10-13) caught `UIScreen::empty()` reading `m_surfaces` on the render thread while the
main thread appended to it in `createSurface()` (fixed the same day: `empty()` takes `m_surfacesMutex`). By reading,
the same pattern remains elsewhere:
- `UIScreen::getSurface()` (both overloads, `src/Overlay/UIScreen.cpp:186`, `:203`) searches `m_surfaces` unlocked;
- the input dispatchers `onKeyPress()`, `onKeyRelease()`, `onCharacterType()`, `onPointerMove()`, `onButtonPress()`,
  `onButtonRelease()`, `onMouseWheel()` (`UIScreen.cpp:250-610`) iterate `m_surfaces` unlocked, and call into the surfaces;
- `Overlay::Manager::enableScreen()`, `disableScreen()` and `screen()` (`src/Overlay/Manager.cpp:408`, `:442`, `:513`, `:528`) search
  `m_screens` without `m_screensAccess`.
They are safe only while every mutation runs on the same thread as the reader; the setters and `createSurface()` are
public and called from other threads (the stress's logic thread, a downstream application's web-view callbacks).

## What remains

- Decide the protocol per reader: a snapshot under the lock (the pattern of `processSurfaceUpdates()` and
  `snapshotScreens()`), or the lock held for the search only (`getSurface()` returns a `shared_ptr`). The input
  dispatchers call into surfaces, which may call back into the screen: a snapshot, never the lock held across the calls.
- `UIScreen::destroySurface()` and `clearSurfaces()` call `device()->waitIdle()` while holding `m_surfacesMutex`: move it
  out of the lock (or retire through the deferred destructor, see `overlay-surface-rebuilt-while-a-frame-is-recorded`).
- Proof: extend `tools/stress/OverlayResizeStress.cpp` with a thread that creates / destroys / reorders surfaces and
  injects pointer events; 0 TSan report over 6 runs.

## References

- engine `docs/caution-points.md` § "Overlay lock order".
