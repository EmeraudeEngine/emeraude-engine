---
id: compute-shaders-blur-ssao
title: Move blur and SSAO to compute shaders
status: open
priority: medium
scope: Graphics/PostProcessing
opened: unknown
tags: [compute, performance]
---

# Move blur and SSAO to compute shaders

## Why

P3 of the Graphics optimization roadmap: blur, SSAO and DoF run as fragment shaders, one pass per
step. Compute with shared memory removes the render-pass transitions and re-reads.

## What remains

- [ ] Port the separable blurs (and SSAO) to compute.

## Context

The pass-merging plan (Phases A-E of `docs/post-processing-pipeline.md` § 5) is **done**: the seven
separable-blur effects already delegate their blur pairs to the shared MRT `DenoisePass` (up to 8
passes → 2 per group). Compute is the next step for the same math, not a replacement for that
work.

⚠️ The phase B+A Linux measurement showed **NO FPS gain on Sponza RT** — the bottleneck there is
the RT TRACE passes, which none of these phases touch. Expect the same here, and measure a scene
that is actually post-process bound.
