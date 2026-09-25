---
id: renderable-instance-readiness-any-program
title: isReadyToRender() accepts ANY program of the renderable, so lit and unlit instances cannot coexist
status: open
priority: unranked
scope: Graphics/RenderableInstance, Graphics/Renderable, Scenes/Scene.rendering
opened: 2026-09-25
tags: [shaders, readiness, silent-failure, measured]
---

# isReadyToRender() accepts ANY program of the renderable, so lit and unlit instances cannot coexist

## Why

`RenderableInstance::Abstract::isReadyToRender()` (`src/Graphics/RenderableInstance/Abstract.cpp`, ~l. 683-718)
returns `Renderable::hasAnyCachedProgramsForRenderPass(renderTarget, handle)`
(`src/Graphics/Renderable/Abstract.cpp`, ~l. 97-118). That is true as soon as ANY program cached on the
RENDERABLE matches the render-pass handle. `Scene::getRenderableInstanceReadyForRendering()`
(`src/Scenes/Scene.rendering.cpp`, ~l. 2284) returns early on it.

So a second instance of the same renderable that needs DIFFERENT programs — lit next to unlit, instanced
next to unique — is declared ready and is never given its own. Measured 2026-09-25 by the foliage diagnosis
workflow: a lit `addMesh` copy of `Broadleaf0`, beside the unlit forest, logged "no suitable render program"
1906 times per pass and was never drawn.

This is not theoretical. The tree renderables are shared BY NAME between demos
(`Toolkit::generateTreeRenderable()` → `getOrCreateResource("Aspen0", …)`), and the forests were unlit
until 2026-09-25. Any mix of states on one renderable in one render target hits it.

## What remains

- Make readiness per INSTANCE: check the instance's own program cache keys, per layer and per pass, the way
  `isReadyToCastShadows()` already does with `buildProgramCacheKey`.
- Mind the cost: this check runs per instance per frame. Cache a "ready" bit per instance and per
  render-pass handle rather than looking the keys up every frame.
- Prove it with the same-frame probe: a lit and an unlit instance of one renderable, both drawn.

## References

- The lighting contract made explicit the same day: `Graphics::RenderableInstance::Lighting`, now a
  required argument of `Visual` / `MultipleVisuals`.
