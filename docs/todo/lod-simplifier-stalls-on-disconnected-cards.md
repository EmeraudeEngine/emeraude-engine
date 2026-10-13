---
id: lod-simplifier-stalls-on-disconnected-cards
title: Automatic LOD stalls on disconnected cards (Sponza's ivy keeps 2.1 M triangles at every level) and logs the target ratio
status: open
priority: high
scope: Graphics/Renderable (MeshResource, MultiLayerMeshResource), base VertexFactory/ShapeDecimator
opened: 2026-10-04
tags: [lod, geometry, performance]
---

# Automatic LOD stalls on disconnected cards (Sponza's ivy keeps 2.1 M triangles at every level) and logs the target ratio

## Why

With `Core/Graphics/LOD/EnableAutomaticGeneration = true`, Sponza's view goes from 10.5 to 7.8 M triangles but
`ScenePass` only from 45 to 40 ms (2026-10-04): `IvySim_Leaves` (thousands of disconnected leaf cards, OPAQUE) gets
LOD 1, 2 and 3 at the SAME 2 134 434 triangles. The view's LOD 0 bucket is 7.4 M triangles, most of it this mesh,
and the ivy alone costs 17 ms of forward lamp passes (hidden: 45.1 → 28.3 ms; the deferred resolve removed most
of that, the geometry itself stays).

Two defects:

1. The decimation does not reduce disconnected cards. The automatic LOD runs base `ShapeDecimator` (QEM,
   Garland & Heckbert) at `src/Graphics/Renderable/MeshResource.cpp:481` and
   `src/Graphics/Renderable/MultiLayerMeshResource.cpp:767` — NOT `ShapeSimplifier` (meshoptimizer, with its
   `allowSloppy` fallback), which only the USD loader uses (`src/Scenes/Loaders/USDLoader.cpp:1712-1726`). The
   decimator multiplies the cost of every boundary / UV-seam edge by `boundaryPenaltyWeight` (default 1000,
   base `src/VertexFactory/ShapeDecimator.hpp:73-77`, applied in `applyBoundaryAndSeamPenalties()`): every edge of
   a disconnected leaf card is a boundary edge, which most likely explains the identical counts (to confirm).
2. The "LOD n ready for '…' (N triangles, P %)" success log prints the TARGET ratio, not the achieved one: three
   identical counts were logged as 33 %, 10 % and 3 % (`MeshResource.cpp:534`, `MultiLayerMeshResource.cpp:821`
   print `ratio * 100`).

## What remains

- Confirm the boundary penalty is what stalls the ivy, then choose (owner decision): route the automatic LOD of
  such meshes through `ShapeSimplifier` (meshopt, `allowSloppy`), or a card-aware decimation (dropping whole cards
  by area, the way foliage LODs are usually built). The decimator change, if chosen, gets its own item in
  emeraude-base's `docs/todo/` at decision time.
- Log the ACHIEVED ratio (and warn when a level is not smaller than the previous one: it is useless memory).
- ⚠️ Owner settings note: the owner's own `settings.json` has the generation OFF.
