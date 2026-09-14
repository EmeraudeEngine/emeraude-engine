---
id: rt-blended-materials-in-tlas
title: Blended (alphaMode BLEND) materials enter the TLAS as opaque instances
status: open
priority: unranked
scope: Scenes/SceneMetaData, Graphics/Effects/Lighting (RTGI, RTAO, RTR, ContactShadows)
opened: 2026-08-30
tags: [ray-tracing, transparency, measurement]
---

# Blended (alphaMode BLEND) materials enter the TLAS as opaque instances

## Why

`SceneMetaData` flags an instance `FORCE_NO_OPAQUE` only when a sub-geometry is ALPHA-TESTED. A
BLEND material (Sponza's `dirt_decal` quads at opacity 0.35, its ivy) is an opaque triangle for
every ray: GI bounce rays and AO/shadow rays hit the whole quad, transparent texels included, and
the RTGI bounce shades the hit with the decal's material (glTF-default metal → black bounce).

## Measured on Sponza, 2026-09-15 — owner-reported symptom, and it is not subtle

The owner's words for it: *"je le trouve nul à cause des gros pâtés fades induits par la
réflexion"*. The subject is the courtyard tree, whose leaves (`LeafSpring`) are **`alphaMode =
BLEND`**; the wall ivy next to it (`IvyLeaf`) is **`OPAQUE`** and stays green under the same light —
which is the control that points straight at this item.

A/B on the Reflections slot (`PostProcess.disable("Reflections")`), at a **PINNED** exposure:

| | with Reflections | without | the slot's share |
|---|---|---|---|
| foliage | **47.98** | 19.41 | **60 %** |
| stone | 48.20 | 46.03 | **4.5 %** |

**Thirteen times more reflection on blended leaves than on opaque stone**, and the canopy loses its
greens with it: the tree renders as a flat beige mush and becomes a green, legible tree the moment
the slot is off. This is the measurement point 1 below asked for, and the answer is that the cost is
not a subtle darkening of bounces — it is the largest single contributor to a hero asset's look.

⚠️ **The exposure triad matters here**: sunny-16 (f/16 · 1/125 · ISO 100) renders this shaded
courtyard at 12/255 and settles nothing. f/5.6 · 1/125 · ISO 100 is the readable one for this pose.

⚠️ **Do not attribute this to the asset.** It was first written down in `src/Builtin/AGENTS.md` as
"the tree is simply pale, do not open it as a defect" — from looking, not measuring. One A/B
reversed it.

## Implemented 2026-09-15 — BLEND is a cutout for rays, at 0.5

Option 2 of the decision below, taken: `GPURTMaterialData::IsBlended` (bit 10) is exported for any
material with `BlendingEnabled`, `SceneMetaData` flags such an instance `FORCE_NO_OPAQUE` (the local
was `anyAlphaTest`, it is `anyNonOpaque` now), and the shared `rtCandidateIsSolid()` runs the same
sampling path for it at `RTBlendedCutoff` = 0.5 — a ray query cannot blend, it confirms a candidate
or it does not. RTGI and RTR declare their own copies of the flag constants and got the two new ones.

**Measured on Sponza at a pinned exposure** (f/5.6 · 1/125 · ISO 100), Reflections on, same pose:

| | before | after |
|---|---|---|
| stone | 46.03 | **52.39** |
| whole frame | 45.88 | 48.42 |
| foliage | 47.98 | 49.58 |

The stone gains 6.4: the sky now reaches it through the canopy instead of being stopped by leaf
quads that were opaque to every ray. Zero VUID, and the frame is otherwise unchanged.

⚠️⚠️ **It does NOT fix the look of the tree, and that was the hypothesis it was built on.** The
foliage barely moves (47.98 → 49.58). This item is about blended geometry BLOCKING rays; the tree's
problem is blended geometry RECEIVING reflection, which is a different half and is now tracked
separately — see [`foliage-takes-most-of-its-light-from-the-reflection.md`](foliage-takes-most-of-its-light-from-the-reflection.md).

## What remains

1. ~~Measure on Sponza how much the blended quads cost.~~ **Done.** Still unmeasured: the RTGI and
   RTAO halves — the A/B above moved the Reflections slot only, and the implementation changed the
   rule for every ray.
2. ⚠️ A 35 %-opaque decal is now INVISIBLE to rays (0.35 < 0.5) where it used to be fully opaque.
   That is the sanctioned trade of a cutout rule, and it is UNMEASURED on the decals themselves.
2. Decide: treat BLEND as a cutout for rays (`FORCE_NO_OPAQUE` + the shared alpha-test rule at 0.5,
   the industry default), or exclude blended renderables from the TLAS.

## ⚠️ Traps

- ⚠️ The raster side was fixed separately (the albedo attachment is opacity-blended, Aug 2026);
  the RT side is a different path with a different rule. Both must agree on what a 35 % decal is.

## References

- `src/Scenes/SceneMetaData.cpp` (instance flags), `src/Graphics/Effects/Shared/RTAlphaTestGLSL.hpp`.
- `docs/caution-points.md` § "big dark squares under Sponza's decals".
