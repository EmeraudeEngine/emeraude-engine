---
id: render-target-view-distance-ignored
title: Every render-target constructor takes a viewDistance that nothing reads — honour it or remove it
status: open
priority: high
scope: Graphics/RenderTarget, Graphics/SceneRenderTarget, Graphics/SelectionDepthTarget, Vulkan/SwapChain
opened: 2026-10-01
tags: [ave-robustus-ii, graphics, render-target, api, defect]
---

# Every render-target constructor takes a viewDistance that nothing reads

## Why

`RenderTarget::Abstract(…, float viewDistance, …)` has ignored its `viewDistance` since the base member was removed
(engine "Version 0.8.31"). It was found twice:
- in triad 7e (2026-10-01), by clang-tidy `misc-unused-parameters` on `RenderTarget/Abstract.hpp`;
- in the Ave Robustus II warning pass (2026-10-08). That pass marked it `[[maybe_unused]]` with a NOTE, now at
  `Abstract.hpp:744`, through 03608b668.

Every subclass constructor takes one too and only forwards it to the base: `View`, `Texture`, `ShadowMap` ×3,
`SceneRenderTarget`, `SelectionDepthTarget`, `ImposterBake` and `Vulkan::SwapChain`. `SwapChain` reads
`Core/Graphics/ViewDistance` only for this. Every `Scene::createRenderTo*()` passes a view distance that has no effect.

The distance a target really uses comes later, from the camera: `updateVideoDeviceProperties()` /
`setViewDistance()` / `updateViewRangesProperties()` put it into the view matrices, and `viewDistance()` reads it back
from them.

## What remains

- **Owner decision**, either way an API change through every construction site (scenes, lights, the toolkit, the
  renderer, `SwapChain`):
  - **honour it**: set an initial far plane in the derived constructors, used before the first camera update. This
    may change the shadow-map ranges, so it needs a visual A/B;
  - **remove it** from the whole chain.
- Check that no target is drawn before its first `updateVideoDeviceProperties()`: without the parameter, its matrices
  hold their defaults until then.

## References

- `src/Graphics/RenderTarget/Abstract.hpp`: the constructor.
- projet-alpha `docs/plans/triad-engine-pass-report.md` § 7e and `docs/plans/ave-robustus-ii.md`.
- Absorbs the former item `render-target-dead-view-distance-parameter` (merged 2026-10-11).
