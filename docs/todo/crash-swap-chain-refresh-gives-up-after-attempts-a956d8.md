---
id: crash-swap-chain-refresh-gives-up-after-attempts-a956d8
title: "Swap-chain — the renderer gives up refreshing it after consecutive attempts"
status: open
priority: high
scope: Graphics/Renderer
opened: 2026-11-10
tags: [crash, crash-report-auto]
---

# Swap-chain — the renderer gives up refreshing it after consecutive attempts

> Dropped by the downstream application's crash-report tool: a cause never seen in its production crash reports.
> The block between the `crash-reports` markers is rewritten at every run of that tool (found by its signature line, names of the application redacted); write outside it.

## Why

<!-- crash-reports:begin -->
| | |
|---|---|
| Machines | 3 — per version: 8.0.4 ×3 |
| Reports | 4 (first 2026-11-06, last 2026-11-10) |
| Versions | 8.0.4 ×4 |
| Platforms | windows ×4 |
| OS | Windows 10 Pro ×2, Windows 11 Home ×2 |
| GPU (selected) | NVIDIA GeForce RTX 3060 (driver 621.1) ×2, Microsoft Direct3D12 (NVIDIA GeForce RTX 4090 Laptop GPU) (driver 26.0.129.35) ×1, NVIDIA GeForce RTX 3070 Ti (driver 621.1) ×1 |
| ANGLE backend | d3d11 ×4 |
| Died at milestone | Stopping ×4 |
| Single-instance lock | Acquired ×4 |

- **Cause (normalized):** `RendererService: Unable to refresh the swap-chain after <n> consecutive attempts: the renderer gives up, the engine stops.`
- **Where:** `vendors/emeraude-engine/src/Graphics/Renderer.cpp` — `EmEn::Graphics::Renderer::retryDegradedSwapChain`
- **Signature:** `a956d827dda9ff3d`

Last error before the death (else the last trace), by number of reports:

- ×4 `died at Stopping`

Traces of the main journal (normalized, latest report; around the Fatal trace when there is one):

```
[VulkanImage][Error] (vendors/emeraude-engine/src/Vulkan/Image.cpp `EmEn::Vulkan::Image::importFromWin32Handle`) Unable to import the external D3D11 texture memory (handle @@0000000000002E64) !
[a line of the downstream application, redacted]
[VulkanSwapChain][Error] (vendors/emeraude-engine/src/Vulkan/SwapChain.cpp `EmEn::Vulkan::SwapChain::createBaseSwapChain`) Unable to create the swap-chain : VK_ERROR_INITIALIZATION_FAILED !
[VulkanSwapChain][Error] (vendors/emeraude-engine/src/Vulkan/SwapChain.cpp `EmEn::Vulkan::SwapChain::recreate`) Unable to recreate the base of the swap-chain !
[VulkanSwapChain][Error] (vendors/emeraude-engine/src/Vulkan/SwapChain.cpp `EmEn::Vulkan::SwapChain::createBaseSwapChain`) Unable to create the swap-chain : VK_ERROR_INITIALIZATION_FAILED !
[VulkanSwapChain][Error] (vendors/emeraude-engine/src/Vulkan/SwapChain.cpp `EmEn::Vulkan::SwapChain::recreate`) Unable to recreate the base of the swap-chain !
[RendererService][Fatal] (vendors/emeraude-engine/src/Graphics/Renderer.cpp `EmEn::Graphics::Renderer::retryDegradedSwapChain`) Unable to refresh the swap-chain after <n> consecutive attempts: the renderer gives up, the engine stops.
[VulkanTransferManagerService][Info] (vendors/emeraude-engine/src/Vulkan/TransferManager.cpp `EmEn::Vulkan::TransferManager::releaseIdleStagingBuffers`) <n> staging buffers released (<n> MiB), <n> buffer and <n> image transfer operations kept.
```

Archives:

- 4 archive(s), in the downstream application's crash-report mirror.
<!-- crash-reports:end -->

## Triage (2026-11-10)

- **Owner**: the engine (`Renderer::retryDegradedSwapChain`).
- **Production (a downstream application, 8.0.4)**: 3 Windows machines, at `Stopping` (NVIDIA RTX 3060 / 3070 Ti driver 621.1, and one run whose selected device is "Microsoft Direct3D12 (NVIDIA GeForce RTX 4090 Laptop GPU)" — the Vulkan-on-D3D12 layered driver was SELECTED, worth a look of its own).
- Related: `crash-fatal-in-rendererservice-unable-to-refresh-the-s-c02de3` (8.0.2, the message before the retry logic).

## What remains

- See the triage above; the journals stay in the downstream application's crash-report mirror.

## References

- The downstream application's crash-report documentation — what an archive holds, the flight recorder milestones.
- The downstream application's crash-report tool documentation — how this item was produced, how the cause is chosen.
