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

> Dropped by the downstream application's crash-report tool: a cause never seen before in the production crash reports.
> **Triage**: confirm it is a defect, give the file a better name, move it to the repository that owns the defect
> (an engine defect goes to `vendors/emeraude-engine/docs/todo/`, …) and rank it. The block between the
> `crash-reports` markers is rewritten at every run, wherever the item went (it is found by its signature line);
> write outside it. Deleting the item closes the cause: a new report of it is then flagged as a regression.

## Why

<!-- crash-reports:begin -->
| | |
|---|---|
| Machines | 9 — per version: 8.0.2 ×4, 8.0.3 ×5 |
| Reports | 9 (first 2026-10-18, last 2026-11-10) |
| Versions | 8.0.3 ×5, 8.0.2 ×4 |
| Platforms | windows ×9 |
| OS | Windows 11 Pro ×6, Windows 10 Pro ×1, Windows 10 Pro for Workstations ×1, Windows 11 Home ×1 |
| GPU (selected) | NVIDIA GeForce RTX 5070 Ti (driver 616.92) ×2, AMD Radeon RX 9060 XT (driver 2.0.395) ×1, AMD Radeon RX 9060 XT (driver 2.0.416) ×1, NVIDIA GeForce GTX 1650 Ti (driver 591.44) ×1, NVIDIA GeForce RTX 4070 (driver 617.14) ×1, NVIDIA GeForce RTX 4090 (driver 610.88) ×1, … (2 more) |
| ANGLE backend | d3d11 ×9 |
| Died at milestone | Stopping ×4, ApplicationStarted ×3, QuitRequested ×2 |
| Single-instance lock | not recorded ×9 |

- **Cause (normalized):** `VulkanSwapChain: Unable to create the depth buffer #<n> !`
- **Where:** `vendors/emeraude-engine/src/Vulkan/SwapChain.cpp` — `EmEn::Vulkan::SwapChain::createImageArray`
- **Signature:** `b57f2b5123b7c98d`

Last error before the death (else the last trace), by number of reports:

- ×4 `died at Stopping`
- ×3 `died at ApplicationStarted`
- ×2 `died at QuitRequested`

Traces of the main journal (normalized, latest report; around the Fatal trace when there is one):

```
[a line of the downstream application, redacted]
[VulkanDeviceMemory][Error] (vendors/emeraude-engine/src/Vulkan/DeviceMemory.cpp `EmEn::Vulkan::DeviceMemory::createOnHardware`) Unable to allocate <n> bytes (alignment:<n> bytes) in the device memory <n> : VK_ERROR_OUT_OF_DEVICE_MEMORY !
[VulkanImage][Error] (vendors/emeraude-engine/src/Vulkan/Image.cpp `EmEn::Vulkan::Image::importFromWin32Handle`) Unable to import the external D3D11 texture memory (handle @@0000000000002C04) !
[a line of the downstream application, redacted]
[VulkanImage][Error] (vendors/emeraude-engine/src/Vulkan/Image.cpp `EmEn::Vulkan::Image::createWithVMA`) Unable to create an image with VMA : VK_ERROR_OUT_OF_DEVICE_MEMORY !
[VulkanSwapChain][Error] (vendors/emeraude-engine/src/Vulkan/SwapChain.cpp `EmEn::Vulkan::SwapChain::createDepthStencilBuffer`) Unable to create image 'Frame0DepthStencilBuffer' !
[VulkanSwapChain][Fatal] (vendors/emeraude-engine/src/Vulkan/SwapChain.cpp `EmEn::Vulkan::SwapChain::createImageArray`) Unable to create the depth buffer #<n> !
[VulkanSwapChain][Error] (vendors/emeraude-engine/src/Vulkan/SwapChain.cpp `EmEn::Vulkan::SwapChain::createFramebuffer`) Unable to create the swap-chain images !
```

Archives:

- 9 archive(s), in the downstream application's crash-report mirror.
<!-- crash-reports:end -->

## Triage (2026-10-21)

- **Owner**: the engine.
- **Machines**: ≈2 (reports grouped by CPU + RAM + OS fingerprint on 2026-10-21; several reports per machine
  when every launch fails).
- **Reading**: The swap-chain's depth buffer cannot be created (after start or while stopping).
- **Next step**: Read the journals for the size and the format requested; a 0-px or huge framebuffer during a resize?
- **Plan**: `CrashReport Solucius`, wave W5 — a plan of the downstream application.

## 8.0.3 status (checked 2026-10-21)

**Possibly affected.** `39236d0b` / `8a5bfd58` change the recreation path; the requested depth extent (a huge width, height 1) is unexplained — unverified.

Checked against the changes between 8.0.2 and 8.0.3 in the four repositories (the application 1.1.1 → 1.1.2, engine 1.0.0 → 1.0.1, its kernel 1.3.1 → 1.3.2, its frontend `47cbe7cd9` → `c3ab7b390`); nothing relevant after 8.0.3. Proof only comes from the 8.0.3 reports (plan § 5).

## Journals read (2026-11-11, 5 of 9, 8.0.3, 5 machines)

- The swap-chain creation logs NEITHER the requested extent, image count, format nor present mode, nor which device / queue family presents: none of the 20 journals read can give them. A creation should log them (once per creation, or on a failure).
- None of the six swap-chain signatures appears in 8.0.4 (the VRAM stop, exit 95, and the device-loss stop, 98, now take these runs; not proven).
- Windows 11; RTX 5070 Ti ×2, RTX 5060, RTX 4090 (+ UHD 750), RTX 5070 Ti Laptop (+ Intel): the dGPU selected each time.
- All 5: **video memory exhausted** — `OUT_OF_DEVICE_MEMORY`, the D3D11 import failing, `Image::createWithVMA` OUT_OF_DEVICE_MEMORY, then "Unable to create image 'Frame<k>DepthStencilBuffer'" (k = 0 ×3, 1, 2). Over the journals: 54 "Unable to complete the framebuffer", 33 creations OUT_OF_DEVICE_MEMORY, one give-up after 30 attempts.

## What remains

- Read the journals of the archives counted above (in the downstream application's crash-report mirror).
- Find the defect, reproduce it, fix it where it lives.

## References

- The downstream application's crash-report documentation — what an archive holds, the flight recorder milestones.
- The downstream application's crash-report tool documentation — how this item was produced, how the cause is chosen.
