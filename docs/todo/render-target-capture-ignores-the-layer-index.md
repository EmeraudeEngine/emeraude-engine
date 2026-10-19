---
id: render-target-capture-ignores-the-layer-index
title: RenderTarget capture() validates its layer index, then always downloads layer 0
status: open
priority: high
scope: engine Graphics/RenderTarget (ShadowMap, Texture), Vulkan/TransferManager
opened: 2026-10-19
tags: [render-target, capture, cubemap, csm, cppcheck]
---

# RenderTarget capture() validates its layer index, then always downloads layer 0

## Why

`RenderTarget::ShadowMap::capture()` and `RenderTarget::Texture::capture()` take a `layerIndex`, check it against the
target's layers (6 for a cubemap, the cascade count for a CSM), clamp a single-layer target to 0 with a warning
("Using layer 0 instead") — and then call `TransferManager::downloadImage()`, which has no layer parameter and copies
`baseArrayLayer = 0, layerCount = 1` (`TransferManager.cpp`). A valid `capture(…, 3, …)` of a cubemap returns face 0,
silently. Found by the engine's first cppcheck sweep (2026-10-19): the clamped `layerIndex` is never read
(`unreadVariable`, suppressed inline at both sites with this item's id).

Nothing in the cascade calls `capture()` today (engine, projet-alpha), so no consumer is affected yet.

## What remains

- `TransferManager::downloadImage()` takes the array layer to copy (validated against `arrayLayers`).
- Both `capture()` pass `layerIndex`; the warning text stays true. The other targets (`View`, `ImposterBake`,
  `SelectionDepthTarget`, `SceneRenderTarget`, `SwapChain`) are checked for the same gap.
- A runtime verification: capture two faces of a cubemap render target (a console command, if none exists — propose it)
  and show they differ; 0 `VUID-`.
- Remove the two inline suppressions.

## References

- `src/Graphics/RenderTarget/ShadowMap.hpp`, `src/Graphics/RenderTarget/Texture.hpp` (`capture()`).
- `src/Vulkan/TransferManager.cpp` (`downloadImage()`, the copy region).
- `docs/static-analysis-ledger.md` § cppcheck.
