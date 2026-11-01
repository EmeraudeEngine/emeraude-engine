---
id: lighting-change-converges-over-frames
title: A global lighting change converges over a FRAME count — seconds on a slow GPU
status: in-progress
priority: unranked
scope: Graphics/PostProcessing (GI denoiser history and anti-lag window, TAA, SSGI feedback), the sky's IBL
opened: 2026-11-01
tags: [gi, temporal, taa, performance]
---

# A global lighting change converges over a FRAME count — seconds on a slow GPU

## Why

The pooled GI anti-lag (engine e2a7320e, graphics doc 13-12 § 16, stage 5) makes a `city` sunset jump (time of day
0.55 → 0.95) converge in about 1 s on an RTX at 40-100 FPS. That replaced 8-10 s. On a slow GPU it still takes
seconds, with no energy loss (verified on the three OS and AMD, 2026-11-01):
- macOS M6 (SSGI, 2560×1440, 25 FPS): the land 187 / 88 / 37.8 / 21.6 at +1 / 2 / 3 / 4 s;
- AMD RADV Renoir (SSGI, 17-22 FPS): 208 / 135 / 73 / 40 / 25 at +1 … 5 s, about 110 frames to fall under 25.

Two causes, measured:

1. **The temporal filters count in frames**: the GI history (`Denoiser/MaxAccumulation` 64), the anti-lag's fast window
   (`Denoiser/AntiLagFastFrames` 6), and TAA. Graphics doc 42 notes that TAA smears a sudden global change over ~1 s
   at 60 FPS, which is ~2.4 s at 25 FPS. A frame count is a different wall time on every GPU.
2. **On the M6, the slow part is the TAA resolve, not the GI** (macOS-PA round 3, 2026-11-01, engine dd77f62b, M6
   2560×1440, one launch, default settings, layer on, 0 VUID; exposure pinned f/8 · 1/125 · ISO 25600; the land region's
   mean in the 8-bit capture after a 0.55 → 0.95 jump). The variants switch one more thing off each time:

   | Variant | FPS | +0.5 s | +1 s | +2 s | +3 s | +6 s |
   |---|---|---|---|---|---|---|
   | V0: `IndirectDiffuse` off | 35 | 179.9 | 84.6 | 25.4 | 16.0 | 13.6 |
   | V1: + `TemporalAA` off | 37 | 13.5 | 13.5 | 13.5 | 13.5 | 13.5 |
   | V2: + `setLightingMode(None)` | 45 | 0.04 (one black frame, below) | 13.5 | 13.5 | 13.5 | 13.5 |
   | V3: + `bypassSceneEffects(1)` | 46 | 13.6 | 13.6 | 13.6 | 13.6 | 13.6 |
   | V4: bypass off, TAA alone back on (lighting family off) | 43 | 154.8 | 54.7 | 18.6 | 14.5 | 13.7 |

   Without TAA the input lands within two frames, ~85 ms after the command (V3 `temporalCapture(72)`: 236 for frames 0-4,
   50.8 at frame 5, 13.5 from frame 6). That is one step: there is no IBL cycle and no blend upstream of the
   post-process. The mechanism is the one graphics doc 13-12 § "The TAA resolve" foresaw: the history is clipped only
   where the DEPTH changed, so a shading change on unchanged geometry fades at the stationary alpha
   (`TemporalAA/Alpha` 0.1 × 0.5 = 0.05, ~20 frames per e-fold). The Karis weights only speed up the bright → dark
   direction. On the RTX at 100+ FPS that is ~0.2 s; on the M6 at 35-45 FPS it is about 3 s to settle.
   Round 2 had read the M6's age view (GI `DebugView` 2) sliding 206 → 142 over 2.4 s, where Linux drops to ~80 within
   0.1 s and holds. It was taken with TAA ON, and the lighting family runs before TAA in the chain (graphics doc 13-12
   § 33), so that view was smeared too. Whether the anti-lag's cap engages on a slow GPU must be re-read with
   `TemporalAA` off.

## What remains

- **TAA: done** (engine 3c25c355, validated on the three OS and AMD, 2026-11-01). The owner chose the graphics
  doc's own note: a colour test reserved to LARGE changes (5×5 pooled exposed luminance, variance clip ramped,
  `TemporalAA/LargeChangeThreshold` 0.25), not an alpha in seconds. With GI off the sunset is final from +0.2-0.5 s
  on every machine, the M6 and the AMD laptop included. Graphics doc 13-12 § "The TAA resolve" § The LARGE-CHANGE test
  holds the measurements and the peers' verdicts.
- **The GI windows: fixed in the working tree (2026-11-02), awaiting the commit and the peers.** The owner chose a RESET
  on a very large step (`Denoiser/AntiLagResetRatio` 4), not windows in seconds: graphics doc 13-12 § 16 § The RESET
  holds the mechanism and the measurements (25 FPS: final within 2-3 frames; no false reset, level and noise unchanged
  on `labyrinth`, `sponza`, `city`). Then the item closes, its knowledge already in the graphics docs. Attributed on 2026-11-02 (Linux, RTX 3070 Ti, `city` quay, TAA OFF,
  exposure pinned f/8 · 1/125 · ISO 25600, the land's 8-bit mean after 0.55 → 0.95, layer on, 0 VUID):

  | Run | RTGI +1 / 2 / 3 s | SSGI +1 / 2 / 3 s |
  |---|---|---|
  | 25 FPS, defaults | 104 / 33 / 18 | 75 / 26 / 17 |
  | 60 FPS, defaults | 22 / 15.3 / 15.1 | 20 / 14.3 / 13.9 |
  | 25 FPS, `IrradianceProbes/Enabled` false | 104 / 33 / 18 (the land is outside the 24 m box) | 75 / 26 / 17 |
  | 25 FPS, `MaxAccumulation` 16 | 102 / 29 / 18 | 71 / 22 / 16 |
  | 25 FPS, `AntiLagSigma` 1.0 | 73 / 21 / 16 | 55 / 19 / 15 |
  | 25 FPS, `AntiLagFastFrames` 1 | **15.6** / 15.3 (final from +0.5 s) | **13.9** / 13.8 |

  The age view (`Denoiser/DebugView` 2, TAA off, 25 FPS) drops to the capped level within 100 ms and holds it during the
  whole change: the anti-lag's cap DOES engage on a slow GPU. What remains is the cap's own length. A day → night step
  divides the land's light by ~1000; at a 6-frame cap the history blends ~1/7 a frame, and 1000× takes ~7 e-folds:
  ~45 frames, 0.75 s at 60 FPS and 1.8 s at 25 FPS. Expressing the windows in seconds would make that the same wall
  time on every GPU (~0.75 s), not shorter. A large step needs the history discarded, not shortened.
- The near-black frames right after a jump were a separate cause, fixed on 2026-11-01. The IBL cycle now runs in
  one frame on a jump, and `city` checks its lamps every logic cycle (graphics doc 42 § "The environment", item 7).
- The Intel iGPU of Windows-PA ran `reflexion-debug` and `labyrinth` at 22-24 FPS on 2026-11-01, against ~47 FPS
  logged in October (not investigated). If a frame-rate regression is confirmed, it is its own item.

## ⚠️ Traps

- Compare at a PINNED exposure, and give the FPS of every run (`Core.RendererService.getStatus`): a wall-clock curve
  means nothing without it.
- At 2560×1440, `temporalCapture` refuses more than 72 frames (1 GiB staging).

## References

- Graphics doc 13-12 § 16 (the GI denoiser, stage 5, the anti-lag and its history); graphics doc 42 § the lag behind the
  sky (the IBL cycle, TAA's smear).
