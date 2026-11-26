---
id: sea-sound-crossing-values-to-hear
title: The sea's crossing gains were never heard — two provisional defaults
status: open
priority: unranked
scope: Scenes/SeaLevelInterface (SeaSoundSettings::crossingGain, crossingGainHF)
opened: 2026-11-26
tags: [audio, sea, tuning, owner-ear]
---

# The sea's crossing gains were never heard — two provisional defaults

## Why

Since 2026-11-26 a sound whose source and listener are on opposite sides of a sea's surface loses the sea's crossing
gains on top of the listener's medium (`docs/subsystems/audio/15-the-listeners-medium.md` § *The surface between a
source and the listener*). Their defaults — `crossingGain` 0.3, `crossingGainHF` 0.05 — were written without the
owner at his desk: the mechanism is measured, the values are not a decision.

## What remains

- The owner listens on a running scene, with a source on the other side of the surface, both ways (the listener in
  the air, then under the level): `setSeaSoundCrossing(gain, gainHF)`, `getSea()`. A world sound placed by the
  console would do it (engine item `console-commands-for-world-sounds-and-ambience`); a sound emitter on an entity
  under the sea does it today.
- ⚠️ To weigh while listening: under the level the two high-frequency gains MULTIPLY (the medium's 0.02 × the
  crossing's 0.05) and the shelf then takes 22 dB off the 125-500 Hz band, not 10.5 (measured, doc 15). Whether a
  source in the air heard from under water should take both, or the crossing alone, is the owner's call.
- The values kept become the defaults in `SeaSoundSettings`, the verdict goes to audio doc 15, this item is deleted.

## References

- `docs/subsystems/audio/15-the-listeners-medium.md`.
- `src/Scenes/SeaLevelInterface.hpp` (`SeaSoundSettings`), `src/Scenes/Scene.cpp` (`Scene::soundPathAt()`).
