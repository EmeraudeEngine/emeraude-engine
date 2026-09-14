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

## Leads, in the order they cost

1. **Read the reflectivity nibble on a leaf.** `reflectivity = max(metalness, 1 - roughness)`, so
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
