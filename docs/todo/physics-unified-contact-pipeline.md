---
id: physics-unified-contact-pipeline
title: One contact pipeline for every solid — persistent manifolds, warm starting, soft step
status: blocked
priority: unranked
scope: Physics (ConstraintSolver, ContactManifold, ContactPoint, MovableTrait), Scenes/Scene.physics.cpp
opened: 2026-10-01
blocked-by: [physics-collision-debug-bench, contact-manifold-generation]
tags: [physics, solver, physics-overhaul]
---

# One contact pipeline for every solid — persistent manifolds, warm starting, soft step

## Why

Phase P2 of the physics overhaul (`docs/physics-overhaul.md` § 2 A, C, D). Today only movable ↔ movable contacts reach
the impulse solver; ground, boundary and static-entity contacts are a summed position push plus a velocity reflection
(`Scene::resolveCollisions()` phase 1, `applyCollisionResponse()`), with no torque and a per-tick friction multiplier.
The solver itself loses the bounce, over-corrects the penetration, never warm-starts, and follows the octree order.

## What remains

- [ ] Every solid contact becomes a manifold for the solver: ground (a static with inverse mass 0, normal and depth from
  the height field), static entities (inverse mass 0), movables. Phase 1's push-and-reflect goes away. Boundaries keep
  their hard clip as a final safety after the solver, outside it (owner decision, `docs/physics-overhaul.md` § 5).
- [ ] Persistent manifolds keyed by the pair (stable entity ids) and matched point by point through the feature ids of
  `contact-manifold-generation`; accumulated impulses carried over and warm-started.
- [ ] Restitution and position: the whole of `physics-solver-restitution-and-position-correction` (restitution from
  the pre-solve normal velocity with a threshold, one `onCollision` per contact per tick, no double correction).
- [ ] Stepping: soft step with sub-steps and relaxation (Box2D v3) at the fixed 60 Hz tick; choose the sub-step count
  on the P0 measurements.
- [ ] Coulomb friction in the solver for every contact, including the ground; remove `v.xz *= 1 − stickiness` from
  `MovableTrait::updateSimulation()`. Decide the combination rule of two materials with the owner (average today;
  geometric mean and max are the usual alternatives).
- [ ] Gravity always applied to a dynamic body (no switch-off while grounded, no zeroed downward velocity): resting
  becomes the solver's job. The grounded state of a dynamic body is read from its manifolds (a contact whose normal is
  within the floor cone), without a 15-tick grace period. ⚠️ The Player and the walkers still rely on the grace period
  until `kinematic-character-controller` replaces them: keep their behaviour or migrate them in the same step.
- [ ] Deterministic order: manifolds and contacts sorted by stable ids before solving (target: same machine, same
  binary, same inputs — owner decision 2026-10-01). Closes `physics-run-to-run-determinism` when 5 launches agree.
- [ ] Re-measure every P0 station; record the numbers in `docs/physics-overhaul.md` / the physics topic docs.

## ⚠️ Traps

- The demos tune actors with forces against the current solver (Paladin, Drone, Fox, Player): re-check them.
- No pair dedup set must come back (`src/Scenes/AGENTS.md` § Octree storage and traversal); the persistent manifold
  map is keyed by pair, it is not a per-tick dedup.
- Never emit `onCollision` while iterating the manifolds if a handler may modify the scene: collect, then emit.

## References

- E. Catto, "Iterative Dynamics with Temporal Coherence" (GDC 2005); E. Catto, "Solver2D" (2024); Box2D v3
  `solver.c`, `contact_solver.c` (MIT).
- `src/Physics/ConstraintSolver.cpp`, `src/Scenes/Scene.physics.cpp`, `src/Physics/MovableTrait.cpp`.
