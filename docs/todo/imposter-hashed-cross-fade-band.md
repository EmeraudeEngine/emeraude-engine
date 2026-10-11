---
id: imposter-hashed-cross-fade-band
title: Octahedral imposters — a hashed cross-fade band instead of the hard switch at 250 m
status: open
priority: medium
scope: Graphics (StandardResource imposter mode, mesh materials, RenderableInstance draw-distance range)
opened: 2026-09-21
tags: [vegetation, lod, imposter]
---

# Octahedral imposters — a hashed cross-fade band instead of the hard switch

## Why

Owner decision 8 (2026-09-23, graphics § 15e): the mesh → imposter switch happens at a distance in metres with a HASHED
cross-fade band (reusing the hashed alpha test), not a hard cut. Today the switch at 250 m is a hard cut per cell.

## What remains

- A per-draw fade factor, computed from the cell distance and the band, read by BOTH the mesh materials and the
  imposter (hashed discard, complementary on each side).
- Proof: no popping across the band on `terrain` in motion, pinned exposure, frozen wind; 0 VUID.

## References

- Graphics § 15e (`docs/subsystems/graphics/24-15e-octahedral-imposters-the-last-rung-of-a-tree.md`).
