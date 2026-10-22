---
id: crash-fatal-in-vulkanswapchain-unable-to-create-the-sw-d481af
title: "Crash reports: Fatal in VulkanSwapChain — Unable to create the swap-chain : VK_ERROR_INITIALIZATION_FAILED !"
status: open
priority: unranked
scope: src/Vulkan/SwapChain.cpp
opened: 2026-10-21
tags: [crash, crash-report-auto]
---

# Crash reports: Fatal in VulkanSwapChain — Unable to create the swap-chain : VK_ERROR_INITIALIZATION_FAILED !

> Dropped by app_system's `tools/crash-reports/crash_reports.py`: a cause never seen before in the production crash reports.
> **Triage**: confirm it is a defect, give the file a better name, move it to the repository that owns the defect
> (an engine defect goes to `vendors/emeraude-engine/docs/todo/`, …) and rank it. The block between the
> `crash-reports` markers is rewritten at every run, wherever the item went (it is found by its signature line);
> write outside it. Deleting the item closes the cause: a new report of it is then flagged as a regression.

## Why

<!-- crash-reports:begin -->
| | |
|---|---|
| Reports | 1 (first 2026-10-21, last 2026-10-21) |
| Versions | 8.0.2 ×1 |
| Platforms | windows ×1 |
| OS | Windows 11 Home ×1 |
| GPU (selected) | NVIDIA GeForce RTX 3070 Laptop GPU (driver 572.16) ×1 |
| ANGLE backend | d3d11 ×1 |
| Died at milestone | ApplicationStarted ×1 |

- **Cause (normalized):** `VulkanSwapChain: Unable to create the swap-chain : VK_ERROR_INITIALIZATION_FAILED !`
- **Where:** `vendors/emeraude-engine/src/Vulkan/SwapChain.cpp` — `EmEn::Vulkan::SwapChain::createBaseSwapChain`
- **Signature:** `d481aff0b2611d4e`

Last error before the death (else the last trace), by number of reports:

- ×1 `died at ApplicationStarted`

Traces of the main journal (normalized, latest report; around the Fatal trace when there is one):

```
[VulkanDeviceMemory][Error] (vendors/emeraude-engine/src/Vulkan/DeviceMemory.cpp `EmEn::Vulkan::DeviceMemory::createOnHardware`) Unable to allocate <n> bytes (alignment:<n> bytes) in the device memory <n> : VK_ERROR_OUT_OF_DEVICE_MEMORY !
[VulkanImage][Error] (vendors/emeraude-engine/src/Vulkan/Image.cpp `EmEn::Vulkan::Image::importFromWin32Handle`) Unable to import the external D3D11 texture memory (handle @@0000000000002A78) !
[WebView][Error] (src/UI/WebView.CefRenderHandler.cpp `Mango3D::System::UI::WebView::OnAcceleratedPaint`) [CEF-ACCEL] Unable to import the accelerated frame for 'View3D' (<n><hex>px) !
[VulkanDeviceMemory][Error] (vendors/emeraude-engine/src/Vulkan/DeviceMemory.cpp `EmEn::Vulkan::DeviceMemory::createOnHardware`) Unable to allocate <n> bytes (alignment:<n> bytes) in the device memory <n> : VK_ERROR_OUT_OF_DEVICE_MEMORY !
[VulkanImage][Error] (vendors/emeraude-engine/src/Vulkan/Image.cpp `EmEn::Vulkan::Image::importFromWin32Handle`) Unable to import the external D3D11 texture memory (handle @@00000000000026B8) !
[WebView][Error] (src/UI/WebView.CefRenderHandler.cpp `Mango3D::System::UI::WebView::OnAcceleratedPaint`) [CEF-ACCEL] Unable to import the accelerated frame for 'View3D' (<n><hex>px) !
[VulkanSwapChain][Fatal] (vendors/emeraude-engine/src/Vulkan/SwapChain.cpp `EmEn::Vulkan::SwapChain::createBaseSwapChain`) Unable to create the swap-chain : VK_ERROR_INITIALIZATION_FAILED !
[VulkanSwapChain][Error] (vendors/emeraude-engine/src/Vulkan/SwapChain.cpp `EmEn::Vulkan::SwapChain::recreate`) Unable to recreate the base of the swap-chain !
```

Archives (local mirror under `tools/crash-reports/data/`, extracted next to them in `extracted/`):

- `reports/8.0.2/1298ad41311466a315cbac54b9cc7e07-7b11e234-24ae-4634-ad95-b005a74c956b.zip` → `archives/8.0.2/1298ad41311466a315cbac54b9cc7e07-7b11e234-24ae-4634-ad95-b005a74c956b.zip`
<!-- crash-reports:end -->

## Triage (2026-10-22)

- **Owner**: the engine.
- **Machines**: ≈1 (reports grouped by CPU + RAM + OS fingerprint; several reports per machine when every
  launch fails).
- **Reading**: Windows, a hybrid laptop (RTX 3070 Laptop GPU, driver 572.16): the swap-chain cannot be created during use (`VK_ERROR_INITIALIZATION_FAILED`). Same family as the other swap-chain failures of hybrid laptops (`crash-fatal-in-vulkanswapchain-unable-to-create-the-sw-9703d3`).
- **Next step**: Read the journal: which device presents (the iGPU drives the screen on a hybrid laptop), what triggered the recreation (resize, monitor change, power state).
- **Plan**: `CrashReport Solucius`, wave W5 — app_system's `docs/plans/crashreport-solucius.md`.

## 8.0.3 status (checked 2026-10-22)

**Possibly affected.** Engine `8a5bfd58` (1.0.1): a failed recreation is retried (at most 30 times, ~3 s), then the engine stops as on a lost device (exit 98) — a Fatal no longer; the cross-GPU presentation is not addressed.

Checked against the changes between 8.0.2 and 8.0.3 in the four repositories (app_system 1.1.1 → 1.1.2, engine 1.0.0 → 1.0.1, app_kernel 1.3.1 → 1.3.2, core `47cbe7cd9` → `c3ab7b390`); nothing relevant after 8.0.3. Proof only comes from the 8.0.3 reports (plan § 5).

## What remains

- Read the journals of the archives above (extracted under `tools/crash-reports/data/extracted/`).
- Find the defect, reproduce it, fix it where it lives.

## References

- app_system's `docs/crash-report.md` — what an archive holds, the flight recorder milestones.
- app_system's `tools/crash-reports/README.md` — how this item was produced, how the cause is chosen.
