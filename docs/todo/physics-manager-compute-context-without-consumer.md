---
id: physics-manager-compute-context-without-consumer
title: Physics::Manager builds a GPU compute context that nothing uses
status: open
priority: unranked
scope: Physics/Manager
opened: 2026-11-15
tags: [physics, vulkan, investigation, clean-up]
---

# Physics::Manager builds a GPU compute context that nothing uses

## Why

Found while drawing `docs/lifecycle-map.html` (Internals → Physics::Manager). `Manager::onInitialize()`
(`src/Physics/Manager.cpp:53`) picks a compute device and starts a TransferManager, a LayoutManager, a descriptor pool
and a command pool ("physics acceleration"). No consumer was found: outside `Core`, nothing calls
`Core::physicsManager()` (engine and projet-alpha), and the simulation runs scene-side on the logic thread
(`Scene::processLogics()`, `src/Scenes/Scene.physics.cpp`: SoftStepSolver, NarrowPhase).

Cost today: a compute device and its pools created at every start, and a secondary service whose failure is a
warning nobody acts on.

## What remains

1. Confirm there is no indirect consumer (the compute device through `Vulkan::Instance`, a planned GPU path in
   `docs/physics-overhaul.md`).
2. Owner decision: keep it as the scaffolding of a GPU physics path (then say so in `docs/physics-system.md`), or
   remove it (then remove it from the lifecycle map's ENTITIES / EVENTS / DEPS).

## References

- `docs/lifecycle-map.html` → Open points.
- `docs/physics-system.md`, `docs/physics-overhaul.md`.
