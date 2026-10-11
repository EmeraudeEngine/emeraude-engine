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

- Find why the extents are not published in time for this asset (external textures delaying the content-ready
  signal?) and wait for them instead of falling back, or publish them from the geometry alone.
- Make the bench report a fallback framing, so it never goes unnoticed again.

## References

- `docs/subsystems/scenes-loaders/07-gltf-conformance-bench-results.md` § Traps; split out of the former item
  `gltf-conformance-loader-gaps` (2026-10-11).
