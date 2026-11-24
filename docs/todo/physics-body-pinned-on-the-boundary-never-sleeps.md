---
id: physics-body-pinned-on-the-boundary-never-sleeps
title: A body pinned on the scene's boundary keeps a velocity and never goes to sleep
status: open
priority: high
scope: Scenes/Scene.physics.cpp
opened: 2026-11-24
tags: [physics, defect, measured]
---

# A body pinned on the scene's boundary keeps a velocity and never goes to sleep

## Why

Found on 2026-11-24 while re-measuring "bodies never come to rest on generated terrain" (closed that day: they do,
`docs/physics-overhaul.md` § E). `balls-of-steel --demo-options 5,0`, engine `develop` 94632582, each ball read every
second for 120 s with `SceneManagerService.getNodePhysics()`: two of the five balls rolled to the scene's edge and
stayed there, at Z = 127.5 — the 128 m boundary less their 0.5 m radius.

- Their position is constant to the fourth decimal for the last 40 s (X, Y and Z).
- They report a constant speed all that time: 0.664 and 0.407 m/s, mostly downward (−0.64 and −0.39 on Y), with an
  angular velocity of about 0.8 rad/s.
- `grounded: false`, `groundedSource: None`, `simulationPaused: false`: they are never put to sleep, so the physics
  steps them on every cycle for as long as the scene lives. The three balls that stopped elsewhere are paused.

`Scene::clipInsideBoundaries()` (`Scene.physics.cpp`) puts the position back inside the boundary, axis by axis
(`setXPosition()` / `setYPosition()` / `setZPosition()`), and does not touch the velocity. A defect, hence `high` by
default; never ranked by the owner.

## What remains

- Read why the body is not grounded there: it rests against the boundary and, by its constant height, on something
  — the ground's edge, or the clip alone holding it against gravity.
- Decide with the owner what the boundary IS for a dynamic body: a wall (the velocity into it is removed, a contact
  the sleep logic sees), or only a clip (then what puts a clipped body to sleep).
- Verify on `balls-of-steel`: every ball paused once it has stopped, the boundary's included; the count of bodies
  stepped falls to zero.

## References

- `docs/physics-overhaul.md` § E (the measurement of 2026-11-24).
- Engine item `physics-octree-drops-an-entity-outside-the-boundary` (another defect of the same boundary).
