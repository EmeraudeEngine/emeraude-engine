---
id: gi-history-keeps-a-switched-off-emitter-lit
title: GI temporal history lags every lighting change — a switched-off emitter stays lit, the sky's IBL arrives late
status: in-progress
priority: high
scope: Graphics/PostProcessing (SSGI, GI denoiser history)
opened: 2026-10-06
tags: [gi, temporal]
---

# GI temporal history keeps a switched-off emitter lit for many frames

## Why

macOS-PA (Apple M2, MoltenVK, 2560×1440, validation ON, SSGI lane, ~6.7 FPS), `labyrinth` at the test pose
(`Act.setPosition(-6, 0, 67.3)` + `Act.lookAt(-6, 3.9, 66)`), 2026-10-06: the 55 flickering lamps switched off, their
linked panels' emission drops to 0 at once (`AbstractLightEmitter::linkEmissiveMaterial()`), yet the panel interior
reads 230 at +5 s, 167 at 10 s, 80 at 15 s, 27 at 20 s, 10 at 25 s, 4.7 at 30 s, 1.4 at 40 s (ceiling 3.1). With
`PostProcess.disable("IndirectDiffuse")` the same switch reads 0.0 at +2 s. The indirect-diffuse history feeds the
panel's old glow back onto its own pixels and decays PER FRAME, so the fade lasts ~30 s at 6.7 FPS. Switching on is
instant. Linux (RTX 3070 Ti, RT lane, ~28 FPS) read the panel at the ceiling level 1.5 s after the switch.

**The same lag under the dynamic sky's IBL** (owner: a defect, 2026-10-25). macOS-PA (Apple M6, MoltenVK, SSGI lane,
~76 FPS), `sky-lab`, exposure pinned (f/8, 1/125, ISO 800), the floor under the sky alone:
- darkening 0.76 → 0.80 (time of day): 39 → 32 → 26 → 18 → 14 → 7 → 2 → 1 at +0.5 / 0.9 / 1.2 / 1.7 / 2.2 / 3.2 / 5.2 /
  8.2 s — an e-fold of ~1.3 s (~100 frames), TAA on or off alike;
- brightening 0.80 → 0.76: 17 → 26 → 31 → 36 → 39 → 41 → 42 over ~3 s (so switching ON lags too here);
- `IndirectDiffuse` disabled: the floor is at its final value by +0.5 s both ways — the IBL itself is re-baked in 8
  frames (graphics doc 42 § "The environment").
The lag is the history's, not the sky's. It is counted in FRAMES: Windows-PA's Intel iGPU (~47 FPS, SSGI) keeps a
picture sky's light on the floor 10–30 s after `setBackground()` (`reflexion-debug`, linear floor 1.0 at 3 s, 0.96 at
10 s, 0.0066 at 30 s; `IndirectDiffuse` off: 0.0073 at once), where NVIDIA at ~900 FPS shows none.
**Linux, RTX 3070 Ti, ~390 FPS, sky-lab, exposure pinned (2026-10-25)** — the floor after a 0.76 → 0.80 jump, at
+0.1 / 0.25 / 0.5 / 1.0 s: RT lane 39 / 21 / 8 / 2; SSGI lane 32 / 19 / 8 / 2; `IndirectDiffuse` off 10 / 1 / 0 / 0 (the
measurement chain's own latency, the IBL cycle included); `Denoiser/MaxAccumulation` 8 instead of 64: RT 26 / 4 / 1 / 0,
SSGI 23 / 5 / 1 / 1. **The cause is the 1/N accumulation's floor, 1/64**, with no history rectification
(`temporalNeighborhoodClamp` off by default, because on the RAW input it cost ~5 % of the GI): an e-fold of ~64
frames whatever changed. (The converged level, ~2/3 of the IBL-only floor, is the separate item
`indirect-diffuse-intensity-scales-the-sky`; TAA's own ~1 s smear of a sudden global change, with SSGI off, is TAA's.)

**Fixed 2026-10-25 (owner: the fast/slow anti-lag):** `GIDenoiser` stage 5 (graphics
`13-12-post-processing-effects/16-…`): RT 39 / 21 / 8 / 2 → 19 / 4 / 1 / 0, SSGI 32 / 19 / 8 / 2 → 21 / 5 / 1 / 0, the
steady-state noise unchanged. Open until the peers confirm it on the original case (`labyrinth`'s lamps, macOS) and on
the Intel iGPU's picture-sky switch.

**⚠️ DEFECT of that fix (macOS-PA, Apple M6, SSGI lane, 2026-10-25; confirmed on Linux the same day).** In `labyrinth`
(the pose above, flickering lamps off or on, 40 s settle), the anti-lag lowers the CONVERGED indirect level: macOS right
wall 13 vs 58 (direct only 3.1); Linux (RTX 3070 Ti, 1920×1080, SSGI lane) right wall (x 1350–1900, y 200–800) 39.8 vs
88.5, whole image 72 vs 88. The RT lane is unaffected (31.0 vs 30.8), and so is sky-lab.
- **Cause, proven**: the tolerance was built from the raw estimate's 3×3 SPATIAL deviation. Where SSGI is sparse (most
  rays miss in a dim corridor), most 3×3 windows hold no hit and read 0, so only the 2 % floor is left. The fast mean
  dips between two hits and crosses that floor nearly every frame, so the slow history is held at 6 frames. SSGI's
  screen-space feedback then amplifies that small per-frame bias into a 55 % loss.
  - `AntiLagSigma` 32 changes nothing (the σ term is ~0 there).
  - The denoiser's age view (`Denoiser/DebugView` 2, made pre-exposure-aware the same day; it was white under any
    exposure since pre-exposure step 6) shows the age capped nearly everywhere in labyrinth: mean 128.6 vs 214.9 OFF.
    sky-lab reads 244.1 vs 244.5.
- **What it means**: on SSGI, much of the measured "acceleration" was that permanently short history. The lamp
  switch-off on Linux shows it (lamp panel region at +1 / 2 / 3 / 5 s):
  - OFF: 50 / 17 / 5.3 / 2.5.
  - Pushed detector: 3.5 / 2.4, but the whole scene's level after the switch drops to 19.4 instead of 29.7.
- **Detectors measured** (Linux; labyrinth right wall; sky-lab floor R after 0.76 → 0.80 at +0.1 / 0.25 / 0.5 / 1 s):

  | Noise term | labyrinth | sky-lab SSGI | sky-lab RT | lamps, +1 / 2 / 3 s |
  |---|---|---|---|---|
  | anti-lag OFF | 88.5 | 32 / 19 / 8 / 2 | 39 / 21 / 8 / 2 | 50 / 17 / 5.3 |
  | spatial σ (pushed) | 39.8 | 21 / 5 / 1 / 0 | 19 / 4 / 1 / 0 | 3.5 / 2.4 / 2.4 |
  | temporal σ (slow moments) | 88.6 | 32 / 19 / 8 / 2 | 28 / 10 / 5 / 1 | — |
  | spatial + 0.5 temporal (RELAX form) | 86.4 | 31 / 16 / 7 / 2 | 27 / 10 / 4 / 1 | — |
  | max(spatial, 0.5 temporal) | 86.4 | 28 / 15 / 7 / 2 | 24 / 10 / 4 / 1 | 53 / 20 / 6.1 |
  | max(spatial, 0.25 temporal) | 64.1 | 24 / 10 / 4 / 1 | 24 / 10 / 4 / 1 | — |
  | tracked RMS of (fast − slow) | 117.7 (+33 %) | 28 / 13 / 6 / 1.8 | 26 / 9 / 4 / 1 | — |

  Any trigger correlated with the noise biases the level: firing between hits loses energy, firing on a hit's
  arrival (the tracked RMS) gains it, and the SSGI feedback amplifies either. A per-pixel 3σ test on a 6-frame mean
  cannot separate SSGI's noise (per-frame deviation of the order of the mean) from a full drop.

**The safe detector on the three OS** (engine 49b6f570, 2026-10-25). The level is restored everywhere, at ~98 % of
anti-lag OFF:
- macOS-PA (M6, SSGI, 2560×1440): labyrinth right wall 82.3 vs OFF 84.3 (first detector 37.4); age view 214.71 vs
  214.92; noise equal per luma bucket.
- Windows-PA (Intel iGPU, SSGI): right wall 85.7 vs OFF 87.6; with `AntiLagTemporalSigma` 0, 42.5.
- The acceleration that remains on SSGI is small:
  - sky-lab +0.5 s: macOS 17 vs OFF 33 (first detector 6); Intel 29.1 vs OFF 30.7.
  - The lamp switch-off and the Intel picture-sky switch are identical to OFF within a few seconds: their ~4 s e-fold
    is longer than the 64-frame history, so the tail comes from SSGI's screen-space feedback, not from the history.
- RT lane (NVIDIA): 24.4 / 10.7 / 4.7 / 1.3, the Linux curve.
- 0 VUID and 0 SYNC-HAZARD with the new default on all three. Windows' A/B runs caught the single-queue upload hazard
  again (item `single-queue-upload-not-ordered-before-draw`).

## What remains

- Measure the decay in frames on Linux in BOTH lanes (SSGI and RTGI) at a pinned frame rate — the labyrinth's lamps and
  sky-lab's time-of-day jump (`setDate`, both directions). The SSGI temporal alpha is 0.1
  (`IndirectDiffuse/TemporalAlpha`): ~9.5 frames for an e-fold by itself — the measured ~100 says more than the alpha
  (the variance-guided à-trous' moments, the normal history) holds the past.
- **Owner decision 2026-10-25: the safe detector now, a pooled test next.** The safe `max(spatial, 0.5 temporal)`
  (`Denoiser/AntiLagTemporalSigma` 0.5) is in the engine: no level change, no false trigger, little SSGI acceleration.
  NEXT, its own lot: a neighbourhood-POOLED test (ReBLUR / RELAX compare neighbourhood statistics: NRD
  `RelaxAntilagSettings` spatialSigmaScale 4.5 + temporalSigmaScale 0.5 on the 5×5 fast history), measured against the
  table above (labyrinth level and age view, sky-lab both lanes, the lamp switch-off).
  - Beyond it: A-SVGF's temporal gradients (C. Schied, C. Peters, C. Dachsbacher, "Gradient Estimation for Real-Time
    Adaptive Temporal Filtering", HPG 2018). They re-shade last frame's samples with this frame's lighting, so the
    difference is nearly noise-free, but the producers must re-evaluate their own samples.
  - Protocol (Linux scripts of 2026-10-25): labyrinth at the pose, settle 40 s, right wall x 1350–1900 / y 200–800 at
    1920×1080. The age view needs `Denoiser/DebugView` 2 at f/4, 1/8 s, ISO 100. The lamp switch-off turns off
    `LabyrinthLamp<N-1>` for each `LabyrinthFlickeringPanel<N>` (55); its panel region is x 860–1060 / y 420–620.
- History rejection or a faster response when the current frame's radiance drops far below the history (luminance
  clamping of the history, A. Lauritzen / B. Karis "High-Quality Temporal Supersampling", SIGGRAPH 2014; the
  "responsive" anti-lag of A. Kozlowski et al., "Real-Time Ray Tracing Gems II" / ReBLUR's history reset on
  disocclusion and lighting change) — design decision for the owner.

## References

- engine `docs/subsystems/animations/02-animations-specific-rules.md` § A lamp's visible panel follows it.
