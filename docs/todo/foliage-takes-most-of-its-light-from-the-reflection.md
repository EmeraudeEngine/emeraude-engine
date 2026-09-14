---
id: foliage-takes-most-of-its-light-from-the-reflection
title: Foliage washed out to grey — the reflection effects owed the ENVIRONMENT BRDF (resolved; residual tracked)
status: open
priority: unranked
scope: Graphics/Effects/Lighting (RTR, SSR), Saphir/LightGenerator
opened: 2026-09-15
blocked-by: []
tags: [reflection, measured, owner-reported]
---

# Foliage washed out to grey — the reflection effects owed the ENVIRONMENT BRDF

## Why

Owner-reported: *"je le trouve nul à cause des gros pâtés fades induits par la réflexion"*, then,
at point-blank range on a single branch: *"on peut voir en raytracing un blanc dégueulasse qui
entoure toutes les aiguilles du cyprès, et en SSR ça existe aussi, mais moins violent"*.

Sponza's cypress renders grey-white where it must be green. It is NOT a distance or coverage
artefact: at point-blank range the mask is sampled near mip 0, every needle silhouette is crisp,
and the canopy washes out anyway.

## The A/B that settles it — one frame, one variable

Same pose, exposure pinned (f/5.6 · 1/125 · ISO 100), `PostProcess.disable("Reflections")`:

| configuration | foliage luminance | saturation | green excess (G−R) |
|---|---|---|---|
| RayTracing + reflections | 85.36 | **8.45 %** | +0.65 |
| ScreenSpace + reflections | 70.86 | 38.72 % | +7.92 |
| **RayTracing, reflections off** | **21.60** | **52.88 %** | **+4.50** |
| ScreenSpace, reflections off | 21.51 | 51.62 % | +4.45 |

⚠️ With the slot off the two lanes agree to within 2 % — that is the control that makes every other
reading here trustworthy. The stone control moves the other way (RT 15.43 % against SS 11.19 %), so
the desaturation is specific to the foliage, not a property of the frame.

## The chain, read in the code and confirmed by measurement (2026-09-15)

1. **The reflection carries no leaf colour, by construction.** `F0 = mix(vec3(0.04), albedo,
   metalness)`; `LeafSpring` is `metallicFactor = 0`, so F0 is a colourless 0.04 and
   `fresnelTint = fresnelColor / fresnel` is **white**. A metal tints its reflection by its albedo;
   a dielectric does not. The owner's words — *"sans prendre la couleur"* — are literally what the
   code does.
2. **The alpha silhouette is a grazing normal, and it is AUTHORED that way.** Measured on
   `Cypress_LF01_Spring_Normal.png` against the base colour's alpha:

   | zone | mean normal Z |
   |---|---|
   | needle body (opaque) | **+0.676** |
   | under the transparent texels | +0.980 (flat, the unpainted default) |
   | **alpha edge** | **+0.272** — near TANGENT |

   A blend between 0.676 and 0.980 cannot produce 0.272: the rolled edge of the needle is encoded
   faithfully. So `NdotV → 0` on every silhouette, and Schlick's Fresnel → 1 there.
3. **Nothing attenuated it.** `roughnessFade = 1 - smoothstep(0.6, 0.9, roughness)` and the leaf
   declares `roughnessFactor = 0.5` (flat, no metallic-roughness texture — its sibling `IvyLeaf`
   has one), so the fade was exactly **1.0**: full mirror treatment.
4. **And `rtrReflectivity = max(IBLIntensity · (1 − roughness), metalness) = 0.5`** multiplies on
   top — a glossiness used as a reflectance weight.

Net: up to **half of every silhouette pixel replaced by white sky**. With thousands of needles the
silhouettes ARE the visible surface.

## ⚠️⚠️ The defect is SHARED, which is what makes it interesting

Owner's observation, then measured: the residual rim occupies the same pixels in both lanes.
Rim = share of foliage pixels that are bright AND desaturated (`lum > 55`, `sat < 18 %`):

| configuration | rim |
|---|---|
| plain Schlick | 71.15 % |
| roughness-bounded Fresnel (delivered) | 51.26 % |
| + a blunt roughness-fade experiment (NOT delivered) | 2.09 % |
| reflections off — the FLOOR | 0.91 % |

- **74.0 % of the RT rim pixels are also rim pixels in SS.** Same zone, same pixels.
- **42.2 % of the RT rim exists with NO reflection at all** — so a large share of it is the DIRECT
  specular highlight, which no reflection-side fix can reach.

⚠️ This reading — three symptoms, one near-tangent shading normal at the mask edge — is correct and
is why the geometric specular antialiasing was applied to the roughness all three read. But it is
NOT what carried the fix, and the conclusion drawn from it at the time ("specular occlusion is not
the right target") was wrong twice over: the occlusion did help, and the real defect was elsewhere
entirely. The grazing normal is the AMPLIFIER; the missing environment BRDF was the CAUSE. See
below.

## What is delivered — and what actually carried it

**The environment BRDF** (Karis, *Real Shading in Unreal Engine 4*, 2013, analytic fit), in
`RTR.cpp` and `SSR.cpp`. The composite weight is now `F0 * A + B` — the integral of the specular
BRDF over the lobe, which is what must multiply a prefiltered environment — instead of a raw
Schlick Fresnel patched up by a roughness fade and a reflectivity nibble.

At grazing incidence on the leaf (F0 = 0.04, roughness 0.5): **0.500 against 0.155**, a 3.2x
overestimate applied precisely on the alpha silhouettes.

⚠️⚠️ **The raster IBL path was already doing this correctly** through `IBLTexture::Role::BRDFLut`.
Only the reflection effects were not — the raster and the reflections disagreed on the same surface.

| configuration | foliage lum | saturation | G−R | rim |
|---|---|---|---|---|
| raw Schlick (RT) | 85.36 | 8.45 % | +0.65 | 71.15 % |
| **environment BRDF (RT)** | **32.17** | **30.92 %** | **+4.07** | **2.17 %** |
| raw Schlick (SS) | 70.86 | 38.72 % | +7.92 | 16.42 % |
| **environment BRDF (SS)** | **55.95** | **47.15 %** | **+8.53** | **2.98 %** |
| reflections off — the FLOOR | 21.60 | 52.88 % | +4.50 | 0.91 % |

⚠️ **It is not "reflections off"** — the stone KEEPS its reflection (34.39 against 28.15 with the
slot off) while the foliage stops being flooded. A smooth metal is untouched by construction
(F0 = 1, roughness 0 returns 1.000), so the calibrated 0.82 mirror cannot move.

Three smaller terms landed with it, cumulative but minor next to it:

1. **Roughness-bounded Fresnel** (Lagarde 2014) — 34 % of the excess luminance on its own. Now
   superseded as the composite weight by the environment BRDF.
2. **Geometric specular antialiasing** folded into the roughness WRITTEN TO THE G-BUFFER
   (`SceneRendering.cpp`), so both lanes and the direct specular inherit it from ONE place.
   ⚠️ It filters SUB-PIXEL normal variance, so it does almost nothing at point-blank range where the
   normal is resolved — it earns its keep on distant foliage. Predicting otherwise cost a detour.
3. **Traced specular occlusion** in RTR: four visibility-only rays across `coneTan` weight the
   sky-miss branch, because the mirror ray is a ONE-SAMPLE estimate of the lobe. ⚠️ MISS branch only.
   ⚠️ RT lane only — SSR cannot trace.

## What remains

- The rim floor is 0.9 % (the legitimately sun-lit needle tips) and the lanes now sit at 2.2 % and
  3.0 %. The residual is small and structured.
- ⚠️ The SS/RT luminance gap that remains (55.95 against 32.17 on foliage, 69.60 against 34.39 on
  stone) is the DOCUMENTED screen-space sky-visibility over-estimate in enclosed spaces, not
  anything introduced here.
- The edge albedo is still only 48 % of the true colour for want of alpha dilation, which is what
  gives any remaining glint its contrast:
  [`alpha-mask-textures-lose-coverage-and-colour-in-the-mip-chain.md`](alpha-mask-textures-lose-coverage-and-colour-in-the-mip-chain.md).

## ⚠️ Traps this has already cost

- ⚠️⚠️ **A retracted measurement lived in this file**: "the reflectivity nibble is 0.195 on the
  foliage against 0.205 on the stone, so that lead is CLOSED". Those numbers are exactly `47.98/255`
  and `48.20/255` — the ORDINARY frame. The debug lane was never active (the lane was missing from
  the program-cache key, so the cached program was served). **An instrument must be PROVEN on before
  anything is read from it**, and a lead must never be closed on an unproven one.
- ⚠️⚠️ **A blunt global lever looks like a fix and is not.** Moving `roughnessFade` to
  `smoothstep(0.2, 0.6, roughness)` took the rim from 51 % to 2.09 % and the owner confirmed it by
  eye — but it dims the reflection of EVERY material under 0.6 roughness in both lanes and would
  wreck the calibrated 0.82 mirror of `post-processor-effect-debug`. Reverted. Its value was
  diagnostic: it proved a ~6x reduction was what the foliage needed, which is almost exactly what
  the environment BRDF later delivered by re-weighting instead of dimming. **The test that tells
  them apart is the stone: a lever dims it too, the correct term leaves it reflecting.**
- ⚠️⚠️ **Three physically-founded fixes in a row moved the number and missed the cause.** Lagarde,
  specular AA and traced occlusion each helped (71 % -> 51 % -> 40 % -> 28 % rim) while the real
  defect was a MISSING TERM, not a mis-tuned one. The tell was available from the start and was not
  read: the raster IBL and the reflection effects computed the same quantity two different ways.
  **When a value looks wrong, check whether another path in the same engine already computes it
  correctly before deriving a correction for it.**
- The first three observations on this frame — "white speckles on the stone", "blown-out leaves",
  "the asset is pale" — were made by LOOKING at a downscaled view and none survived measurement.
- A wrong exposure triad hides the whole thing: at sunny-16 both A/B frames sit near black.
- ⚠️ **The app SAVES its settings on exit**: a debug key set for a session is written back to the
  owner's `settings.json` by `Core.shutdown()`. Restore it AFTER the process is gone, not before.
