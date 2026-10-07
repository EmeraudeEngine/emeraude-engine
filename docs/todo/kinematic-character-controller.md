---
id: kinematic-character-controller
title: Kinematic character controller — a walking state that is not a collision bounce
status: in-progress
priority: medium
scope: Physics, Scenes (Node), a new character trait or component
opened: 2026-10-01
tags: [physics, character-controller, physics-overhaul]
---

# Kinematic character controller — a walking state that is not a collision bounce

## Why

Phase P4 of the physics overhaul (`docs/physics-overhaul.md` § 1.2, § 2 F). The owner wants a body walking on the ground
or on another object to be in a special state, so that walking does not "misbehave" by reacting like a basic
collision. Owner decision (2026-10-01): a KINEMATIC character (collide and slide), the way PhysX, Jolt, Godot and Unity
do it. The ethereal player (free fly) keeps today's behaviour; choosing between the two belongs to projet-alpha.

## Done (2026-10-02, engine)

The controller, its component and its step in the scene (`docs/subsystems/physics/17-kinematic-character.md`): capsule,
collide and slide, slope limit, step-up with the stair walk, ground probe and snapping, moving support, bounded push,
landed / left ground / hit wall events, console commands; `collision-debug` ROW 6 stations — flat, 30° and 50° ramps,
the 0.29 m stairs, the moving platform, standing on a box, pushing a box, no vertical jitter on any.

## What remains

- [ ] projet-alpha: the actors on the controller (`actors-kinematic-character-migration`), citadel's stairs first.
- The dead teleport step-up is removed (`MovableTrait::setStepHeight()`, 2026-10-02) and `physics-step-up-pass` closed.

## ⚠️ Traps

- The Player's jump is a force ramped over 16 ticks today (`projet-alpha src/Actor/Player.cpp`); a kinematic jump is a
  launch velocity. Agree the feel with the owner before migrating.
- The citadel stairs are 34 steps of 0.29 m and the player's step height is 0.35 m: the first acceptance test.

## References

- K. Fauerby, "Improved Collision Detection and Response" (2003); PhysX Character Controller guide; Jolt
  `CharacterVirtual` (MIT); Godot `CharacterBody3D.move_and_slide()`.
- projet-alpha `actors-kinematic-character-migration` (the consumer side).
