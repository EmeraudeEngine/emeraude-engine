---
id: render-target-dead-view-distance-parameter
title: Every render-target constructor takes a viewDistance that nothing reads
status: open
priority: low
scope: Graphics/RenderTarget, Graphics/SceneRenderTarget, Graphics/SelectionDepthTarget, Vulkan/SwapChain
opened: 2026-10-01
tags: [cleanup, api, render-target]
---

# Every render-target constructor takes a viewDistance that nothing reads

## Why

Found in triad 7e (2026-10-01, clang-tidy `misc-unused-parameters` on `RenderTarget/Abstract.hpp`). The base
constructor `RenderTarget::Abstract(…, float viewDistance, …)` ignores its `viewDistance`. Every subclass constructor
(`View`, `Texture`, `ShadowMap` ×3, `SceneRenderTarget`, `SelectionDepthTarget`, `ImposterBake`, `Vulkan::SwapChain`)
takes one too and only forwards it to that base. The distance a target really uses comes later, from the camera,
through `updateVideoDeviceProperties()` into the view matrices; `viewDistance()` reads it back from them.

Removing it from the base alone leaves the parameter dead one level up: the cleanup goes through every construction
site of a render target (scenes, lights, the toolkit, the renderer, `SwapChain`, which reads `Core/Graphics/ViewDistance`
only for this). That is an API change, not a mechanical triad fix, so 7e left it as it is.

## What remains

- [ ] Remove the parameter from `RenderTarget::Abstract` and from every subclass constructor and call site, or give
  it a meaning (an initial far plane before the first camera update), which is an owner decision.
- [ ] Check that no target is drawn before its first `updateVideoDeviceProperties()` (with the parameter gone, its
  matrices hold their defaults until then).

## References

- `src/Graphics/RenderTarget/Abstract.hpp`: the constructor.
- projet-alpha `docs/plans/triad-engine-pass-report.md` (per-section record) § 7e.
