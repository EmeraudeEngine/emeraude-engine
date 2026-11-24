---
id: unsignalled-emissive-step-fades-for-seconds-on-the-screen-space-lane
title: An emissive material switched off without a lighting mark — its own pixels fade for 2.3 s on macOS's screen-space lane, in one frame on Linux's ray-traced lane; the cause is not established
status: open
priority: unranked
scope: src/Graphics/Effects (TAA, the glare, the screen-space lane's indirect passes), the temporal-discontinuity contract
opened: 2026-11-23
tags: [temporal, emissive, taa, macos, screen-space]
---

# An emissive material switched off without a lighting mark: 2.3 s of fade on one lane, one frame on the other

## Why

Measured on 2026-11-22 while a vehicle's brake lamps (emissive materials, no light of the light set) were switched in
one logic cycle, BEFORE the code that owns them raised the lighting mark (graphics doc 13-12 § 43, § The public
raise):

| Machine | Lane | The lamps' OWN pixels after the release | The glow around them |
|---|---|---|---|
| Linux, RTX 3070 Ti, 9 ms a frame | ray-traced, TAA on | switched in ONE frame | built up and faded over more than half a second |
| macOS, Apple M6, 17 ms a frame | screen-space (its only lane) | median red 255 for 0.17 s, 127 at 0.88 s, 99 at 1 s, 10 at 2.26 s, the idle 4-5 at 3 s | gone on the FIRST frame (mean 107 → 10 / 255) |

The two machines do the reverse of each other, and nothing explains it. With the raise the lamps read 7 / 255 on the
first frame on macOS: the vehicle is cured. What is not: an emissive surface that changes CONTINUOUSLY (a blinking
turn signal, a pulsing sign, a picture playing on a screen) cannot raise at every step — a raise marks 25 frames and
puts TAA's large-change test and the denoiser's reset on each of them — and without it such a surface would drag its
own picture for seconds on the lane every modest GPU runs.

⚠️ The owner accepted on 2026-11-03 that an unsignalled emissive change "converges at the pre-2026-11-01 rate — TAA
~20 frames per e-fold, the GI's 6-frame cap". Whether 2.3 s on the surface's own pixels is that trade-off (a blend of
~20 frames per e-fold on a value far above the display white does last seconds) or another stage is exactly what is
not known: this is an investigation, ranked by the owner.

## What remains

- Separate the LANE from the MACHINE: record the same release on Linux in the screen-space lane
  (`PostProcess.setLightingMode("ScreenSpace")`), without the raise (a test build that skips the call),
  `temporalCapture` across the step.
- Then find the stage, one switch at a time (`PostProcess.disable(slot)`): TAA, the glare, the screen-space indirect
  passes and their denoiser, the auto-exposure (pin it first).
- Say why the ray-traced lane's own pixels switch in one frame with the same TAA.
- Decide, with the owner, what a continuously animated emissive surface gets: nothing (documented), a per-pixel
  rejection on unmarked frames, or a cheaper kind of mark.

## ⚠️ Traps

- Before 2026-11-22 a window-less macOS process ran at a fifth of its rate past 30 s (`docs/caution-points.md`
  § *macOS: App Nap demoted a window-less process after 30 s*): the first macOS reading of this fade was taken that
  way, at about 16 frames a second, and read longer. The numbers above are from an instance at 58 frames a second.
- A red traffic signal lights red paint: count the lamps' own pixels, not the red around the vehicle.

## References

- `docs/subsystems/graphics/13-12-post-processing-effects/43-the-temporal-discontinuity-contract.md` (§ The public
  raise; the NOTE on a change nobody signals).
- § 06 (the TAA resolve) and § 16 (the anti-lag) of the same directory.
