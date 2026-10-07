---
id: flying-character-oscillates-against-a-mesh
title: A flying character pushed into a model swings back and forth before it settles
status: open
priority: high
scope: Physics/CharacterController (flying mode)
opened: 2026-10-05
tags: [physics, character, measured]
---

# A flying character pushed into a model swings back and forth before it settles

## Why

Measured by Windows-PA (2026-10-05, NVIDIA, alpha `446a16f9`: the ethereal player is now the flying character):
on `geometry-loader`, the player spawned at (0, -0.895, 1.5) and flew forward (W held 1 s) into the model at the
origin. z went 1.50 → -0.40 (inside the model), was thrown back to +0.33, swung -0.18 → +0.10, then settled at 0.17
about 0.4 s after the release — identical on 2 runs (final z 0.166782). Against a wall (simple-room) the same flight
stops cleanly at the wall. So the push-out against this model is an oscillation, not a stop.

## What remains

- Find what the character collides with there (the model's collision shape: a box, a triangle mesh?) and why the
  sweep lets it in 1.9 m before pushing out (a missed contact at 10 m/s, then a depenetration that overshoots).
- Compare with the walking mode against the same model.

## References

- `src/Physics/CharacterController.cpp` (`fly()`, the depenetration), projet-alpha `docs/subsystems/actor/06-6-walking-actors.md`.
