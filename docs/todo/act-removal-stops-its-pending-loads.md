---
id: act-removal-stops-its-pending-loads
title: Removing an act (a scene) stops the resource loads it started instead of waiting for them
status: open
priority: medium
scope: engine Resources (containers, ResourceTrait dependencies), Scenes (scene deletion); downstream Stage::unloadActiveAct()
opened: 2026-10-15
tags: [ave-robustus-ii, loading, shutdown, responsiveness]
---

# Removing an act (a scene) stops the resource loads it started instead of waiting for them

## Why

The stop already cancels what is still queued when `Resources::Manager::stopBackgroundWork()` runs (first step of
`Core::terminate()`, `docs/subsystems/resources/06-async-lambda-capture-safety.md`). But the application removes its
act BEFORE that, and the removal waits for every pending load of the act to complete: nothing is left to cancel when
the stop comes. Owner decision 2026-10-15: of the three ways to stop in-flight work earlier — (a) a "proceed stop" at
the shutdown request, (b) the act's loads stopped when the act is removed, (c) interruptible large loads — (b) is the
one to do.

## Measured

- Windows (RTX 3500 Ada, 2026-10-15): the act removal took 4.6 s during the sponza load; one uninterruptible IvySim
  load ~1.2 s.
- Linux (RTX 3070 Ti, test-only build with a 400 ms sleep per image file, never committed): `Core.shutdown()` executed
  at 3.17 s, "Removing the act 'sponza'" then lasted 84 s while the images loaded one after another
  (`Sprites/explosion005_NN`, the `MovieResource` `explosion005` and its `SpriteResource` last), then every container
  unloaded and `stopBackgroundWork()` reported 0 ms.
- macOS (M6): a quit at 2 s during the sponza load cancels ~390 loads at the stop and takes 0.55-0.91 s, because there
  the stop comes while the loads are still queued.

## What remains

- Find WHAT the removal waits on (a backtrace of the main thread during the wait in the slow build above): the scene
  deletion, an actor or a component holding a resource whose load is synchronous (`MovieResource` frames look
  sequential), or a container wait.
- Design (owner decision before code): how a load knows it belongs to an act or a scene (a requester token / a
  `TaskGroup` per scene / a stop token handed to the loads it starts), and what its dependents do when it is abandoned
  (the abandoned path exists: `ResourceTrait::abandonLoading()`, `loadAbandoned()`, its parents stay silent).
- A resource SHARED with another act or the default stores must not be cancelled by one act's removal.

## ⚠️ Traps

- A built-in act is still loaded synchronously on the main thread (`act-load-blocks-main-thread`): a console
  `Core.shutdown()` is only served once that load is over. This item is about the async loads that remain afterwards.
- Never cancel by a global flag (Ave Robustus II): the owner of the loads stops and waits for them.

## References

- Downstream: projet-alpha `src/Stage.cpp` `Stage::unloadActiveAct()` (disable the scene, erase the act, delete its
  scene).
- `src/Resources/Manager.cpp` `stopBackgroundWork()`, `src/Resources/Container.hpp` `stopBackgroundWork()`,
  `src/Resources/ResourceTrait.cpp` `abandonLoading()` / `goOnAfterDependencyFailure()`.
- Related: `act-load-blocks-main-thread`, `jobs-owned-by-their-starter`.
