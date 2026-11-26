---
id: underwater-sound
title: The muffled sound under the sea — a low-pass and a reverberation when the listener is submerged
status: in-progress
priority: unranked
scope: Audio, Scenes (sea level)
opened: 2026-11-23
tags: [water, ocean, audio]
---

# The muffled sound under the sea — a low-pass and a reverberation when the listener is submerged

## Why

Owner (2026-11-23, with `underwater-view`): the muffled sound under the water is wanted, after the base.

## Done (2026-11-26, Linux — audio doc 15)

The listener's medium in the audio manager, stated by the scene each logic cycle from the listener's depth under
its sea; every pooled source muffled and sent to the water's reverberation, the music left alone; `setSeaSound` and
`SoundEmitter.play` in the console. Measured on the mix written to a file. The owner's four decisions of 2026-11-26
and everything that must outlive this item are in `docs/subsystems/audio/15-the-listeners-medium.md`. The owner
heard it on a running scene the same day and kept the first values.

## What remains

- macOS and Windows (the peers); AMD.
- The waves: the sound follows the sea's level at rest until the sea answers its waves on the CPU
  (`ocean-fft-surface`, step 6) — nothing to change here then.
- `water-world` plays no sound of its own: nothing is heard there without the console. A sea ambience is a content
  decision of the owner's (projet-alpha).
- A medium per source (a source under water heard from the air, and the reverse): decided later by the owner, the
  listener's state first.

## ⚠️ Traps

- A sound emitter follows its entity through `move()` only: after a displacement that notifies nobody
  (`node-setlocalcoordinates-notifies-nobody`) its source stays behind and the distance attenuation looks like the
  filter (−25 dB at 18 m in the first measurement). Turn the head (`Act.lookAt`) after an `Act.setPosition` before
  measuring.
- The test sounds of the store hold almost nothing above 3.5 kHz: read the bands that carry signal only.

## References

- `docs/subsystems/audio/15-the-listeners-medium.md`.
