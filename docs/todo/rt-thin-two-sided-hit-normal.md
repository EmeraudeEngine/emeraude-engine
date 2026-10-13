---
id: rt-thin-two-sided-hit-normal
title: RT hit shading is one-sided on the unoriented vertex normal — a leaf hit from behind is lit from its far side
status: open
priority: high
scope: Graphics/Effects/Lighting (RTGI, RTR), Graphics/IrradianceProbeVolume, Graphics/Material/GPURTMaterialData
opened: 2026-09-25
tags: [ray-tracing, vegetation, two-sided]
---

# RT hit shading is one-sided on the unoriented vertex normal — a leaf hit from behind is lit from its far side

## Why

By reading (2026-09-25, leaf-translucency design, confirmed by an adversarial code check; lines checked 2026-10-13):
- RTGI (`src/Graphics/Effects/Lighting/RTGI.cpp`): `NdotL = max(dot(hitNormal, L), 0)` and the light skipped when
  `<= 0` (l.427-430); shadow origin `hitPos + hitNormal * bias` (l.443); `hitNormal` never faced toward the ray
  (l.677). RTR (`src/Graphics/Effects/Lighting/RTR.cpp`) does the same (l.554-557, hit normal l.767) and then PERTURBS
  the hit normal with the normal map (l.807-827).
- Probes (`src/Graphics/IrradianceProbeVolume.cpp`): `dot(hitNormal, direction) > 0` is treated as a BACK FACE
  (l.305): the ray is skipped with `continue` in the blend (l.419-423: its weight not added, so the direction is
  renormalised from the others) and its distance moment is shortened, which feeds the Chebyshev visibility.
So a two-sided leaf card hit on its authored back is lit from its FAR side in RTGI/RTR (an accidental T = R) and
skipped by the probes, while the raster orients the normal toward the viewer by the GEOMETRIC normal
(`src/Saphir/LightGenerator.PBR.cpp:705-723`, since engine `0c29b08a`; not a `gl_FrontFacing` test).

## What remains

- For materials that are two-sided (cull none) and thin: orient the GEOMETRIC hit normal toward the ray origin,
  before RTR's normal-map perturbation, and carry the orientation into it; treat their back faces as valid in the
  probe blend. A flag bit in `GPURTMaterialData` (free bits 11-15, 20-31); the four literal `* 7u` strides
  (`RTGI.cpp:628`, `RTR.cpp:750`, `IrradianceProbeVolume.cpp:312`, `RTAlphaTestGLSL.hpp:199`) become one
  generated constant if the struct grows.
- ⚠️ This CHANGES today's traced foliage (the accidental back-face lighting disappears, the probes near canopies
  change): owner-approved as a prerequisite of `foliage-diffuse-transmission` (2026-09-25). Measure before/after.
