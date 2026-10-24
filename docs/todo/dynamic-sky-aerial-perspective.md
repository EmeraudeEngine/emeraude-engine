---
id: dynamic-sky-aerial-perspective
title: Dynamic sky — aerial perspective, fog and clouds lit by the same atmosphere as the sky
status: open
priority: unranked
scope: Graphics/Effects/Atmosphere (AtmosphericFog, VolumetricClouds, VolumetricScattering), Graphics (atmosphere LUTs)
opened: 2026-10-24
tags: [sky, atmosphere, fog, clouds, design]
---

# Dynamic sky — aerial perspective, fog and clouds lit by the same atmosphere as the sky

## Why

Under a dynamic sky, a distant mountain must turn blue by day and pink at sunset, and a cloud must
be lit by the sun's REAL colour and by the sky's ambient. `AtmosphericFog`, `VolumetricClouds` and
`VolumetricScattering` take the sun as a colour × lux today (`VolumetricClouds.hpp:153-156`), with
their own models. If they keep separate models, the horizon does not match the sky drawn behind
it.

## What remains

- [ ] **The aerial perspective volume** (Hillaire 2020, a ~32³ froxel volume of in-scattering and
      transmittance along the view), applied to the opaque scene. Decide with the owner whether it
      REPLACES `AtmosphericFog`'s distance fog under a dynamic sky, or adds to it (height fog stays
      a separate, local medium).
- [ ] **Clouds**: their sun term reads the transmittance toward the sun at the cloud's altitude, and
      their ambient term reads the sky (the irradiance bake of `dynamic-sky-amortised-ibl`).
- [ ] **The far horizon**: the geometry's far edge and the sky must meet with no seam (same
      in-scattering at the same distance).

## References

- the physical sky, lot 1 (graphics doc 42, `42-physical-sky-atmosphere.md`), `dynamic-sky-amortised-ibl`.
- `src/Graphics/Effects/Atmosphere/`, `docs/post-processing-pipeline.md`.
