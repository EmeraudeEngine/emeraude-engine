---
id: imposter-per-view-parallax
title: Octahedral imposters — per-view parallax (depth in the atlas) against ghosting when the eye turns
status: open
priority: medium
scope: Graphics (ImposterAtlas, imposter bake, imposter material mode)
opened: 2026-09-21
tags: [vegetation, lod, imposter]
---

# Octahedral imposters — per-view parallax

## Why

The atlas stores no depth, so the three blended views are combined without per-view parallax: a mild ghosting when the
eye turns fast around a near imposter (graphics § 15e).

## What remains

- Store a depth (or height) channel in the bake and apply Brucks' ray–plane step per view before the three samples.
- Measure the cost (one more texture fetch per view) and the ghosting before/after on `terrain`.

## References

- R. Brucks, *Octahedral Impostors* (Unreal) — the per-view parallax step.
- Graphics § 15e (`docs/subsystems/graphics/24-15e-octahedral-imposters-the-last-rung-of-a-tree.md`).
