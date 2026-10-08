---
id: audio-thread-member-order
title: Audio threads declared before the state they use (TrackMixer, Recorder)
status: open
priority: high
scope: src/Audio/TrackMixer.hpp, src/Audio/Recorder.hpp
opened: 2026-10-08
tags: [ave-robustus-ii, concurrency, defect]
---

# Audio threads declared before the state they use (TrackMixer, Recorder)

## Why
`Audio/TrackMixer.hpp:514-517`: `Base::Thread m_eventThread;` is declared BEFORE `m_stateAccess`, `m_fadeCv` and
`m_stopThread` — members are destroyed in reverse order, so without `onTerminate()` (path of
`core-init-failure-skips-terminate`) the destructor joins a thread that may wait on an already destroyed condition
variable: UB, hang at exit. `Audio::Recorder`: same with `m_renderRunning`; its ALC capture device / context are raw
handles released only in `onTerminate()`.

## What remains
- P0: declare every thread member AFTER the state it uses, and stop + join in the destructor.
- P1 follow-up: `Base::Thread` stop_token model removes the hand-made stop flag.
- ALC device / context in RAII holders (decision D3-a).

## References
- projet-alpha `docs/plans/ave-robustus-ii.md` § 3.1 H2.
