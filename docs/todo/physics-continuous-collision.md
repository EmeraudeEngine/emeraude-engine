---
id: physics-continuous-collision
title: Continuous collision for fast bodies — projectiles must not tunnel
status: in-progress
priority: unranked
scope: Physics, Scenes/Scene.physics.cpp
opened: 2026-10-01
tags: [physics, ccd, physics-overhaul]
---

# Continuous collision for fast bodies — projectiles must not tunnel

## Why

Phase P5 of the physics overhaul (`docs/physics-overhaul.md`). Collisions are tested at discrete positions only, once
per 60 Hz tick. A body that moves more than its own thickness in one tick can cross a wall (projet-alpha's Grenade,
Rocket and Shell are launched with `setLinearVelocity(forward * speed)`).

## Done (2026-10-02, the order revised: P5 before P4)

- Fast bodies (more than half their round core per step) are swept against the static world — ground triangles, static,
  kinematic and sleeping bodies — and put back at the first contact, their velocity bounced (`docs/physics-overhaul.md`
  § 1b; `subsystems/physics/02-physics-specific-rules.md` step 4b). Bench `BenchBulletBall` / `BenchBulletBox` (80 m/s
  at a 0.2 m wall): stopped at the wall.

## What remains

- [ ] Dynamic ↔ dynamic: two fast bodies, or a fast one against a moving one, are not swept (Box2D v3 sweeps "bullet"
  bodies against dynamic ones too). Decide with the owner whether a bullet flag is wanted.
- [ ] A box is swept as its inscribed sphere: conservative (it stops a little later than its faces would). A box
  caster in the base casts would make it exact.
- The GROUND no longer tunnels (one-sided and solid below, 2026-10-02): this item is about walls, thin solids and
  body ↔ body. ⚠️ A velocity-scaled speculative margin was tried on the ground and NOT kept: with the soft-step solver
  the whole stop of a fast impact then happens in one sub-step and the sequential impulses send a box sideways and in
  yaw (0.21 m for a box dropped flat from 5 m, 3.5 m from 95 m). Speculative contacts here need the solver's impact
  behaviour solved first (`docs/physics-overhaul.md` § 1b, the ground defect).
- Chosen by the owner (2026-10-02): the SWEPT test from the old to the new position, for every fast body (no flag).

## References

- E. Catto, "Continuous Collision" (GDC 2013); Box2D v3 `solver.c`, its continuous pass for "bullet" bodies (MIT).
