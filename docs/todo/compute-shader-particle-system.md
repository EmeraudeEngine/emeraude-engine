---
id: compute-shader-particle-system
title: Create a particle system using compute shaders
status: open
priority: low
scope: Scenes/Component/ParticlesEmitter, Graphics (compute)
opened: unknown
tags: [compute, gpu]
---

# Create a particle system using compute shaders

## What remains

- [ ] GPU particle simulation in a compute shader.

## References

- The compute-inside-an-effect precedent is complete in SSR's Hi-Z pyramid (~200 lines of private
  layout/pipeline/pool per pass, storage images written by raw `vkUpdateDescriptorSets`).
- The compute infrastructure: `docs/subsystems/graphics/29-gpu-compute-pipeline.md`, and `src/Vulkan/AGENTS.md`.
- Today's emitter is CPU-side: `src/Scenes/Component/ParticlesEmitter.cpp`.
- Related: `lit-particles-pipeline` (a LIT emitter has no valid pipeline) — the GPU path must not inherit it.

## Origin

Inherited from the historical root `TODO.md`.
