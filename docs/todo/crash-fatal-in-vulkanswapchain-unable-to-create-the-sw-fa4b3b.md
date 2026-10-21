---
id: crash-fatal-in-vulkanswapchain-unable-to-create-the-sw-fa4b3b
title: "Crash reports: Fatal in VulkanSwapChain — Unable to create the swap-chain : VK_ERROR_OUT_OF_HOST_MEMORY !"
status: open
priority: unranked
scope: src/Vulkan/SwapChain.cpp
opened: 2026-10-21
tags: [crash, crash-report-auto]
---

# Crash reports: Fatal in VulkanSwapChain — Unable to create the swap-chain : VK_ERROR_OUT_OF_HOST_MEMORY !

> Dropped by app_system's `tools/crash-reports/crash_reports.py`: a cause never seen before in the production crash reports.
> **Triage**: confirm it is a defect, give the file a better name, move it to the repository that owns the defect
> (an engine defect goes to `vendors/emeraude-engine/docs/todo/`, …) and rank it. The block between the
> `crash-reports` markers is rewritten at every run, wherever the item went (it is found by its signature line);
> write outside it. Deleting the item closes the cause: a new report of it is then flagged as a regression.

## Why

<!-- crash-reports:begin -->
| | |
|---|---|
| Reports | 3 (first 2026-10-17, last 2026-10-19) |
| Versions | 8.0.2 ×3 |
| Platforms | windows ×3 |
| OS | Windows 11 Home ×2, Windows 11 Home Single Language ×1 |
| GPU (selected) | Intel(R) UHD Graphics (driver 100.9805) ×2, Intel(R) UHD Graphics 600 (driver 101.2140) ×1 |
| ANGLE backend | d3d11 ×3 |
| Died at milestone | ApplicationStarted ×2, Stopping ×1 |

- **Cause (normalized):** `VulkanSwapChain: Unable to create the swap-chain : VK_ERROR_OUT_OF_HOST_MEMORY !`
- **Where:** `vendors/emeraude-engine/src/Vulkan/SwapChain.cpp` — `EmEn::Vulkan::SwapChain::createBaseSwapChain`
- **Signature:** `fa4b3be05a76caed`

Last error before the death (else the last trace), by number of reports:

- ×2 `died at ApplicationStarted`
- ×1 `died at Stopping`

Traces of the main journal (normalized, latest report; around the Fatal trace when there is one):

```
[WebView][Success] (src/UI/WebView.CefLoadHandler.cpp `Mango3D::System::UI::WebView::OnLoadEnd`) WebView 'ViewUI' loaded 'https:<path>' !
[Application][Info] (src/Application.cpp `Mango3D::System::Application::onCoreSurfaceRefreshed`) Core updated the application surface.
[Application][Info] (src/Application.cpp `Mango3D::System::Application::onCoreSurfaceRefreshed`) Core updated the application surface.
[WebView][Info] (src/UI/WebView.CefLoadHandler.cpp `Mango3D::System::UI::WebView::OnLoadStart`) WebView 'ViewUI' loading 'https:<path>' ...
[WebView][Success] (src/UI/WebView.cpp `Mango3D::System::UI::WebView::onBrowserReady`) The webview 'ViewUI' is ready!
[WebView][Success] (src/UI/WebView.CefLoadHandler.cpp `Mango3D::System::UI::WebView::OnLoadEnd`) WebView 'ViewUI' loaded 'https:<path>' !
[VulkanSwapChain][Fatal] (vendors/emeraude-engine/src/Vulkan/SwapChain.cpp `EmEn::Vulkan::SwapChain::createBaseSwapChain`) Unable to create the swap-chain : VK_ERROR_OUT_OF_HOST_MEMORY !
[VulkanSwapChain][Error] (vendors/emeraude-engine/src/Vulkan/SwapChain.cpp `EmEn::Vulkan::SwapChain::recreate`) Unable to recreate the base of the swap-chain !
```

Archives (local mirror under `tools/crash-reports/data/`, extracted next to them in `extracted/`):

- `reports/8.0.2/64a6cf3dedf6e43952fe5eba8969df4e-b28f5da6-1836-46e2-946a-0cedaa9ef200.zip` → `archives/8.0.2/64a6cf3dedf6e43952fe5eba8969df4e-b28f5da6-1836-46e2-946a-0cedaa9ef200.zip`
- `reports/8.0.2/6f3f754fb278cfa73f0eedf5565cc5f1-76a55ebf-b04d-4dfd-b163-47223da838b8.zip` → `archives/8.0.2/6f3f754fb278cfa73f0eedf5565cc5f1-76a55ebf-b04d-4dfd-b163-47223da838b8.zip`
- `reports/8.0.2/f2c134c7e6c5a9e92ab1563c01f9f9da-cb83253a-e971-40f8-9cd1-d2ae6689ebc0.zip` → `archives/8.0.2/f2c134c7e6c5a9e92ab1563c01f9f9da-cb83253a-e971-40f8-9cd1-d2ae6689ebc0.zip`
<!-- crash-reports:end -->

## Triage (2026-10-21)

- **Owner**: the engine.
- **Machines**: ≈2 (reports grouped by CPU + RAM + OS fingerprint on 2026-10-21; several reports per machine
  when every launch fails).
- **Reading**: Swap-chain creation answers `VK_ERROR_OUT_OF_HOST_MEMORY` on old Intel UHD drivers (100.9805, 101.2140).
- **Next step**: Read the journals (requested image count / size). Probably a driver limit: a retry with a smaller swap-chain, or a refusal told to the user.
- **Plan**: `CrashReport Solucius`, wave W5 — app_system's `docs/plans/crashreport-solucius.md`.

## 8.0.3 status (checked 2026-10-21)

**Likely fixed.** Old Intel UHD drivers, after the application started: engine `39236d0b` (1.0.1) — on an Intel iGPU a minimise reports a 0x0 `maxImageExtent`, the recreation is now skipped, and the swap-chain is created into a local handle (the old one was destroyed twice); `8a5bfd58` retries a failed recreation.

Checked against the changes between 8.0.2 and 8.0.3 in the four repositories (app_system 1.1.1 → 1.1.2, engine 1.0.0 → 1.0.1, app_kernel 1.3.1 → 1.3.2, core `47cbe7cd9` → `c3ab7b390`); nothing relevant after 8.0.3. Proof only comes from the 8.0.3 reports (plan § 5).

## What remains

- Read the journals of the archives above (extracted under `tools/crash-reports/data/extracted/`).
- Find the defect, reproduce it, fix it where it lives.

## References

- app_system's `docs/crash-report.md` — what an archive holds, the flight recorder milestones.
- app_system's `tools/crash-reports/README.md` — how this item was produced, how the cause is chosen.
