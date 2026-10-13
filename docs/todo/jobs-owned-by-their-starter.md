---
id: jobs-owned-by-their-starter
title: Every Base::Thread owner stops its thread through the thread's std::stop_token; remove the hand-made stop flags
status: blocked
priority: high
scope: the engine's Base::Thread owners (Audio/TrackMixer, Audio/Recorder, Audio/ExternalInput, Console/RemoteListener, Graphics/Recorder, TracerLogger, Core loops)
opened: 2026-10-08
blocked-by: [task-handle-and-stop-token]
tags: [ave-robustus-ii, concurrency, lifetime]
---

# Every Base::Thread owner stops its thread through the thread's std::stop_token; remove the hand-made stop flags

## Why

Phase P2 of Ave Robustus II, its dedicated-thread half (plan § 4.2): a thread stops itself. Today every engine owner of
a `Base::Thread` stops it with a hand-made flag (and often a condition variable), and its safety rests on the order of
its member declarations — the H2 class of the census (a thread declared before the stop flag or the condition variable
it waits on: the destructor joins a thread that may wait on a destroyed member). The thread-pool half (the LOD jobs,
`FrameCapture`, `Net::Manager`, `APIClient`, the `Resources::Manager` release pass, the `Container` loading tasks) was
split out on 2026-10-13 into `thread-pool-jobs-owned-through-a-task-handle`, which the owner ordered started now: the base already
ships what it needs.

⚠️ **Blocked on emeraude-base** `task-handle-and-stop-token` (another repository): `Base::Thread` must follow the
`std::jthread` model first — a `std::stop_token` handed to the body, request_stop + join in the destructor, no
`detach()` (base `src/Thread.hpp:134`, `src/Thread.cpp:269` today).

The `Base::Thread` owners (verified 2026-10-13):
- `Audio/TrackMixer.hpp:636` `m_eventThread` — `m_stopThread` (`:627`) and `m_fadeCv` (`:611`, a
  `condition_variable_any`), declared before the thread since the P0 fix of H2.
- `Audio/Recorder.hpp:255` `m_renderThread` — `std::atomic< bool > m_renderRunning` (`:247`).
- `Audio/ExternalInput.hpp:180` `m_process`.
- `Console/RemoteListener.hpp:232` `m_networkThread` — `std::atomic< bool > m_running` (`:243`).
- `Graphics/Recorder.hpp:475` and `:537` `encodingThread` (one per session kind) — `queueMutex` / `queueCV` before it,
  ⚠️ `threadRunning` (and `finished`) declared AFTER it: destroyed first if the session dies with its thread running.
- `TracerLogger.hpp:315` `m_thread` — ⚠️ `m_condition` (`:319`) declared after it (same hazard).
- `Core.cpp` `logicsThread` / `renderingThread` (locals of `Core::run()`, `:528-531`), stopped by the loop flags.
Not in scope: `Graphics/Renderer.hpp:1947` `m_pipelineCacheRestore` (an uninterruptible driver call, joined in
`onTerminate()` and the destructor: owner D3) and `PlatformSpecific/Helpers.linux.cpp:168` (a local worker joined
before return) — both already self-contained.

## What remains

- After base `task-handle-and-stop-token` (the `Base::Thread` part): each owner above gives its body the thread's
  `std::stop_token`, waits with `std::condition_variable_any::wait(lock, stopToken, predicate)` where it waits, and
  drops its hand-made stop flag; the destructor's request_stop + join replaces the hand-written stop sequence. The
  member-order hazards above disappear with the flags.
- Proof: each owner's start / stop / destroy-without-terminate path under TSan (the H1 / H2 scenarios: an
  `initialize*()` failure, a destructor without `onTerminate()`), 0 report; shutdown unchanged on the three OS.

## ⚠️ Traps

- A thread joining itself aborts in every build (base D2, `BaseThreadDeathTest.aThreadJoiningItselfAborts`): an owner
  destroyed from its own thread is a contract fault, not a case to tolerate.

## References

- projet-alpha `docs/plans/ave-robustus-ii.md` § 3.1 (H1, H2), § 4.2, § 5 P2.
- Items: `thread-pool-jobs-owned-through-a-task-handle` (the thread-pool half), base `task-handle-and-stop-token`.
