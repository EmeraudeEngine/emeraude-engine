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

> Dropped by app_system's `tools/crash-reports/crash_reports.py`: a cause never seen before in the production crash reports.
> **Triage**: confirm it is a defect, give the file a better name, move it to the repository that owns the defect
> (an engine defect goes to `vendors/emeraude-engine/docs/todo/`, …) and rank it. The block between the
> `crash-reports` markers is rewritten at every run, wherever the item went (it is found by its signature line);
> write outside it. Deleting the item closes the cause: a new report of it is then flagged as a regression.

## Why

<!-- crash-reports:begin -->
| | |
|---|---|
| Machines | 1 — per version: 8.0.2 ×1 |
| Reports | 1 (first 2026-10-20, last 2026-10-20) |
| Versions | 8.0.2 ×1 |
| Platforms | windows ×1 |
| OS | Windows 11 Pro ×1 |
| GPU (selected) | NVIDIA GeForce RTX 5070 Ti Laptop GPU (driver 595.79) ×1 |
| ANGLE backend | d3d11 ×1 |
| Died at milestone | ApplicationStarted ×1 |
| Single-instance lock | not recorded ×1 |

- **Cause (normalized):** `VulkanSwapChain: Unable to create the swap-chain : VK_ERROR_OUT_OF_DEVICE_MEMORY !`
- **Where:** `vendors/emeraude-engine/src/Vulkan/SwapChain.cpp` — `EmEn::Vulkan::SwapChain::createBaseSwapChain`
- **Signature:** `9703d338859b9c76`

Last error before the death (else the last trace), by number of reports:

- ×1 `died at ApplicationStarted`

Traces of the main journal (normalized, latest report; around the Fatal trace when there is one):

```
[VulkanDeviceMemory][Error] (vendors/emeraude-engine/src/Vulkan/DeviceMemory.cpp `EmEn::Vulkan::DeviceMemory::createOnHardware`) Unable to allocate <n> bytes (alignment:<n> bytes) in the device memory <n> : VK_ERROR_OUT_OF_DEVICE_MEMORY !
[VulkanImage][Error] (vendors/emeraude-engine/src/Vulkan/Image.cpp `EmEn::Vulkan::Image::importFromWin32Handle`) Unable to import the external D3D11 texture memory (handle @@<n>) !
[WebView][Error] (src/UI/WebView.CefRenderHandler.cpp `Mango3D::System::UI::WebView::OnAcceleratedPaint`) [CEF-ACCEL] Unable to import the accelerated frame for 'View3D' (<n><hex>px) !
[VulkanDeviceMemory][Error] (vendors/emeraude-engine/src/Vulkan/DeviceMemory.cpp `EmEn::Vulkan::DeviceMemory::createOnHardware`) Unable to allocate <n> bytes (alignment:<n> bytes) in the device memory <n> : VK_ERROR_OUT_OF_DEVICE_MEMORY !
[VulkanImage][Error] (vendors/emeraude-engine/src/Vulkan/Image.cpp `EmEn::Vulkan::Image::importFromWin32Handle`) Unable to import the external D3D11 texture memory (handle @@000000000000222C) !
[WebView][Error] (src/UI/WebView.CefRenderHandler.cpp `Mango3D::System::UI::WebView::OnAcceleratedPaint`) [CEF-ACCEL] Unable to import the accelerated frame for 'View3D' (<n><hex>px) !
[VulkanSwapChain][Fatal] (vendors/emeraude-engine/src/Vulkan/SwapChain.cpp `EmEn::Vulkan::SwapChain::createBaseSwapChain`) Unable to create the swap-chain : VK_ERROR_OUT_OF_DEVICE_MEMORY !
[VulkanSwapChain][Error] (vendors/emeraude-engine/src/Vulkan/SwapChain.cpp `EmEn::Vulkan::SwapChain::recreate`) Unable to recreate the base of the swap-chain !
```

Archives (local mirror under `tools/crash-reports/data/`, extracted next to them in `extracted/`):

- `reports/8.0.2/fb2a3f110f68eacfc3eaf1f125a45eba-49104b42-bfb1-4193-b24d-ef89f1256c89.zip` → `archives/8.0.2/fb2a3f110f68eacfc3eaf1f125a45eba-49104b42-bfb1-4193-b24d-ef89f1256c89.zip`
<!-- crash-reports:end -->

## Triage (2026-10-21)

- **Owner**: the engine.
- **Machines**: ≈1 (reports grouped by CPU + RAM + OS fingerprint on 2026-10-21; several reports per machine
  when every launch fails).
- **Reading**: Swap-chain creation answers `VK_ERROR_OUT_OF_DEVICE_MEMORY` (laptop RTX 5070 Ti, driver 595.79).
- **Next step**: Probably the cross-GPU case (hybrid laptop: window on the iGPU, device on the dGPU). Check the selected device against the presenting one.
- **Plan**: `CrashReport Solucius`, wave W5 — app_system's `docs/plans/crashreport-solucius.md`.

## 8.0.3 status (checked 2026-10-21)

**Possibly affected.** `8a5bfd58` retries, then exit 98; a cross-GPU swap-chain (hybrid laptop) is not addressed.

Checked against the changes between 8.0.2 and 8.0.3 in the four repositories (app_system 1.1.1 → 1.1.2, engine 1.0.0 → 1.0.1, app_kernel 1.3.1 → 1.3.2, core `47cbe7cd9` → `c3ab7b390`); nothing relevant after 8.0.3. Proof only comes from the 8.0.3 reports (plan § 5).

## What remains

- Read the journals of the archives above (extracted under `tools/crash-reports/data/extracted/`).
- Find the defect, reproduce it, fix it where it lives.

## References

- app_system's `docs/crash-report.md` — what an archive holds, the flight recorder milestones.
- app_system's `tools/crash-reports/README.md` — how this item was produced, how the cause is chosen.
