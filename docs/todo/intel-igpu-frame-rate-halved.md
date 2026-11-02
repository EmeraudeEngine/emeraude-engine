---
id: intel-igpu-frame-rate-halved
title: The Windows Intel iGPU's frame time since October — not halved, a few attributable costs
status: in-progress
priority: medium
scope: Graphics (SSGI, ScenePass, the GI anti-lag, TAA) on the Intel Raptor Lake iGPU
opened: 2026-11-02
tags: [performance, intel, windows]
---

# The Windows Intel iGPU's frame time since October — not halved, a few attributable costs

## Why

Opened on a false premise: "~47 FPS in October, 20 today" on `reflexion-debug`. The ~47 FPS were `sky-lab` captures
attributed to `reflexion-debug` (the night of the DST change, a local hour matched twice). Windows-PA built the Oct 25
tree (projet-alpha 7e2a41dd, engine 29077468, base f00c579) and ran it on today's machine and driver (Intel
32.0.101.7088, unchanged since October): `reflexion-debug` **21.7 FPS (43.2 ms GPU) then, 19.8 FPS (47.2 ms) now: +9 %,
not ×2**. `sky-lab` ~45 FPS then and now.

The bisect of the +4 ms (Intel, 1280×720, layer off, 3 launches per point, medians of `getGPUTimings`):

| Cost | Where | Intel | RTX 3070 Ti |
|---|---|---|---|
| SSGI +0.73 ms | engine e64e67da (the matrix view reconstruction at every march step) | 13.35 → 14.08 | +0.02 |
| SSGI +0.41 ms | engine f85402e1 (the light-face test: one material-properties fetch per hit) | 14.21 → 14.62 | +0.075 |
| SSGI +0.8 ms | after 95fe78cd (14.37) up to today (15.17): engine or projet-alpha, not yet separated | — | ~+0.06 |
| ScenePass +0.9 ms | between f85402e1 (8.44) and 95fe78cd (9.35) | — | — |
| GI anti-lag +0.84 ms | the 5×5 pooled detector (e2a7320e) | 0.38 → 1.22 | 0.08 → 0.14 |
| TAA +0.93 ms | the large-change test (3c25c355) | 4.57 → 5.50 | 0.20 → 0.30 |

**Fixed (2026-11-02, working tree): e64e67da's cost**, by the view block's FAST forms built as two pipeline variants
(graphics doc 13-12 § 41, The FAST forms): SSGI 15.17 → 14.55 ms on the Intel.

## What remains

- Separate the SSGI +0.8 ms after 95fe78cd: build engine af4eb78b with the October projet-alpha (+ the `Paladin.cpp`
  initializer patch) in Windows-PA's `.claude-build-oct`, then bisect the engine half if it shows there.
- Bisect the ScenePass +0.9 ms between f85402e1 and 95fe78cd.
- Owner decisions on whatever they find; the anti-lag and TAA costs are known features (record, or optimise).

## ⚠️ Traps

- A capture's unix-second stem is UTC: on a DST night a local hour happens twice. Match a capture to its run by its
  metadata (camera, exposure), never by the hour alone.
- The October projet-alpha builds against later engines with one patch only up to today's head: the `Paladin.cpp`
  `nonEmissiveMaterialNames` initializer (engine 861a7d64); later bases are needed after ~6f2a930f (base e31c56b).
- A per-call uniform branch in a march loop cost the Intel +0.85 ms (graphics doc 13-12 § 41).

## References

- Graphics doc 13-12 § 41 (the view reconstruction and its FAST forms), § 16 (the anti-lag), § The TAA resolve.
