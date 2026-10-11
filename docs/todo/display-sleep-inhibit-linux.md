---
id: display-sleep-inhibit-linux
title: Keep the display awake while the window is open on Linux (macOS and Windows done)
status: open
priority: unranked
scope: src/PlatformSpecific (DisplaySleepInhibitor), src/Window.cpp
opened: 2026-10-11
tags: [window, power, cross-platform]
---

# Keep the display awake while the window is open on Linux

## Why

Owner decision 2026-10-11: the display stays awake while the engine window exists (setting
`Core/Video/Window/PreventDisplaySleep`, default false since 2026-10-11 — an application opts in), as SDL does by default. macOS and Windows have it
(`PlatformSpecific::DisplaySleepInhibitor` held by `Window`: an IOKit `PreventUserIdleDisplaySleep` assertion on macOS, a
`PowerRequestDisplayRequired` power request on Windows; engine `docs/caution-points.md` § Platform-Specific). Linux still
lets the display sleep during a run.

## What remains

- Linux: the `org.freedesktop.ScreenSaver.Inhibit` / `UnInhibit` D-Bus calls (X11 and Wayland desktops), or
  `zwp_idle_inhibit_manager_v1` on the window's `wl_surface` under Wayland (GLFW does not expose it). The engine has no
  D-Bus dependency yet: an owner decision.
- The same `DisplaySleepInhibitor` interface on Linux, then drop the `#if IS_MACOS || IS_WINDOWS` in `Window` and
  `PlatformSpecific/Helpers.hpp`.

## References

- SDL `SDL_HINT_VIDEO_ALLOW_SCREENSAVER` (default: inhibit while a window exists): `SDL_DisableScreenSaver()`
  implementations per platform (SDL 2/3 sources, zlib licence).
