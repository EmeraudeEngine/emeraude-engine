---
id: gpu-profiler-v2
title: GPU profiler V2 — cover the shadow map and render-to-texture passes
status: open
priority: medium
scope: Vulkan/Profiling
opened: unknown
tags: [profiling, vulkan]
---

# GPU profiler V2 — cover the shadow map and render-to-texture passes

## Why

V1 (`Vulkan::GPUProfiler`) times the main command buffer only. The shadow map and
render-to-texture passes are submitted through **separate command buffers before the main one**,
so the V1 single-pool-per-frame reset — recorded at the top of the main command buffer — would
wipe their timestamps on the GPU timeline.

## What remains

- [ ] One query range (or one pool) per submission, so every pass can be timed.

## References

- V1 is documented in `docs/ai-runtime-control.md` §6 and `src/Vulkan/AGENTS.md`.
