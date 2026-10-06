---
id: gpu-profiler-deferred-lights-scope-misparented
title: GPU profiler prints DeferredLights under FinalComposite on macOS
status: open
priority: unranked
scope: Graphics/GPUProfiler, Graphics/Renderer
opened: 2026-10-06
tags: [profiling, macos]
---

# GPU profiler prints DeferredLights under FinalComposite on macOS

## Why

macOS-PA (Apple M2, MoltenVK), `labyrinth`, 2026-10-06: `Core.RendererService.getGPUTimings()` lists the
`DeferredLights` scope indented under `FinalComposite`, with its own sample count (72 against 130 for its siblings),
where Linux prints it inside `ScenePass`. The scope is opened between the opaque and the resumed halves of the split
scene pass (`Renderer.cpp`, the `DeferredLights` `ScopedZone`). The values may still be right; the tree is not.

## What remains

- Reproduce the listing on Linux after a scene-target recreation, and read how the profiler parents a zone opened
  OUTSIDE a render pass between two passes of the same frame; check the sample counts.

## References

- engine `docs/subsystems/graphics/38-deferred-punctual-lights.md` § A/B and diagnosis.
