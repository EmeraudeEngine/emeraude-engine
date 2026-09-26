---
id: tick-timers-truncate-to-16-ms
title: Tick-based timers add 16 ms per 60 Hz tick instead of 16.67 — every Sequence and lifetime runs 4 % slow
status: open
priority: unranked
scope: Animations/Sequence.cpp, Audio/Ambience.cpp, Physics/Particle.cpp; projet-alpha actors (Explosion, Fire, Smoke)
opened: 2026-09-26
tags: [timing, animation, correctness]
---

# Tick-based timers add 16 ms per 60 Hz tick instead of 16.67 — every Sequence and lifetime runs 4 % slow

## Why

`WorldPhysicsUpdateCycleDurationMS< uint32_t >` is `1000 / 60` in INTEGER arithmetic: **16**, not 16.67 ms
(`src/Constants.hpp` ~88-97). Every timer that adds it once per logic tick loses 0.67 ms per tick, i.e. runs
**4.2 % slow** in real time: `Animations/Sequence.cpp:535` (every animated value — a `Sequence(15000)` lasts 15.6 s,
a `Sequence(30000)` 31.25 s, as measured for basic-scenery's flying lights), `Audio/Ambience.cpp:440`,
`Physics/Particle.cpp:452-454` (particle lifetimes), and projet-alpha `Explosion.cpp:105`, `Fire.cpp:92, 129`,
`Smoke.cpp:86` (lifetimes, damage timers). Found 2026-09-26 while explaining the macOS peer's measured smoke-loop period.

## What remains

1. Count TICKS and derive the time exactly (`elapsedMS = ticks * 1000 / 60` in 64-bit integers), or keep the time in
   microseconds with the tick's exact remainder carried — never accumulate a truncated (or a float) step.
2. Fix every site listed above, then grep again for `CycleDurationMS< uint32_t >` / `< int >`.
3. A unit-level check: a `Sequence(15000)` reaches its end after exactly 900 ticks.

## ⚠️ Traps

- Do not "fix" it with a float accumulator (project rule: no floating-point accumulation — the error grows with time).
- The measured macOS smoke period (20.8 s for a 15 s sequence) is only partly this (15.6 s): the rest points at the logic
  running below 60 ticks per second under load — item `logic-tick-rate-under-render-load`.
