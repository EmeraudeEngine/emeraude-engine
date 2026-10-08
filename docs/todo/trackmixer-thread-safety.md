---
id: trackmixer-thread-safety
title: TrackMixer reads its playback state without the lock from two threads
status: open
priority: high
scope: src/Audio/TrackMixer.cpp
opened: 2026-10-08
tags: [ave-robustus-ii, audio, concurrency, defect]
---

# TrackMixer reads its playback state without the lock from two threads

## Why
Found during the Ave Robustus II P0 fix of the event-thread lifetime (2026-10-08). The event loop and the
cross-fade are now under `m_stateAccess`, but `TrackMixer::next()` (called from the main thread AND from the event
thread) still reads `m_userState`, `m_playlist` and other members before taking the lock, while other methods write
them under it — a data race (UB), not proven harmful yet.

## What remains
- One locking protocol for every member (document it: "m_stateAccess guards …"), Clang thread-safety annotations
  (`GUARDED_BY`, decision D12) on them, then an engine TSan run with music playing, skipping and cross-fading.

## References
- projet-alpha `docs/plans/ave-robustus-ii.md` § 5.
