---
id: crash-fatal-in-vulkanswapchain-unable-to-create-the-de-b57f2b
title: "Crash reports: Fatal in VulkanSwapChain — Unable to create the depth buffer #<n> !"
status: open
priority: unranked
scope: src/Vulkan/SwapChain.cpp
opened: 2026-10-21
tags: [crash, crash-report-auto]
---

# Crash reports: Fatal in VulkanSwapChain — Unable to create the depth buffer #<n> !

> Dropped by app_system's `tools/crash-reports/crash_reports.py`: a cause never seen before in the production crash reports.
> **Triage**: confirm it is a defect, give the file a better name, move it to the repository that owns the defect
> (an engine defect goes to `vendors/emeraude-engine/docs/todo/`, …) and rank it. The block between the
> `crash-reports` markers is rewritten at every run, wherever the item went (it is found by its signature line);
> write outside it. Deleting the item closes the cause: a new report of it is then flagged as a regression.

## Why

<!-- crash-reports:begin -->
| | |
|---|---|
| Reports | 2 (first 2026-10-18, last 2026-10-19) |
| Versions | 8.0.2 ×2 |
| Platforms | windows ×2 |
| OS | Windows 10 Pro ×1, Windows 11 Pro ×1 |
| GPU (selected) | AMD Radeon RX 9060 XT (driver 2.0.395) ×1, NVIDIA GeForce RTX 4070 (driver 617.14) ×1 |
| ANGLE backend | d3d11 ×2 |
| Died at milestone | ApplicationStarted ×1, Stopping ×1 |

- **Cause (normalized):** `VulkanSwapChain: Unable to create the depth buffer #<n> !`
- **Where:** `vendors/emeraude-engine/src/Vulkan/SwapChain.cpp` — `EmEn::Vulkan::SwapChain::createImageArray`
- **Signature:** `b57f2b5123b7c98d`

Last error before the death (else the last trace), by number of reports:

- ×1 `died at ApplicationStarted`
- ×1 `died at Stopping`

Traces of the main journal (normalized, latest report; around the Fatal trace when there is one):

```
[VulkanDeviceMemory][Error] (vendors/emeraude-engine/src/Vulkan/DeviceMemory.cpp `EmEn::Vulkan::DeviceMemory::createOnHardware`) Unable to allocate <n> bytes (alignment:<n> bytes) in the device memory <n> : VK_ERROR_OUT_OF_DEVICE_MEMORY !
[VulkanImage][Error] (vendors/emeraude-engine/src/Vulkan/Image.cpp `EmEn::Vulkan::Image::importFromWin32Handle`) Unable to import the external D3D11 texture memory (handle @@<n>) !
[WebView][Error] (src/UI/WebView.CefRenderHandler.cpp `Mango3D::System::UI::WebView::OnAcceleratedPaint`) [CEF-ACCEL] Unable to import the accelerated frame for 'View3D' (<n><hex>px) !
[Application][Info] (src/Application.cpp `Mango3D::System::Application::onCoreSurfaceRefreshed`) Core updated the application surface.
[VulkanImage][Error] (vendors/emeraude-engine/src/Vulkan/Image.cpp `EmEn::Vulkan::Image::createWithVMA`) Unable to create an image with VMA : VK_ERROR_OUT_OF_DEVICE_MEMORY !
[VulkanSwapChain][Error] (vendors/emeraude-engine/src/Vulkan/SwapChain.cpp `EmEn::Vulkan::SwapChain::createDepthStencilBuffer`) Unable to create image 'Frame0DepthStencilBuffer' !
[VulkanSwapChain][Fatal] (vendors/emeraude-engine/src/Vulkan/SwapChain.cpp `EmEn::Vulkan::SwapChain::createImageArray`) Unable to create the depth buffer #<n> !
[VulkanSwapChain][Error] (vendors/emeraude-engine/src/Vulkan/SwapChain.cpp `EmEn::Vulkan::SwapChain::createFramebuffer`) Unable to create the swap-chain images !
```

Archives (local mirror under `tools/crash-reports/data/`, extracted next to them in `extracted/`):

- `reports/8.0.2/be9494886e163e59f82d514315dc3799-40133149-44c3-4d5e-a389-ef7696e36b7d.zip` → `archives/8.0.2/be9494886e163e59f82d514315dc3799-40133149-44c3-4d5e-a389-ef7696e36b7d.zip`
- `reports/8.0.2/53699404bd4d1c727ef35b1dc8b6858b-3cfd0d2b-3081-4361-b93b-7eab8fe3a41c.zip` → `archives/8.0.2/53699404bd4d1c727ef35b1dc8b6858b-3cfd0d2b-3081-4361-b93b-7eab8fe3a41c.zip`
<!-- crash-reports:end -->

## Triage (2026-10-21)

- **Owner**: the engine.
- **Machines**: ≈2 (reports grouped by CPU + RAM + OS fingerprint on 2026-10-21; several reports per machine
  when every launch fails).
- **Reading**: The swap-chain's depth buffer cannot be created (after start or while stopping).
- **Next step**: Read the journals for the size and the format requested; a 0-px or huge framebuffer during a resize?
- **Plan**: `CrashReport Solucius`, wave W5 — app_system's `docs/plans/crashreport-solucius.md`.

## What remains

- Read the journals of the archives above (extracted under `tools/crash-reports/data/extracted/`).
- Find the defect, reproduce it, fix it where it lives.

## References

- app_system's `docs/crash-report.md` — what an archive holds, the flight recorder milestones.
- app_system's `tools/crash-reports/README.md` — how this item was produced, how the cause is chosen.
