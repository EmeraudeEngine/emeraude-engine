---
id: macos-window-less-launch-wakes-the-displays
title: On macOS a window-less launch wakes the sleeping displays although it shows nothing
status: open
priority: high
scope: src/PlatformManager.cpp (the macOS block of onInitialize()), src/PlatformSpecific (wakeDisplays())
opened: 2026-11-22
tags: [macos, window-less, display, power]
---

# On macOS a window-less launch wakes the sleeping displays although it shows nothing

## Why

Seen by the macOS peer on 2026-11-22 (Mac mini M6, macOS 27.2.1, the session locked, every display asleep): a
`--window-less` run writes `Every display was asleep : woken to create the window.` and the displays light up. A
window-less run creates no window: every unattended run (a sweep, a peer's verification, a server) wakes the screens of
the machine it runs on. The owner ranked it a defect (2026-11-22).

The mechanism: `PlatformManager::onInitialize()` calls `PlatformSpecific::wakeDisplays()` before `glfwInit()` in EVERY
run (owner decision 2026-10-11: GLFW skips a sleeping display when it lists the monitors, and a launch while the
display slept found "no monitor"). It never reads `--window-less`; the services that do are `Window`
(`Window::onInitialize()`: "Creating a fake window ...", it returns BEFORE its monitor check), `Graphics::Renderer` and
`Input::Manager`.

## What remains

- Do not wake the displays when the run is window-less: `PlatformManager` has the arguments
  (`m_primaryServices.arguments()`), the switch is `-W` / `--window-less`.
- First prove nothing else of a window-less run needs GLFW's monitor list (`/usr/bin/grep -rn` `glfwGetMonitors` /
  `glfwGetPrimaryMonitor` and their callers; `Window` is ruled out above).
- Proof on the Mac, the displays asleep (`pmset displaysleepnow`): a window-less launch leaves them asleep, loads its
  scene and exits 0, with no `woken` line; a windowed launch still wakes them and finds its monitor (the proof of
  2026-10-11, `docs/caution-points.md`).
- Then update that section of `docs/caution-points.md` and the `wakeDisplays()` Doxygen.

## ⚠️ Traps

- The switch is read by each service on its own (`isSwitchPresent("-W", "--window-less")` in three places): read it the
  same way, or ask the owner whether one owner of that fact should exist.
- `wakeDisplays()` holds a user-activity assertion only for the time of the call; the display sleeps again on the
  user's idle timer. The cost of the defect is the wake itself, not a display kept on.

## References

- `docs/caution-points.md` § *macOS: a sleeping display is woken to start, and kept awake while the window is open*.
- `src/PlatformManager.cpp` (`PlatformManager::onInitialize()`), `src/PlatformSpecific/Helpers.hpp` (`wakeDisplays()`),
  `src/Window.cpp` (`Window::onInitialize()`).
