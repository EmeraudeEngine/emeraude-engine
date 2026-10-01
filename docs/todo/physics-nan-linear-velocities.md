---
id: physics-nan-linear-velocities
title: Physics — NaN linear velocities on 3 of 41 sampled bodies
status: open
priority: high
scope: Physics
opened: 2026-08-25
tags: [physics, measured]
---

# Physics — NaN linear velocities on 3 of 41 sampled bodies

> [!IMPORTANT]
> **Owner decision: this is a SEPARATE subject from the Y-up migration.** It does not depend on an
> axis sign; it was merely surfaced while measuring the Y-up physics gate. Do not fold it back
> into that migration.

## Why

Measured on `balls-of-steel`: **3 of 41 sampled bodies** carry `linearVelocity = [-nan, -nan,
-nan]` while their position is still finite and they report `grounded: false`.

## What remains

- [ ] **Attribute it first.** It is NOT known whether this predates the grounded-decay fix of the
  same day (2026-08-25). Attribution is the first task, before any correction.
- [ ] Find the producer, not the symptom.

Read the state with `Core.SceneManagerService.getNodePhysics(<node>)`.

**2026-10-01, after triad 11** (engine `a777ddf7`): `balls-of-steel`, every root node read through `getNodePhysics()`
45 s after the load, 2 runs: **0 of 1001** nodes with a NaN or an infinity. Triad 11 removed two NaN producers that
fit the symptom: a contact normal `mtv / depth`, which is NaN for a depth at or below epsilon (a 1-ulp overlap), and the
explicit drag, which diverged to NaN for a light, fast body. It is NOT attributed: there is no pre-11 run on the same
machine. Close this item only after a pre-11 run reproduces the NaN and the post-11 build shows 0.

## ⚠️ Traps

- A finite position with a NaN velocity means the NaN has **not propagated yet**: something either
  skips integration for those bodies or resets the position. Chasing the position will find
  nothing.
