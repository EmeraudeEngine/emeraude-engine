---
id: gi-history-keeps-a-switched-off-emitter-lit
title: GI temporal history keeps a switched-off emitter lit for many frames
status: open
priority: unranked
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

## What remains

- Measure the decay in frames on Linux in BOTH lanes (SSGI and RTGI) at a pinned frame rate.
- History rejection or a faster response when the current frame's radiance drops far below the history (luminance
  clamping of the history, A. Lauritzen / B. Karis "High-Quality Temporal Supersampling", SIGGRAPH 2014; the
  "responsive" anti-lag of A. Kozlowski et al., "Real-Time Ray Tracing Gems II" / ReBLUR's history reset on
  disocclusion and lighting change) — design decision for the owner.

## References

- engine `docs/subsystems/animations/02-animations-specific-rules.md` § A lamp's visible panel follows it.
