---
id: compute-shader-particle-system
title: Create a particle system using compute shaders
status: open
priority: unranked
scope: Physics
opened: unknown
tags: [compute, gpu]
---

# Create a particle system using compute shaders

## What remains

- [ ] GPU particle simulation in a compute shader.

## References

- The compute-inside-an-effect precedent is complete in SSR's Hi-Z pyramid (~200 lines of private
  layout/pipeline/pool per pass, storage images written by raw `vkUpdateDescriptorSets`).
- See also `docs/gpu-compute.md` if it exists at the time of reading, and `src/Vulkan/AGENTS.md`.

## Origin

Inherited from the historical root `TODO.md`.
