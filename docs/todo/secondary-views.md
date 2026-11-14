---
id: secondary-views
title: Secondary views — a second rendered output (another window, a panorama, a VR eye) beside the main view
status: open
priority: unranked
scope: Graphics/Renderer (a frame stage), Graphics/RenderTarget, Scenes/Scene (render lists, motion-vector header), Graphics/Types (RenderTargetType)
opened: 2026-11-14
tags: [feature, render-target, architecture, owner-decision]
---

# Secondary views — a second rendered output beside the main view

## Why

The engine had a class for it, `RenderTarget::View< ViewMatrices2DUBO | ViewMatrices3DUBO >`, with
`Scene::createRenderToView()` and `createRenderToCubicView()` ("final display", "VR or panoramic rendering"). It was
removed on 2026-11-14 (owner decision) because it was inert, and correcting it meant building this feature:

- **Nothing created one**: no caller in the engine nor in its known consumers.
- **Nothing rendered one**: `Renderer::renderFrame()` renders the sky, the shadow maps (`renderShadowMaps()`), the
  render-to-textures (`renderRenderToTextures()`), then the main target. The scene's view list is only walked to
  publish matrices; an image of that class was never drawn into.
- **Its type collided with the main view**: `RenderTargetType::View` is what `Scene.rendering.cpp` reads as "the
  primary view" in three places — the frame's render statistics, the instance-transforms header (the previous and
  current view-projection of the motion vectors), and the advance of the per-instance model-matrix history. A second
  target of that type, rendered in the same frame, would rewrite the velocity header and advance the history twice:
  the main view's TAA and motion blur would break.
- **Its image could not be read**: colour usage `COLOR_ATTACHMENT` alone (no `SAMPLED`, no `TRANSFER_SRC`), final
  layout `PRESENT_SRC_KHR` on an image nothing presents, no sampler (`writeCombinedImageSampler()` returned false).
- **The cubic form was invalid**: a one-layer image under a `VK_IMAGE_VIEW_TYPE_CUBE` view, a depth image of one
  layer, and a render pass without multiview while the generators emit multiview programs for a cubemap target.

## What remains

Nothing until the owner ranks it. If it is taken up, the design decisions to bring to the owner first:

- **What consumes a secondary view** — presented in another window (a second swap chain), sampled (then it is a
  `RenderTarget::Texture`, which exists), or only read back (a 360° capture: six faces of a `Texture` cubemap through
  `dumpRenderTarget` already do it).
- **A frame stage** in the renderer for it, and where it sits (before the main view, as the render-to-textures).
- **A target type of its own** (not `RenderTargetType::View`), so that the three "primary view" sites stay the main
  view's; whether a secondary view gets motion vectors and a post-process chain of its own (VRAM cost on an 8 GB card
  first).
- **The cubic form**: six layers, the cube-compatible flag, 2D-array views and a multiview render pass, as
  `RenderTarget::Texture` does for a cubemap.

## References

- `src/Graphics/Renderer.cpp` — `renderFrame()`, `renderRenderToTextures()` (the model of an off-screen stage).
- `src/Scenes/Scene.rendering.cpp` — the three `renderType() == RenderTargetType::View` sites; `Scene.cpp` registers
  the swap chain in the view list.
- `src/Graphics/RenderTarget/Texture.hpp` — the correct cubemap target (layers, views, multiview).
- The removed class: `git show 466f2527:src/Graphics/RenderTarget/View.hpp`.
