---
id: scene-controllers-move-nodes-outside-the-logic-cycle
title: The scene's node controllers record on the main thread and the logic thread applies - done on Linux, awaits the peers
status: in-progress
priority: high
scope: Scenes/AbstractNodeController, Scenes/NodeController, Scenes/OrbitController, Scenes/Scene
opened: 2026-11-15
tags: [concurrency, input, defect]
---

# The scene's node controllers record on the main thread and the logic thread applies - done on Linux, awaits the peers

## Why

`NodeController` and `OrbitController` are input listeners: their callbacks run on the MAIN thread, inside the input
manager's event pump, and they moved the controlled node right there while the logic thread processed and published
it. Confirmed for `OrbitController` (the engine's model and image viewers): 31 ThreadSanitizer reports on one drag -
the camera's view rewritten under the logic thread's publication. `NodeController` has no caller that gives it a node;
its keypad path had the same shape.

Done on 2026-11-17 (owner decisions: the input records, the logic applies; the same contract for both; one family):
`Scenes::AbstractNodeController` carries the contract, the two controllers are its finalities, the scene updates and
releases them through the base. The contract is scenes doc 02 § Controllers; the proof is input doc 03 § *What a
listener DOES*; the lifecycle map has the synchronization point S22.

## What remains

- **The peers' round** (macOS, Windows): Release and Debug with 0 warning (a new source file,
  `src/Scenes/AbstractNodeController.cpp`: RECONFIGURE; a class deriving a base and a listener interface, exported
  with `EMEN_API`; designated initializers in a `constexpr std::array` of a local struct; the thread-safety
  annotations on two mutexes per controller); the model viewer opened through `Core.openFiles`, a drag and wheel
  notches injected: the camera node's position at each stage as on Linux (input doc 03), exit 0, 0 VUID.
- Record the verdicts in input doc 03 and the ledger, then delete this file.

## ⚠️ Traps

- `NodeController`'s gamepad page cannot be exercised by injection (no console command injects a gamepad button): its
  bindings were compared with the code they replace, nothing more.
- `controlNode()` places the node at once: it belongs to the thread that builds the scene, or to the logic thread -
  called on a running scene from another thread, it is the race this item removed.

## References

- `src/Scenes/AbstractNodeController.hpp` (the contract), scenes doc 02 § Controllers, input doc 03.
