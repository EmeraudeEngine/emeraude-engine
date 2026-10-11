---
id: display-sleep-inhibit-windows-linux
title: Keep the display awake while the window is open on Windows and Linux (macOS done)
status: open
priority: unranked
scope: src/PlatformSpecific (DisplaySleepInhibitor), src/Window.cpp
opened: 2026-10-11
tags: [window, power, cross-platform]
---

# Keep the display awake while the window is open on Windows and Linux

## Why

Owner decision 2026-10-11: the display stays awake while the engine window exists (setting
`Core/Video/Window/PreventDisplaySleep`, default false since 2026-10-11 — an application opts in), as SDL does by default. macOS has it
(`PlatformSpecific::DisplaySleepInhibitor`, an IOKit `PreventUserIdleDisplaySleep` assertion held by `Window`). The
other two platforms still let the display sleep during a run.

## What remains

- Windows: `SetThreadExecutionState(ES_CONTINUOUS | ES_DISPLAY_REQUIRED)` on the window's thread while held,
  `ES_CONTINUOUS` alone to release (or a power request: `PowerCreateRequest` + `PowerSetRequest(PowerRequestDisplayRequired)`,
  which is per object rather than per thread — preferable for an RAII object).
- Linux: the `org.freedesktop.ScreenSaver.Inhibit` / `UnInhibit` D-Bus calls (X11 and Wayland desktops), or
  `zwp_idle_inhibit_manager_v1` on the window's `wl_surface` under Wayland (GLFW does not expose it). The engine has no
  D-Bus dependency yet: an owner decision.
- The same `DisplaySleepInhibitor` interface on the three platforms, then drop the `#if IS_MACOS` in `Window`.

## References

- SDL `SDL_HINT_VIDEO_ALLOW_SCREENSAVER` (default: inhibit while a window exists): `SDL_DisableScreenSaver()`
  implementations per platform (SDL 2/3 sources, zlib licence).
