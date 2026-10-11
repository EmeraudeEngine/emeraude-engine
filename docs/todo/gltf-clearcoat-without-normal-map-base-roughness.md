---
id: gltf-clearcoat-without-normal-map-base-roughness
title: A clear coat WITHOUT a normal map samples the environment at the BASE roughness
status: open
priority: high
scope: Saphir/LightGenerator (clear coat, ambient / IBL), Graphics/Material
opened: 2026-09-14
tags: [gltf, material, clearcoat, defect]
---

# A clear coat WITHOUT a normal map samples the environment at the BASE roughness

## Why

A coat is smoother than what it covers, so its environment lookup must use the COAT roughness. The 2026-09-14 fix did
that only for coats with their own normal map, and the rows without one were scoped out ON PURPOSE: they stayed
bit-exact as the control of that fix (`ClearCoatTest`, `Simple coating`).

## What remains

- Sample the environment at the coat roughness for a coat without a normal map too.
- Fixing it moves `ClearCoatTest`'s `Simple coating` and `Roughness variations` rows, so it needs its own measured
  before/after at a pinned exposure (bench method:
  `docs/subsystems/scenes-loaders/07-gltf-conformance-bench-results.md`).

## References

- Split out of the former item `gltf-conformance-loader-gaps` (2026-10-11).
