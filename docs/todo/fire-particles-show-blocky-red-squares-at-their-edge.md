---
id: fire-particles-show-blocky-red-squares-at-their-edge
title: A bright fire is surrounded by blocky red-orange square fragments
status: open
priority: high
scope: not located — candidates: the particles' sprite material (flipbook texture, its compression), the HDR path around a saturated source (bloom, the overflow census, the temporal resolve), the traced effects at half resolution
opened: 2026-11-16
tags: [defect, rendering, measured]
---

# A bright fire is surrounded by blocky red-orange square fragments

## Why

The testbed's `game-logic`, a night scene: the fire at the foot of the white statue is a saturated white-yellow core,
and its edge is lined with small axis-aligned SQUARES of red and orange, a few pixels wide each, instead of a soft
falloff. Seen on two machines on 2026-11-16, 20 s after the start, window-less:

- Linux (RTX 3070 Ti, Release, ray-traced lane, the validation layer loaded, 0 VUID): three captures of three builds
  of that day — before and after the day's changes — show the same squares.
- macOS (Apple M6, MoltenVK, screen-space lane): "its edges show blocky orange-red square fragments in both images,
  more visible in the first" (the peer's reading, round 14).
- Windows (RTX 3500 Ada) described "red particles" around the fire without calling them blocks.

The owner ranked it a defect. Nothing was measured beyond the pictures: which pass produces the squares is unknown.

## What remains

- Locate the pass: disable the post-process effects one at a time on the live scene
  (`Core.SceneManagerService.PostProcess.*`: `getStatus`, `disable`, `select`) and capture the fire each time; then the
  particles alone (their material, their flipbook texture's format) — a RenderDoc capture of the particles' draw and of
  the first pass that shows the squares.
- The squares are axis-aligned and of a few sizes: compare their size with a texel of the flipbook at that distance,
  with a block of its compressed format, and with a pixel of a half-resolution effect.
- Proof when fixed: the same capture, a soft edge, on Linux and one peer.

## ⚠️ Traps

- `game-logic` moves: the fire's shape differs at every capture, and the squares with it. Compare several captures.
- A capture's fire is found by its colour (red above 200, blue under 140 in the 8-bit picture), not by a fixed
  position: the camera view shifts between launches.

## References

- Graphics doc 13-12 § *The overflow census* (what FP16 does to the scene radiance around a saturated source).
