---
id: gltf-bench-animated-capture
title: The glTF conformance bench captures every asset at rest only — an animated skin defect goes unseen
status: open
priority: unranked
scope: tools/gltf-conformance-bench (bench.py, README)
opened: 2026-10-01
tags: [gltf, conformance, skinning, animation, tooling]
---

# The glTF conformance bench captures every asset at rest only — an animated skin defect goes unseen

## Why

The bench calls `Core.resetAnimation()` before each capture, on purpose: a stray space bar once moved CesiumMan's A/B
from 0.10/255 to 9.22 (README). As a result, a defect that only shows while a clip plays is invisible to it.

BrainStem fell flat for months, from its first animated frame (fixed 2026-10-01: a skinned joint's non-joint
ancestors were lost, `docs/subsystems/scenes-loaders/03-implemented-loaders/01-gltfloader.md` § Skins). Its rest pose
was right all along.

## What remains

- [ ] Add a deterministic animated capture: a console command that evaluates the viewer's clip at a GIVEN time and
  freezes it, for example `Core.evaluateAnimation(clipIndex, seconds)`, so the frame does not depend on timing.
  `cycleAnimation()` plus a delay is not reproducible.
- [ ] Capture the skinned set (CesiumMan, Fox, BrainStem, RiggedFigure, SimpleSkin) at a fixed time of their first
  clip, and A/B it like the rest captures.
- [ ] Check an upright figure in BrainStem's animated capture: compare the bounding box height to the width, or keep a
  reference image.

## References

- `tools/gltf-conformance-bench/bench.py` (the `Core.resetAnimation()` call), `README.md` (the space-bar trap).
