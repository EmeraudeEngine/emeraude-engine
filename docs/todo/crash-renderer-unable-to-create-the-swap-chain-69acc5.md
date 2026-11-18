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
| Machines | 3 — per version: 8.0.4 ×3 |
| Reports | 7 (first 2026-11-09, last 2026-11-15) |
| Versions | 8.0.4 ×7 |
| Platforms | windows ×7 |
| OS | Windows 10 Pro ×5, Windows 10 Home Single Language ×1, Windows 11 Pro ×1 |
| GPU (selected) | unknown ×7 |
| ANGLE backend | d3d11 ×7 |
| Died at milestone | SessionOpened ×7 |
| Single-instance lock | Acquired ×7 |

- **Cause (normalized):** `RendererService: Unable to create the swap-chain!`
- **Where:** `vendors/emeraude-engine/src/Graphics/Renderer.cpp` — `EmEn::Graphics::Renderer::initializeRenderingStack`
- **Signature:** `69acc51c3c04c51d`

Last error before the death (else the last trace), by number of reports:

- ×7 `died at SessionOpened`

Traces of the main journal (normalized, latest report; around the Fatal trace when there is one):

```
[RendererService][Success] (vendors/emeraude-engine/src/Graphics/Renderer.cpp `EmEn::Graphics::Renderer::initializeSubServices`) PostProcessorService service up!
[RendererService][Warning] (vendors/emeraude-engine/src/Graphics/Renderer.cpp `EmEn::Graphics::Renderer::initializeSubServices`) GraphicsExternalInputService service failed or disabled at startup!
[RendererService][Warning] (vendors/emeraude-engine/src/Graphics/Renderer.cpp `EmEn::Graphics::Renderer::initializeSubServices`) GraphicsRecorderService service failed or disabled at startup!
[VulkanSwapChain][Error] (vendors/emeraude-engine/src/Vulkan/SwapChain.cpp `EmEn::Vulkan::SwapChain::createBaseSwapChain`) Unable to create the swap-chain : VK_ERROR_OUT_OF_DEVICE_MEMORY !
[VulkanSwapChain][Error] (vendors/emeraude-engine/src/Vulkan/SwapChain.cpp `EmEn::Vulkan::SwapChain::onCreate`) Unable to create the base of the swap-chain !
[RenderTarget][Error] (vendors/emeraude-engine/src/Graphics/RenderTarget/Abstract.cpp `EmEn::Graphics::RenderTarget::Abstract::createRenderTarget`) Unable to create a complete render target! Destroying it...
[RendererService][Fatal] (vendors/emeraude-engine/src/Graphics/Renderer.cpp `EmEn::Graphics::Renderer::initializeRenderingStack`) Unable to create the swap-chain!
[RendererService][Info] (vendors/emeraude-engine/src/Graphics/Renderer.cpp `EmEn::Graphics::Renderer::requestShutdown`) Graceful shutdown requested. Waiting for frames in-flight to complete...
```

Archives:

- 7 archive(s), in the downstream application's crash-report mirror.
<!-- crash-reports:end -->

## Triage (2026-11-10)

- **Owner**: the engine. 1 Windows machine, 5 reports in 8.0.4, at `SessionOpened`, GPU not recorded.
- **Cause (2026-11-10)**: its journal shows the Microsoft Direct3D12 translation layer selected "(PREFERRED by the
  application)": the preferred-device loop took the layer, which shares the native driver's LUID.
- **Fixed (engine, 2026-11-10)**: translation layers rank below every native device and the preferred-device match goes
  from the best (`docs/subsystems/vulkan/21-gpu-selection-modes-and-preferred-device.md` § Translation layers rank
  last).

## What remains

- Confirm on the reports of the first release that ships the fix: this signature should disappear; then delete the
  item (the knowledge is in the GPU selection doc).

## References

- The downstream application's crash-report documentation — what an archive holds, the flight recorder milestones.
- The downstream application's crash-report tool documentation — how this item was produced, how the cause is chosen.
