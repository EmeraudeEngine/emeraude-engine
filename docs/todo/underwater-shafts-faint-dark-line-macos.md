---
id: underwater-shafts-faint-dark-line-macos
title: A faint dark line across the shafts' glow toward the sun, on macOS
status: open
priority: high
scope: Graphics/Effects/Atmosphere (WaterVolume, the marched variants)
opened: 2026-11-25
tags: [water, ocean, defect, macos, moltenvk]
---

# A faint dark line across the shafts' glow toward the sun, on macOS

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

## What remains

- What the anti-aliasing does on that row on the Mac and not on Linux (both lanes, the anti-aliasing on: no row more
  than 0.35 under its neighbours): its debug views (`TemporalAA/DebugView` 1, 2, 3), the nearly raw frame
  (`TemporalAA/Alpha` 0.99), the inverse-luminance weights off (`TemporalAA/LumaWeighting`), through settings copies.
  Asked of the Mac peer on 2026-11-25; not answered yet.
- Whether an anisotropy is needed or only makes it visible: not measured with the round's metric at 0 nor with the
  shafts off.

## References

- `docs/subsystems/graphics/45-the-sun-under-a-sea.md` § 5.
