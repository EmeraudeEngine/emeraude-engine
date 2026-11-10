---
id: crash-fatal-in-vulkanswapchain-unable-to-create-the-sw-9703d3
title: "Crash reports: Fatal in VulkanSwapChain — Unable to create the swap-chain : VK_ERROR_OUT_OF_DEVICE_MEMORY !"
status: open
priority: unranked
scope: src/Vulkan/SwapChain.cpp
opened: 2026-10-21
tags: [crash, crash-report-auto]
---

# Crash reports: Fatal in VulkanSwapChain — Unable to create the swap-chain : VK_ERROR_OUT_OF_DEVICE_MEMORY !

> Dropped by the downstream application's crash-report tool: a cause never seen before in the production crash reports.
> **Triage**: confirm it is a defect, give the file a better name, move it to the repository that owns the defect
> (an engine defect goes to `vendors/emeraude-engine/docs/todo/`, …) and rank it. The block between the
> `crash-reports` markers is rewritten at every run, wherever the item went (it is found by its signature line);
> write outside it. Deleting the item closes the cause: a new report of it is then flagged as a regression.

## Why

<!-- crash-reports:begin -->
| | |
|---|---|
| Machines | 7 — per version: 8.0.2 ×4, 8.0.3 ×3 |
| Reports | 7 (first 2026-10-20, last 2026-11-02) |
| Versions | 8.0.2 ×4, 8.0.3 ×3 |
| Platforms | windows ×7 |
| OS | Windows 11 Pro ×4, Windows 11 Home ×2, Windows 11 Home Single Language ×1 |
| GPU (selected) | NVIDIA GeForce RTX 3050 Laptop GPU (driver 617.42) ×1, NVIDIA GeForce RTX 4060 Ti (driver 610.88) ×1, NVIDIA GeForce RTX 4090 (driver 621.1) ×1, NVIDIA GeForce RTX 5070 Laptop GPU (driver 610.60) ×1, NVIDIA GeForce RTX 5070 Ti (driver 617.14) ×1, NVIDIA GeForce RTX 5070 Ti Laptop GPU (driver 595.79) ×1, … (1 more) |
| ANGLE backend | d3d11 ×7 |
| Died at milestone | Stopping ×3, ApplicationStarted ×2, QuitRequested ×2 |
| Single-instance lock | not recorded ×7 |

- **Cause (normalized):** `VulkanSwapChain: Unable to create the swap-chain : VK_ERROR_OUT_OF_DEVICE_MEMORY !`
- **Where:** `vendors/emeraude-engine/src/Vulkan/SwapChain.cpp` — `EmEn::Vulkan::SwapChain::createBaseSwapChain`
- **Signature:** `9703d338859b9c76`

Last error before the death (else the last trace), by number of reports:

- ×3 `died at Stopping`
- ×2 `died at ApplicationStarted`
- ×2 `died at QuitRequested`

Traces of the main journal (normalized, latest report; around the Fatal trace when there is one):

```
[VulkanDeviceMemory][Error] (vendors/emeraude-engine/src/Vulkan/DeviceMemory.cpp `EmEn::Vulkan::DeviceMemory::createOnHardware`) Unable to allocate <n> bytes (alignment:<n> bytes) in the device memory <n> : VK_ERROR_OUT_OF_DEVICE_MEMORY !
[VulkanImage][Error] (vendors/emeraude-engine/src/Vulkan/Image.cpp `EmEn::Vulkan::Image::importFromWin32Handle`) Unable to import the external D3D11 texture memory (handle @@0000000000002F64) !
[a line of the downstream application, redacted]
[VulkanDeviceMemory][Error] (vendors/emeraude-engine/src/Vulkan/DeviceMemory.cpp `EmEn::Vulkan::DeviceMemory::createOnHardware`) Unable to allocate <n> bytes (alignment:<n> bytes) in the device memory <n> : VK_ERROR_OUT_OF_DEVICE_MEMORY !
[VulkanImage][Error] (vendors/emeraude-engine/src/Vulkan/Image.cpp `EmEn::Vulkan::Image::importFromWin32Handle`) Unable to import the external D3D11 texture memory (handle @@0000000000002E00) !
[a line of the downstream application, redacted]
[VulkanSwapChain][Fatal] (vendors/emeraude-engine/src/Vulkan/SwapChain.cpp `EmEn::Vulkan::SwapChain::createBaseSwapChain`) Unable to create the swap-chain : VK_ERROR_OUT_OF_DEVICE_MEMORY !
[VulkanSwapChain][Error] (vendors/emeraude-engine/src/Vulkan/SwapChain.cpp `EmEn::Vulkan::SwapChain::recreate`) Unable to recreate the base of the swap-chain !
```

Archives:

- 7 archive(s), in the downstream application's crash-report mirror.
<!-- crash-reports:end -->

## Triage (2026-10-21)

- **Owner**: the engine.
- **Machines**: ≈1 (reports grouped by CPU + RAM + OS fingerprint on 2026-10-21; several reports per machine
  when every launch fails).
- **Reading**: Swap-chain creation answers `VK_ERROR_OUT_OF_DEVICE_MEMORY` (laptop RTX 5070 Ti, driver 595.79).
- **Next step**: Probably the cross-GPU case (hybrid laptop: window on the iGPU, device on the dGPU). Check the selected device against the presenting one.
- **Plan**: `CrashReport Solucius`, wave W5 — a plan of the downstream application.

## 8.0.3 status (checked 2026-10-21)

**Possibly affected.** `8a5bfd58` retries, then exit 98; a cross-GPU swap-chain (hybrid laptop) is not addressed.

Checked against the changes between 8.0.2 and 8.0.3 in the four repositories (the application 1.1.1 → 1.1.2, engine 1.0.0 → 1.0.1, its kernel 1.3.1 → 1.3.2, its frontend `47cbe7cd9` → `c3ab7b390`); nothing relevant after 8.0.3. Proof only comes from the 8.0.3 reports (plan § 5).

## What remains

- Read the journals of the archives counted above (in the downstream application's crash-report mirror).
- Find the defect, reproduce it, fix it where it lives.

## References

- The downstream application's crash-report documentation — what an archive holds, the flight recorder milestones.
- The downstream application's crash-report tool documentation — how this item was produced, how the cause is chosen.
