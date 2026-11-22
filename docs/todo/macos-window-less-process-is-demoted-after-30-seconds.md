---
id: macos-window-less-process-is-demoted-after-30-seconds
title: On macOS a window-less process is demoted about 30 s after its launch and its logic falls to ~15 cycles/s
status: open
priority: high
scope: src/PlatformSpecific (macOS process activity), src/Core.cpp (the logic thread's deadline)
opened: 2026-11-22
tags: [macos, timing, window-less, logic-thread]
---

# On macOS a window-less process is demoted about 30 s after its launch

## Why

Measured by the macOS peer on 2026-11-22 (Mac mini M6, macOS 27.2, engine 0afa7ad5, a window-less instance of a
downstream testbed's city scene, sampled every 5 s):

| Process age | `ps` priority | Scene cycles per second |
|---|---|---|
| 12-27 s | 97 (the logic thread's time-constraint policy) | 59.9-60.1 |
| 33 s | 4 | 43 |
| beyond | 4, for all 68 threads (`4T` in `ps -M`) | 12.1-16.9, until the end |

The logic thread then sleeps 92 % of the time (1096 of 1192 samples in `nanosleep`): its `sleep_until` oversleeps by
about 50 ms a cycle, as if the Mach time-constraint policy of `PlatformSpecific::setCurrentThreadTimeConstraint()` were
lost. The shell that launched it stays at priority 31; a background Terminal.app sits at 4 too. The simulation runs at
about 20 % of the wall clock: a car asked to accelerate for 2.5 s reached 1.7-4 m/s instead of 13.

The peer's reading, NOT proven: App Nap, or the background demotion of an application that has no visible window. The
engine never declares an activity (`NSProcessInfo beginActivityWithOptions:reason:` appears nowhere).

## What remains

- Prove the cause: the same sampling with `NSAppSleepDisabled`, with an activity held
  (`NSActivityUserInitiated | NSActivityLatencyCritical`), and with a WINDOWED instance left in the background.
- Decide (owner) where the activity is held and for which runs: every run, or the window-less ones only.
- Measure it fixed: 60 scene cycles/s over two minutes at least, the logic thread's priority still 97 at the end.

## ⚠️ Traps

- **Every macOS window-less measurement older than 30 s is suspect**, the recorded ones included: the "60.00 cycles/s"
  of `docs/caution-points.md` § *macOS coalesces every non-real-time timed wait* was measured over 3 × 10 s.
- A verification protocol that waits in WALL seconds (hold a key 2.5 s, capture 1 s later) gets a fifth of the logic
  cycles it expects: count cycles (`getNodePhysics().sceneCycle`), or run it within the first 30 s of a fresh instance.
- A slow frame rate stretches every temporal effect in wall time (TAA and the GI converge per FRAME).

## References

- `docs/caution-points.md` § *macOS coalesces every non-real-time timed wait by 5 to 8 ms* (the deadline, the Mach
  time-constraint policy).
- `src/PlatformSpecific/` (`setCurrentThreadTimeConstraint()`), `Core::logicsTask()`.
