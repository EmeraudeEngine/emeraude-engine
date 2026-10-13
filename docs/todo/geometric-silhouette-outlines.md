---
id: geometric-silhouette-outlines
title: Geometric silhouette outlines from VertexFactory::Silhouette, for debug and wireframe tools (B.1)
status: open
priority: medium
scope: Graphics (segment drawing: the Path ribbon), Scenes (editor / debug tools), emeraude-base VertexFactory::Silhouette
opened: 2026-10-13
tags: [graphics, lines, outline, debug, editor, owner-request]
---

# Geometric silhouette outlines from VertexFactory::Silhouette, for debug and wireframe tools (B.1)

## Why

The owner's segment-rendering request (2026-09-28) named emeraude-base's `VertexFactory::Silhouette` algorithm for
outlining an object. The SELECTION outline was then built screen-space (B.2, a "custom depth" pass: graphics doc 34),
and **B.1, the geometric silhouette, stays for debug / wireframe tools, later** (owner decision, 2026-09-28
afternoon). It is the one user of the segment family not built yet. Split out of the former segment-rendering item
on 2026-10-13 (its record: graphics docs 33-36).

What exists to build it on:
- `Base::VertexFactory::Silhouette` (`src/VertexFactory/Silhouette.hpp` of emeraude-base): `prepare(shape)` (position-
  welded adjacency and triangle planes, once), `build(eyePosition)` / `buildOrthographic(viewDirection)` (no allocation
  past the peak), `edges()` = oriented `ShapeEdge`s from the front-facing triangle. Not used by the engine yet.
- The general polyline primitive ("primitive A"): the Path's camera-facing ribbon drawn by vertex pulling
  (`Component::Path`, `Saphir::PathGLSL`, `Geometry::PulledVertexResource`, graphics doc 35), width in metres or pixels,
  depth-tested world mode or always-on-top debug mode.

## What remains

- Design with the owner: which tools use it (wireframe / debug view, editor), and how the edges reach the ribbon — the
  Path draws POLYLINES, `Silhouette` returns an unordered set of EDGES: chain them into polylines on the CPU, or add a
  segment-list mode to the ribbon (the bevel/miter joins then do not apply).
- Re-run `build()` when the viewer moves (per frame for a moving camera): the cost on a large mesh must be measured;
  `prepare()` again on deformation (skinning) — a skinned mesh may be out of scope for a first pass.
- Proof: a capture on `geometry-generator` with the silhouette drawn, its cost per frame, 0 `VUID-`.

## ⚠️ Traps

- Hardware wide lines are optional (`Vulkan/Instance.cpp` only warns without `wideLines`) and Metal has none: segments
  must be GEOMETRY, never `lineWidth` (`Saphir/Generator/GizmoRendering.cpp` draws at 1 px).
- `Silhouette` welds by EXACT position: vertices that differ by float noise are not welded and their edges read as
  boundaries; a double-sided card reports its interior edges.
- The ribbon refuses instancing, MDI, cubemap and CSM (instance-transforms SSBO path only, graphics doc 35).

## References

- Akenine-Möller, Haines, Hoffman et al., "Real-Time Rendering", 4th ed., § 15.2 (silhouette edges).
- Rougier, "Shader-Based Antialiased, Dashed, Stroked Polylines", JCGT 2013; Matt DesLauriers, "Drawing Lines is Hard"
  (2015).
- engine `docs/subsystems/graphics/34-the-selection-outline-custom-depth.md` (B.2, the screen-space outline),
  `35-paths-polylines-and-curves.md` (the ribbon).
