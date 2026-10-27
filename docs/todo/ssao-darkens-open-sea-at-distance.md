---
id: ssao-darkens-open-sea-at-distance
title: SSAO darkens the open sea, more with distance, and draws a screen-fixed diagonal step and row streaks under the horizon
status: open
priority: high
scope: Graphics/Effects/Lighting/SSAO
opened: 2026-10-27
blocked-by: []
tags: [ssao, depth, precision, screen-space]
---

# SSAO darkens the open sea, more with distance, and draws a screen-fixed diagonal step and row streaks under the horizon

## Why

A defect (a wrong image), found by the macOS peer on projet-alpha's `water-world` (2026-10-27) and confirmed on Linux.
The ray-traced lane (RTAO) does not show it: the screen-space lane only.

- **macOS M6** (screen-space only), slot sweep at a 400 m pose, one post-process slot off at a time: ONLY
  `AmbientOcclusion` removes a band ~60 rows tall under the horizon with horizontal row streaks and a straight DIAGONAL
  step at a fixed screen x (≈ 1850-1900 / 2560, identical at three poses of the same yaw). Left of it the far sea is
  ~11 luma darker (ref Δ 12.3 → SSAO off Δ 1.6; row-streak index 1.13 → 0.34). SSR, TAA, VolumetricLight, the others:
  no change. Present at a 4000 m view distance already (before and after engine 33df4a4a / 29a923b8).
- **Linux** (RTX 3070 Ti, lane forced with `PostProcess.setLightingMode("ScreenSpace")`, exposure pinned f/11 1/250 ISO
  100, 400 m up looking at the open sea): mean luma of the far-sea band 46.0 (view distance 25 km) / 55.4 (4 km) against
  68.8 with the AO slot off; the near sea 32.9 / 35.4 against 43.1. The SSAO darkens the WHOLE open sea by 20-30 %, and
  the far band more at 25 km. A wavy sea is nearly a plane: it should take little occlusion. The diagonal was not
  isolated at 3840 × 2057 (not looked for at the macOS screen x).
- **Windows, Intel RaptorLake-S iGPU only** (screen-space lane): STATIC horizontal stripes on the sea — per-row mean minus a
  9-row moving average, RMS 2.4-2.8/255, **period 13 px**, autocorrelation 0.85-0.93, visible by eye; only
  `disable(AmbientOcclusion)` (or `bypassSceneEffects(1)`) removes them (RMS 2.48 → 1.02, aperiodic). **Independent of
  the far plane**: 2.39 / 2.70 / 2.97 at 25 000 / 4 000 / 1 500 m. Present on the former 256 m island too. The same
  pass on NVIDIA in the screen-space lane shows no 13-px periodicity.

## What remains

- Attribute before fixing (each a hypothesis, none proven):
  1. the constant view-space `bias` (0.025 m) and `radius` (0.5 m) against the reconstructed depth's quantization —
     conventional D32 depth (engine item `reversed-z-depth`): far away one depth step is metres, the samples read
     "behind" and the surface occludes itself; a bias scaled with the view depth (or with the depth's own ulp) and a
     distance fade are the usual answers;
  2. the per-pixel rotation from `hash(vUV)` (screen-space UV): a precision-degrading hash draws screen-FIXED patterns —
     a fit for the diagonal at a fixed screen x and the row streaks, and the strongest suspect for Intel's periodic
     stripes, which do NOT depend on the far plane: it IS the classic
     `fract(sin(dot(p, vec2(12.9898, 78.233))) * 43758.5453)`, whose `sin` precision is vendor-dependent; an integer hash
     (PCG, Jarzynski & Olano 2020) or interleaved gradient noise is vendor-independent;
  3. the waves' real relief (FFT displacement + normal map) at grazing angles — legitimate occlusion, but bounded by
     the radius; measure it against RTAO on the same frame.
- Measure on a flat plane at 100 m … 20 km (no waves) to separate 1-2 from 3.
- The fix with its A/B: same frame, SSAO vs RTAO vs off, 0 `VUID-`, the three OS.

## ⚠️ Traps

- Pin the exposure: the metering moves with the AO's darkening otherwise.
- `PostProcess.disable(AmbientOcclusion)` stays off across lane switches: `select(AmbientOcclusion, SSAOEffect)` brings it
  back.
- A LIVE `Camera.setViewDistance()` keeps the cascade scale of the load: compare AO, not shadows, across distances.

## References

- engine `src/Graphics/Effects/Lighting/SSAO.cpp` (the kernel: `bias`, `radius`, `rangeCheck`, `hash(vUV)`);
  `SettingKeys.hpp` § AmbientOcclusion/ScreenSpace.
- projet-alpha `docs/subsystems/builtin/16-14-waterworld-scene.md` § The sea to the horizon.
- The macOS peer's captures (2026-10-27, M6 scratchpad): `sw-ref.png`, `sw-noAmbientOcclusion.png`, `sw-montage1/2.png`.
