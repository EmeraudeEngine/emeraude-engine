---
id: time-jump-renders-black-frames
title: A time-of-day jump renders three near-black frames (indirect diffuse off)
status: open
priority: high
scope: Scenes/CelestialClock, the sky's IBL publication (Scene::updateDynamicSkyIBL, the IBL ring), the frame's ambient
opened: 2026-11-01
tags: [defect, sky, ibl, lighting]
---

# A time-of-day jump renders three near-black frames (indirect diffuse off)

## Why

Right after `CelestialClock.setTimeOfDay(CitySun0, CitySunClock, 0.95)` from 0.55 on `city`, with `IndirectDiffuse`
switched off, the scene renders about three near-black frames before the night view appears:

- Linux, RTX 3070 Ti, `FrameRateLimit` 40, TAA on with its large-change test, engine dd77f62b plus the working tree of
  2026-11-01. In the `temporalCapture(60)` after the jump, frames 8-10 (+182 to +231 ms) have a whole-image mean of
  0.12-0.13 and only 4.6 % of the pixels non-zero, where the frames before read 157 and the frames after read 7.9.
  It happened in one jump out of two (the second run's frame at +200 ms read 15.95, a normal night).
- macOS-PA, M6, the round-3 V2 run (lighting family off, TAA off): one capture 0.5 s after the jump had a mean of 0.073
  and 3.5 % of the pixels non-zero, with one lamp and none of the emissive windows. The next second was normal.
- With RTGI on, no such frame was seen (+200 ms read 226-230).

TAA without the large-change test averaged these frames away: in the same configuration with the test off, the land
dips from 249 to 244 at frame 8. The test now shows them as they are. That is a defect upstream of TAA, and it is
visible since the TAA change.

## What remains

- Attribute it. Light probe: the sun is below the horizon at 0.95, so the land is lit by the ambient (the sky's IBL
  irradiance under `applyAmbient`), the lamps, and the emissive windows (which switch on at a later logic tick). A
  black ambient for three frames points at the IBL publication after a jump (graphics doc 42 § "The environment":
  one capture cycle is 8 frames plus one logic tick; `Scene::IBLRingSize`): a slot published before its irradiance is
  recorded, or a re-bake that starts from a cleared image. Check with `temporalCapture` and the IBL slots' generation
  counters, one jump at a time.
- Why only with `IndirectDiffuse` off: does the GI's own history (or its sky-miss term) hide the same three frames?
- Intermittent (one jump out of two): count over ten jumps before and after the fix.

## ⚠️ Traps

- Compare at a PINNED exposure and read the frames from a `temporalCapture`: a once-a-second screenshot catches
  75 ms only by chance.
- TAA's large-change test is not the cause: it shows the input as it is (TAA off on macOS showed it too).

## References

- Graphics doc 42 § "The environment: the sky's own IBL, amortised"; graphics doc 13-12 § "The TAA resolve" § The
  LARGE-CHANGE test.
- Engine item `lighting-change-converges-over-frames` (the round-3 measurements).
