---
id: deferred-resolve-animated-cubemap-projection
title: A point light projecting an animated cubemap keeps its forward passes — the deferred resolve does not bind the cubemap arrays
status: open
priority: unranked
scope: src/Graphics/DeferredLightResolve.cpp, src/Scenes/LightSet.cpp (isDeferredPunctualLight)
opened: 2026-11-23
tags: [deferred-lights, projection, performance]
---

# A point light projecting an animated cubemap keeps its forward passes

## Why

Since 2026-11-22 the deferred light resolve samples a light's colour projection (graphics doc 38 § *A light that
projects a texture*): a spot's picture and a point light's STILL cubemap leave the forward passes. A point light whose
projection is ANIMATED (a frame of a cubemap array) is still refused by the predicate
(`LightSet::isDeferredPunctualLight()`, `projectionResolved`): the resolve's shader does not declare the cubemap-array
table, so that light stays a forward pass over every lit batch.

What a forward projected light costs was measured on two spots (Linux, RTX 3070 Ti, 1280×720): `ScenePass` +1.47 ms
as forward passes, +0.05 ms in the resolve; on an Apple M6, 41 frames a second against 56-57. No scene with an
animated cubemap was measured: this is a performance gain to rank, not a wrong picture.

## What remains

- Declare the cubemap-array binding of the bindless table in the resolve's fragment shader, carry the frame index in
  `DeferredProjectionData` (`textureBoost.z` and `.w` are free, `src/Graphics/DeferredLightResolve.cpp:122`), a third
  kind in `DeferredLightData::cone[2]`.
- Accept the light in the predicate; prove the picture against the forward pass with
  `Core.RendererService.setDeferredPunctualLights(0 | 1)` at a pinned exposure, as for the spots.
- A reduced bindless profile (macOS: the table cut to the device budget) must still fit the binding.

## References

- `docs/subsystems/graphics/38-deferred-punctual-lights.md` § *A light that projects a texture* (the predicate row).
- Related: `traced-lanes-miss-directional-and-animated-cubemap-projections` (the same light in the traced lanes).
