---
id: rt-reflections-depth-discontinuity
title: RTR glossy cone — one hit distance per pixel over-blurs a near feature in front of a far one
status: open
priority: unranked
scope: Graphics/Effects/Framebuffer/RTR
opened: 2026-08-30
tags: [ray-tracing, reflections, measurement]
---

# RTR glossy cone — one hit distance per pixel over-blurs a near feature in front of a far one

## Why

Measured on projet-alpha's `post-processor-effect-debug` (option 3 = 1: an emissive band 6 m behind
the camera, the wall 11.5 m further): the rays reflected NEXT to the band hit the far wall and size
their cone from that distance, pulling the band's energy with a wide kernel — kernel +27 % at
roughness 0.2, +14 % at 0.3, +8 % at 0.4 against the flush-wall reference (v2 was +65 %).

## What remains

1. Decide whether it shows in content (Sponza: bright openings in front of far walls).
2. If it does: a min-hitT over a small neighbourhood of the cone width map (errs toward sharp, the
   perceptually safe side), or per-sample hit distances in the gather (Stachowiak 2015).

## ⚠️ Traps

- ⚠️ Any change here is judged on BOTH bench configurations (option 3 = 0 and 1): the flush one must
  not move.

## References

- `src/Graphics/AGENTS.md` § "RTR glossy cone".
- projet-alpha `src/Builtin/PostProcessorEffectDebug.AGENTS.md` § results v3.
