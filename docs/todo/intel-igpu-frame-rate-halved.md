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
| GI anti-lag +0.83 ms | the 5×5 pooled detector (e2a7320e) — `GIDenoiser/antiLag` is a CHILD of `SSGIEffect/internal`: it is the whole "SSGI +0.8 after 95fe78cd" (engine-side; projet-alpha weighs ≤ 0.1 ms) | 0.378 → 1.212 | 0.08 → 0.14 |
| ScenePass +0.77 ms | engine 33df4a4a, the direct lights' geometric specular antialiasing (3e70d6a4 8.47 → 33df4a4a 9.24; 29a923b8, the IBL and clear coat, +0.04) | 8.47 → 9.24 | — |
| TAA +0.92 ms | the large-change test (3c25c355) | 4.57 → 5.49 | 0.20 → 0.30 |

**Fixed (2026-11-02, working tree): e64e67da's cost**, by the view block's FAST forms built as two pipeline variants
(graphics doc 13-12 § 41, The FAST forms): SSGI 15.17 → 14.55 ms on the Intel.

## What remains

- The bisect is complete (2026-11-02, rounds 1-4): ~3.7 ms of the +4 ms attributed above. The owner chose to attack the
  three feature costs. The GI anti-lag: a cheaper sampling regressed the shimmer, not adopted (graphics doc 13-12 § 16).
  Left: the direct lights' specular antialiasing (+0.77: `dFdx`/`dFdy` of the normal at the top of EVERY light pass,
  while the G-buffer pass already computes the same filtered roughness — NOT the derivatives paid by fragments discarded
  afterwards: the block moved after the reach discards measured ScenePass 9.23 → 9.20 ms, noise, Windows-PA 2026-11-02;
  `reflexion-debug` has one light pass, so +0.77 ms is far above the block's ALU. Splits on the Intel, Windows-PA
  2026-11-02, ScenePass medians of 3 launches, A = 9.22-9.28 ms: the derivatives on the interpolated geometric normal
  instead of the normal map 9.18, coarse derivatives 9.27 — no change; the light pass's block STUBBED (raw roughness)
  8.41 / 8.46 in two alternated pairs, −0.8 ms = the pre-33df4a4a 8.47. The cost is the block's presence, not its
  placement, input or derivative kind. The compiler's own statistics (`VK_KHR_pipeline_executable_properties`, a
  throwaway patch, Intel 32.0.101.7088: instruction count, basic blocks, scratch, loops per SIMD8/16/32 executable;
  deterministic, cache on or off) REFUTE a codegen cause on the pipeline `reflexion-debug` draws (its only lights are
  sky-derived directionals: `DirectionalLightPassCSM`): same SIMD widths, no spill, 1.5 % fewer instructions without the
  block (1369 → 1352 SIMD8). Round 5 split the block: the DERIVATIVES alone (raw roughness kept, `dFdx`/`dFdy` alive
  through a 1e-20 term) 9.19 ms = A; the filter's arithmetic alone (the derivatives replaced by scaled normals) 8.48 ms
  = the stub's 8.45 (A 9.22 / 9.28 around them). The whole ~0.8 ms is the explicit derivatives of a COMPUTED value
  (any form: fine, coarse, of the normal map or of the interpolated TBN axis), invisible in the static statistics
  (same widths, no spill, ±3 instructions). The directional pass has no `discard`, so it is not a helper-lane-after-
  discard effect; the exact hardware mechanism is not identified (no vendor tool used). Not seen on the RTX.
  **Owner decision 2026-11-02: the cost is ACCEPTED** (recorded in `docs/caution-points.md` § the direct specular's
  geometric antialiasing))
- Found on the way (2026-11-02, same statistics): on the POINT and SPOT light passes the block costs real codegen on the
  Intel — the mid-size variants lose their SIMD32 executable (`PointLightPass` 294/328 instructions SIMD8/16, no SIMD32;
  without the block 277/311/553), and the `Full` / `ShadowMap` SIMD16 variants spill 448-1216 bytes of scratch (16
  spilling executables with the block, 6 without). Not timed: `reflexion-debug` draws no point or spot light
  (every light-pass variant is built per renderable, `Scene::prepareRenderPassTypes()`); a lamp scene (city) would.
- TAA's large-change test (+0.92 on the Intel, +0.10 on the RTX; already 9 + 9 bilinear taps: only a reduced-resolution pre-pass would cut it).
  The owner chose to run it only on the frames a discontinuity marks: item `temporal-discontinuity-contract` (the
  block compiled out measured −0.99 ms against today's default on the Intel, 2026-11-03; the GI anti-lag's +0.83 goes
  the same way).

## ⚠️ Traps

- A capture's unix-second stem is UTC: on a DST night a local hour happens twice. Match a capture to its run by its
  metadata (camera, exposure), never by the hour alone.
- The October projet-alpha builds against later engines with one patch only up to today's head: the `Paladin.cpp`
  `nonEmissiveMaterialNames` initializer (engine 861a7d64); later bases are needed after ~6f2a930f (base e31c56b).
- A per-call uniform branch in a march loop cost the Intel +0.85 ms (graphics doc 13-12 § 41).

## References

- Graphics doc 13-12 § 41 (the view reconstruction and its FAST forms), § 16 (the anti-lag), § The TAA resolve.
