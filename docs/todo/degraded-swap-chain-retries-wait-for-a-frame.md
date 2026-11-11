---
id: degraded-swap-chain-retries-wait-for-a-frame
title: The recreations of a degraded swap-chain run only when a frame is rendered
status: open
priority: high
scope: src/Graphics/Renderer.cpp (renderFrame, retryDegradedSwapChain)
opened: 2026-11-11
tags: [vulkan, swap-chain, video-memory]
---

# The recreations of a degraded swap-chain run only when a frame is rendered

## Why
`Renderer::retryDegradedSwapChain()` is called from `renderFrame()` only, when no image was acquired. An application
that renders on demand (a downstream application draws when one of its web surfaces repaints) renders no frame while
nothing changes. Then the retries stop at "attempt 1/30" until something repaints.

Measured on 2026-11-11 (Linux, RTX 3070 Ti, Xvfb, a downstream application, `--inject-swap-chain-out-of-video-memory`
then `Core.WindowService.resize()`): the process stayed at "Unable to refresh the swap-chain (attempt 1/30)", 0 % CPU,
for minutes. Console commands did not move it. A page animation that repainted every 50 ms made the 30 attempts run,
and the stop with exit code 95 followed.

On a real desktop a mouse move repaints, so the window most likely stays frozen until the user touches it. Unproven:
no field report says so.

## What remains
- Decide: keep requesting frames while the swap-chain is degraded (Core schedules a cycle every
  `SwapChainRecreationRetryInterval`), or move the retries to the main loop.
- Then measure the same scenario with an idle application: the 30 attempts and the stop, without any repaint.

## References
- `src/Graphics/Renderer.cpp` (`renderFrame`, `retryDegradedSwapChain`), `src/Core.cpp` (`injectRuntimeFailure`).
- `docs/subsystems/vulkan/24-swap-chain-given-up-for-lack-of-video-memory.md`,
  `docs/subsystems/source-tree/08-startup-fault-injection.md`.
