---
id: crash-video-memory-exhausted-swap-chain-cannot-be-recreated-358d9b
title: "Crash reports: the video memory is exhausted and the swap-chain cannot be recreated (controlled stop)"
status: open
priority: high
scope: src/Graphics/Renderer.cpp (retryDegradedSwapChain), src/Core.cpp (stopOnVideoMemoryExhaustion)
opened: 2026-11-18
tags: [crash, crash-report-auto, vram, swap-chain]
---

# Crash reports: the video memory is exhausted and the swap-chain cannot be recreated (controlled stop)

Triaged 2026-11-18. First reports from the release that added the controlled stop (4 machines in 2 days, Windows,
always at `Stopping`).

**What it is.** `Renderer::retryDegradedSwapChain` gives up after consecutive failures to recreate the swap-chain
(`VK_ERROR_OUT_OF_DEVICE_MEMORY` on the depth-stencil images) and `Core::stopOnVideoMemoryExhaustion` stops the engine
with its exit code. This is the intended behaviour: a clean stop with a reason, instead of the former crashes on the
same exhaustion.

**The real question is upstream:** what exhausts the video memory. In the downstream application the long-session VRAM
growth of the embedded browser's GPU process is a known lead (tracked on its side). On the engine side: check what the
engine itself holds in VRAM when the swap-chain recreation fails (staging buffers, textures), and whether releasing
caches before the retries would let the recreation succeed.

## Why

<!-- crash-reports:begin -->
| | |
|---|---|
| Machines | 4 — per version: 8.0.5 ×4 |
| Reports | 4 (first 2026-11-16, last 2026-11-17) |
| Versions | 8.0.5 ×4 |
| Platforms | windows ×4 |
| OS | Windows 11 Home ×3, Windows 11 Pro ×1 |
| GPU (selected) | NVIDIA GeForce RTX 4070 SUPER (driver 616.64) ×1, NVIDIA GeForce RTX 5060 Laptop GPU (driver 617.42) ×1, NVIDIA GeForce RTX 5060 Ti (driver 617.42) ×1, NVIDIA GeForce RTX 5080 (driver 610.88) ×1 |
| ANGLE backend | d3d11 ×4 |
| Died at milestone | Stopping ×4 |
| Single-instance lock | Acquired ×4 |

- **Cause (normalized):** `RendererService: Unable to refresh the swap-chain after <n> consecutive attempts: the video memory is exhausted, the renderer gives up, the engine stops.`
- **Where:** `vendors/emeraude-engine/src/Graphics/Renderer.cpp` — `EmEn::Graphics::Renderer::retryDegradedSwapChain`
- **Signature:** `358d9b556f5fe392`

Last error before the death (else the last trace), by number of reports:

- ×4 `died at Stopping`

Traces of the main journal (normalized, latest report; around the Fatal trace when there is one):

```
[VulkanSwapChain][Error] (vendors/emeraude-engine/src/Vulkan/SwapChain.cpp `EmEn::Vulkan::SwapChain::recreate`) Unable to complete the framebuffer !
[VulkanImage][Error] (vendors/emeraude-engine/src/Vulkan/Image.cpp `EmEn::Vulkan::Image::createWithVMA`) Unable to create an image with VMA : VK_ERROR_OUT_OF_DEVICE_MEMORY !
[VulkanSwapChain][Error] (vendors/emeraude-engine/src/Vulkan/SwapChain.cpp `EmEn::Vulkan::SwapChain::createDepthStencilBuffer`) Unable to create image 'Frame1DepthStencilBuffer' !
[VulkanSwapChain][Error] (vendors/emeraude-engine/src/Vulkan/SwapChain.cpp `EmEn::Vulkan::SwapChain::createImageArray`) Unable to create the depth buffer #<n> !
[VulkanSwapChain][Error] (vendors/emeraude-engine/src/Vulkan/SwapChain.cpp `EmEn::Vulkan::SwapChain::createFramebuffer`) Unable to create the swap-chain images !
[VulkanSwapChain][Error] (vendors/emeraude-engine/src/Vulkan/SwapChain.cpp `EmEn::Vulkan::SwapChain::recreate`) Unable to complete the framebuffer !
[RendererService][Fatal] (vendors/emeraude-engine/src/Graphics/Renderer.cpp `EmEn::Graphics::Renderer::retryDegradedSwapChain`) Unable to refresh the swap-chain after <n> consecutive attempts: the video memory is exhausted, the renderer gives up, the engine stops.
[Core][Error] (vendors/emeraude-engine/src/Core.cpp `EmEn::Core::stopOnVideoMemoryExhaustion`) The video memory is exhausted and the swap-chain cannot be recreated : the engine stops now (exit code <n>).
```

Archives:

- 4 archive(s), in the downstream application's crash-report mirror.
<!-- crash-reports:end -->

## What remains

- Read the journals of the archives counted above (in the downstream application's crash-report mirror).
- Find the defect, reproduce it, fix it where it lives.

## References

- The downstream application's crash-report documentation — what an archive holds, the flight recorder milestones.
- The downstream application's crash-report tool documentation — how this item was produced, how the cause is chosen.
