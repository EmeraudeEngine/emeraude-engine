---
id: rt-lane-underlights-enclosed-spaces
title: The traced lane may UNDER-light an enclosed space — 0.05 of the sky where geometry says 0.45
status: open
priority: unranked
scope: Graphics (Effects/Lighting RTGI, RTAO; IrradianceProbeVolume)
opened: 2026-09-13
tags: [rendering, lighting, ray-tracing, measurement, owner-decision]
---

# The traced lane may under-light an enclosed space

## Why

The screen-space lane was corrected on 2026-09-13 because it lit enclosed spaces with the
unoccluded sky (`src/Graphics/AGENTS.md` § "The screen-space sky visibility"). The measurement that
closed that work raised the symmetrical question about the OTHER lane, and it is not answered.

**Measured on `sponza`, atrium floor** (`setPosition(-9, 5, 0)` + `lookAt(-9, 0, 0.6)`, exposure
pinned f/11 · 1/250 s · ISO 100, 2880×1620, RTX 3070 Ti, 0 VUID, all values linearised through the
gamma and the ACES fit):

| Lane | Linear value | Fraction of the unoccluded sky |
|---|---|---|
| Raster, unoccluded sky (`setLightingMode("None")`) | 0.1013 | 1.00 |
| ScreenSpace, with its new sky visibility | 0.0449 | **0.44** |
| RayTracing | 0.0048 | **0.05** |

The analytic visibility of a floor at the bottom of a slot 10 m wide and 10 m high is
`sin(atan(W / 2H))` = **0.45**, which is what the screen-space lane measures. The traced lane is
**9× below** that, on a floor whose light at this pose is essentially all indirect (the ratios above
could not hold if the sun reached it).

## What remains

1. Attribute the term: measure RTGI alone (disable `AmbientOcclusion` and `Reflections`), then the
   irradiance probe volume alone, against the same analytic target.
2. Suspects, in order: the `RTAO` multiply on top of an already-occluded indirect diffuse; the
   irradiance probe volume's own occlusion; a sky-ray rejection in the RTGI gather.
3. ⚠️ Nothing here says the traced lane is wrong and the screen-space one right — the analytic
   figure ignores the albedo of the walls and every bounce. It says the 3.7× residual between the
   two lanes on the gallery pose is NOT necessarily all on the screen-space side, which is how it
   has been read so far.
4. **Owner decision needed** before anything is changed: which lane is the reference?

## References

- `src/Graphics/AGENTS.md` § "The screen-space sky visibility" (the full measurement table).
- `docs/caution-points.md` § "the screen-space lane lit enclosed spaces with the UNOCCLUDED sky".
