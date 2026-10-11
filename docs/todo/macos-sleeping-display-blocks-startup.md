---
id: macos-sleeping-display-blocks-startup
title: macOS — a launch while the display sleeps finds no monitor and the window service fails
status: in-progress
priority: high
scope: src/PlatformManager.cpp, src/PlatformSpecific/Helpers.mac.cpp
opened: 2026-10-11
tags: [macos, window, glfw, defect]
---

# macOS — a launch while the display sleeps finds no monitor and the window service fails

## Why

macOS peer, 2026-10-11 (M6, display asleep: `system_profiler` "Display Asleep: Yes", `displaysleep 180`): every
launch failed at once with `[Fatal][WindowService] "There is no monitor on the system !"`, a clean teardown and exit 1.
GLFW skips a sleeping display when it lists the monitors (`dependencies/glfw/src/cocoa_monitor.m`,
`if (CGDisplayIsAsleep(displays[i])) continue;`). An unattended or remote launch on a Mac whose display sleeps could
not start. Owner decision 2026-10-11: the engine wakes the display to start.

## Fix (2026-10-11, waiting for the macOS proof)

`PlatformSpecific::wakeDisplays()` (`Helpers.mac.cpp`), called by `PlatformManager::onInitialize()` before
`glfwInit()`: when every online display is asleep it declares a local user activity
(`IOPMAssertionDeclareUserActivity(kIOPMUserActiveLocal)`, what `caffeinate -u` does), polls `CGDisplayIsAsleep()`
every 50 ms up to 5 s, releases the assertion and logs the outcome (`Woken`, `StillAsleep`, `Failed`; nothing when a
display was already awake). CoreGraphics and IOKit are linked explicitly. Not compiled on Linux.

During the run (owner decision 2026-10-11, the same day): `Window` holds a `PlatformSpecific::DisplaySleepInhibitor`
(an IOKit `PreventUserIdleDisplaySleep` assertion, RAII, released in `onTerminate()` and its destructor) while the
window exists, when `Core/Video/Window/PreventDisplaySleep` is true (default). "The display is kept awake while the
window is open." in the log. Windows and Linux: item display-sleep-inhibit-windows-linux.

## What remains

- macOS peer: build (paranoid, 0 warning), then let the display sleep (`pmset displaysleepnow`) and launch unattended:
  "Every display was asleep : woken to create the window.", the scene loads, exit 0. Also a launch with the display
  awake: no new trace.
- `pmset -g assertions` during a run lists "Emeraude engine: a window is open" (PreventUserIdleDisplaySleep); it is
  gone after exit. With the setting false: no assertion, the display sleeps on its timer.
- Then move the knowledge to `docs/caution-points.md` and delete this item.

## ⚠️ Traps

- `wakeDisplays()` only wakes; the in-run inhibitor is what keeps the monitor for GLFW during a long run.
