---
id: crash-fatal-in-vulkanswapchain-unable-to-create-the-sw-094481
title: "Crash reports: Fatal in VulkanSwapChain — Unable to create the swap-chain : VK_ERROR_DEVICE_LOST !"
status: open
priority: unranked
scope: src/Vulkan/SwapChain.cpp
opened: 2026-10-21
tags: [crash, crash-report-auto]
---

# Crash reports: Fatal in VulkanSwapChain — Unable to create the swap-chain : VK_ERROR_DEVICE_LOST !

> Dropped by app_system's `tools/crash-reports/crash_reports.py`: a cause never seen before in the production crash reports.
> **Triage**: confirm it is a defect, give the file a better name, move it to the repository that owns the defect
> (an engine defect goes to `vendors/emeraude-engine/docs/todo/`, …) and rank it. The block between the
> `crash-reports` markers is rewritten at every run, wherever the item went (it is found by its signature line);
> write outside it. Deleting the item closes the cause: a new report of it is then flagged as a regression.

## Why

<!-- crash-reports:begin -->
| | |
|---|---|
| Reports | 2 (first 2026-10-17, last 2026-10-21) |
| Versions | 8.0.2 ×2 |
| Platforms | windows ×2 |
| OS | Windows 10 Pro ×1, Windows 11 Home ×1 |
| GPU (selected) | AMD Radeon RX 6800 XT (driver 2.0.353) ×1, Intel(R) Iris(R) Xe Graphics (driver 101.7080) ×1 |
| ANGLE backend | d3d11 ×2 |
| Died at milestone | Stopping ×2 |

- **Cause (normalized):** `VulkanSwapChain: Unable to create the swap-chain : VK_ERROR_DEVICE_LOST !`
- **Where:** `vendors/emeraude-engine/src/Vulkan/SwapChain.cpp` — `EmEn::Vulkan::SwapChain::createBaseSwapChain`
- **Signature:** `094481c5fcd9065f`

Last error before the death (else the last trace), by number of reports:

- ×2 `died at Stopping`

Traces of the main journal (normalized, latest report; around the Fatal trace when there is one):

```
[WindowService][Info] (vendors/emeraude-engine/src/Window.cpp `EmEn::Window::monitorConfigurationChanged`) New monitor removed from the configuration !
[WindowService][Error] (vendors/emeraude-engine/src/Window.cpp `EmEn::Window::getDesktopSize`) Unable to query the primary monitor !
[WindowService][Error] (vendors/emeraude-engine/src/Window.cpp `EmEn::Window::getDesktopSize`) Unable to query the primary monitor !
[WindowService][Error] (vendors/emeraude-engine/src/Window.cpp `EmEn::Window::getDesktopSize`) Unable to query the primary monitor !
[WindowService][Error] (vendors/emeraude-engine/src/Window.cpp `EmEn::Window::getDesktopSize`) Unable to query the primary monitor !
[WindowService][Error] (vendors/emeraude-engine/src/Window.cpp `EmEn::Window::getDesktopSize`) Unable to query the primary monitor !
[VulkanSwapChain][Fatal] (vendors/emeraude-engine/src/Vulkan/SwapChain.cpp `EmEn::Vulkan::SwapChain::createBaseSwapChain`) Unable to create the swap-chain : VK_ERROR_DEVICE_LOST !
[VulkanSwapChain][Error] (vendors/emeraude-engine/src/Vulkan/SwapChain.cpp `EmEn::Vulkan::SwapChain::recreate`) Unable to recreate the base of the swap-chain !
```

Archives (local mirror under `tools/crash-reports/data/`, extracted next to them in `extracted/`):

- `reports/8.0.2/80c50dacdf90f2d2c76a485bb5940825-6f777b89-1551-41e6-b553-4fab4cc50806.zip` → `archives/8.0.2/80c50dacdf90f2d2c76a485bb5940825-6f777b89-1551-41e6-b553-4fab4cc50806.zip`
- `reports/8.0.2/3e4e199ddd844b540d67f1e987292e91-d58f1c5b-21cc-4f3e-b98f-b1a36c964607.zip` → `archives/8.0.2/3e4e199ddd844b540d67f1e987292e91-d58f1c5b-21cc-4f3e-b98f-b1a36c964607.zip`
<!-- crash-reports:end -->

## Triage (2026-10-21)

- **Owner**: the engine.
- **Machines**: ≈2 (reports grouped by CPU + RAM + OS fingerprint on 2026-10-21; several reports per machine
  when every launch fails).
- **Reading**: Swap-chain re-creation answers `VK_ERROR_DEVICE_LOST`: the device was already lost when the window changed.
- **Next step**: Route this path to `stopOnDeviceLoss()` (exit 98) instead of a Fatal, if it is not already the case in 1.0.1.
- **Plan**: `CrashReport Solucius`, wave W5 — app_system's `docs/plans/crashreport-solucius.md`.

## What remains

- Read the journals of the archives above (extracted under `tools/crash-reports/data/extracted/`).
- Find the defect, reproduce it, fix it where it lives.

## References

- app_system's `docs/crash-report.md` — what an archive holds, the flight recorder milestones.
- app_system's `tools/crash-reports/README.md` — how this item was produced, how the cause is chosen.
