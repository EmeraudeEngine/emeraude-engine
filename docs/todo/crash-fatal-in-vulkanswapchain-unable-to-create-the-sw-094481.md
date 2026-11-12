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

> Dropped by the downstream application's crash-report tool: a cause never seen before in the production crash reports.
> **Triage**: confirm it is a defect, give the file a better name, move it to the repository that owns the defect
> (an engine defect goes to `vendors/emeraude-engine/docs/todo/`, …) and rank it. The block between the
> `crash-reports` markers is rewritten at every run, wherever the item went (it is found by its signature line);
> write outside it. Deleting the item closes the cause: a new report of it is then flagged as a regression.

## Why

<!-- crash-reports:begin -->
| | |
|---|---|
| Machines | 3 — per version: 8.0.2 ×2, 8.0.3 ×1 |
| Reports | 3 (first 2026-10-17, last 2026-10-27) |
| Versions | 8.0.2 ×2, 8.0.3 ×1 |
| Platforms | windows ×3 |
| OS | Windows 10 Pro ×2, Windows 11 Home ×1 |
| GPU (selected) | AMD Radeon RX 6700 XT (driver 2.0.353) ×1, AMD Radeon RX 6800 XT (driver 2.0.353) ×1, Intel(R) Iris(R) Xe Graphics (driver 101.7080) ×1 |
| ANGLE backend | d3d11 ×3 |
| Died at milestone | Stopping ×3 |
| Single-instance lock | not recorded ×3 |

- **Cause (normalized):** `VulkanSwapChain: Unable to create the swap-chain : VK_ERROR_DEVICE_LOST !`
- **Where:** `vendors/emeraude-engine/src/Vulkan/SwapChain.cpp` — `EmEn::Vulkan::SwapChain::createBaseSwapChain`
- **Signature:** `094481c5fcd9065f`

Last error before the death (else the last trace), by number of reports:

- ×3 `died at Stopping`

Traces of the main journal (normalized, latest report; around the Fatal trace when there is one):

```
[GLFW][Error] Win32: Failed to query display settings (errno:<n>)
[GLFW][Error] Win32: Failed to query monitor DPI (errno:<n>)
[WindowService][Info] (vendors/emeraude-engine/src/Window.cpp `EmEn::Window::monitorConfigurationChanged`) New monitor removed from the configuration !
[WindowService][Error] (vendors/emeraude-engine/src/Window.cpp `EmEn::Window::getDesktopSize`) Unable to query the primary monitor ! (×3)
[VulkanSwapChain][Fatal] (vendors/emeraude-engine/src/Vulkan/SwapChain.cpp `EmEn::Vulkan::SwapChain::createBaseSwapChain`) Unable to create the swap-chain : VK_ERROR_DEVICE_LOST !
[VulkanSwapChain][Error] (vendors/emeraude-engine/src/Vulkan/SwapChain.cpp `EmEn::Vulkan::SwapChain::recreate`) Unable to recreate the base of the swap-chain !
```

Archives:

- 3 archive(s), in the downstream application's crash-report mirror.
<!-- crash-reports:end -->

## Triage (2026-10-21)

- **Owner**: the engine.
- **Machines**: ≈2 (reports grouped by CPU + RAM + OS fingerprint on 2026-10-21; several reports per machine
  when every launch fails).
- **Reading**: Swap-chain re-creation answers `VK_ERROR_DEVICE_LOST`: the device was already lost when the window changed.
- **Next step**: Route this path to `stopOnDeviceLoss()` (exit 98) instead of a Fatal, if it is not already the case in 1.0.1.
- **Plan**: `CrashReport Solucius`, wave W5 — a plan of the downstream application.

## 8.0.3 status (checked 2026-10-21)

**Possibly affected.** Engine `8a5bfd58` (1.0.1): a failed swap-chain recreation is retried (at most 30 times, ~3 s), then the engine stops as on a lost device (exit 98) — a Fatal no longer. The device itself is still lost.

Checked against the changes between 8.0.2 and 8.0.3 in the four repositories (the application 1.1.1 → 1.1.2, engine 1.0.0 → 1.0.1, its kernel 1.3.1 → 1.3.2, its frontend `47cbe7cd9` → `c3ab7b390`); nothing relevant after 8.0.3. Proof only comes from the 8.0.3 reports (plan § 5).

## Journals read (2026-11-11, 1 of 3, 8.0.3)

- The swap-chain creation logs NEITHER the requested extent, image count, format nor present mode, nor which device / queue family presents: none of the 20 journals read can give them. A creation should log them (once per creation, or on a failure).
- None of the six swap-chain signatures appears in 8.0.4 (the VRAM stop, exit 95, and the device-loss stop, 98, now take these runs; not proven).
- Windows 10, RX 6700 XT (2.0.353), the only device. Trigger: **a monitor change** — two "New monitor removed", three "Unable to query the primary monitor", then the creation DEVICE_LOST and "attempt 1/30" (in the whole run: 3 removals, 3 additions, 9 failed primary-monitor queries).

## What remains

- Read the journals of the archives counted above (in the downstream application's crash-report mirror).
- Find the defect, reproduce it, fix it where it lives.

## References

- The downstream application's crash-report documentation — what an archive holds, the flight recorder milestones.
- The downstream application's crash-report tool documentation — how this item was produced, how the cause is chosen.
