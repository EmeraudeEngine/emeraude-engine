---
id: physics-run-to-run-determinism
title: The physics gives a different result on every launch of the same scene
status: open
priority: unranked
scope: Physics, Scenes (the physics step and the broad phase)
opened: 2026-10-01
tags: [physics, determinism, measured]
---

# The physics gives a different result on every launch of the same scene

## Why

This came out of the triad 11 peer validations (2026-10-01, engine `a777ddf7`). In `collision-debug`, `DynTopCube10`
falls on the edge of `DynBottomCube9` and tips over. It comes to rest at a different place on every launch:

| Machine | Resting x (m) |
|---|---|
| Windows, NVIDIA RTX 3060, same binary, 2 runs | 18.87, 12.60 |
| Windows, AMD, same binary, 2 runs | 10.86, 2.66 |
| macOS M2 | 18.23 |
| Linux RTX 3070 Ti, 2 runs | 6.86, 6.86 |

`DynBottomCube9` is static and identical everywhere. The two Linux runs agree, but the four Windows runs on one machine
do not, so this is not an x86 / arm64 floating-point divergence. Something in the simulation changes from one run to
the next. A tipping cube is chaotic, so any tiny difference grows. The owner wants the physics reviewed as a whole
("il faudra revoir la physique").

**2026-10-01, the P0 bench** (`docs/physics-overhaul.md` § 6, Linux, 5 launches): `DynTopCube` rests at x 6.855 on all
5, but the 5-box stack (`BenchStack*`, movable ↔ movable contacts through the solver) ends at positions up to 8 cm apart
and with different velocities from one launch to the next. The divergence exists on Linux too; it is not only Windows.

## What remains

> Expected to close with `physics-unified-contact-pipeline` (deterministic contact order, phase P2 of
> `docs/physics-overhaul.md`).


- [ ] Find the source of the run-to-run difference. The candidates, none measured yet:
  - the order of the contact pairs, which comes from the octree traversal and may follow addresses or an unordered
    container; a Gauss-Seidel solver gives a different result for a different order;
  - a step tied to wall-clock time (a variable dt or a variable number of substeps);
  - an unseeded random number.
- Target DECIDED by the owner on 2026-10-01 (`docs/physics-overhaul.md` § 1.3): the same
  machine, the same binary, the same inputs. No `-ffast-math` for now, but the owner may enable it one day, so
  correctness must not depend on bit-exact floats. Cross-platform bit-exactness is not a goal.
- [ ] Re-measure: the same resting position on 5 launches.

## References

- `projet-alpha/src/Builtin/CollisionDebug.cpp` (the dynamic rotation test), `Core.SceneManagerService.getNodePhysics()`.
- Related physics items: `physics-solver-restitution-and-position-correction`, `physics-nan-linear-velocities`,
  `rotational-physics`.
- projet-alpha `docs/plans/triad-engine-pass-report.md` (per-section record) § 11 (the peer readings).
