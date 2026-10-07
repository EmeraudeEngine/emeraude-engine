---
id: particles-emitter-timeout-unit
title: ParticlesEmitter::start(duration) arms its timeout in microseconds, documented as milliseconds
status: open
priority: high
scope: Scenes/Component/ParticlesEmitter
opened: 2026-09-27
tags: [particles, time, threading]
---

# ParticlesEmitter::start(duration) arms its timeout in microseconds, documented as milliseconds

## Why

`ParticlesEmitter::start(uint32_t duration)` is declared with "A timeout in milliseconds to stop the
emission automatically", but `ParticlesEmitter.cpp` builds the timer as
`Time::TimedEvent< uint64_t, std::micro >`: a 1000 ms request stops the emission after 1 ms. Found on
2026-09-27 while exposing the component to the console; nobody passes a duration today (every caller
uses `start()`), so the defect has never shown. The console command `ParticlesEmitter.start` exposes no
duration until this is fixed.

## What remains

1. Decide the unit (milliseconds as documented, or seconds like the rest of the scene API) and make
   the timer and the declaration agree.
2. The timer fires on the `TimedEvent` thread and calls `stop()` (a flag write) there, while the logic
   thread reads the flag: move the stop onto the logic cycle (count cycles in `processLogics()`, which
   also makes the timeout exact and deterministic) rather than keep a timer thread per emitter.
3. Then add the optional duration to the console command (`AnimationConsoleAdapters.cpp`).

## References

- `src/Scenes/Component/ParticlesEmitter.cpp` `start()`, `src/Scenes/Component/ParticlesEmitter.hpp`.
- `emeraude-base` `Time/TimedEvent.hpp` (`period_t` defaults to `std::milli`).
