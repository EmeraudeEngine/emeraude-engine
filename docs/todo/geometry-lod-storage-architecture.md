---
id: geometry-lod-storage-architecture
title: Geometry LOD — the storage architecture is unresolved
status: in-progress
priority: medium
scope: Graphics/Geometry
opened: 2026-03-23
tags: [lod, resources]
---

# Geometry LOD — the storage architecture is unresolved

## Why

Automatic LOD generation (QEM decimation via `ShapeDecimator`) is wired through the rendering
pipeline — `RenderBatch` carries a `LODLevel`, `render()`/`castShadows()` propagate it,
`Geometry::Interface::geometryForLOD()` exists, `Scene::selectLODLevel()` selects on screen-space
coverage. The generation code was REVERTED out of `IndexedVertexResource` once, and has been LIVE at the
`Renderable` level since af93e0340 (`MeshResource.cpp` ~385, `MultiLayerMeshResource.cpp`). That is option 3 below,
in practice: each level is a full `IndexedVertexResource` (`MeshResource.cpp` ~501), and shared geometries get
their own LOD copies. The **storage** design itself was never accepted by the owner.

The constraint: LOD geometries should NOT be full `ResourceTrait` resources (no
resource-inside-a-resource), yet `IndexedVertexResource` inherits `ResourceTrait`, which forces LOD
variants into the resource lifecycle.

## What remains

- [ ] Owner: accept option 3 as it runs today, or choose another storage design. Options:
  1. a lightweight LOD variant class (VBO/IBO/subGeometries only, no `ResourceTrait`),
  2. bypass the resource lifecycle (create VBO/IBO directly, no `load()`/`setLoadSuccess()`),
  3. keep LODs at the `Renderable` level (LIVE today, but duplicates LODs for shared geometries, and each level is
     still a resource inside a resource).
- [ ] Multi-group decimation: `deduplicateVertices(false, false)` may corrupt group info — save
  groups BEFORE dedup; protect group boundary edges (penalty or per-group decimation).
  Per-group independent decimation is the most reliable approach for multi-material meshes.

## ⚠️ Traps

- The screen-space LOD selection reads the WORLD radius: renderable radius × the mesh's `uniformScale` × the
  entity frame's largest scale (`Scene.rendering.cpp` `worldRadius()`, 2026-09-28). Setting the SAME scale on
  the renderable AND on the entity — the former way to feed this radius — now doubles it.

## Why it matters now

`JungleRuins` (the gold-goal scene) names this hole out loud, together with texture streaming.
