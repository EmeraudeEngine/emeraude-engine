---
id: physics-solver-restitution-and-position-correction
title: The constraint solver loses the bounce and over-corrects penetration
status: blocked
priority: unranked
scope: Physics/ConstraintSolver, Physics/ContactPoint
opened: 2026-10-01
blocked-by: [physics-collision-debug-bench]
tags: [physics, solver, restitution, stability]
---

# The constraint solver loses the bounce and over-corrects penetration

## Why

Triad 11 found these (2026-10-01, a review of `ConstraintSolver.cpp`, re-checked against the code). The owner chose a
dedicated solver pass, validated by runtime measurements, over a fix inside the triad.

- **Restitution, re-applied at every velocity iteration.**
  - `solveVelocityConstraints()` aims at `-(1 + e) · vn`, where `vn` is the CURRENT relative normal velocity. It does
    this on each of the 8 iterations.
  - For one movable body against a static one, the approach speed after k iterations is `(-e)^k · v`, so after 8
    iterations the bounce is mostly gone.
  - With `e = 1` (both bounciness values 1.0, which `setBounciness()` allows), the accumulated impulse alternates
    2mv, 0, 2mv, 0… and ends at 0. The velocity phase then does nothing, and only the position phase keeps the bodies
    apart.
  - Side effect: `onCollision` fires on every iteration, so up to 8 times per contact per tick, with alternating
    magnitudes.
- **Position correction, applied three times with a stale depth.**
  - The 3 position iterations reuse `penetrationDepth()`, which none of them updates. Each iteration moves the bodies by
    `0.8 · (depth − slop)` again.
  - That is about 2.4× the penetration in total for one movable body: it is pushed out past contact, falls back, and
    jitters.
  - On top of that, the Baumgarte velocity bias (`prepareContacts()`) corrects the same penetration a second time.

**2026-10-01, the P0 bench** (`docs/physics-overhaul.md` § 6): the restitution against the GROUND is right (e_eff 0.494
for 0.5, 0.998 for 1.0 — that path does not go through this solver). The 5-box stack, which does, never settles: boxes
B…E keep a downward velocity of 0.2-1.6 m/s while resting and move 0.16-0.33 m over the last 5 s of a 30 s run.

## What remains

> Since 2026-10-01 this is done INSIDE `physics-unified-contact-pipeline` (phase P2 of the physics overhaul,
> `docs/physics-overhaul.md`), measured on the `collision-debug` stations of projet-alpha's
> `physics-collision-debug-bench`.


- [ ] Measure first: the bounce height of a ball dropped with `e` = 0.5 and 1.0, and the resting jitter of a stack of
  boxes (position amplitude over 5 s). Use `collision-debug`, `physics-debug`, `lighten-marbles`.
- [ ] Restitution: compute the target once in `prepareContacts()`, from the PRE-solve normal velocity
  (`-e · vn0`), with a speed threshold below which `e = 0` (Box2D uses ~1 m/s). Each iteration then solves
  `lambda = (-vn + target + bias) · effectiveMass`.
- [ ] Fire `onCollision` once per contact per tick (the accumulated impulse), not once per iteration.
- [ ] Position: track the accumulated correction per contact (or recompute the separation from the current positions)
  so that the iterations converge on the remaining penetration. Decide between the position pass and the Baumgarte
  bias, so the same penetration is not corrected twice.
- [ ] Re-measure; record the before / after numbers in `docs/` (physics topic) and in `docs/caution-points.md`.

## ⚠️ Traps

- The demos tune actors (Paladin, Drone, Fox, Player) with forces against this solver: re-check them after the change.
- Since triad 11, `Vector / s` is avoided on contact normals: it returns NaN for `|s| <= epsilon`. Keep it out of
  any new code here as well.

## References

- `src/Physics/ConstraintSolver.cpp`: `prepareContacts()`, `solveVelocityConstraints()`,
  `solvePositionConstraints()`.
- E. Catto, "Iterative Dynamics with Temporal Coherence" (GDC 2005) and Box2D `b2ContactSolver` (restitution from the
  pre-solve velocity, restitution threshold, accumulated impulses).
- projet-alpha `docs/plans/triad-engine-pass-report.md` (per-section record) § 11.
