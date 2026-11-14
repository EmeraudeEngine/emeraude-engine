---
id: cubic-view-render-target-single-layer-image
title: A cubic View render target creates a one-layer image and a CUBE view on it
status: open
priority: high
scope: Graphics/RenderTarget/View.hpp, Scenes/Scene (createRenderToCubicView)
opened: 2026-11-14
tags: [defect, render-target, cubemap, vulkan, dead-path]
---

# A cubic View render target creates a one-layer image and a CUBE view on it

## Why

`RenderTarget::View< ViewMatrices3DUBO >` answers `isCubemap() == true`, but `onCreate()` builds its colour image with
no create flag, one mip level and ONE array layer (`View.hpp`, the `Vulkan::Image` constructor's defaults spelled
`0, 1, 1`), then an image view of type `VK_IMAGE_VIEW_TYPE_CUBE` with `layerCount = arrayLayers` = 1. A cube view needs
six layers and `VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT` on its image: this is invalid by construction. Read on 2026-11-14
while checking every render target's layers; **not run**: `Scene::createRenderToCubicView()` has no caller in the
cascade (engine, projet-alpha), so nothing exercises it. `RenderTarget::Texture` does it right (6 layers, the flag, a
2D-array view to render and a cube view to sample).

`View::capture()` warns that a view "has only single-layer images": true of what is created, wrong for what a cubic
view claims to be.

## What remains

- **Owner decision 2026-11-14: correct it** (not remove it). Six layers with the cube-compatible flag, a 2D-array
  view to render and a cube view to sample, as `Texture` does; `capture()` downloads the layer it is given.
- Verify with the validation layer: a cubic view created and rendered, 0 `VUID-`.

## References

- `src/Graphics/RenderTarget/View.hpp` — `isCubemap()`, `onCreate()` (the colour image and its view), `capture()`.
- `src/Scenes/Scene.rendering.cpp` — `createRenderToCubicView()`.
- `src/Graphics/RenderTarget/Texture.hpp` — the correct cubemap creation.
