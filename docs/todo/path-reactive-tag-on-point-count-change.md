---
id: path-reactive-tag-on-point-count-change
title: A path whose point count changes sets no reactive tag, so the TAA blends its old shape
status: open
priority: high
scope: Graphics/RenderableInstance/PathPoints, Graphics/Material/PathResource, Scenes/SceneInstanceTransforms (path directory), Saphir (reactive mask output)
opened: 2026-10-13
tags: [graphics, taa, paths, defect]
---

# A path whose point count changes sets no reactive tag, so the TAA blends its old shape

## Why

Owner decision (2026-09-29, the Path's first pass): every point carries `{current, previous}`, and **a point-count
change = velocity 0 + the reactive tag that frame** — the points of two different counts are unrelated, so the TAA
must not reuse the history there. Only the first half exists. `PathPoints::stage()` pairs the previous points only
when the count is the same (`src/Graphics/RenderableInstance/PathPoints.cpp:59-60`; doc at `PathPoints.hpp:58`) —
otherwise previous = current, velocity 0 — but nothing sets the reactive tag: nothing in
`src/Scenes/Component/Path.cpp` or `PathPoints.cpp` touches it (checked 2026-10-13), and `Material::PathResource`
has no `reactiveMaskExpression()` (only `Material::BeamResource` writes the mask, `BeamResource.cpp:373`).

The result: with velocity 0 the TAA reprojects the history at the same pixel, where the OLD shape still is, and the
neighbourhood clip lets part of it survive — a ghost of the previous path for a few frames after a re-tessellation
(new points from the console, a tolerance change, a curve edited live). A wrong image: a defect. Not measured yet.
Split out of the former segment-rendering item on 2026-10-13 (its record:
graphics docs 33-36).

## What remains

- Reproduce first: `paths` demo, TAA on, change a path's point count from the console
  (`SceneManager_Path_setPoints` with a different number of points) and capture the following frames; TAA off as
  the control. Record the ghost (pixels, frames) before fixing.
- Make the frame of a count change write the reactive mask over the path's pixels. The mask is per MATERIAL today
  (a GLSL expression fixed at program generation, `Saphir/Generator/SceneRendering.cpp:741-746`); a path needs a
  PER-FRAME value. A candidate, for the owner: the path directory entry is a `PathSpan {firstPoint, pointCount,
  reserved0, reserved1}` (`src/Scenes/SceneInstanceTransforms.cpp:204`, a `uvec4` in GLSL) — stage an "unpaired this
  frame" flag in a reserved field from `PathPoints::stage()` and let `PathResource::reactiveMaskExpression()` read it
  (the vertex stage already reads the entry).
- Proof: the same capture sequence, no ghost; 0 `VUID-`; a static path keeps its TAA (the tag lasts one frame).

## ⚠️ Traps

- The reactive tag lasts ONE frame in the TAA history (alpha 0); the mask is read over the 3x3 reconstruction
  footprint and a tagged history is REPLACED, not clipped (graphics doc 33 § The reactive mask): reuse that
  machinery, do not add a second one.
- The reactive attachment exists only when the velocity does (`m_hasReactiveAttachment`); the light passes never
  write it.
- The previous points are a RENDER-side history advanced by the primary view only (`advanceHistory`): the tag must
  follow the same rule, or a secondary view would clear it.

## References

- engine `docs/subsystems/graphics/35-paths-polylines-and-curves.md` (the Path), `33-beams-lasers-and-electric-arcs.md`
  § The reactive mask.
- AMD FidelityFX Super Resolution 2, "Reactive mask" (GPUOpen documentation); Unreal Engine "Responsive AA".
