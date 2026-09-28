---
id: segment-rendering-beams-curves-outlines
title: Graphics — segment rendering (selection silhouette, paths and curves, lasers and electric arcs)
status: open
priority: unranked
scope: Graphics / Saphir
opened: 2026-09-28
tags: [graphics, shaders, lines, beams, photometry, owner-request]
---

# Graphics — segment rendering (selection silhouette, paths and curves, lasers and electric arcs)

## Why

The owner wants one shader family that draws **segments**, with three users:

1. **Selection silhouette**: outline the selected object as seen from the camera, so what is targeted
   in 3D is obvious. The request names the `VertexFactory::Silhouette` algorithm of emeraude-base.
2. **Paths and Bézier curves**: draw paths (AI routes, camera rails, splines) and Bézier curves.
3. **Lasers and electric arcs, Half-Life style**: GoldSrc's `env_beam` / `env_laser` took a start point,
   an end point, a noise amplitude (0 = a straight laser, above 0 = an arc that jitters), a colour and a
   width. Here the engine is PBR/photometric, so the beam also takes an **intensity** in a physical unit.

## What exists today

- `emeraude-base/src/VertexFactory/Silhouette.hpp` — rewritten and tested 2026-09-28 (it used to refuse
  every valid shape and returned the front wireframe). `prepare(shape)` once, then `build(eyePosition)` or
  `buildOrthographic(viewDirection)` in the shape's LOCAL space; edges come from the visible triangle,
  counter-clockwise. Contract and traps: emeraude-base `docs/subsystems/vertexfactory/02-architecture.md`
  § Processing & Analysis. It welds by exact position: a loaded mesh whose seam positions differ by
  float noise shows a hole there.
- `emeraude-base/src/Math/BezierCurve.hpp`, `Math/BSpline.hpp` — the curve maths exist and are tested
  (`test_MathBezierCurve.cpp`, `test_MathBSpline.cpp`).
- `Saphir/Generator/GizmoRendering.cpp:211` draws with `lineWidth = 1.0F`. Hardware wide lines are an
  optional feature (`Vulkan/Instance.cpp:1553` only warns when `wideLines` is missing) and Metal has none,
  so MoltenVK caps the width at 1 px: **segments must be expanded to geometry (camera-facing quads /
  ribbons), never rely on `lineWidth`.**

## What remains

### A. The segment primitive (shared by the three users)

- A polyline / segment list expanded to camera-facing ribbons in the vertex stage (or a mesh / geometry
  stage — to decide), with mitred or round joins and caps, anti-aliased edges in the fragment stage.
- Width in world units OR in pixels (outlines and debug paths want pixels, beams want world units).
- Colour is sRGB (`…Color` naming rule) converted to linear; **intensity** in the engine's photometric
  emissive unit (`Graphics/Photometry.hpp`), so a laser feeds bloom and exposure like any emissive.
- Depth: tested (beams, paths in the world) or always-on-top / occluded-dashed (selection, debug).

### B. Selection silhouette

- Two families, the owner decides:
  1. **Geometric** (the request): CPU silhouette edges from `VertexFactory::Silhouette` (fixed, see
     above), rebuilt when the camera or the object moves, drawn with primitive A. Exact, crisp, but CPU per frame on
     large meshes, and skinned / displaced / alpha-tested geometry does not match the GPU silhouette.
  2. **Screen-space**: render the selected object's mask (stencil or ID), dilate it (jump flood for wide
     outlines), composite. Matches whatever the GPU drew (skinning, alpha test), constant cost, the
     usual choice in editors and games.

### C. Paths and Bézier curves

- Tessellate `BezierCurve` / `BSpline` on the CPU (adaptive by screen-space error) and feed primitive A.
- Optional dashes, arrows / direction marks, per-vertex colour.

### D. Beams (lasers, electric arcs)

- Parameters, after `env_beam`: start, end (points or entity + offset), width, colour, intensity,
  **noise amplitude** (0 = straight), segment count, noise refresh rate, texture + scroll rate, life,
  optional random re-strike.
- The arc displacement: midpoint displacement (fractal lightning) or noise along the segment,
  perpendicular to the beam, deterministic per seed so it can be re-evaluated.
- Probably a `Scenes::Component` (like `ParticlesEmitter`), so a beam lives on an entity and is drivable
  from the console / MCP.
- Optional: a beam that lights the scene (a point light at the impact, or a line light) — separate
  decision.

## ⚠️ Traps

- TAA: a jittering arc and a moving beam need motion vectors, or they smear — see
  `particles-and-translucents-report-no-motion`.
- Ray tracing: a beam is emissive translucent geometry; keep it out of the TLAS unless decided otherwise
  (`rt-blended-materials-in-tlas`).
- High-intensity thin geometry aliases and flickers under TAA: the anti-aliased edge must fade width
  below ~1 px into alpha, not collapse to sub-pixel triangles.
- Pre-exposure: a beam's intensity sits in the scene-colour range — `scene-colour-pre-exposure`.

## Owner decisions to escalate before coding

1. Selection silhouette: geometric (B.1) or screen-space (B.2), or both (B.1 for debug / wireframe
   tools, B.2 for gameplay targeting)?
2. Ribbon expansion stage: vertex shader (portable), mesh shader, or geometry shader (absent on MoltenVK)?
3. Beam as a `Scenes::Component`, a renderable resource, or both?
4. Does a beam emit light into the scene?
5. The intensity unit exposed to the user (luminance in cd/m², or the existing emissive convention).

## References

- Half-Life (GoldSrc) FGD, `env_beam` / `env_laser` entities (start / end entity, amplitude noise,
  brightness, width, texture scroll rate, life, strike again time).
- Matt DesLauriers, "Drawing Lines is Hard" (2015) — screen-space polyline expansion, joins and caps.
- Ben Golus, "The Quest for Very Wide Outlines" (2020) — screen-space outlines, jump flood.
- Rong & Tan, "Jump Flooding in GPU with Applications to Voronoi Diagram and Distance Transform",
  I3D 2006.
- Unity `LineRenderer` / `TrailRenderer`, Unreal Niagara ribbon / beam emitter — existing APIs to compare.
