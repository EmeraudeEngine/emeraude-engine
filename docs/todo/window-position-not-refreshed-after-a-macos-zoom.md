---
id: window-position-not-refreshed-after-a-macos-zoom
title: Window getState keeps the old position after a macOS zoom (Window > Zoom)
status: open
priority: high
scope: Window (state, GLFW position callback, macOS)
opened: 2026-10-14
tags: [window, macos, defect]
---

# Window getState keeps the old position after a macOS zoom (Window > Zoom)

## Why

macOS peer (M6, 2026-10-14, engine 01d6f8a1): after a zoom through Window > Zoom (System Events), the window is
1920x928 (framebuffer 3840x1856) and the rendering follows, but `Core.WindowService.getState()` keeps reporting
`windowXPosition` / `windowYPosition` 556,171 — the position before the zoom. Through 5 zoom / unzoom cycles. Any
consumer of the state (the monitor choice of `Core::updateNoScenePacing()`, a saved window position) reads a stale value.

Suspected, not verified: GLFW reports no position callback for a zoom on macOS (the frame changes through
`-[NSWindow zoom:]`), and `Window` only refreshes the position from that callback.

## What remains

- Reproduce on the M6 with a trace of the GLFW position / size callbacks during a zoom.
- Fix in `Window`: refresh the position with the size when GLFW reports no move (e.g. read `glfwGetWindowPos()` in the
  size / framebuffer callback, as the framebuffer callback already reads `glfwGetWindowSize()` for Wayland).
- Proof: getState after 5 zoom / unzoom cycles reports the real position each time; Linux / Windows unchanged.

## References

- `src/Window.cpp` framebuffer-size callback (the Wayland window-size refresh is the same pattern).
