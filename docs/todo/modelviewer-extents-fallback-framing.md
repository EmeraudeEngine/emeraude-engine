---
id: modelviewer-extents-fallback-framing
title: ModelViewer misses the extents of TextureTransformTest and silently falls back to a default framing
status: open
priority: high
scope: Scenes (ModelViewer / +ModelViewer framing), tools/gltf-conformance-bench
opened: 2026-09-14
tags: [gltf, measurement, defect]
---

# ModelViewer misses the extents and falls back to a default framing

## Why

`TextureTransformTest` (a `.gltf` with external textures) logs *"The imported content published no extents in time,
using the fallback framing"* on every attempt: reproducible, not a fluke. The capture stays usable, but its framing is
not the computed one, and the warning appears only in the engine log. A capture whose framing silently differs is a
measurement hazard.

## What remains

- **Cause found 2026-11-17** (after the viewer's wait was repaired, item
  `model-viewer-always-times-out-waiting-for-the-model-extents`): not the textures. Its nine meshes are FLAT quads
  (`POSITION` min / max `[-0.5, -0.5, 0]` / `[0.5, 0.5, 0]`) and a box without volume is not valid
  (`AACuboid::isValid()` asks max > min on EVERY axis), so no entity has a render box and the viewer waits its whole
  budget: "0 of 9 mesh(es) not ready after 5000 ms". Every flat content is concerned (a plane, a decal, a card).
- Decide (owner) what a flat extent is: a valid box of zero thickness in the foundation, a minimal thickness given
  by whoever builds the box, or a viewer that frames on the positions' range. The first changes a foundation
  contract every culling test relies on - measure before choosing.
- Make the bench report a fallback framing, so it never goes unnoticed again.

## References

- `docs/subsystems/scenes-loaders/07-gltf-conformance-bench-results.md` § Traps; split out of the former item
  `gltf-conformance-loader-gaps` (2026-10-11).
