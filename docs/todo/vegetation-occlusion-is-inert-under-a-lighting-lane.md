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

## ⚠️⚠️ The obvious fix was BUILT, MEASURED and REVERTED (2026-09-22)

Option 3 below — feeding the baked density to the lane as a **floor** on its own visibility,
`V = min(V_search, V_baked)` — was implemented end to end and reverted after measurement. Keep this
section before trying it again.

- The publication path worked: the **R low nibble** of the material-properties G-buffer, which was
  `reserved` and written as a literal 0, carried the bound with **15 as the neutral value** (0 would
  have meant "sees no sky" for every surface that forgot to publish). The SSGI **horizon pass**
  took a third input and clamped its result. Zero validation error.
- It changed **nothing**: with the wind frozen so the two runs are geometrically identical (0.100 %
  of the image differs), foliage luminance moved **-0.01 %**, the darkest quarter +0.69 %, the
  brightest -0.18 %, sky and grass controls 0.00 %.
- **Why, and it invalidates the argument that motivated it**: the horizon search already measures,
  inside a canopy, a visibility BELOW the baked bound (median 0.87). Its blindness is for occluders
  that are **off screen**; a leaf deep in a tree is occluded by other leaves that are mostly on
  screen and near. The Sponza floor at 12x is not the same situation as a canopy.
- Reverted on the owner's call: nothing inert ships in the engine.

⚠️ A measurement trap found doing it: **freeze the wind before comparing two runs**. With the trees
swaying, two captures are at different wind phases and the comparison is worthless — the first one
reported the canopy interior getting 13 % BRIGHTER under a floor, which a floor cannot do.

## What remains

An owner decision, with one branch now closed:

1. **Leave it.** The channel serves the no-lane case (-2.44 %) and costs nothing otherwise. The
   canopy keeps looking flat under a lane. This is the current state.
2. **Attenuate the lane's indirect diffuse by it.** ⚠️⚠️ Both lanes already estimate their own
   occlusion, so multiplying a vertex-baked density on top **double-darkens** the inside of the
   canopy, and the result would swing with the camera angle. It needs a rule for what each term
   owns, not a multiply.
3. ~~A floor on the lane's visibility~~ — **tried, inert, reverted**. See above.
4. **Find the case where the search really does over-estimate** (foliage under a roof, a camera
   below the canopy against a wall) and measure the bound there before building anything. That is
   what the floor attempt skipped.

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
