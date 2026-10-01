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

**2026-10-01, the P0 bench** (`docs/physics-overhaul.md` § 6): compared at the same physics cycles
(`tools/physics-bench.py --compare`), the 5 Linux runs are identical on every station. Bodies that never rest end at
different final states only because their last sample falls on a different cycle — do not compare final states.

**2026-10-01, compared by cycle on the three OS** (`docs/physics-overhaul.md` § 6, "The peers compared by cycle"): the
stack diverges on Windows (both GPUs) and macOS, `DynTopCube` on the RTX 3060 and the M2, the single-pair twin nowhere.
Working hypothesis: the completion order of the asynchronous geometry loads, which drives both the re-derived
properties and the physics octree insertion order (hence the solver's pair order).

**2026-10-01, the peers' bench (final states)** (`docs/physics-overhaul.md` § 6): `DynTopCube` repeats on Linux and on the Windows AMD
iGPU (X 6.855, 5/5 each), not on the RTX 3060 (a few discrete outcomes, two runs bit-identical) nor the M2. Hypothesis:
its entity-level properties and non-overridden shape are re-derived when the geometry load completes, at a cycle that
depends on the machine (`docs/caution-points.md` § the body properties set on an entity are overwritten). Test: the
twin station `BenchTipCube`.

**2026-10-01, every cycle recorded on the three OS** (`docs/physics-overhaul.md` § 6, "The per-cycle recorder"): the
single-pair twin is bit-identical 5/5 on Linux, the M2, the RTX 3060 and the AMD iGPU; the stack and `DynTopCube` fork
in DISCRETE states (Windows: before cycle 60; the M2: once at cycle 72) — an order effect. Acceptance after P2: 0
differing samples on every station, 5 runs, three OS.

**2026-10-01, after P2** (engine `dd823ab5`, `docs/physics-overhaul.md` § 1b): Linux 5/5 bit-identical on all 17
stations; the M2 0 differing samples on 16 of 17, the stack included (the persistent order fixed the order effect).
The 17th, `DynTopCube`, was a TIME SHIFT, not a solver difference: its shape was not overridden, so it joined the
simulation only when its geometry finished loading (cycle 40 / 50 / 52). Fixed in the demo (decision (h): component
properties, overridden shapes), Linux 3/3 identical; waiting for the peers' 5 runs and the Windows P2 report.

## What remains

> Expected to close with `physics-unified-contact-pipeline` (deterministic contact order, phase P2 of
> `docs/physics-overhaul.md`).


- Source FOUND and fixed by P2: the order of the contact pairs (octree traversal) fed a Gauss-Seidel solver; P2 sorts
  bodies by creation number and pairs / manifolds by key. A body whose shape waits for an async load starts late: a
  scene that needs reproducibility overrides its shapes.
- Target DECIDED by the owner on 2026-10-01 (`docs/physics-overhaul.md` § 1.3): the same
  machine, the same binary, the same inputs. No `-ffast-math` for now, but the owner may enable it one day, so
  correctness must not depend on bit-exact floats. Cross-platform bit-exactness is not a goal.
- [ ] Re-measure on the peers: 0 differing samples on every station, 5 recorded runs, macOS + Windows (NVIDIA, AMD).

## References

- `projet-alpha/src/Builtin/CollisionDebug.cpp` (the dynamic rotation test), `Core.SceneManagerService.getNodePhysics()`.
- Related physics items (the solver one closed by P2, 2026-10-01): `physics-unified-contact-pipeline`, `physics-nan-linear-velocities`,
  `rotational-physics`.
- projet-alpha `docs/plans/triad-engine-pass-report.md` (per-section record) § 11 (the peer readings).
