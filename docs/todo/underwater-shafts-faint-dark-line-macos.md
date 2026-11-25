---
id: underwater-shafts-faint-dark-line-macos
title: A faint dark line across the shafts' glow toward the sun, on macOS
status: parked
priority: high
scope: Graphics/Effects/Atmosphere (WaterVolume, the marched variants)
opened: 2026-11-25
tags: [water, ocean, defect, macos, moltenvk]
---

# A faint dark line across the shafts' glow toward the sun, on macOS

## Parked (owner, 2026-11-26)

The defect is faint (5 levels of 255 at most, two rows, intermittent) and shows on macOS only, with a forward
anisotropy. It is made inside the temporal anti-aliasing's history, and finding its cause needs a probe built on the
Mac, several rounds of it. The owner's decision: not now; the temporal anti-aliasing will be reviewed as a whole
another time, and this item is taken up again with that review (the motion marker flooding a grazing surface, under
"What remains", belongs to it).

## Why

Found by the macOS peer on 2026-11-25 while verifying the sun's shafts (engine bb6f9d02, graphics doc 45 § 5), on an
Apple M6 (MoltenVK 1.4.2, screen-space lane, temporal anti-aliasing on):
- projet-alpha `water-world`, window-less 1280 × 720, exposure pinned f/11 – 1/250 s – ISO 100, free flight, the eye
  at (900, −6, 900) looking at (900, −2.85, 909), `setSeaShafts(1.0, 0.6)`: a nearly horizontal dark line about two
  rows thick at rows 421 to 425, 2 to 2.7 levels darker than the rows around it on the bright left side, fading out
  by x ≈ 900.
- Present in three of four identical captures: intermittent.
- Absent with an anisotropy of 0, with the shafts off, and toward the three other targets of the verification.
- It follows the view DIRECTION, not the eye's height: the eye and the target one metre higher leave it at row 423;
  the target one metre lower moves it up to rows 378 to 382 (about 7.5 rows a degree); the view turned 10° to the
  right moves its start about 160 pixels right. From the row and the pitch: about 11° above the horizontal, in the
  sun's azimuth.

NOT reproduced on Linux (RTX 3070 Ti, both lanes, the anti-aliasing off and on, three captures each): no row of the
left half is more than 0.35 level under its neighbours. Windows (RTX 3500 Ada) saw no hard edge at 1:1.

## Established since (2026-11-25, console-only rounds on the Mac, then Linux)

- **The line sits on the sea's HORIZON seen from below.** With the water volume switched off
  (`PostProcess.disable("WaterVolume")`) the row holds the hard edge between the sea's underside and the sky under
  the horizon — correct geometry, which the water's veil hides. ⚠️ The eye stands 1.645 m above the position given
  to `setPosition`: the verification's view was pitched 9.5° up, and "11° above the horizontal" above is the horizon.
- **Only the temporal anti-aliasing removes it**: 0 capture of 4 with `disable("TemporalAA")`, 3 or 4 of 4 in every
  other state. Still there with almost no shaft deviation (`setSeaShafts(0.001, 0.6)`), without the caustics' gain,
  and with each of VolumetricLight, Reflections, IndirectDiffuse, AmbientOcclusion, ContactShadows, LensFlare,
  DepthOfField, MotionBlur and Glare switched off in turn. With the round's metric (columns 0-639, a row against
  the mean of the rows four above and four below) it reads −5 at row 422; −4.3 at an anisotropy of 0.3, −1.9 at 0.9.
- It keeps its row when the eye moves 400 m, and moves 3 to 4 rows for 3 m of depth.
- **Refuted**: a ray taken from the depth's reconstruction at the far end of the buffer (a patch taking the ray from
  the near-plane point: the line in 8 captures of 8, as deep). **A wrong lead, not to retry**: "the sea's underside
  stops short of the horizon" — a test camera pitched 10° down without knowing it; the ocean's nodes are drawn to
  kilometres (read in a RenderDoc capture's command stream).

## The anti-aliasing's own views (2026-11-25, the Mac through settings copies, then Linux)

- **No raw frame carries the line**: with `TemporalAA/Alpha` 0.99 (the history weighs half on a still pixel), 0
  capture of 16. The line is made in the HISTORY.
- **An anisotropy is needed**: `setSeaShafts(1.0, 0.0)` and `(0.0, 0.0)`, eight captures each, no row more than 0.5
  under its neighbours.
- `TemporalAA/LumaWeighting` false: still there (5 captures of 8, as deep): not the inverse-luminance weights.
- `DebugView` 2: the MOTION MARKER covers every column of the seven rows just above the horizon (419 to 425 of 720)
  and stops dead on the first sky row; nothing there is rejected (view 1: no red; view 2: no yellow, no history depth
  outside the 3 × 3 range). `DebugView` 3: a luminance gap between the kept history and the frame on rows 420 to 424,
  in the capture where the line shows, none in the other.
- **Linux shows the same marker band** (rows 420 to 424 at 1.00, 426 at 0) and NO gap in view 3: the structure is
  common, its consequence is the Mac's.
- The velocity buffer on Linux (a RenderDoc capture, 1920 × 1080, the horizon at row 639): exactly 0 on the eight
  rows above the horizon (the far, flat sea: its waves are faded out there), and up to ±1 pixel a frame, changing
  sign from a row to the next, on the wavy underside above them. The reactive mask is 0 everywhere there.
- A reading, NOT proven: the marker spreads by itself over the rows where the linear depth changes by more than 5 %
  a pixel (it is kept while the 3 × 3 depth range is an "edge", and a gathered history texel that carries it marks
  its neighbours), so the far sea's last rows are permanently "moving" and never rejected. What makes their history
  darker than every frame it accumulated is not found.

## What remains

- On the Mac, with a probe in the resolve (the Mac is the only place it shows): what the history holds on those rows
  against the reconstruction of the frame, and the velocity the dilation gives them — frame after frame.
- Whether the velocities of the sea's underside (±1 pixel a frame at about 200 m, on a 250 frames-a-second run) are
  right: not checked against the waves' real speed.
- Whether the motion marker should flood a grazing surface at all (the far sea, any ground toward its horizon).

## References

- `docs/subsystems/graphics/45-the-sun-under-a-sea.md` § 5.
