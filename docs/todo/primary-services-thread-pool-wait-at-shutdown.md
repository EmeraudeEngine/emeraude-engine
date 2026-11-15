---
id: primary-services-thread-pool-wait-at-shutdown
title: PrimaryServices still calls ThreadPool::wait() at shutdown, against the shutdown-sequence doctrine
status: open
priority: unranked
scope: PrimaryServices, docs/subsystems/source-tree/09-shutdown-sequence.md
opened: 2026-11-15
tags: [shutdown, threadpool, doc-vs-code, owner-decision]
---

# PrimaryServices still calls ThreadPool::wait() at shutdown, against the shutdown-sequence doctrine

## Why

`docs/subsystems/source-tree/09-shutdown-sequence.md` § "`ThreadPool::wait()` is not a shutdown tool" says the
shutdown no longer calls it: every pool job is owned by its submitter (`TaskHandle` / `TaskGroup`), which stops and
waits it. Yet `PrimaryServices::Impl::terminate()` (`src/PrimaryServices.cpp:376`) still calls
`m_threadPool->wait()` before releasing the primary services, from `~Core()`.

At that point every job owner should be gone, so the wait should return at once. Either it is a harmless last guard,
or it hides an owner that forgot to wait its jobs (and it inherits `wait()`'s documented limits: it may return while
another thread keeps enqueuing).

## What remains

1. Verify it returns at once on a normal run (trace the time spent in it on a bench demo, three OS).
2. Owner decision: keep it (and say so in doc 09, as the one deliberate pool wait), or replace it with a check that
   the pool is idle (a contract fault naming the leaking owner), or remove it.
3. Update doc 09 and the lifecycle map (EVENTS "ThreadPool::wait()", NOTES).

## References

- `docs/lifecycle-map.html` → Open points; Chronogram, phase `~Core()`.
- emeraude-base `ThreadPool.hpp` (`wait()` contract).
