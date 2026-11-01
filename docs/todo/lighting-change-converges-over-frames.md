---
id: lighting-change-converges-over-frames
title: A global lighting change converges over a FRAME count — seconds on a slow GPU
status: open
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

Two causes, measured, not yet attributed in detail:

1. **The temporal filters count in frames**: the GI history (`Denoiser/MaxAccumulation` 64), the anti-lag's fast window
   (`Denoiser/AntiLagFastFrames` 6), and TAA. Graphics doc 42 notes that TAA smears a sudden global change over ~1 s
   at 60 FPS, which is ~2.4 s at 25 FPS. A frame count is a different wall time on every GPU.
2. **On the M6 the INPUT of the GI is itself slow**: with `IndirectDiffuse` disabled, the land still reads
   95 / 26.7 / 16.4 at +1 / 2 / 3 s at 25 FPS (macOS-PA; the validation layer does not change it). On Linux at ~42 FPS
   it is final from +1 s. A slowly changing input never trips the anti-lag: its cap does not engage there (the age
   view slides 206 → 142 over 2.4 s, where Linux drops to ~80 within 0.1 s and holds).

## What remains

- Attribute the M6's slow input (macOS-PA): TAA off; the lighting family off (`setLightingMode(None)`); the IBL alone.
  Which frame-counted filter carries the 3 s?
- Owner decision, once attributed: keep frame counts, or express the windows in SECONDS. A per-frame blend weight
  alpha = 1 − exp(−dt / τ) makes a slow GPU converge in the same wall time; it costs the noise reduction a slow GPU
  gets from its longer history. This applies to the GI history, the anti-lag window, and maybe TAA's feedback, each
  measured on its own.
- The Intel iGPU of Windows-PA ran `reflexion-debug` and `labyrinth` at 22-24 FPS on 2026-11-01, against ~47 FPS
  logged in October (not investigated). If a frame-rate regression is confirmed, it is its own item.

## ⚠️ Traps

- Compare at a PINNED exposure, and give the FPS of every run (`Core.RendererService.getStatus`): a wall-clock curve
  means nothing without it.
- At 2560×1440, `temporalCapture` refuses more than 72 frames (1 GiB staging).

## References

- Graphics doc 13-12 § 16 (the GI denoiser, stage 5, the anti-lag and its history); graphics doc 42 § the lag behind the
  sky (the IBL cycle, TAA's smear).
