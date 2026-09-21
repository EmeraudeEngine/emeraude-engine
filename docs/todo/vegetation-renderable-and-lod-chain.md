---
id: vegetation-renderable-and-lod-chain
title: Vegetation — a renderable resource for a generated tree, with its LOD chain
status: blocked
priority: unranked
scope: Graphics/Renderable, Graphics/Geometry
blocked-by: []
tags: [vegetation, geometry, lod]
opened: 2026-09-21
---

# Vegetation — a renderable resource for a generated tree, with its LOD chain

## Why

emeraude-base will produce a tree as a `Shape` carrying **two groups** (bark, leaves) plus a
skeleton and a per-LOD variant list. The engine side has every piece already, but nothing wires
them together:

- `Geometry::Interface::buildSubGeometries(subGeometries, shape)` already turns `Shape` groups
  into sub-geometries.
- `Renderable::MultiLayerMeshResource::load(geometry, materialList, rasterizationOptions)` already
  takes one material per layer, and already holds a `StaticVector< …, MaxLODLevels >` of
  geometries.

What is missing is the entry point that takes a generated tree and returns a ready
`MultiLayerMeshResource`: bark material on layer 0, alpha-masked **double-sided** leaf material on
layer 1, and the base-generated LOD meshes filed as LOD levels rather than decimated here.

## What remains

1. Decide where it lives: a `Scenes::Toolkit` helper (matches how demos build things today,
   `generateRenderableInstance< StaticEntity >(name, shape, material)`) or a
   `Graphics::Geometry::ResourceGenerator` entry. Pick one, do not add both.
2. Feed the LOD levels from the generator's own chain — do **not** call `ShapeDecimator` on
   foliage, QEM cannot merge leaf cards into bigger cards.
3. The leaf layer needs alpha-mask rasterization options and two-sided lighting; the bark layer is
   plain opaque.
4. The vertex colour channels the generator fills (R/G/B trunk-bend/branch-bend/flutter, A = AO)
   must actually reach the vertex buffer — check the attribute presence path, cf. the open item
   `vertex-attribute-presence-belongs-to-geometry`.

## Traps

- ⚠️ Foliage already has a known lighting defect worth reading first:
  `foliage-takes-most-of-its-light-from-the-reflection`.
- ⚠️ Alpha-masked leaf textures lose their coverage in the mip chain — see
  `alpha-mask-textures-lose-coverage-and-colour-in-the-mip-chain`. A leaf canopy is exactly the
  content that exposes it.

## References

- Producer side: emeraude-base `docs/todo/tree-generator-skinning-lod-and-wind-channels`.
- `src/Graphics/Geometry/Interface.hpp:534` — `buildSubGeometries(…, Shape)`.
- `src/Graphics/Renderable/MultiLayerMeshResource.hpp:276` — the multi-material load.
