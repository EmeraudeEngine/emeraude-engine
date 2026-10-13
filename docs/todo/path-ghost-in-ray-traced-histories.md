---
id: path-ghost-in-ray-traced-histories
title: A path's old shape survives in the ray-traced effects' temporal histories after a point-count change
status: open
priority: high
scope: Graphics/Effects/Lighting (RTR, RTGI temporal histories), Graphics/Renderer (markInvalidTemporalHistory)
opened: 2026-10-13
tags: [graphics, temporal, paths, ray-tracing, defect]
---

# A path's old shape survives in the ray-traced effects' temporal histories after a point-count change

## Why

The TAA ghost of a path whose point count changes is fixed (2026-10-13: the old shape's screen region is cleared to
1 in the reactive mask, graphics doc 35 § The point-count change — 2 565 ghost pixels → 0 from frame 0). The same
measurement (`paths`, zigzag → straight line, TAA on, 30 frames by `temporalCapture`, RT lane) left a band of
**~650 pixels just under the old zigzag, identical before and after the fix**. Suspected: the ribbon's REFLECTION
(or its GI bounce) kept by the ray-traced effects' own temporal histories, which never read the reactive mask. A
path casts no shadow (`disableShadowCasting()`), so a shadow history is not a candidate. A wrong image for some
frames: a defect.

## What remains

- Attribute it first: the same sequence with `PostProcess.disable(...)` on each RT effect in turn (reflections,
  indirect diffuse), then on the ScreenSpace lane — the band must vanish with one of them. Record pixels and frames.
- Then a design decision for the owner: let the RT effects' history rejection read the reactive mask (or the
  invalidated regions themselves, `SceneInstanceTransforms::temporalHistoryInvalidations()`), versus a per-effect
  disocclusion test. Reuse the region invalidation; do not add a second mechanism.

## ⚠️ Traps

- The demo's Catmull-Rom loop ROTATES and pollutes any whole-frame difference mask: count inside the zigzag's area
  (the band sits just under x 419-909, y 672-733 at 3840x1990 from the default pose).
- `temporalCapture` blocks the main thread: send `setPoints` first, then start the capture.
- Related, distinct: `gi-history-keeps-a-switched-off-emitter-lit` (a radiance DROP on unchanged geometry).

## References

- engine `docs/subsystems/graphics/35-paths-polylines-and-curves.md` § The point-count change, `33-beams-lasers-and-electric-arcs.md` § The reactive mask.
- AMD FidelityFX Super Resolution 2, "Reactive mask"; NVIDIA ReBLUR history reset on disocclusion (NRD documentation).
