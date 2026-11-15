---
id: scene-controllers-move-nodes-outside-the-logic-cycle
title: NodeController and OrbitController move the controlled node from the main thread, outside the logic cycle (suspected race)
status: open
priority: high
scope: Scenes/NodeController, Scenes/OrbitController, Input
opened: 2026-11-15
tags: [concurrency, suspected, verification]
---

# NodeController and OrbitController move the controlled node from the main thread, outside the logic cycle (suspected race)

## Why

Found while drawing `docs/lifecycle-map.html` (data flow "a key press to the screen"). Both controllers are input
listeners registered by `Scene::enable()`, so their callbacks run on the MAIN thread, inside
`Input::Manager::waitSystemEvents()`. They move the node right there:

- `src/Scenes/NodeController.cpp:88` `NodeController::onKeyRelease()` calls `m_controlledNode->moveX/moveY/moveZ()`;
- `src/Scenes/OrbitController.cpp` `onPointerMove()` / `onButtonPress()` / `onMouseWheel()` → `applyToNode()` (:185).

Neither file takes `betweenLogicCycles()` nor an exclusive scene access, and the input manager knows nothing of scenes.
Meanwhile the logic thread reads and writes the same node (`Scene::processLogics()`, physics, publication) under a
SHARED access: nothing orders the two. If confirmed, it is the same family as the logic ↔ rendering races fixed on
2026-11-15 (scenes doc 12 § *Plain state shared by the logic and rendering threads*) — main ↔ logic here.

## What remains

1. Confirm the thread of each callback and whether the node's transform is guarded elsewhere (a lock inside `Node`,
   a deferred command queue) — read `Node::moveX()` and what `processLogics()` touches.
2. If it races: reproduce under TSan (drive a scene with the orbit / node controller while the logic runs), then
   decide the contract with the owner — defer the change to the logic thread (a command applied in the next cycle),
   wrap it in `betweenLogicCycles()`, or keep the controllers' state as atomics read by the logic.
3. Record the outcome in `docs/subsystems/input/03-device-state-across-threads.md` and the lifecycle map (NOTES, and
   a synchronization point if one is added).

## ⚠️ Traps

- `betweenLogicCycles()` must not be called inside a scene access (three-thread deadlock, `Scenes/Manager.hpp`).
- Listener switches and device slots already have a rule (change them inside `betweenLogicCycles()`); this item is
  about what the listeners DO, not their registration.

## References

- `docs/lifecycle-map.html` → Open points; Data flows → "A key press".
- `docs/subsystems/scenes/12-frame-synchronization-double-buffering-contract.md`.
