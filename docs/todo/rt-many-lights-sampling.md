---
id: rt-many-lights-sampling
title: RT lane — sample many lights instead of keeping the 128 closest
status: open
priority: unranked
scope: Scenes/LightSet, Graphics RT effects
opened: 2026-10-06
tags: [raytracing, lighting]
---

# RT lane — sample many lights instead of keeping the 128 closest

## Why

The RT light SSBO holds `LightSet::MaxRTLights` (128) entries, and every RT effect loops over all of them per hit.
Since 2026-10-06 (`labyrinth`, 548 lamps) the punctual and line-segment lights beyond 128 are chosen by distance to
the main camera (`LightSet::updateVideoMemory()`, `max(0, distance - reach)`), directional lights first. That is a
stopgap: a far lamp seen in a reflection, or bouncing into the GI, is missing, and a lamp crossing the 128th rank
pops in the RT terms when the camera moves.

## What remains

- A light-sampling scheme for the RT effects, so the cost no longer grows with the light count and no light is
  dropped: a light BVH with importance (A. Conty Estevez, C. Kulla, "Importance Sampling of Many Lights with
  Adaptive Tree Splitting", HPG 2018 / SIGGRAPH 2017 talk), or reservoir resampling (B. Bitterli et al.,
  "Spatiotemporal reservoir resampling for real-time ray tracing with dynamic direct lighting" — ReSTIR DI,
  SIGGRAPH 2020).
- Architecture decision for the owner (the effects share one light SSBO today).

## ⚠️ Traps

- The SSBO is rewritten each frame while frames are in flight: item `rt-light-ssbo-rewritten-while-in-flight`.

## References

- `docs/subsystems/graphics/38-deferred-punctual-lights.md` § The frame's selection.
