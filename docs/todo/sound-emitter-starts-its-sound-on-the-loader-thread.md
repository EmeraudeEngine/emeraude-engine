---
id: sound-emitter-starts-its-sound-on-the-loader-thread
title: SoundEmitter starts (or drops) its attached sound from the resource's loading thread
status: open
priority: high
scope: Scenes/Component/SoundEmitter
opened: 2026-11-15
tags: [concurrency, defect, code-reading]
---

# SoundEmitter starts (or drops) its attached sound from the resource's loading thread

## Why

`SoundEmitter::play()` observes a sound that is not loaded yet; `onNotification()` then answers
`ResourceTrait::LoadFinished` with `playAttachedSound()` and `LoadFailed` with `releaseAttachedSound()`. A resource
notifies on the thread that completed its load — a pool worker — so those two run there: `playAttachedSound()` WRITES
`m_source` (a `std::shared_ptr`, `requestSource()`), reads `m_gain`, `m_pitch`, the flags and `m_attachedSound`, and
drives the source; the logic thread reads, tests and resets the same `m_source` in `processLogics()` and `move()`
(`m_source->isStopped()` → `m_source.reset()`), and the owner's thread calls `stop()` / `clear()`. Nothing orders them.

**By reading the code** (audit of the load handlers, 2026-11-15) — NOT reported by ThreadSanitizer: one run of the
testbed's `game-logic` showed no report on this class (its sounds were loaded before an emitter waited for one). The
audio manager's source pool is locked (`m_sourcePoolMutex`); the emitter's own members are not.
`play()` also has the check-then-act window `CloudVolume` covers and this class does not: a sound that finishes loading
between `isLoaded()` and `observe()` is never played.

## What remains

- The contract exists since 2026-11-15 (scenes doc 12 § *A resource's load reaches a scene object on the logic
  thread*): the handler calls `Component::Abstract::recordResourceLoaded()` and `processLogics()` consumes it —
  `playAttachedSound()` then runs on the logic thread. The failure needs its own record (a second flag, or a small
  atomic state: loaded / failed).
- `play()`: observe, THEN test `isLoaded()` again.
- Proof: a TSan run of a scene where an emitter plays a sound still loading (a large sound requested and played in
  the same cycle): the report before, 0 after; the sound is heard (the TrackMixer / source state through the console).

## References

- `Scenes/Component/CloudVolume.cpp` (the check-then-act written out).
