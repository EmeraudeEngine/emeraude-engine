---
id: character-node-velocity
title: A kinematic character's node velocity reads 0 — mirror the controller's velocity, or document it
status: open
priority: high
scope: Scenes (AbstractEntity / Node), Physics (CharacterController)
opened: 2026-10-02
tags: [physics, physics-overhaul, character]
---

# A kinematic character's node velocity reads 0 — mirror the controller's velocity, or document it

## Why

Seen by the macOS peer (2026-10-02): `getNodePhysics()`' top-level `linearVelocity` reads 0 for a P4 character while it
walks or flies; only `character.velocity` carries its motion. A character is MOVED by its controller (the scene sets
its position), so `MovableTrait::linearVelocity()` is never written. Consumers that read a node's velocity (projet-alpha
read `linearVelocity()` for the paladin animation before P4 — now `Abstract::actualSpeed()`; audio, a camera's motion
cues, gameplay code) see a still node.

## What remains

- [ ] Owner decision: the scene writes the controller's velocity into the node's `MovableTrait` after step 0 (one
  velocity everywhere — check that nothing then integrates it a second time), or the node velocity stays 0 by contract
  and every consumer asks the controller (documented in `subsystems/physics/17-kinematic-character.md`).
- [ ] List the readers of `linearVelocity()` / `getWorldVelocity()` in the cascade that can meet a character.

## References

- `src/Scenes/Scene.physics.cpp` step 0; `src/Scenes/Manager.console.cpp` (`getNodePhysics`).
