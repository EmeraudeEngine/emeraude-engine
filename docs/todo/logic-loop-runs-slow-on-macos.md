---
id: logic-loop-runs-slow-on-macos
title: The logic loop runs at ~44 cycles/s on macOS instead of 60 (sleep overshoot)
status: open
priority: high
scope: Core (logicsTask), platform timers
opened: 2026-10-05
tags: [timing, macos, determinism, measured]
---

# The logic loop runs at ~44 cycles/s on macOS instead of 60

## Why

Measured by macOS-PA on the M2 (2026-10-05, engine `61943c08`), while validating the ethereal player: the scene cycle
counter (`getNodePhysics()` → `sceneCycle`) grew by 450 in 10 s on `forest` and 438 on `geometry-loader` — 43.5-45
cycles/s, windowed and window-less alike, App Nap ruled out (`-NSAppSleepDisabled YES`). `sample` puts the logic
thread 96 % inside `Core::logicsTask()`'s `std::this_thread::sleep_for(logicsUpdateFrequency - duration)`
(`src/Core.cpp`, around line 198) → `nanosleep`: a cycle lasts ~22.7 ms for ~0.9 ms of work, so a ~15.8 ms sleep
overshoots by ~6 ms. No `logicsTask` overrun warning is logged: it is not a slow cycle, it is a long sleep.

Consequence: everything simulated runs at ~73 % of real time on macOS (the player flies a correct 10 m/s per cycle,
~7 m/s on the wall clock). Windows may be hit the same way (15.6 ms default timer granularity) — not measured yet.

## What remains

- Owner decision (architectural): an absolute-deadline loop (`sleep_until` the next tick, catching up a missed one),
  a higher thread QoS on macOS (`QOS_CLASS_USER_INTERACTIVE`) for the logic thread, both — or a platform timer.
- Windows (2026-10-05, NVIDIA, simple-room): 564 / 562 / 564 cycles in 3 × 10 s = 56.2-56.4 cycles/s — slow too,
  less than macOS. Measure Linux the same way (scene cycles over 10 s of wall clock).

## ⚠️ Traps

- A relative `sleep_for(period - work)` accumulates every overshoot; only an absolute deadline cancels it.
- The physics bench compares BY CYCLE (`physics-bench.py --compare`): the cycle rate does not change those
  results, only the wall-clock pace.
