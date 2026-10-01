---
id: rotational-physics
title: Enable rotational physics
status: blocked
priority: unranked
scope: Physics (MovableTrait, BodyPhysicalProperties), Scenes (Node)
opened: unknown
blocked-by: [rigid-body-math-helpers, physics-unified-contact-pipeline]
tags: [physics, physics-overhaul]
---

# Enable rotational physics

Inherited from the historical root `TODO.md` (marked WIP, no date). Since 2026-10-01 it is phase P3 of the physics
overhaul (`docs/physics-overhaul.md` § 2 D), with `physics-oriented-box-collision-model`.

## What is there

The solver already computes angular impulses from off-centre contacts (`ConstraintSolver.cpp`), `MovableTrait` keeps an
angular velocity and a cached inverse world inertia (`updateInverseWorldInertia()`, refreshed by
`Node::onLocationDataUpdate()`), and `enableRotationPhysics()` switches it on per entity (`collision-debug`'s
`DynTopCube` uses it). Only movable ↔ movable contacts produce torque until `physics-unified-contact-pipeline` lands.

## Done in P2 (2026-10-01)

- `Node::rotateFromPhysics()` takes a WORLD axis (bench `BenchSpinner`: upward Y and backward Y now constant); the
  solver integrates the orientation with `RigidBody::integrateOrientation()` (exp-map, world space).

## What remains

- [ ] The inertia tensor defaults to the identity whatever the mass and the shape (the toolkit's generated components
  carry `{}` = the identity; `docs/caution-points.md` § the body properties set on an entity are overwritten): derive it from the collision shape
  and the mass (`rigid-body-math-helpers`) unless the author sets one. Decide with the owner whether the JSON may set
  it (today it is code-only, `InertiaKey` is not read).
- [ ] Angular drag `ω *= 1 − drag` per tick is tick-rate dependent: integrate it exactly, like the linear drag
  (`Physics::getDragVelocityFactor()`).
- [ ] Rolling: a sphere rolling down a slope rolls without slipping (needs friction in the solver for ground contacts).
- [ ] `collision-debug`: the tipping cube rotates about the right axis and comes to rest on a face; 5 launches agree.
