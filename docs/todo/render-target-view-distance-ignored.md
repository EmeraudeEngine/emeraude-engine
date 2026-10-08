---
id: render-target-view-distance-ignored
title: RenderTarget constructors ignore their viewDistance parameter
status: open
priority: high
scope: src/Graphics/RenderTarget/Abstract.hpp and derived
opened: 2026-10-08
tags: [ave-robustus-ii, graphics, defect]
---

# RenderTarget constructors ignore their viewDistance parameter

## Why
`RenderTarget::Abstract`'s constructor parameter `viewDistance` has been ignored since the base member was removed
(engine "Version 0.8.31"): every `Scene::createRenderTo*()` and the `ShadowMap` / `Texture` / `View` constructors pass a
view distance that has no effect; the far plane comes only from `setViewDistance()` / `updateViewRangesProperties()`.
Marked `[[maybe_unused]]` with a NOTE on 2026-10-08.

## What remains
- Owner: honour it (set the far plane in the derived constructors — may change shadow-map ranges, needs a visual A/B) or
  remove it from the whole chain (API change).

## References
- Found by the Ave Robustus II warning pass (2026-10-08, projet-alpha `docs/plans/ave-robustus-ii.md`); not raised by a warning, so left for its own fix.
