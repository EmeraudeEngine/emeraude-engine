---
id: physics-unified-contact-pipeline
title: One contact pipeline for every solid — persistent manifolds, warm starting, soft step
status: in-progress
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

## Owner decisions (2026-10-01)

`docs/physics-overhaul.md` § 1.6: one pipeline; the models mapped to the base primitives (AABB axis-aligned in P2);
the ground's real triangles through a new `GroundLevelInterface` query; a creation number per entity for sorting and
manifold keys; Box2D v3 soft step; friction geometric mean, restitution max; walkers may regress until P4;
`DynTopCube` moved to component properties; the old MTV overlap tests retired afterwards.

## Done (2026-10-01)

The pipeline, the narrow phase, the soft-step solver, persistent impulses, materials, gravity always on, grounded state
from the manifolds, sorted manifolds, the boundary pass after the solver, kinematic bodies, world-axis rotation — see
`docs/physics-overhaul.md` § 1b (P2.b/c) for the measurements. Linux 5 launches bit-identical.

## Done (2026-10-02)

- The peers' recorded bench: 0 differing samples on all 17 stations, 5 runs, on macOS M2 and Windows NVIDIA + AMD
  (`docs/physics-overhaul.md` § 1b); run-to-run determinism closed on the three OS.
- The collision models' MTV tests (`isCollidingWith()`, `collideWith*()`, `CollisionDetectionResults`) removed with
  their four `.cpp` files: nothing called them since P2 (P3.a, with `BoxCollisionModel`).

## What remains

- [ ] Retire the MTV overloads of base `Math/Space3D/Collisions/` (`isColliding(a, b, mtv)`, owner decision 6i); keep
  the boolean overlaps (lights, octree, editor, push modifiers use them) and fix only their boolean defects
  (`collision-pair-test-defects`).

## ⚠️ Traps

- The demos tune actors with forces against the current solver (Paladin, Drone, Fox, Player): re-check them.
- No pair dedup set must come back (`src/Scenes/AGENTS.md` § Octree storage and traversal); the persistent manifold
  map is keyed by pair, it is not a per-tick dedup.
- Never emit `onCollision` while iterating the manifolds if a handler may modify the scene: collect, then emit.

## References

- E. Catto, "Iterative Dynamics with Temporal Coherence" (GDC 2005); E. Catto, "Solver2D" (2024); Box2D v3
  `solver.c`, `contact_solver.c` (MIT).
- `src/Physics/ConstraintSolver.cpp`, `src/Scenes/Scene.physics.cpp`, `src/Physics/MovableTrait.cpp`.
