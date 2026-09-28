---
id: physics-step-up-pass
title: Physics — a pass on the step-up (stairs climb, with problems)
status: open
priority: unranked
scope: Physics
opened: 2026-09-28
tags: [physics, player]
---

# Physics — a pass on the step-up (stairs climb, with problems)

## Why

The step-up (`MovableTrait::setStepHeight()`, `Scene::accumulateStaticEntityCorrections()`,
`docs/physics-system.md` § Phase 1) was added on 2026-09-28 so the player can climb `citadel`'s
stairs to the wall walk (34 solid steps of 0.29 m, player step height 0.35 m). The owner walked
them: **"ça monte mais avec des soucis"** — the player climbs, with problems not yet characterized.
The owner scheduled a dedicated physics pass in another session.

## What is known

- The lift is a teleport of the whole rise in one tick (no smoothing), decided per static and SUMMED
  with the other statics' corrections of the same tick.
- It needs the body GROUNDED (grace period 15 ticks), and a correction whose vertical part is under
  30 % of the penetration depth.
- There is no headroom test (a step under a low ceiling would still lift).
- The player is an axis-aligned box; steps and walls are AABBs.

## What remains

- [ ] Characterize the problems on `citadel`'s two flights (keyboard, window launch).
- [ ] Decide the fixes with the owner (smoothing the lift, headroom, the summing, …).
