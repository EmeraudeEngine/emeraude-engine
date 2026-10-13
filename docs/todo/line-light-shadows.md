---
id: line-light-shadows
title: Line lights cast no shadow (raster or traced)
status: open
priority: medium
scope: Scenes/Component/LineLight, Graphics (LineLightPass, shadow maps), RT effects (RTGI / RTR / probes)
opened: 2026-10-13
tags: [graphics, lighting, shadows, line-light, beams]
---

# Line lights cast no shadow (raster or traced)

## Why

`Scenes::Component::LineLight` (a Lambertian tube along a polyline, LTC diffuse + GGX specular; a beam drives one
through `Beam::setLight`) shipped on 2026-09-30 with **no shadow in the first pass** (owner decision of that evening):
`src/Scenes/Component/LineLight.hpp:50` (the class contract), `:150` and `:168` (`setPCFRadius()` / `setShadowBias()`
ignored, "A line light casts no shadow"), `createShadowDescriptorSet()` (`:290`) does nothing. The tube therefore
lights through walls, in the raster pass and in the traced effects alike (graphics doc 36 § Limits and traps: "The
next pass (owner decision pending)").

Split out of the former segment-rendering item on 2026-10-13 (its record:
graphics docs 33-36).

## What remains

- **Owner decision** on the technique — research the state of the art first and bring options. Directions to weigh:
  traced visibility (the RT lane already sees the polyline: `GPULightData` per segment) with stochastic samples along
  the segment and a denoiser, for the RT lane; for the raster lane, shadow maps along the tube (an omni map per
  segment or per sample point: cost scales with the polyline's 9 points) or a cheaper occlusion approximation; what
  the ScreenSpace lane does without RT.
- Then implement, with the measured cost on `beams` (`ArcPylons`, `CoilArc`), 0 `VUID-`.

## ⚠️ Traps

- **The light follows the CURVE, not the arc's GPU wander** (graphics doc 36): a shadow computed from the CPU polyline
  will not match the drawn arc exactly; accept it or move the wander's reference to the CPU.
- **9 points** (`LineLightMaxPoints`): a longer polyline is resampled by arc length; the shadow must use the same
  resampled points as the lighting.
- RT: a line light costs one `GPULightData` per segment out of `MaxRTLights` (128) and is dropped first when the budget
  is full — a shadowed line light must not silently lose its shadow when that happens.
- A beam is emissive translucent geometry, out of the TLAS (`rt-blended-materials-in-tlas`): it must not occlude its own
  light.

## References

- engine `docs/subsystems/graphics/36-line-lights-ltc-and-the-beam-that-lights.md` (design, photometry, limits).
- E. Heitz, J. Dupuy, S. Hill, D. Neubelt, "Real-Time Polygonal-Light Shading with Linearly Transformed Cosines",
  ACM TOG 35(4) (SIGGRAPH 2016); E. Heitz, S. Hill, "Real-Time Line- and Disk-Light Shading with Linearly Transformed
  Cosines", SIGGRAPH 2017 course.
- E. Heitz, S. Hill, M. McGuire, "Combining Analytic Direct Illumination and Stochastic Shadows", I3D 2018 (the LTC
  ratio estimator for shadowed area lights).
