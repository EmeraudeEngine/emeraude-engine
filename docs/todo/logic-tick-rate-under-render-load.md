---
id: logic-tick-rate-under-render-load
title: The logic may run below 60 ticks per second under a heavy render — to MEASURE before anything else
status: open
priority: unranked
scope: Core (logics task), Animations (tick-based time)
opened: 2026-09-26
tags: [timing, logic, measure-first]
---

# The logic may run below 60 ticks per second under a heavy render — to MEASURE before anything else

## Why

The macOS peer (Apple M2, validation ON, ~66-70 ms/frame) measured `game-logic`'s smoke circuit — a `Sequence(15000)`
advanced once per logic tick — at a **20.8 s** period (node positions polled every 0.28 s over 21.3 s). The integer
16 ms step explains 15.6 s (item `tick-timers-truncate-to-16-ms`); the remaining ×1.33 would mean ~45 logic ticks per
second instead of 60, i.e. the fixed 60 Hz logic loop falling behind (or skipping) while the render is slow. It is an
INFERENCE from one measurement, not a measured tick rate.

## What remains

1. Measure the logic tick rate directly under load (a tick counter over wall-clock time, e.g. through the console or
   `getStateSyncStatistics()`), on the M2 and on this machine, at a light and a heavy scene.
2. If it is below 60: find why (thread scheduling, a lock with the render thread, the logic's own cost) and decide the
   policy (catch-up ticks, or wall-clock-driven animation time).

## ⚠️ Traps

- Measure a timing defect with a COUNTER, never by eye (project memory: the logic→render slip of 2026-09-24).
- Fix `tick-timers-truncate-to-16-ms` first or subtract its 4.2 % before reading any period.
