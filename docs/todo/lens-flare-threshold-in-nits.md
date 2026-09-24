---
id: lens-flare-threshold-in-nits
title: The lens flare thresholds the chain colour in nits with a [0,1] constant, so outdoors the whole sky becomes ghosts
status: open
priority: unranked
scope: Graphics/Effects/Camera
opened: 2026-09-24
tags: [photometry, post-process, lens-flare]
---

# The lens flare thresholds the chain colour in nits with a [0,1] constant, so outdoors the whole sky becomes ghosts

## Why

On `forest` looking toward the sun (2026-09-24), the flare draws saturated rainbow streaks along the
sun's radial direction, over the canopy and the ground. Isolated at runtime: with
`PostProcess.disable("LensFlare")` they vanish and the god rays alone are clean; with
`disable("VolumetricLight")` they remain.

**Cause (read in `LensFlare.cpp`, not yet measured pixel by pixel):** the bright pass keeps
`max(r, g, b) > threshold` with `Parameters::threshold = 0.8`, and `LensFlare` sits in the camera phase
where the chain colour is an ABSOLUTE luminance in nits. The `AutumnFieldPureSky` sky is 15 380 nits:
every sky pixel and most sunlit surfaces pass, and each ghost is a copy of that whole bright image
with `chromaticDistortion` separating R, G and B along the ghost axis — through the leaves, streaks.
It is the engine rule "a [0,1] constant that reaches the scene colour buffer is ALWAYS a bug"
(the `AtmosphericFog` 0.6-nit fog was the same class).

## What remains

1. Make the threshold exposure-relative: compare `brightness × displayExposure` (the
   `FrameContext::displayExposure` the tone mapper publishes, nit → display value) with a threshold in
   DISPLAY units, so only what the sensor sees far above white (the sun disc, speculars) feeds the
   ghosts. Owner decision on the default value (the demo rules cite a bloom threshold ≥ 1.2).
2. Measure before/after on `forest` at the sun pose (`setPosition(30, 0, -40)` +
   `lookAt(100.5, 48.6, 11.7)`), and on an indoor scene with one bright source where the flare looked
   right, so the fix does not lose it there.

## ⚠️ Traps

- The camera phase runs BEFORE the tone mapping: the chain colour there is nits, not display values.
- The god rays are not involved: they use their own depth-derived mask (and, since 2026-09-24, the
  clouds' transmittance).

## References

- `src/Graphics/Effects/Camera/LensFlare.{hpp,cpp}` — the threshold pass, `Parameters::threshold`.
- `src/Graphics/AGENTS.md` § *The light shafts and the lens flare see the clouds*.
