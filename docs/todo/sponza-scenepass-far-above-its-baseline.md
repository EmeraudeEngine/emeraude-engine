---
id: sponza-scenepass-far-above-its-baseline
title: Sponza's ScenePass is far above its 2026-10-04 baseline (12.8 M triangles at "LOD 3")
status: open
priority: unranked
scope: Graphics (geometry LOD, scene pass), projet-alpha sponza demo
opened: 2026-10-06
tags: [performance, lod, regression]
---

# Sponza's ScenePass is far above its 2026-10-04 baseline (12.8 M triangles at "LOD 3")

## Why

Measured 2026-10-06 by Linux-PA (RTX 3070 Ti, 2880×1620, `--load-demo=sponza`, launch pose ~10 s after the load, scene
effects bypassed, a settings COPY of the owner's): `ScenePass` 34-40 ms with the deferred resolve and **305 ms**
forward-only (`setDeferredPunctualLights(0)`), validation ON or OFF alike. `docs/subsystems/graphics/38-deferred-punctual-lights.md`
recorded 13.8 ms / 45.5 ms on 2026-10-04 (then 10.2 ms after the cypress cutout). `getRenderStatistics()` reported
454 batches, **12 833 689 triangles, all at "LOD 3"**.

NOT caused by the frame light selection or the tiled culling: the forward-only path does not run that code, the
resolve costs 0.77-1.3 ms, and the culling on/off frames are bit-identical. The cause is NOT investigated.

## What remains

- Bisect between engine `8c461d12` (2026-10-04, the baseline) and `develop` at the same pose and settings; compare
  `getRenderStatistics()` (a coarsest LOD holding 12.8 M triangles looks wrong: memory notes the ivy's L1 = L2 = L3 =
  2 134 434 triangles — the LOD simplifier stalls on disconnected cards, item `lod-simplifier-stalls-on-disconnected-cards`).
- Check whether the owner's settings (LOD generation, `ReleaseLocalData`) changed since the baseline.

## References

- `docs/subsystems/graphics/38-deferred-punctual-lights.md` § Measured.
