---
id: node-animation-notifies-per-written-node
title: A node animation notifies once per written node — quadratic on a deep animated chain, and it keeps the physics awake
status: open
priority: unranked
scope: Scenes/Component (NodeAnimation, Vehicle), Scenes (Node)
opened: 2026-11-26
tags: [scenes, animation, performance]
---

# A node animation notifies once per written node — quadratic on a deep animated chain, and it keeps the physics awake

## Why

Since 2026-11-26 `setLocalCoordinates()` notifies like every other displacement (scenes doc 11 § *Every displacement
notifies*). `NodeAnimation::evaluateAtTime()` writes each node its clip drives through it, every logic cycle: each
write walks the written node's whole subtree (`Node::onLocationDataUpdate()`), so a chain of N animated nodes costs
about N² / 2 visits a cycle, and each visit wakes the node's physics simulation (`pauseSimulation(false)`): an
animated node never sleeps.

Measured on one animated node only (ChronographWatch): 1.0 to 1.5 µs a cycle. No asset at hand has a clip driving a
deep hierarchy, so the quadratic case is a reading of the code, not a measurement.

## What remains

- A measurement on a clip that drives many nodes of one hierarchy (an asset to find or to build), against the
  16.7 ms of a logic cycle.
- If it weighs: write every node first, then notify once per TOPMOST written node (each subtree walked once). It
  needs a write that does not notify, reserved to the animation — a contract to decide with the owner.
- Whether an animated node that carries a body should be kept awake every cycle.

## References

- `src/Scenes/Component/NodeAnimation.cpp` (`evaluateAtTime()`, `stop()`), `src/Scenes/Component/Vehicle.cpp`,
  `src/Scenes/Node.cpp` (`onLocationDataUpdate()`).
- `docs/subsystems/scenes/11-critical-points.md` § *Every displacement notifies*.
