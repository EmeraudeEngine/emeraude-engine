---
id: continuous-window-resizing-exhausts-video-memory
title: Continuous window resizing exhausts the video memory in about 4 minutes (deferred releases never catch up)
status: open
priority: high
scope: src/Vulkan (deferred destruction, DeferredDestructor / Device::destroyAfter), the swap-chain and render-target recreation on resize
opened: 2026-11-18
tags: [vram, resize, deferred-destruction, swap-chain]
---

# Continuous window resizing exhausts the video memory in about 4 minutes (deferred releases never catch up)

## Why

Measured 2026-11-18 on Linux main (RTX 3070 Ti 8 GB, NVIDIA 620.70, GNOME 48 Wayland), projet-alpha (`--load-demo
default --disable-cef`, Release, engine `ccd738b1`), the window resized through the remote console
(`Core.WindowService.resize`) alternately between 1280x720 and 1700x1000 every 50 ms:

- `GPU memory AT ITS BUDGET on device-local heap 0: 5801.06 MiB used for a budget of 5801.06 MiB` after ~241 s; the
  budget warning listed thousands of images spilled to heap 1 (2316 + 830 + 829 + 1343);
- the first `VK_ERROR_OUT_OF_DEVICE_MEMORY` (`Image::createWithVMA`, a scene render target's depth image) after ~258 s;
- then the swap-chain given up for lack of video memory and the controlled stop (exit 95) at ~266 s.

**Not a leak: a release lag.** After 300 resizes at 50 ms the largest image group held 589 allocations against 45
before; 5 s of idle brought every group back to (or below) its starting count, stable 10 s and 15 s later. Under
continuous resizing the deferred releases never catch up with the recreations. The same stress in bursts (150 resizes,
then a 4 s pause) ran 10 minutes without a stop (~5.9 GiB used during each burst).

A user does not resize 20 times a second for 4 minutes, but an interactive drag-resize, a tiling window manager or a
monitor hot-plug storm recreates as fast, and a machine with less video memory reaches the budget much sooner.

## What remains

1. Measure what holds the memory during the storm: which images (the swap-chain's, the scene render targets', the
   overlay's) and how many frames / timeline values their release waits for.
2. Decide the fix: release the previous generation as soon as the GPU is done with it (its timeline value) rather than
   after a fixed delay, coalesce the recreations of a resize storm (recreate the render targets once the size has been
   stable for a frame or two), or both.
3. Re-run the same stress: it must hold 10 minutes at 50 ms without reaching the budget.

## References

- `docs/subsystems/vulkan/12-critical-deferred-destruction-contract.md` (the deferred destruction contract).
- `docs/subsystems/vulkan/24-swap-chain-given-up-for-lack-of-video-memory.md` (the exit-95 stop reached here).
- The measurement's journals: Linux main scratch of the projet-alpha session, 2026-11-18 (`step3-journal.log`).
