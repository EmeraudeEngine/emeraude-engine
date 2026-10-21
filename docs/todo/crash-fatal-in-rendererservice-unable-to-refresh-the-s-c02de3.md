---
id: crash-fatal-in-rendererservice-unable-to-refresh-the-s-c02de3
title: "Crash reports: Fatal in RendererService — Unable to refresh the swap-chain!"
status: open
priority: unranked
scope: src/Graphics/Renderer.cpp
opened: 2026-10-21
tags: [crash, crash-report-auto]
---

# Crash reports: Fatal in RendererService — Unable to refresh the swap-chain!

> Dropped by app_system's `tools/crash-reports/crash_reports.py`: a cause never seen before in the production crash reports.
> **Triage**: confirm it is a defect, give the file a better name, move it to the repository that owns the defect
> (an engine defect goes to `vendors/emeraude-engine/docs/todo/`, …) and rank it. The block between the
> `crash-reports` markers is rewritten at every run, wherever the item went (it is found by its signature line);
> write outside it. Deleting the item closes the cause: a new report of it is then flagged as a regression.

## Why

<!-- crash-reports:begin -->
| | |
|---|---|
| Reports | 3 (first 2026-10-15, last 2026-10-21) |
| Versions | 8.0.2 ×3 |
| Platforms | linux ×3 |
| OS | Debian GNU/Linux 13 (trixie) ×1, Ubuntu 26.04.1 LTS ×1, Zorin OS 18.1 ×1 |
| GPU (selected) | Intel(R) HD Graphics 520 (SKL GT2) (driver 25.2.8) ×1, Intel(R) Iris(R) Xe Graphics (TGL GT2) (driver 25.0.7) ×1, Intel(R) Iris(R) Xe Graphics (TGL GT2) (driver 26.0.8) ×1 |
| ANGLE backend | vulkan ×3 |
| Died at milestone | Stopping ×2, ApplicationStarted ×1 |

- **Cause (normalized):** `RendererService: Unable to refresh the swap-chain!`
- **Where:** `vendors/emeraude-engine/src/Graphics/Renderer.cpp` — `EmEn::Graphics::Renderer::renderFrame`
- **Signature:** `c02de321f3e2fc4c`

Last error before the death (else the last trace), by number of reports:

- ×2 `died at Stopping`
- ×1 `died at ApplicationStarted`

Traces of the main journal (normalized, latest report; around the Fatal trace when there is one):

```
[OverlaySurface][Success] (vendors/emeraude-engine/src/Overlay/Surface.cpp `EmEn::Overlay::Surface::commitTransitionBuffer`) Surface 'ViewUI' transition buffer committed (3405x1384px, scale:<n>).
[OverlaySurface][Success] (vendors/emeraude-engine/src/Overlay/Surface.cpp `EmEn::Overlay::Surface::commitTransitionBuffer`) Surface 'ViewLib' transition buffer committed (3x1px, scale:<n>).
[VulkanPhysicalDevice][Error] (vendors/emeraude-engine/src/Vulkan/PhysicalDevice.cpp `EmEn::Vulkan::PhysicalDevice::getSurfaceFormats`) Unable to get surface format count : VK_ERROR_SURFACE_LOST_KHR !
[VulkanPhysicalDevice][Error] (vendors/emeraude-engine/src/Vulkan/PhysicalDevice.cpp `EmEn::Vulkan::PhysicalDevice::getSurfacePresentModes`) Unable to get present mode count : VK_ERROR_SURFACE_LOST_KHR !
[VulkanSwapChain][Error] (vendors/emeraude-engine/src/Vulkan/SwapChain.cpp `EmEn::Vulkan::SwapChain::chooseSurfaceFormat`) The surface reports no format (lost or not presentable by this device) ! Unable to create the swap-chain.
[VulkanSwapChain][Error] (vendors/emeraude-engine/src/Vulkan/SwapChain.cpp `EmEn::Vulkan::SwapChain::recreate`) Unable to recreate the base of the swap-chain !
[RendererService][Fatal] (vendors/emeraude-engine/src/Graphics/Renderer.cpp `EmEn::Graphics::Renderer::renderFrame`) Unable to refresh the swap-chain!
```

Archives (local mirror under `tools/crash-reports/data/`, extracted next to them in `extracted/`):

- `reports/8.0.2/d24ec66a3458545d7f13686139ef8408-17ef72ed-46da-4255-9dab-d9a9fdc182fa.zip` → `archives/8.0.2/d24ec66a3458545d7f13686139ef8408-17ef72ed-46da-4255-9dab-d9a9fdc182fa.zip`
- `reports/8.0.2/385b83920ce1b488bff71c380a9d637f-c55ea309-f825-47f0-8f11-5dfb991e6eea.zip` → `archives/8.0.2/385b83920ce1b488bff71c380a9d637f-c55ea309-f825-47f0-8f11-5dfb991e6eea.zip`
- `reports/8.0.2/ec13a09beaf8739d3ced0493fb60965f-e600ffdc-28c1-40ad-b8a1-dbf00f039d33.zip` → `archives/8.0.2/ec13a09beaf8739d3ced0493fb60965f-e600ffdc-28c1-40ad-b8a1-dbf00f039d33.zip`
<!-- crash-reports:end -->

## Triage (2026-10-21)

- **Owner**: the engine.
- **Machines**: ≈3 (reports grouped by CPU + RAM + OS fingerprint on 2026-10-21; several reports per machine
  when every launch fails).
- **Reading**: Linux, Intel iGPU (Mesa 25/26): the swap-chain cannot be refreshed (resize / out-of-date).
- **Next step**: Read the journals for the `vkCreateSwapchainKHR` / acquire result; Wayland or X11?
- **Plan**: `CrashReport Solucius`, wave W5 — app_system's `docs/plans/crashreport-solucius.md`.

## What remains

- Read the journals of the archives above (extracted under `tools/crash-reports/data/extracted/`).
- Find the defect, reproduce it, fix it where it lives.

## References

- app_system's `docs/crash-report.md` — what an archive holds, the flight recorder milestones.
- app_system's `tools/crash-reports/README.md` — how this item was produced, how the cause is chosen.
