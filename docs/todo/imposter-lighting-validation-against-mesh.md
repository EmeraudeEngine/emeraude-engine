---
id: imposter-lighting-validation-against-mesh
title: Octahedral imposters — validate the imposter lighting against the mesh at the switch distance
status: open
priority: medium
scope: Graphics (imposter material mode, ImposterAtlas normals)
opened: 2026-09-21
tags: [vegetation, lod, imposter, validation]
---

# Octahedral imposters — validate the imposter lighting against the mesh

## Why

The atlas geometry and coverage are validated (graphics § 15e), but the imposter's decoded normals and its runtime
lighting were never compared side by side with the mesh they replace.

## What remains

- At the switch distance, same pose, pinned exposure, frozen wind (`Scene::setVegetationWind(dir, 0, 0)`): mesh and
  imposter captures, compared per pixel (mean luminance and per-channel distribution of the tree's pixels), for each of
  the 4 species, sun high and low.
- State the normal frame of the atlas (view space of the baking direction) in the doc if it is not already.

## References

- Graphics § 15e, "Traps of the imposter bake" (`docs/subsystems/graphics/24-15e-octahedral-imposters-the-last-rung-of-a-tree.md`).
