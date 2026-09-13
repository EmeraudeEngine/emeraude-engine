---
id: ssao-consumes-the-sky-visibility-lane
title: SSAO could consume SSGI's horizon visibility instead of running its own hemisphere kernel
status: open
priority: unranked
scope: Graphics (Effects/Lighting SSAO, SSGI; PostProcessStack::syncSlotPairings)
opened: 2026-09-13
tags: [rendering, lighting, screen-space, ambient-occlusion, performance]
---

# SSAO could consume SSGI's horizon visibility

## Why

Since Sep 2026 `SSGI` computes a GTAO horizon search (bent normal + cosine-weighted visibility V,
half-res RGBA16F) to give the screen-space lane its sky term — see `src/Graphics/AGENTS.md`
§ "The screen-space sky visibility". `SSAO` still runs its own hemisphere kernel next to it
(0.40 ms measured on Sponza, 2880×1620): the same frame therefore integrates the same visibility
twice, with two different estimators, and the AO multiply is the cruder of the two.

The owner deliberately kept them apart for the delivery of the sky term (2026-09-13) so that ONE
variable moved at a time: the sky term is measurable on its own, and the AO look of every
screen-space scene did not change in the same batch.

## What remains

1. Publish the visibility: the SAME producer/consumer shape as the existing ambient-occlusion lane
   (`providesOcclusionLane()` / `consumesOcclusionLane()` / `setOcclusionLaneSource()`, wired once
   per frame by `PostProcessStack::syncSlotPairings()`) — RTGI already feeds RTAO that way.
   ⚠️ The direction is forced by the slot order: `IndirectDiffuse` runs BEFORE `AmbientOcclusion`
   in the command buffer, so SSGI can produce for SSAO and never the reverse.
2. `SSAO` reads the lane when it is given one, and keeps its own kernel when it is not (SSGI
   disabled, or its sky visibility switched off) — the standalone path is a requirement, exactly as
   for RTAO.
3. ⚠️ The two integrals do NOT answer the same question: the sky visibility is a FAR-field term
   (16 m radius by default) while an ambient occlusion is a near-field one (0.5 m). Reading the
   far-field V as an AO would flatten every contact shadow. The horizon loop must accumulate a
   SECOND visibility over a short falloff in the same pass — same samples, one more accumulator —
   before SSAO can consume anything.
4. Measure: AO look before/after on Sponza and on `light-and-shadow-debug`, and the GPU cost of the
   two-accumulator search against `SSAOEffect/trace` + the current search.

## References

- `src/Graphics/IndirectPostProcessEffect.hpp` § "Ambient-occlusion LANE protocol".
- `src/Graphics/Effects/Lighting/SSGI.cpp` (`SSGIHorizonFragmentShader`).
- Jimenez, Wu, Pesce, Jarabo, "Practical Realtime Strategies for Accurate Indirect Occlusion"
  (SIGGRAPH 2016 courses); Intel XeGTAO (MIT).
