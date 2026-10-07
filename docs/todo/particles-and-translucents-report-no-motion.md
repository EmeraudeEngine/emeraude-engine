---
id: particles-and-translucents-report-no-motion
title: Particles and translucent surfaces give the temporal passes no correct motion
status: open
priority: high
scope: Scenes/Component/ParticlesEmitter, Graphics/RenderableInstance/Multiple, Saphir/Generator/SceneRendering
opened: 2026-09-26
tags: [taa, temporal, velocity, particles, translucency]
---

# Particles and translucent surfaces give the temporal passes no correct motion

## Why

Found while analysing the TAA trails (2026-09-26); no owner report of it yet. Every temporal pass (the TAA,
the RTGI/SSGI denoisers, the motion blur, a future upscaler) reprojects with the velocity G-buffer. Two
kinds of content write a wrong one:

- **Particles report zero object motion**: `ParticlesEmitter.hpp:86`, and sprites are excluded from the motion
  history (`RenderableInstance/Multiple.cpp:74`). A moving particle is reprojected as if it stood still.
- **Translucent surfaces overwrite the velocity and depth of what is behind them** (G-buffer blending off,
  `Saphir/Generator/SceneRendering.cpp:1104-1113`). The opaque surface seen through them loses its own motion
  and depth.

## What remains

1. Reproduce and measure first. A particle emitter over a detailed background, camera parked and then moving:
   `temporalCapture` with `Core/Graphics/PostProcessing/TemporalAA/DebugView` 1 and 3. Then a translucent pane
   over a moving object. Check that the wrong velocity really shows before changing anything.
2. The known answers, to present to the owner with measurements:
   - particles: a real per-particle previous position (the emitter knows it), or a "reactive" mask that
     lowers the history weight where they are drawn (FSR 2's reactive mask);
   - ⚠️ BOTH MECHANISMS EXIST SINCE 2026-09-28 (the beams needed them): `Material::Interface::writesGeometryBuffer()`
     (false = the G-buffer, velocity included, is masked — the surface behind keeps its own) and the REACTIVE MASK
     (`Material::Interface::reactiveMaskExpression()`, `R8_UNORM` attachment, read by the TAA; graphics doc 33). What
     is left for this item is to decide WHICH particles / translucents use them (StandardResource has neither yet),
     and to measure;
   - translucents: do not write velocity/depth at all (keep the opaque surface's), plus the same reactive
     mask for their own colour.

## ⚠️ Traps

- Check the velocity SOURCE before the resolve: every trail fixed in September 2026 was a source reporting a
  wrong or missing previous position (the skinned pose per logic tick, the FFT ocean). Engine TAA resolve doc,
  § Trails under motion.
- `temporalCapture` renders at ~290 fps: projet-alpha `docs/temporal-stability-measurement.md` § 1e.

## References

- `docs/subsystems/graphics/13-12-post-processing-effects/06-the-taa-resolve-the-history-is-rejected-by-depth-never-by-co.md`
- `docs/todo/temporal-upscalers-fsr-xess-dlss.md` (an upscaler needs the same fix, or a reactive mask).
