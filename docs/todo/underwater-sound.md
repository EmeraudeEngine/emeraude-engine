---
id: underwater-sound
title: The muffled sound under the sea — a low-pass and a reverberation when the listener is submerged
status: open
priority: unranked
scope: Audio, Scenes (sea level)
opened: 2026-11-23
blocked-by: [underwater-view]
tags: [water, ocean, audio]
---

# The muffled sound under the sea — a low-pass and a reverberation when the listener is submerged

## Why

Owner (2026-11-23, with `underwater-view`): the muffled sound under the water is wanted, after the base.

The audio already has what it takes: `Audio::Filters::LowPass`, `Audio::Effects::Reverb` / `EAXReverb`.

## What remains

Everything. To decide with the owner before any code:
- who decides "the listener is submerged": the CPU does not know the waves (`OceanResource::getLevelAt()` answers the
  flat level — `ocean-fft-surface`, step 6: a CPU copy of the displacement, one frame late); with the flat level alone
  the sound switches up to a wave's height away from what the image shows;
- the switch itself: a cross-fade, never a step (a listener bobbing at the surface must not chatter);
- what is filtered: every source heard from under the water, and the sources that are themselves under it.
