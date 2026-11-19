---
id: resize-storm-presents-no-frame
title: During a window resize storm no frame is presented (every frame recreates the swap-chain, then drops)
status: open
priority: unranked
scope: src/Graphics/Renderer.cpp (renderFrame() drop path, retryDegradedSwapChain(), recreateRenderingSubSystem())
opened: 2026-11-19
tags: [resize, swap-chain, presentation]
---

# During a window resize storm no frame is presented (every frame recreates the swap-chain, then drops)

## Why

Measured 2026-11-19 on Linux main (RTX 3070 Ti, NVIDIA, GNOME Wayland, scale 1.5), projet-alpha `--load-demo default
--disable-cef`, Release, the window resized through the remote console (`Core.WindowService.resize`) alternately
between 1280x720 and 1700x1000 at 20 resizes per second: **100 % of the frames were dropped** for the whole storm
(about 14 frame attempts per second, each one finding the swap-chain out of date, recreating it with its render
targets, and returning without rendering). The window shows its last image until the storm stops.

The video memory side of the same storm is fixed (Vulkan doc 12, "A dropped frame does not tick"): this item is only
about what the user sees. A user does not resize 20 times a second, but an interactive drag-resize on a slow machine,
or a tiling window manager animating, can recreate as fast as a recreation lasts.

## What remains

1. Measure the duration of one `recreateRenderingSubSystem()` at that size (swap-chain, scene target, post-processor,
   grab pass) and how many frames a real drag-resize drops (GNOME, KDE, Windows, macOS).
2. Decide the behaviour: render after a recreation in the same frame (acquire again) instead of dropping, coalesce
   the recreations while the size keeps changing, or both — without presenting an out-of-date swap-chain (illegal for
   `VK_ERROR_OUT_OF_DATE_KHR`).
3. Re-run the same storm: frames presented during it, no growth of the deferred destructions.

## References

- `docs/subsystems/vulkan/12-critical-deferred-destruction-contract.md` ("A dropped frame does not tick").
- `docs/subsystems/vulkan/24-swap-chain-given-up-for-lack-of-video-memory.md`.
