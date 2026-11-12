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

> Dropped by the downstream application's crash-report tool: a cause never seen before in the production crash reports.
> **Triage**: confirm it is a defect, give the file a better name, move it to the repository that owns the defect
> (an engine defect goes to `vendors/emeraude-engine/docs/todo/`, …) and rank it. The block between the
> `crash-reports` markers is rewritten at every run, wherever the item went (it is found by its signature line);
> write outside it. Deleting the item closes the cause: a new report of it is then flagged as a regression.

## Why

<!-- crash-reports:begin -->
| | |
|---|---|
| Machines | 12 — per version: 8.0.2 ×10, 8.0.3 ×2 |
| Reports | 18 (first 2026-10-17, last 2026-11-05) |
| Versions | 8.0.2 ×15, 8.0.3 ×3 |
| Platforms | windows ×18 |
| OS | Windows 10 Pro ×7, Windows 11 Home ×4, Windows 11 Pro ×4, Windows 10 Home ×2, Windows 11 Home Single Language ×1 |
| GPU (selected) | Intel(R) UHD Graphics 610 (driver 101.2111) ×5, AMD Radeon 740M Graphics (driver 2.0.395) ×2, Intel(R) HD Graphics 620 (driver 100.9171) ×2, Intel(R) UHD Graphics (driver 100.9805) ×2, AMD Radeon RX 7900 XTX (driver 2.0.406) ×1, AMD Radeon(TM) 860M Graphics (driver 2.0.373) ×1, … (5 more) |
| ANGLE backend | d3d11 ×18 |
| Died at milestone | ApplicationStarted ×14, Stopping ×3, QuitRequested ×1 |
| Single-instance lock | not recorded ×18 |

- **Cause (normalized):** `VulkanSwapChain: Unable to create the swap-chain : VK_ERROR_OUT_OF_HOST_MEMORY !`
- **Where:** `vendors/emeraude-engine/src/Vulkan/SwapChain.cpp` — `EmEn::Vulkan::SwapChain::createBaseSwapChain`
- **Signature:** `fa4b3be05a76caed`

Last error before the death (else the last trace), by number of reports:

- ×14 `died at ApplicationStarted`
- ×3 `died at Stopping`
- ×1 `died at QuitRequested`

Traces of the main journal (normalized, latest report; around the Fatal trace when there is one):

```
[a line of the downstream application, redacted] (×6)
[VulkanSwapChain][Fatal] (vendors/emeraude-engine/src/Vulkan/SwapChain.cpp `EmEn::Vulkan::SwapChain::createBaseSwapChain`) Unable to create the swap-chain : VK_ERROR_OUT_OF_HOST_MEMORY !
[VulkanSwapChain][Error] (vendors/emeraude-engine/src/Vulkan/SwapChain.cpp `EmEn::Vulkan::SwapChain::recreate`) Unable to recreate the base of the swap-chain !
```

Archives:

- 18 archive(s), in the downstream application's crash-report mirror.
<!-- crash-reports:end -->

## Triage (2026-10-21)

- **Owner**: the engine.
- **Machines**: ≈2 (reports grouped by CPU + RAM + OS fingerprint on 2026-10-21; several reports per machine
  when every launch fails).
- **Reading**: Swap-chain creation answers `VK_ERROR_OUT_OF_HOST_MEMORY` on old Intel UHD drivers (100.9805, 101.2140).
- **Next step**: Read the journals (requested image count / size). Probably a driver limit: a retry with a smaller swap-chain, or a refusal told to the user.
- **Plan**: `CrashReport Solucius`, wave W5 — a plan of the downstream application.

## 8.0.3 status (checked 2026-10-21)

**Likely fixed.** Old Intel UHD drivers, after the application started: engine `39236d0b` (1.0.1) — on an Intel iGPU a minimise reports a 0x0 `maxImageExtent`, the recreation is now skipped, and the swap-chain is created into a local handle (the old one was destroyed twice); `8a5bfd58` retries a failed recreation.

Checked against the changes between 8.0.2 and 8.0.3 in the four repositories (the application 1.1.1 → 1.1.2, engine 1.0.0 → 1.0.1, its kernel 1.3.1 → 1.3.2, its frontend `47cbe7cd9` → `c3ab7b390`); nothing relevant after 8.0.3. Proof only comes from the 8.0.3 reports (plan § 5).

## Journals read (2026-10-22)

6 reports (5 machines), all Intel UHD on old drivers (100.9805, 101.21xx); in all of them a swap-chain RECREATION fails
(`Unable to recreate the base of the swap-chain`), long after the startup, never the first creation. The journals of
engine 1.0.0 do not trace a minimise, so the link with the 0x0-extent fix of `39236d0b` cannot be proven from them:
the closest events before the failure are "Core updated the application surface" (a window resize) and, once, a close
request. **Kept open** while the "likely fixed" stays unproven (the four verified causes of the family were closed on
2026-10-22). Worst case on 1.0.1: the recreation is retried 30 times, then a clean stop with exit 98 (`8a5bfd58`) —
"Changed", not fixed. ⚠️ On 1.0.1 the retried failure still traces a Fatal (`src/Vulkan/SwapChain.cpp`): this signature reappears on 8.0.3
for a session that recovered and was reported for another reason. ⚠️ From 1.1.0 it is an Error
(`docs/tracer-system.md` § Available Severity Levels): this signature can no longer appear, the same failure is signed
by the renderer's Fatal ("Unable to refresh the swap-chain after N consecutive attempts", exit 98) — its absence on a
1.1.0 release proves nothing.

## Journals read (2026-11-11, 3 of 18, 8.0.3, 2 machines)

- The swap-chain creation logs NEITHER the requested extent, image count, format nor present mode, nor which device / queue family presents: none of the 20 journals read can give them. A creation should log them (once per creation, or on a failure).
- None of the six swap-chain signatures appears in 8.0.4 (the VRAM stop, exit 95, and the device-loss stop, 98, now take these runs; not proven).
- Windows 11, AMD proprietary driver: Radeon 740M (2 journals), RX 7900 XTX (1).
- 2 of 3 (740M, at start): **a resize storm** — 15 and 19 "resized during the swap-chain (re)creation" lines, the surface going from 1224x675 to 1x1 and 2x1 px.
- 1 of 3 (7900 XTX, at quit): video memory first (`Queue::submit` OUT_OF_DEVICE_MEMORY, the accelerated copy failing, `acquireNextImage` OUT_OF_DEVICE_MEMORY), then the creation `OUT_OF_HOST_MEMORY`, during a resize.

## What remains

- On the 8.0.3 reports: this signature (or exit 98 after a retried recreation) on an Intel UHD machine means the fix did
  not hold; none on a comparable number of machines closes it (plan § 5).
- If it comes back: a minimise on an Intel iGPU with an old driver (the Windows peer reproduced the 0x0 extent on a
  current one).

## References

- The downstream application's crash-report documentation — what an archive holds, the flight recorder milestones.
- The downstream application's crash-report tool documentation — how this item was produced, how the cause is chosen.
