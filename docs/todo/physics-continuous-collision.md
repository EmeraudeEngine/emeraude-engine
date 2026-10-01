---
id: physics-continuous-collision
title: Continuous collision for fast bodies — projectiles must not tunnel
status: blocked
priority: unranked
scope: Physics, Scenes/Scene.physics.cpp
opened: 2026-10-01
blocked-by: [shape-casts-with-hit-normal]
tags: [physics, ccd, physics-overhaul]
---

# Continuous collision for fast bodies — projectiles must not tunnel

## Why

Phase P5 of the physics overhaul (`docs/physics-overhaul.md`). Collisions are tested at discrete positions only, once
per 60 Hz tick. A body that moves more than its own thickness in one tick can cross a wall (projet-alpha's Grenade,
Rocket and Shell are launched with `setLinearVelocity(forward * speed)`).

## What remains

- [ ] Measure first: the speed at which a 0.1 m sphere crosses a 0.2 m wall at 60 Hz in `collision-debug`.
- The GROUND no longer tunnels (one-sided and solid below, 2026-10-02): this item is about walls, thin solids and
  body ↔ body. ⚠️ A velocity-scaled speculative margin was tried on the ground and NOT kept: with the soft-step solver
  the whole stop of a fast impact then happens in one sub-step and the sequential impulses send a box sideways and in
  yaw (0.21 m for a box dropped flat from 5 m, 3.5 m from 95 m). Speculative contacts here need the solver's impact
  behaviour solved first (`docs/physics-overhaul.md` § 1b, the ground defect).
- [ ] Choose with the owner: speculative contacts (Catto, GDC 2013 "Continuous Collision") or a swept test against
  statics only for bodies flagged as fast (Box2D v3 "bullet" bodies).

## References

- E. Catto, "Continuous Collision" (GDC 2013); Box2D v3 `solver.c`, its continuous pass for "bullet" bodies (MIT).
