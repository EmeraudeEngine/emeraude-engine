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
coverage — but the **storage** of the generated LOD geometries has no accepted design, and the
generation code was REVERTED out of `IndexedVertexResource`.

The constraint: LOD geometries should NOT be full `ResourceTrait` resources (no
resource-inside-a-resource), yet `IndexedVertexResource` inherits `ResourceTrait`, which forces LOD
variants into the resource lifecycle.

## What remains

- [ ] Choose the storage design. Options to explore:
  1. a lightweight LOD variant class (VBO/IBO/subGeometries only, no `ResourceTrait`),
  2. bypass the resource lifecycle (create VBO/IBO directly, no `load()`/`setLoadSuccess()`),
  3. keep LODs at the `Renderable` level (worked, but duplicates LODs for shared geometries).
- [ ] Multi-group decimation: `deduplicateVertices(false, false)` may corrupt group info — save
  groups BEFORE dedup; protect group boundary edges (penalty or per-group decimation).
  Per-group independent decimation is the most reliable approach for multi-material meshes.

## ⚠️ Traps

- `uniformScale` on `Renderable` MUST be set, or the screen-space LOD selection is wrong.

## Why it matters now

`JungleRuins` (the gold-goal scene) names this hole out loud, together with texture streaming.
