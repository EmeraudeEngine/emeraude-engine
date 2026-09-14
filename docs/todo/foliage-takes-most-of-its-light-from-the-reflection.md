---
id: foliage-takes-most-of-its-light-from-the-reflection
title: Sponza's foliage takes 60 % of its luminance from the Reflections slot — in BOTH lanes
status: open
priority: unranked
scope: Graphics/Effects/Lighting (RTR, SSR), Graphics/Material
opened: 2026-09-15
blocked-by: []
tags: [reflection, measured, owner-reported]
---

# Sponza's foliage takes 60 % of its luminance from the Reflections slot

## Why

Owner-reported, in his words: *"je le trouve nul à cause des gros pâtés fades induits par la
réflexion"*. The courtyard tree renders as a flat beige mush and becomes a green, legible tree the
moment the Reflections slot is switched off.

**Measured** at a pinned exposure (f/5.6 · 1/125 · ISO 100 — sunny-16 renders this shaded courtyard
at 12/255 and settles nothing), `PostProcess.disable("Reflections")` as the A/B:

| | with | without | the slot's share |
|---|---|---|---|
| foliage | **47.98** | 19.41 | **60 %** |
| stone | 48.20 | 46.03 | **4.5 %** |

Thirteen times more reflection on the leaves than on the stone, in ABSOLUTE terms (+28.6 against
+2.2), and the canopy loses its greens with it.

## What is already ruled out

- ⚠️ **NOT the asset.** First written down as "the tree is simply pale"; one A/B reversed that.
- ⚠️ **NOT blended geometry blocking rays.** That is a real defect and it was fixed the same day
  ([`rt-blended-materials-in-tlas.md`](rt-blended-materials-in-tlas.md)) — the stone gained 6.4 from
  it. The foliage moved 47.98 → 49.58, i.e. not at all. Two different halves of the same word.
- ⚠️ **NOT the RT trace.** Switching the lane to ScreenSpace keeps the over-lighting: foliage 45.13
  against 19.41 with no reflection. **Both lanes do it, so the cause is the WEIGHT the reflection is
  given on those pixels, not how the reflection is gathered.** The traced lane additionally
  desaturates (saturation 9.75 against SSR's 14.87) — the owner's own reading: *"en Screen-Space
  c'est mieux, mais il reste des trucs zarbi quand même"*.

## The chain, traced end to end 2026-09-15

**The reflectivity nibble is NOT the culprit** — measured with the lane displayed as the frame
(`Core/Graphics/DebugMaterialPropertiesLane = 1`, added the same day):

| | nibble |
|---|---|
| foliage | **0.195** (median 0.186, 0.1 % above 0.7) |
| stone | **0.205** (median 0.210, 0.0 % above 0.7) |

Identical. The leaves ask for no more participation than the wall. What differs is what the ray
BRINGS BACK and how it is applied:

```glsl
/* RTR.cpp:827 — in the trace */        float confidence = fresnel * roughnessFade;
/* RTR.cpp:833 */                       outReflection = vec4(envColor * fresnelTint * confidence, confidence);
/* RTR.cpp:1979 — at the composite */   em_Color.rgb = mix(em_Color.rgb,
                                            rtrData.rgb / max(rtrData.a, 0.001),
                                            rtrOwnConfidence * intensity * rtrReflectivity);
```

⚠️ `rtrData.rgb / rtrData.a` **divides the confidence back out**, recovering the full environment
colour, and the composite then re-weights by `fresnel · roughnessFade · intensity · nibble`. On a
canopy seen from the side the leaf cards sit at **grazing incidence**: `NdotV → 0`, so Schlick's
Fresnel → **1**. What is left is ≈ `0.195 · roughnessFade` of a very bright sky, applied as a
`mix()` — a REPLACEMENT of the shaded colour, not an additive term. `mix(19, ~150, 0.2) ≈ 45`, and
47.98 is what the frame reads.

So the arithmetic is consistent and each factor is defensible on its own. What is missing is the
term that would stop a leaf in the MIDDLE of a canopy from reflecting a full, unoccluded sky:
**specular occlusion** — [`specular-occlusion-from-the-bent-normal.md`](specular-occlusion-from-the-bent-normal.md),
whose own words are "a rough metal inside a closed room still reflects a full, unoccluded
environment". Replace the metal with a leaf and the room with a canopy.

⚠️ **The owner's own hypothesis, and what is and is not established**: *"le mode RayTracing ignore
l'alpha des feuilles quand il applique son apport depuis la réflexion, ce qui peut faire sens
puisqu'on se retrouve dans le G-buffer"*. Half-confirmed. The composite has **no notion of pixel
coverage** — it mixes at full strength on a pixel that may be one-third leaf — but the
material-properties alpha lane is a SELECT bit (`step(0.5, opacity)`, Aug 2026), so a pixel under
50 % leaf keeps the BACKGROUND's nibble rather than the leaf's. Whether the residual partial-coverage
error is significant next to the grazing-Fresnel term is UNMEASURED. Do not pick between the two
without an A/B.

## Leads, in the order they cost

0. ~~Read the reflectivity nibble on a leaf.~~ **Done — it is 0.195 against the stone's 0.205, so
   this lead is CLOSED.** The instrument is permanent now: `Core/Graphics/DebugMaterialPropertiesLane`
   (0 off, 1 reflectivity, 2 AO response), read once per program generation, so it takes effect on
   the next launch and costs nothing at runtime.
1. ~~**Read the reflectivity nibble on a leaf.**~~ `reflectivity = max(metalness, 1 - roughness)`, so
   `LeafSpring` (metal 0, roughness 0.5) asks for **0.5** — half-reflective, against the stone's
   measured 0.42. That formula conflates *smooth* with *reflective*: a dielectric's F0 is 0.04
   whatever its roughness. Displaying that lane as the frame is the cheapest instrument there is and
   it is what settled the owner's previous "gros pâtés flous" (the dirt decals, Aug 2026) — there is
   no permanent switch for it, that fix used a temporary code change.
2. **The leaf cards are `doubleSided`.** Two-sided lighting flips `N` toward the viewer
   (`N = dot(N,V) < 0 ? -N : N`); a flipped normal sends the reflection ray somewhere the surface
   does not actually face — skyward, on a canopy seen from below.
3. Only then the trace itself.

## ⚠️ Traps this already cost

- The first three observations on this frame — "white speckles on the stone", "blown-out leaves",
  "the asset is pale" — were all made by LOOKING at a downscaled view and none survived measurement
  (stone: **0.00 %** of pixels above 250; foliage: **0.3 %** near-white).
- A wrong exposure triad hides the whole thing: at sunny-16 both A/B frames sit near black and the
  60 % is unreadable.
