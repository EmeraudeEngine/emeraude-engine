---
id: rt-thin-two-sided-hit-normal
title: RT hit shading is one-sided on the unoriented vertex normal — a leaf hit from behind is lit from its far side
status: open
priority: unranked
scope: Graphics/Effects/Lighting (RTGI, RTR), Graphics/IrradianceProbeVolume, Graphics/Material/GPURTMaterialData
opened: 2026-09-25
tags: [ray-tracing, vegetation, two-sided]
---

# RT hit shading is one-sided on the unoriented vertex normal — a leaf hit from behind is lit from its far side

## Why

By reading (2026-09-25, leaf-translucency design, confirmed by an adversarial code check):
- RTGI: `NdotL = max(dot(hitNormal, L), 0)` and the light skipped when `<= 0` (`RTGI.cpp` ~410-416); shadow origin
  `hitPos + hitNormal * bias` (~425); `hitNormal` never faced toward the ray (~654-660). RTR does the same (~533,
  ~741-746) and then PERTURBS the hit normal with the normal map (~806).
- Probes: `dot(hitNormal, direction) > 0` is treated as a BACK FACE (`IrradianceProbeVolume.cpp` ~286-290): the ray
  is skipped with `continue` (its weight not added, so the direction is renormalised from the others) and its
  distance moment is shortened, which feeds the Chebyshev visibility.
So a two-sided leaf card hit on its authored back is lit from its FAR side in RTGI/RTR (an accidental T = R) and
skipped by the probes, while the raster flips the normal toward the viewer (`LightGenerator.PBR.cpp` ~577/587).

## What remains

- For materials that are two-sided (cull none) and thin: orient the GEOMETRIC hit normal toward the ray origin,
  before RTR's normal-map perturbation, and carry the orientation into it; treat their back faces as valid in the
  probe blend. A flag bit in `GPURTMaterialData` (free bits 11-15, 20-31); the four literal `* 7u` strides
  (`RTGI.cpp` ~611, `RTR.cpp` ~729, `IrradianceProbeVolume.cpp` ~295, `RTAlphaTestGLSL.hpp` ~199) become one
  generated constant if the struct grows.
- ⚠️ This CHANGES today's traced foliage (the accidental back-face lighting disappears, the probes near canopies
  change): owner-approved as a prerequisite of `foliage-diffuse-transmission` (2026-09-25). Measure before/after.
