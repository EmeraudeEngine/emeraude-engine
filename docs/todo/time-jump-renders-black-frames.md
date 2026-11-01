---
id: time-jump-renders-black-frames
title: A time-of-day jump renders near-black frames (the sky, the land's light and the ambient change on different frames)
status: open
priority: high
scope: Scenes/CelestialClock, the sky's IBL publication (Scene::updateDynamicSkyIBL, the IBL ring), the frame's ambient
opened: 2026-11-01
tags: [defect, sky, ibl, lighting]
---

# A time-of-day jump renders near-black frames (the sky, the land's light and the ambient change on different frames)

The jump is not applied in one frame. Linux (B, GI off, 40 FPS, `temporalCapture` right after the command):
- frames #0-#7: the sky is already night (2.4), the land still day (240);
- #8-#10: near-black (the land 0.05);
- then the night.
macOS-PA (M6, ~46 FPS) shows the same steps: black frames (lamps only, ~3.7 % non-zero), then ~6.5 (8.5 % non-zero,
no windows), then ~12.6 (windows lit, ~20 %). Over 4 jumps it saw black frames 3 times, lasting 5, 14 and 6 frames
(110-300 ms). Before TAA's large-change test (engine 3c25c355), TAA averaged this order away.

## Why

Right after `CelestialClock.setTimeOfDay(CitySun0, CitySunClock, 0.95)` from 0.55 on `city`, with `IndirectDiffuse`
switched off, the scene renders about three near-black frames before the night view appears:

- Linux, RTX 3070 Ti, `FrameRateLimit` 40, TAA on with its large-change test, engine dd77f62b plus the working tree of
  2026-11-01. In the `temporalCapture(60)` after the jump, frames 8-10 (+182 to +231 ms) have a whole-image mean of
  0.12-0.13 and only 4.6 % of the pixels non-zero, where the frames before read 157 and the frames after read 7.9.
  It happened in one jump out of two (the second run's frame at +200 ms read 15.95, a normal night).
- macOS-PA, M6, the round-3 V2 run (lighting family off, TAA off): one capture 0.5 s after the jump had a mean of 0.073
  and 3.5 % of the pixels non-zero, with one lamp and none of the emissive windows. The next second was normal.
- Windows-PA, RTX 3500 Ada (RayTracing lane, 73-100 FPS), engine 3c25c355, TAA test on, GI off: the screenshot at
  +0.5 s was near-black (whole image 0.13, 2.3 % of the pixels non-zero), in one jump out of one. At ~100 FPS, 0.5 s
  is ~50 frames: the black is not a fixed three frames. It may be time-bound, a logic tick or an IBL cycle; the
  screenshot's own latency (0.2-0.35 s) is not ruled out. After it, the land undershoots (13.08 at +1 s), then climbs
  back to 15.0 by +2 s. TAA had clipped its history down to the black frames, and the climb back (under 25 %) is
  below the large-change threshold, so it runs at the stationary alpha. Linux's first test-on run shows the same
  (13.0 at +0.3 s, 14.65 at +1.4 s). The Intel iGPU (13-19 FPS, ScreenSpace lane) showed no black frame at +0.5 s.
- With RTGI on, no such frame was seen (+200 ms read 226-230).

TAA without the large-change test averaged these frames away: in the same configuration with the test off, the land
dips from 249 to 244 at frame 8. The test now shows them as they are. That is a defect upstream of TAA, and it is
visible since the TAA change.

## What remains

- Decide what "applied in one frame" means for a jump, and attribute each step:
  - the sky reads the clock state published for rendering at once (`CelestialClock::publishStateForRendering()`,
    graphics doc 42);
  - the sun light on the land follows ~90-180 ms later: a logic tick, or the light's own update;
  - the ambient (IBL) appears to come last, after the black frames;
  - the city's windows come last of all (the demo's logic).
  The fix may be a single publication point for a jump, owner's decision.
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
