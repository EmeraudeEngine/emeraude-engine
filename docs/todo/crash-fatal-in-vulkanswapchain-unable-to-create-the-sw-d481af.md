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

> Dropped by the downstream application's crash-report tool: a cause never seen before in the production crash reports.
> **Triage**: confirm it is a defect, give the file a better name, move it to the repository that owns the defect
> (an engine defect goes to `vendors/emeraude-engine/docs/todo/`, …) and rank it. The block between the
> `crash-reports` markers is rewritten at every run, wherever the item went (it is found by its signature line);
> write outside it. Deleting the item closes the cause: a new report of it is then flagged as a regression.

## Why

<!-- crash-reports:begin -->
| | |
|---|---|
| Machines | 12 — per version: 8.0.2 ×7, 8.0.3 ×6 |
| Reports | 16 (first 2026-10-21, last 2026-11-08) |
| Versions | 8.0.2 ×9, 8.0.3 ×7 |
| Platforms | windows ×10, linux ×6 |
| OS | Ubuntu 24.04.5 LTS ×6, Windows 10 Pro ×3, Windows 11 Home ×3, Windows 11 Pro ×2, Windows 10 Home ×1, Windows 10 Home Single Language ×1 |
| GPU (selected) | unknown ×6, Microsoft Direct3D12 (NVIDIA GeForce RTX 3070 Ti Laptop GPU) (driver 26.0.129.35) ×1, NVIDIA GeForce GTX 1050 Ti (driver 581.80) ×1, NVIDIA GeForce RTX 2070 (driver 617.42) ×1, NVIDIA GeForce RTX 3050 (driver 617.14) ×1, NVIDIA GeForce RTX 3070 Laptop GPU (driver 572.16) ×1, … (5 more) |
| ANGLE backend | d3d11 ×10, vulkan ×6 |
| Died at milestone | Stopping ×9, SessionOpened ×6, ApplicationStarted ×1 |
| Single-instance lock | not recorded ×16 |

- **Cause (normalized):** `VulkanSwapChain: Unable to create the swap-chain : VK_ERROR_INITIALIZATION_FAILED !`
- **Where:** `vendors/emeraude-engine/src/Vulkan/SwapChain.cpp` — `EmEn::Vulkan::SwapChain::createBaseSwapChain`
- **Signature:** `d481aff0b2611d4e`

Last error before the death (else the last trace), by number of reports:

- ×9 `died at Stopping`
- ×6 `died at SessionOpened`
- ×1 `died at ApplicationStarted`

Traces of the main journal (normalized, latest report; around the Fatal trace when there is one):

```
[a line of the downstream application, redacted] (×5)
[VulkanDevice][Error] (vendors/emeraude-engine/src/Vulkan/Device.cpp `EmEn::Vulkan::Device::waitIdle`) Unable to wait the device 00000235DAF132E0 : VK_ERROR_OUT_OF_DEVICE_MEMORY ! Call location: Renderer::recreateSystem()
[VulkanSwapChain][Fatal] (vendors/emeraude-engine/src/Vulkan/SwapChain.cpp `EmEn::Vulkan::SwapChain::createBaseSwapChain`) Unable to create the swap-chain : VK_ERROR_INITIALIZATION_FAILED !
[VulkanSwapChain][Error] (vendors/emeraude-engine/src/Vulkan/SwapChain.cpp `EmEn::Vulkan::SwapChain::recreate`) Unable to recreate the base of the swap-chain !
```

Archives:

- 16 archive(s), in the downstream application's crash-report mirror.
<!-- crash-reports:end -->

## Triage (2026-10-22)

- **Owner**: the engine.
- **Machines**: ≈1 (reports grouped by CPU + RAM + OS fingerprint; several reports per machine when every
  launch fails).
- **Reading**: Windows, a hybrid laptop (RTX 3070 Laptop GPU, driver 572.16): the swap-chain cannot be created during use (`VK_ERROR_INITIALIZATION_FAILED`). Same family as the other swap-chain failures of hybrid laptops (`crash-fatal-in-vulkanswapchain-unable-to-create-the-sw-9703d3`).
- **Next step**: Read the journal: which device presents (the iGPU drives the screen on a hybrid laptop), what triggered the recreation (resize, monitor change, power state).
- **Plan**: `CrashReport Solucius`, wave W5 — a plan of the downstream application.

## 8.0.3 status (checked 2026-10-22)

**Possibly affected.** Engine `8a5bfd58` (1.0.1): a failed recreation is retried (at most 30 times, ~3 s), then the engine stops as on a lost device (exit 98) — a Fatal no longer; the cross-GPU presentation is not addressed.

Checked against the changes between 8.0.2 and 8.0.3 in the four repositories (the application 1.1.1 → 1.1.2, engine 1.0.0 → 1.0.1, its kernel 1.3.1 → 1.3.2, its frontend `47cbe7cd9` → `c3ab7b390`); nothing relevant after 8.0.3. Proof only comes from the 8.0.3 reports (plan § 5).

## Journals read (2026-11-11, 6 of 16, 8.0.3, 6 machines)

- The swap-chain creation logs NEITHER the requested extent, image count, format nor present mode, nor which device / queue family presents: none of the 20 journals read can give them. A creation should log them (once per creation, or on a failure).
- None of the six swap-chain signatures appears in 8.0.4 (the VRAM stop, exit 95, and the device-loss stop, 98, now take these runs; not proven).
- Windows 5, Ubuntu 24.04 1. Triggers: **after a device loss** 3 (DEVICE LOST at `Device::waitIdle`; its marker `transfer:image-layout-transition` is the only one a non-RT run can reach, see `crash-exit-code-98-after-stopping-80d5c2`; then the recreation fails `INITIALIZATION_FAILED`, exit 98 in 2); `waitIdle` answering `OUT_OF_DEVICE_MEMORY` 1; video memory exhausted by D3D11 imports after 23 "resized during (re)creation" lines 1; **the very first creation at startup** 1 (Ubuntu 24.04, Intel ARL on Mesa 25.2.8, llvmpipe also presenting, a simple-framebuffer adapter).
- One machine selected the Dozen translation layer over the native NVIDIA device "(PREFERRED by the application)" — fixed since (translation layers rank last).

## What remains

- Read the journals of the archives counted above (in the downstream application's crash-report mirror).
- Find the defect, reproduce it, fix it where it lives.

## References

- The downstream application's crash-report documentation — what an archive holds, the flight recorder milestones.
- The downstream application's crash-report tool documentation — how this item was produced, how the cause is chosen.
