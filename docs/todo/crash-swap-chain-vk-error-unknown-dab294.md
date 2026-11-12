---
id: crash-swap-chain-vk-error-unknown-dab294
title: "Swap-chain — creation answers VK_ERROR_UNKNOWN (Radeon RX 580)"
status: open
priority: high
scope: Vulkan/SwapChain
opened: 2026-11-10
tags: [crash, crash-report-auto]
---

# Swap-chain — creation answers VK_ERROR_UNKNOWN (Radeon RX 580)

> Dropped by the downstream application's crash-report tool: a cause never seen in its production crash reports.
> The block between the `crash-reports` markers is rewritten at every run of that tool (found by its signature line, names of the application redacted); write outside it.

## Why

<!-- crash-reports:begin -->
| | |
|---|---|
| Machines | 1 — per version: 8.0.3 ×1 |
| Reports | 2 (first 2026-10-29, last 2026-10-31) |
| Versions | 8.0.3 ×2 |
| Platforms | windows ×2 |
| OS | Windows 11 Pro ×2 |
| GPU (selected) | Radeon RX 580 Series (driver 2.0.233) ×2 |
| ANGLE backend | d3d11 ×2 |
| Died at milestone | ApplicationStarted ×1, Stopping ×1 |
| Single-instance lock | not recorded ×2 |

- **Cause (normalized):** `VulkanSwapChain: Unable to create the swap-chain : VK_ERROR_UNKNOWN !`
- **Where:** `vendors/emeraude-engine/src/Vulkan/SwapChain.cpp` — `EmEn::Vulkan::SwapChain::createBaseSwapChain`
- **Signature:** `dab29460cd772348`

Last error before the death (else the last trace), by number of reports:

- ×1 `died at ApplicationStarted`
- ×1 `died at Stopping`

Traces of the main journal (normalized, latest report; around the Fatal trace when there is one):

```
[VulkanDeviceMemory][Error] (vendors/emeraude-engine/src/Vulkan/DeviceMemory.cpp `EmEn::Vulkan::DeviceMemory::createOnHardware`) Unable to allocate <n> bytes (alignment:<n> bytes) in the device memory <n> : VK_ERROR_UNKNOWN !
[VulkanImage][Error] (vendors/emeraude-engine/src/Vulkan/Image.cpp `EmEn::Vulkan::Image::importFromWin32Handle`) Unable to import the external D3D11 texture memory (handle @@00000000000021E8) !
[a line of the downstream application, redacted]
[VulkanDeviceMemory][Error] (vendors/emeraude-engine/src/Vulkan/DeviceMemory.cpp `EmEn::Vulkan::DeviceMemory::createOnHardware`) Unable to allocate <n> bytes (alignment:<n> bytes) in the device memory <n> : VK_ERROR_UNKNOWN !
[VulkanImage][Error] (vendors/emeraude-engine/src/Vulkan/Image.cpp `EmEn::Vulkan::Image::importFromWin32Handle`) Unable to import the external D3D11 texture memory (handle @@00000000000025A4) !
[a line of the downstream application, redacted]
[VulkanSwapChain][Fatal] (vendors/emeraude-engine/src/Vulkan/SwapChain.cpp `EmEn::Vulkan::SwapChain::createBaseSwapChain`) Unable to create the swap-chain : VK_ERROR_UNKNOWN !
[VulkanSwapChain][Error] (vendors/emeraude-engine/src/Vulkan/SwapChain.cpp `EmEn::Vulkan::SwapChain::recreate`) Unable to recreate the base of the swap-chain !
```

Archives:

- 2 archive(s), in the downstream application's crash-report mirror.
<!-- crash-reports:end -->

## Triage (2026-11-10)

- **Owner**: the engine. 1 Windows machine (Radeon RX 580, driver 2.0.233), 8.0.3, 2 reports.

## Journals read (2026-11-11, 2 of 2, 8.0.3, 1 machine)

- The swap-chain creation logs NEITHER the requested extent, image count, format nor present mode, nor which device / queue family presents: none of the 20 journals read can give them. A creation should log them (once per creation, or on a failure).
- None of the six swap-chain signatures appears in 8.0.4 (the VRAM stop, exit 95, and the device-loss stop, 98, now take these runs; not proven).
- Windows 11, RX 580 (driver 22.20.27.09 / 2.0.233, Vulkan 1.3.217), the only device. Both: **video memory exhaustion surfacing as `VK_ERROR_UNKNOWN`** (repeated device-memory allocations failing UNKNOWN, D3D11 imports failing), then the creation UNKNOWN; 30 attempts and the give-up in one. No resize or monitor trace.

## What remains

- See the triage above; the journals stay in the downstream application's crash-report mirror.

## References

- The downstream application's crash-report documentation — what an archive holds, the flight recorder milestones.
- The downstream application's crash-report tool documentation — how this item was produced, how the cause is chosen.
