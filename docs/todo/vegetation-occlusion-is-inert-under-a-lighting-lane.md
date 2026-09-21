---
id: vegetation-occlusion-is-inert-under-a-lighting-lane
title: The vegetation baked occlusion is nearly inert while a lighting lane owns the indirect diffuse
status: open
priority: unranked
scope: Saphir/LightGenerator, Graphics/PostProcessing
tags: [vegetation, lighting, indirect-diffuse]
opened: 2026-09-22
---

# The vegetation baked occlusion is nearly inert while a lighting lane owns the indirect diffuse

## Why

The A channel of a generated tree is read since 2026-09-22: `LightGenerator` folds it into the
DIFFUSE AMBIENT factor, beside a material AO texture and never touching the direct lighting. It
works — but it only bites on the raster ambient leg, and both lighting lanes took that leg over
(`SSGI` and `RTGI` own the indirect diffuse; the raster's diffuse IBL is off in both).

Measured on the `tree-generator` bench at a PINNED exposure (f/8, 1/125, ISO 100), comparing the
foliage pixels with the declaration ON and OFF, everything else identical:

| Lighting lane | Effect of the baked occlusion |
|---|---|
| Active (default) | **-0.37 %** mean foliage luminance |
| `setLightingMode("None")` | **-2.44 %**, and the foliage contrast rises 35.11 → 36.15 |

Sky and bare-grass controls moved 0.00 % in both, so the measurement is clean.

The data itself is not the problem: on a quaking aspen the channel has a median of 0.87, a mean of
0.84 and 88 % of its vertices below 0.95, over a 0.45–1.0 range. It is the term it multiplies that
has almost no weight once a lane is on.

## What remains

An owner decision, because the obvious move is also the dangerous one:

1. **Leave it.** The channel serves the no-lane case and costs nothing otherwise. Honest, and the
   canopy keeps looking flat under a lane.
2. **Attenuate the lane's indirect diffuse by it too.** ⚠️⚠️ Both lanes already estimate their own
   occlusion — SSGI runs a GTAO horizon search, RTGI traces — so multiplying a vertex-baked density
   on top **double-darkens** the inside of the canopy. If this is chosen, it needs a rule for what
   each term owns, not just a multiply.
3. **Make it a hint the lane consumes**, e.g. as a floor or a bias on its own visibility rather
   than a multiplier. More work, and it is the only shape that cannot double-count by construction.

## Traps

- ⚠️ Measure at a PINNED exposure (`Act.setExposure`), or the auto-exposure absorbs the whole
  effect and the two captures look identical.
- ⚠️ The A channel is a DENSITY estimate, not ray-traced occlusion — a shading hint, never a
  photometric quantity. Do not let it into anything that claims to be physical.
- ⚠️ Name the surface: measure the FOLIAGE pixels (saturated green), not a rectangle that also
  holds grass and sky.

## References

- `Saphir/LightGenerator.cpp` — where the factor is folded, next to the material AO.
- Producer: emeraude-base `TreeSkinner::ambientOcclusion()`.
