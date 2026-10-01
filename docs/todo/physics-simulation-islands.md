---
id: physics-simulation-islands
title: Simulation islands — bodies in contact sleep and wake together
status: open
priority: unranked
scope: Physics, Scenes/Scene.physics.cpp
opened: 2026-10-01
tags: [physics, performance, physics-overhaul]
---

# Simulation islands — bodies in contact sleep and wake together

## Why

Phase P5 of the physics overhaul (`docs/physics-overhaul.md`). Each body sleeps on its own today
(`MovableTrait::checkSimulationInertia()`: 30 stable ticks on terrain or a boundary only). A body resting on another
body never sleeps, and waking one body of a stack does not wake the others.

## What remains

- [ ] Build the contact graph's connected components each tick; an island sleeps when all of its bodies have been
  slow for long enough, and wakes as a whole.
- [ ] Measure on `balls-of-steel` and `lighten-marbles` (the logic-thread time per tick) before and after.

## References

- E. Catto, Box2D v3 `island.c` (MIT).
