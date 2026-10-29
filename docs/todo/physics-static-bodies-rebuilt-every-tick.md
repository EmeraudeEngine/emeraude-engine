---
id: physics-static-bodies-rebuilt-every-tick
title: The physics step gathers, sorts and bounds EVERY collidable entity each tick, the static ones included
status: open
priority: high
scope: Scenes/Scene.physics.cpp (resolveCollisions), Scenes/Scene.cpp (processLogics), Physics/TriangleMeshCollisionModel
opened: 2026-10-29
tags: [physics, performance, scale, measured]
---

# The physics step gathers, sorts and bounds EVERY collidable entity each tick, the static ones included

## Why

A scene's per-tick physics cost grows with its STATIC world, not just with what moves. `Scene::resolveCollisions()`
walks every sector of the physics octree each tick, collects every entity with a collision model, sorts them by
creation number, then gives each one a body slot (`bodies.emplace_back()`, `bodyIndices[entity] = 0` for a static
one), and the broad phase computes world bounds (`TriangleMeshCollisionModel::getAABB(worldFrame)`: eight corners
transformed) again. `Scene::processLogics()` also calls `processLogics()` on every static entity each tick.

Measured on projet-alpha's `city` at side 128 (2026-10-29), the first street-props lot, which had ~4100 more entities
(about 1000 of them solid region meshes):
- Windows (RTX 3500 Ada, CPU-bound): the logic thread went from 66-70 % to 97-98 %, the frame rate halved.
- Linux: the logic thread took 25.55 ms of CPU a frame. A `gdb` sampling found it in `resolveCollisions()` in 11 of 18
  samples (`TriangleMeshCollisionModel::getAABB`, the octree walk).

The demo was fixed on its side: one instanced entity per region instead of one entity per merged mesh. The engine
cost stays: the next demo with many static solids (buildings, props) pays it again.

## What remains

- [ ] Profile the logic thread on `city 128,1,12,0` (day, spawn, `ab-cpu`-style CPU per frame plus a `gdb` sampling
  run from the binary's directory: `./libEmeraude.so.1` is found relative to the working directory). Split the time
  between `processLogics` over the static entities, the gather and sort, the body setup, and the broad phase.
- [ ] Decide with the owner. Candidates:
  - keep the static bodies between ticks and rebuild them only when the static set changes (an entity added, removed,
    moved, or its collision model replaced), with their world bounds cached;
  - let the broad phase query only the sectors the MOVING bodies reach, instead of gathering every body;
  - skip `processLogics()` for a static entity that has nothing to process (no animated component, no logic).
- [ ] Prove it: physics determinism unchanged BY CYCLE (`physics-bench.py --compare`); the logic thread's CPU per frame
  on `city 128` before and after (median of 5 runs).

## ⚠️ Traps

- The body order is the creation order (index 0 = the static world): the determinism by cycle depends on it.
- A static entity can move (an editor, a script): the cache must be invalidated by the octree's own relocation path
  (`checkEntityLocationInOctrees()`), never by a guess.

## References

- `src/Scenes/Scene.physics.cpp` (`resolveCollisions()`, step 1 "THE BODIES"), `src/Scenes/Scene.cpp`
  (`processLogics()`), `src/Physics/TriangleMeshCollisionModel.cpp` (`getAABB(worldFrame)`).
- projet-alpha `docs/subsystems/builtin/27-city-the-tile-built-city.md` § Street props (the measurements).
