---
id: traced-lanes-miss-directional-and-animated-cubemap-projections
title: The traced lanes shade a directional light's projection and a point light's animated cubemap with the light's full emission
status: open
priority: high
scope: src/Scenes/LightSet.cpp (the RT light list), src/Graphics/Effects/Shared/TracedLightGLSL.hpp, RTR, RTGI, IrradianceProbeVolume
opened: 2026-11-23
tags: [ray-tracing, lights, projection, rtr, rtgi]
---

# The traced lanes shade a directional light's projection and a point light's animated cubemap with the light's full emission

## Why

Since 2026-11-22 a traced light entry carries the light's colour projection (`LightSet::GPULightData`, nine vec4;
scenes doc 16 § *A traced light entry carries the light's COLOUR PROJECTION*): RTR, RTGI and the irradiance probes
shade their hit points with the picture the raster projects. Two kinds of projecting light were left out; their entry
says `NoProjection` and the traced lanes keep the light's FULL emission while the raster shows the picture:

| Light | Why it was left out | Where |
|---|---|---|
| A DIRECTIONAL light with a colour projection | its matrix depends on the block layout (classic or cascaded); a cascaded light does not project in the raster either | `src/Scenes/LightSet.cpp:905` |
| A POINT light with an ANIMATED cubemap (a frame of a cubemap array) | the traced lanes do not bind the cubemap-array table | `src/Scenes/LightSet.cpp:972` |

It is the defect the spot lights had: a headlight whose pattern lets 2.4 % of its cone through lit the reflections 42
times too much (a car's bonnet read 50.95 / 255 with the beam, 9.72 without, 18.68 once the projection was carried).
⚠️ Known BY READING only: no scene with a projecting directional light or an animated cubemap was measured in the RT
lane.

## What remains

- Find or build the two cases (a classic directional light with a picture, a point light with an animated cubemap)
  and measure them as the spot was: a reflector, a pinned exposure, the Reflections slot on and off, the projection
  removed.
- Directional: carry the classic layout's matrix, index and boost (`DirectionalLight::ColorProjectionIndexOffset`),
  and decide what a CASCADED light does — it does not project in the raster today, so the traced lanes are right to
  ignore it until it does.
- Animated cubemap: bind the cubemap-array table in the traced lanes' descriptor sets and carry the frame index
  (`PointLight::ColorProjectionFrameIndexOffset`); a fourth kind in `GPULightData::projectionKind` and in
  `emTracedLightProjection()`.
- The structure (144 bytes, asserted) and the GLSL chunk change together; the three readers share the chunk.

## ⚠️ Traps

- `EmTracedLightStride` and `sizeof(GPULightData)` are in lockstep: a field added on one side only shifts every light.
- The traced zones' cost was flat within 0.05 ms for the spot's picture; a cubemap array adds a binding to three
  pipelines — measure it on the 8 GB card.

## References

- `docs/subsystems/scenes/16-lightset-background-derived-lighting.md` (the traced projection, "Not carried").
- `src/Graphics/Effects/Shared/TracedLightGLSL.hpp`, `src/Graphics/Effects/Lighting/RTR.cpp`, `RTGI.cpp`,
  `src/Graphics/IrradianceProbeVolume.cpp`.
- Related: `deferred-resolve-animated-cubemap-projection` (the same light in the raster's resolve).
