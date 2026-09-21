---
id: vegetation-octahedral-imposter-atlas
title: Vegetation — bake an octahedral imposter atlas for the last LOD
status: open
priority: unranked
scope: Graphics
tags: [vegetation, lod, offscreen]
opened: 2026-09-21
---

# Vegetation — bake an octahedral imposter atlas for the last LOD

## Why

The last LOD of a tree is a card, and a card is only convincing if what it shows was rendered from
the real mesh. emeraude-base can only produce the card **geometry**; baking the atlas needs a
renderer, so it belongs here.

## What remains

1. Render the LOD-0 tree from N x N directions on an octahedral parametrisation into an atlas:
   albedo, normal, depth/alpha. The engine already renders offscreen (see the
   `offscreen-rendering` demo in projet-alpha).
2. A shader that picks and blends the three nearest octahedral cells for the view direction.
3. Decide when the atlas is baked: offline into the data store, or once at load time. A load-time
   bake costs a visible hitch; a stored atlas costs disk and a pipeline step. **Owner decision.**

## Traps

- ⚠️ Bake at a **pinned exposure**. Baking through the auto-exposure burns whatever the camera
  happened to be metering into the atlas, and the imposter then never matches the mesh it
  replaces.
- ⚠️ The normals in the atlas are in view space of the baking direction. Write down which frame
  they are in, or the lighting on the far LOD will silently disagree with the near one.

## References

- The renderable side is DONE: `Scenes::Toolkit::generateTreeRenderable()` builds a
  `MultiLayerMeshResource` from a `TreeMesh`, and `TreeMesh::imposter()` is the card waiting
  for the atlas this item bakes. See `src/Graphics/AGENTS.md` § 15b and the projet-alpha
  `tree-generator` bench.
- Geometry side: DONE. `TreeMesh::imposter()` is the crossed-quads card, kept apart from the
  level chain because it carries ONE group: it samples the atlas this item bakes.
