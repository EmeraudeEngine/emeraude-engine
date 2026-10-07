---
id: lod-simplifier-stalls-on-disconnected-cards
title: Automatic LOD stalls on disconnected cards (Sponza's ivy keeps 2.1 M triangles at every level) and logs the target ratio
status: open
priority: high
scope: Graphics/Renderable (MeshResource, MultiLayerMeshResource), base VertexFactory/ShapeSimplifier
opened: 2026-10-04
tags: [lod, geometry, performance]
---

# Automatic LOD stalls on disconnected cards

## Why

With `Core/Graphics/LOD/EnableAutomaticGeneration = true`, Sponza's view goes from 10.5 to 7.8 M triangles but
`ScenePass` only from 45 to 40 ms (2026-10-04): `IvySim_Leaves` (thousands of disconnected leaf cards, OPAQUE) gets
LOD 1, 2 and 3 at the SAME 2 134 434 triangles. The view's LOD 0 bucket is 7.4 M triangles, most of it this mesh,
and the ivy alone costs 17 ms of forward lamp passes (hidden: 45.1 → 28.3 ms; the deferred resolve removed most
of that, the geometry itself stays).

Two defects:

1. The simplification does not reduce disconnected cards: base `ShapeSimplifier` falls back to
   `meshopt_simplifySloppy` only with `allowSloppy` and only when `meshopt_simplify` stalls above twice the target —
   either the option is off for these meshes or the sloppy pass does not reach the target either. To measure.
2. The "LOD n ready for '…' (N triangles, P %)" success log prints the TARGET ratio, not the achieved one: three
   identical counts were logged as 33 %, 10 % and 3 %.

## What remains

- Measure which of the two branches runs for the ivy, and what `simplifySloppy` (or a card-aware decimation:
  dropping whole cards by area, the way foliage LODs are usually built) achieves.
- Log the ACHIEVED ratio (and warn when a level is not smaller than the previous one: it is useless memory).
- ⚠️ Owner settings note: the owner's own `settings.json` has the generation OFF.
