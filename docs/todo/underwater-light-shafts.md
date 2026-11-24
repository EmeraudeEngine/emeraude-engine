---
id: underwater-light-shafts
title: Light shafts under the sea — the sun's beams coming down from the surface
status: open
priority: unranked
scope: Graphics/Effects/Atmosphere
opened: 2026-11-23
blocked-by: [underwater-view]
tags: [water, ocean, participating-medium, volumetric]
---

# Light shafts under the sea — the sun's beams coming down from the surface

## Why

Owner (2026-11-23, with `underwater-view`): the beams of light under the water are wanted, after the base.

`Effects::Atmosphere::VolumetricLight` cannot draw them: it is a radial blur in screen space around the sun's
position, occluded by the depth buffer. Under water the sun is seen through Snell's window, its light is refracted and
broken by the waves, and the beams come DOWN from the surface whatever the view direction.

## What remains

Everything. To decide with the owner before any code:
- a march through the sea's `WaterMedium` (its scattering and phase anisotropy) from the eye to the depth, lit by the
  refracted sun — the world-space single-scattering march of `volumetric-light-single-scattering` is the same family;
- what breaks the beams: the sea's own waves (the FFT slopes at the point where the refracted ray leaves the surface);
- what `VolumetricLight` does under water meanwhile. Since 2026-11-23 (owner decision, after the shafts had
  vanished under the sea's underside): the air's shafts seen THROUGH the surface — its occlusion mask counts the sky
  behind the sea for what the surface transmits from below, the water volume attenuates them (graphics doc 13-12
  § 44, part 5). A glow around the sun's place in Snell's window, not beams.
  ⚠️ Its FORM is a debt to settle here: a shader switch driven by a push constant (`seaWindowEnabled`, with a
  stand-in descriptor — the file's own precedent, `cloudTransmittanceEnabled`) where the owner's decisions of
  2026-11-02 and 2026-11-03 build two pipeline variants (the marching loops, the temporal discontinuity — whether
  that rule extends to this case is his call), and an effect of the air that now knows the sea
  (`FrameContext::seaImmersion`, the index of `Scenes::WaterMedium`).
  ⚠️ Since 2026-11-24 the term is on for every frame that is not `Clear` (the frame's one decision, no longer the
  eye's height against the level at rest): on an `Astride` frame seen from the AIR it wrongly lights a translucent
  surface other than the sea, seen more than 41° above the horizon in front of the sky — taken for the sea's
  underside. No demo shows it (`water-world` has no such surface); a per-pixel answer would end it.

## References

- Engine item `volumetric-light-single-scattering` (the owner's target for the air's shafts).
