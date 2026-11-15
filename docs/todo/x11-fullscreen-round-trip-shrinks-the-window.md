---
id: x11-fullscreen-round-trip-shrinks-the-window
title: On X11 a fullscreen round trip brings the window back 74 pixels shorter than its stored size
status: open
priority: high
scope: src/Window.cpp (switchToFullscreenMode, switchToWindowedMode, getSize), the GLFW fork (x11_window.c)
opened: 2026-11-15
tags: [window, x11, xwayland, fullscreen, measured-once]
---

# On X11 a fullscreen round trip brings the window back 74 pixels shorter than its stored size

## Why

Seen 2026-11-15 while proving another change (Linux, GNOME on Wayland, the application forced on X11 through
XWayland, Release, a 5120x2880 screen at a scale of 2): Shift+F11 on a 900x600 window stores 900x600
(`Core/Video/Window/Width|Height`), goes fullscreen (5120x2880), and Shift+F11 again brings the window back at
**900x526**. From a stored 1100x700 it comes back at **1100x626**. 74 pixels of height lost in both cases, the width
exact.

`switchToWindowedMode()` gives the stored size to `glfwSetWindowMonitor()`; `Core.WindowService.getState()` then
reads 74 less. The next fullscreen switch stores what `getSize()` says — 900x526 — so each round trip would shrink
the window again: not tried.

**Not established:** what the 74 pixels are (a title bar counted twice — 37 points at a scale of 2 — is the first
guess); whether the size is wrong on screen or only as read; whether a native X11 session or Wayland does it (on
Wayland the same key was not tried that day); the height after several round trips.

## What remains

- [ ] Repeat on Wayland and on a native X11 session; three round trips in a row, reading `getState()` each time.
- [ ] Compare the window size (`getSize()`) and the framebuffer size before and after, and the frame extents the
  window manager reports (`_NET_FRAME_EXTENTS`).
- [ ] Fix where the height is lost (the size given back on the windowed switch, or the size stored).

## References

- `src/Window.cpp` (`switchToFullscreenMode()`, `switchToWindowedMode()`), `docs/subsystems/console/04-4-common-ai-operations.md`
  (the fullscreen toggle from the console is Shift+F11: `keyPress(300, 1)`).
