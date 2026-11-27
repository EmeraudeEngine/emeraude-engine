---
id: ambience-plays-its-loop-from-the-loading-thread
title: Audio::Ambience starts its loop from the sound's loading thread, on members the logic thread owns
status: open
priority: high
scope: Audio/Ambience (onNotification(), start(), stop(), its looped source and loop sound)
opened: 2026-11-27
tags: [defect, concurrency, audio, read-not-observed]
---

# Audio::Ambience starts its loop from the sound's loading thread, on members the logic thread owns

## Why

Read on 2026-11-27, not observed (no report, no crash): `Ambience::start()` observes a loop sound that is still
loading (`this->observe(m_loopSound.get())`); `Ambience::onNotification()` then runs on the worker that ends the
load (a resource emits `LoadFinished` there: scenes doc 12, the hand-off of a load completion) and, with no lock:

- `LoadFinished` → `m_loopedSource->play(m_loopSound, PlayMode::Loop)`: the logic thread (the scene's cycle) and the
  console commands (`stopAmbience()`, `setAmbienceLoop()`, under the scene's exclusive access the worker does not
  take) write the same two members; `stop()` resets `m_loopedSource`, so a stop that lands between the start and the
  end of the load leaves the handler a null pointer to call through;
- `LoadFailed` → `m_loopSound.reset()` on the worker while the logic thread may read it.

The family's rule (scenes doc 12): a `LoadFinished` handler writes nothing but a flag; the object's logic step acts
on it. `Scenes::Component::SoundEmitter` already follows it for its awaited play (started by its `processLogics()`,
cancelled by `stop()`).

## What remains

- The owner's choice of the shape — the sound emitter's (an atomic record, the ambience's update on the logic thread
  starts the loop and `stop()` cancels the wait) is the one at hand.
- Proof: ThreadSanitizer, a scene whose ambience loop is a sound not yet loaded (the pool held busy or the process
  on two cores, see the traps of `scene-reads-a-background-texture-while-it-is-created`), `stopAmbience()` sent
  during the load: 0 report naming `Ambience`, no crash.

## References

- `src/Audio/Ambience.cpp` (`onNotification()`, `start()`, `stop()`), `src/Scenes/Component/SoundEmitter.cpp` (the
  awaited play).
- `docs/subsystems/scenes/12-frame-synchronization-double-buffering-contract.md` (the load completion's hand-off).
