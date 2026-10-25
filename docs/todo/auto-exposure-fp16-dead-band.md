---
id: auto-exposure-fp16-dead-band
title: Auto-exposure freezes in a dead band that depends on the frame rate — the adapted luminance is a half float
status: open
priority: high
scope: Graphics/Effects (ToneMapping, the AdaptLum targets), its CPU readback
opened: 2026-10-25
tags: [exposure, tone-mapping, precision, defect]
---

# Auto-exposure freezes in a frame-rate-dependent dead band

## Why

Windows-PA, 2026-10-25 (found while checking the sky's IBL; independent of it). `ToneMapping` keeps the ADAPTED
ln-luminance in RGBA16F (`lumFormat`, the 1×1 AdaptLum targets) and moves it by an exponential step
Δ · (1 − e^(−dt · speed)) (speed 1.5 / 2.0). Near ln L ≈ 8.4 (4 400 nits) a half float's step is 2^-7: a step under half
of it (|Δ| · k < 2^-8) rounds to nothing, so the meter stops wherever it is within a dead band whose width grows with
the frame rate:

- ~900 FPS (NVIDIA, MAILBOX, dt 1.1 ms): k ≈ 0.0022, a dead band of about ±1.8 in ln — a factor of ~6;
- ~47 FPS (Intel FIFO, or `FrameRateLimit` 48): about ±10 %.

Measured at noon in sky-lab, the same scene: NVIDIA uncapped drifts 3 485 → 4 371 nits (only the RT noise moves it);
NVIDIA capped at 48 FPS freezes at exactly 4 726.49 nits for 30 s; Intel freezes at 5 314.13, 4 992.17 or 5 070.78
depending on the run — so the same scene exposes differently by machine, frame rate and history.
`setPreExposureOverride()` does not move the meter: not the pre-exposure.

## What remains

- [ ] Keep the adapted luminance in R32_SFLOAT (the CPU readback's half-to-float conversion goes with it), or integrate
      in a form that does not underflow (owner's choice if several fit).
- [ ] Prove it: the 48 FPS cap and the uncapped run converge to the same meter (within 1 %) from two different
      starting skies.

## References

- `src/Graphics/Effects/` ToneMapping (the AdaptLum targets, `lumFormat`); `docs/subsystems/graphics/41-scene-colour-pre-exposure.md`.
