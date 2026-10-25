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

## What remains

- Measure the decay in frames on Linux in BOTH lanes (SSGI and RTGI) at a pinned frame rate — the labyrinth's lamps and
  sky-lab's time-of-day jump (`setDate`, both directions). The SSGI temporal alpha is 0.1
  (`IndirectDiffuse/TemporalAlpha`): ~9.5 frames for an e-fold by itself — the measured ~100 says more than the alpha
  (the variance-guided à-trous' moments, the normal history) holds the past.
- History rejection or a faster response when the current frame's radiance drops far below the history (luminance
  clamping of the history, A. Lauritzen / B. Karis "High-Quality Temporal Supersampling", SIGGRAPH 2014; the
  "responsive" anti-lag of A. Kozlowski et al., "Real-Time Ray Tracing Gems II" / ReBLUR's history reset on
  disocclusion and lighting change) — design decision for the owner.

## References

- engine `docs/subsystems/animations/02-animations-specific-rules.md` § A lamp's visible panel follows it.
