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

## ⚠️ Traps

- A finite position with a NaN velocity means the NaN has **not propagated yet**: something either
  skips integration for those bodies or resets the position. Chasing the position will find
  nothing.
