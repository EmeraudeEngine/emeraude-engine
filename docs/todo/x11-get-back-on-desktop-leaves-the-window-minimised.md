---
id: x11-get-back-on-desktop-leaves-the-window-minimised
title: On X11 Window::getBackOnDesktop() leaves a minimised window minimised
status: open
priority: high
scope: src/Window.cpp (getBackOnDesktop, isIconified), the GLFW fork (x11_window.c: restore)
opened: 2026-11-15
tags: [window, x11, xwayland, console, measured-once]
---

# On X11 Window::getBackOnDesktop() leaves a minimised window minimised

## Why

Seen 2026-11-15 while proving another change, with the console commands added that day (Linux, GNOME on Wayland, the
application forced on X11 through XWayland by `Core/Video/Window/GLFW/UsePlatform = X11`, Release, RTX 3070 Ti):
`Core.WindowService.iconify()` minimises the window — `getState()` says `"iconified": true` — and
`Core.WindowService.getBackOnDesktop()`, which calls `glfwRestoreWindow()`, changes nothing: `"iconified"` is still
true 3 s later, and until the exit ("The window is minimised or empty: its properties are not saved."). A fullscreen
round trip in between (Shift+F11 twice) does not clear it either. Two runs, the same result.

`getBackOnDesktop()` is what brings an instance back when a second launch hands it over: on such a session the first
instance would stay in the dock.

**Not established:** whether the window is really still minimised or only reported so (`GLFW_ICONIFIED` reads the
window's `WM_STATE` on X11) — nobody looked at the screen; whether a native X11 session behaves the same (this was
XWayland under Mutter); whether another window manager does. On Windows the same function was fixed on 2026-10-22
(it tested "visible" instead of "iconified").

## What remains

- [ ] Look at the screen: after `iconify()` then `getBackOnDesktop()`, is the window back?
- [ ] Read the window's state from outside (`xprop -id <window> WM_STATE _NET_WM_STATE`) before and after.
- [ ] Try a native X11 session and another window manager.
- [ ] According to the answer: the restore request (a map request is not always honoured for a minimised window:
  an activation request, `_NET_ACTIVE_WINDOW`, may be needed), or the state GLFW reports.

## References

- `src/Window.cpp` (`getBackOnDesktop()`, `iconify()`, `isIconified()`), `src/Window.console.cpp` (the commands).
