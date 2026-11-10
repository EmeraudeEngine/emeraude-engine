---
id: crash-renderer-unable-to-create-the-swap-chain-69acc5
title: "Swap-chain — creation fails at startup (RendererService)"
status: open
priority: high
scope: Graphics/Renderer
opened: 2026-11-10
tags: [crash, crash-report-auto]
---

# Swap-chain — creation fails at startup (RendererService)

> Dropped by the downstream application's crash-report tool: a cause never seen in its production crash reports.
> The block between the `crash-reports` markers is rewritten at every run of that tool (found by its signature line, names of the application redacted); write outside it.

## Why

<!-- crash-reports:begin -->
| | |
|---|---|
| Machines | 1 — per version: 8.0.4 ×1 |
| Reports | 5 (first 2026-11-09, last 2026-11-09) |
| Versions | 8.0.4 ×5 |
| Platforms | windows ×5 |
| OS | Windows 10 Pro ×5 |
| GPU (selected) | unknown ×5 |
| ANGLE backend | d3d11 ×5 |
| Died at milestone | SessionOpened ×5 |
| Single-instance lock | Acquired ×5 |

- **Cause (normalized):** `RendererService: Unable to create the swap-chain!`
- **Where:** `vendors/emeraude-engine/src/Graphics/Renderer.cpp` — `EmEn::Graphics::Renderer::initializeRenderingStack`
- **Signature:** `69acc51c3c04c51d`

Last error before the death (else the last trace), by number of reports:

- ×5 `died at SessionOpened`

Traces of the main journal (normalized, latest report; around the Fatal trace when there is one):

```
[VulkanImage][Error] (vendors/emeraude-engine/src/Vulkan/Image.cpp `EmEn::Vulkan::Image::createWithVMA`) Unable to create an image with VMA : VK_ERROR_OUT_OF_DEVICE_MEMORY !
[VulkanSwapChain][Error] (vendors/emeraude-engine/src/Vulkan/SwapChain.cpp `EmEn::Vulkan::SwapChain::createDepthStencilBuffer`) Unable to create image 'Frame0DepthStencilBuffer' !
[VulkanSwapChain][Error] (vendors/emeraude-engine/src/Vulkan/SwapChain.cpp `EmEn::Vulkan::SwapChain::createImageArray`) Unable to create the depth buffer #<n> !
[VulkanSwapChain][Error] (vendors/emeraude-engine/src/Vulkan/SwapChain.cpp `EmEn::Vulkan::SwapChain::createFramebuffer`) Unable to create the swap-chain images !
[VulkanSwapChain][Error] (vendors/emeraude-engine/src/Vulkan/SwapChain.cpp `EmEn::Vulkan::SwapChain::onCreate`) Unable to complete the framebuffer !
[RenderTarget][Error] (vendors/emeraude-engine/src/Graphics/RenderTarget/Abstract.cpp `EmEn::Graphics::RenderTarget::Abstract::createRenderTarget`) Unable to create a complete render target! Destroying it...
[RendererService][Fatal] (vendors/emeraude-engine/src/Graphics/Renderer.cpp `EmEn::Graphics::Renderer::initializeRenderingStack`) Unable to create the swap-chain!
[RendererService][Info] (vendors/emeraude-engine/src/Graphics/Renderer.cpp `EmEn::Graphics::Renderer::requestShutdown`) Graceful shutdown requested. Waiting for frames in-flight to complete...
```

Archives:

- 5 archive(s), in the downstream application's crash-report mirror.
<!-- crash-reports:end -->

## Triage (2026-11-10)

- **Owner**: the engine. 1 Windows machine, 5 reports in 8.0.4, at `SessionOpened`, GPU not recorded.

## What remains

- See the triage above; the journals stay in the downstream application's crash-report mirror.

## References

- The downstream application's crash-report documentation — what an archive holds, the flight recorder milestones.
- The downstream application's crash-report tool documentation — how this item was produced, how the cause is chosen.
