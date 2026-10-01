---
id: kinematic-character-controller
title: Kinematic character controller — a walking state that is not a collision bounce
status: blocked
priority: unranked
scope: Physics, Scenes (Node), a new character trait or component
opened: 2026-10-01
blocked-by: [shape-casts-with-hit-normal, physics-unified-contact-pipeline]
tags: [physics, character-controller, physics-overhaul]
---

# Kinematic character controller — a walking state that is not a collision bounce

## Why

Phase P4 of the physics overhaul (`docs/physics-overhaul.md` § 1.2, § 2 F). The owner wants a body walking on the ground
or on another object to be in a special state, so that walking does not "misbehave" by reacting like a basic
collision. Owner decision (2026-10-01): a KINEMATIC character (collide and slide), the way PhysX, Jolt, Godot and Unity
do it. The ethereal player (free fly) keeps today's behaviour; choosing between the two belongs to projet-alpha.

## What remains

- [ ] Architecture, to present to the owner before any code: a `Physics` trait or a component on a `Node`; how it
  coexists with `MovableTrait` (a kinematic body is moved by the controller, never integrated by the solver, but is a
  solid for the dynamic bodies); its public API (wanted horizontal velocity, jump, state query, events).
- [ ] Shape: a capsule (feet at the node origin, like the actors' feet-anchored AABB today).
- [ ] Move by collide and slide: sweep the capsule along the wanted motion (`shape-casts-with-hit-normal`), stop at the
  first hit, slide along the hit plane, a bounded number of iterations; depenetrate at the start of a tick.
- [ ] States: grounded (walking), airborne (jumping, falling), with explicit transitions and no grace-period counter.
  Grounded = a downward probe finds a walkable surface within a snap distance.
- [ ] Slope limit (a walkable angle per character); a steeper surface is a wall.
- [ ] Step-up by sweep (up, forward, down) with a headroom test; step-down / ground snapping so walking down stairs and
  slopes does not go airborne. Supersedes `physics-step-up-pass` (the teleport step-up in
  `Scene::accumulateStaticEntityCorrections()`).
- [ ] Moving support: inherit the velocity of the body under the feet (a platform, another node).
- [ ] Interaction: push dynamic bodies with a bounded force; dynamic bodies see the character as a solid.
- [ ] Events: landed (with the impact speed, for fall damage — today `NodeCollision` carries an impact force),
  left the ground, hit a wall.
- [ ] `collision-debug` station: walk a flat floor, a 30° and a 50° slope, the 0.29 m steps, a moving platform, stand
  on a box; no vertical jitter on any of them (record the Y amplitude over 5 s).

## ⚠️ Traps

- The Player's jump is a force ramped over 16 ticks today (`projet-alpha src/Actor/Player.cpp`); a kinematic jump is a
  launch velocity. Agree the feel with the owner before migrating.
- The citadel stairs are 34 steps of 0.29 m and the player's step height is 0.35 m: the first acceptance test.

## References

- K. Fauerby, "Improved Collision Detection and Response" (2003); PhysX Character Controller guide; Jolt
  `CharacterVirtual` (MIT); Godot `CharacterBody3D.move_and_slide()`.
- projet-alpha `actors-kinematic-character-migration` (the consumer side).
