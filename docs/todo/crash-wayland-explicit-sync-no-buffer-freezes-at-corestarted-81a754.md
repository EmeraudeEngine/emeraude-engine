---
id: crash-wayland-explicit-sync-no-buffer-freezes-at-corestarted-81a754
title: "Crash reports: Wayland explicit sync error 'no buffer attached' freezes the main loop at CoreStarted on Linux"
status: open
priority: high
scope: src/Window.linux.cpp (Wayland), the swap-chain presentation on Wayland
opened: 2026-11-19
tags: [crash, crash-report-auto, linux, wayland, freeze, startup]
---

# Crash reports: Wayland explicit sync error 'no buffer attached' freezes the main loop at CoreStarted on Linux

## Triage

Triaged 2026-11-19. 1 report, Fedora Linux 44 Workstation, NVIDIA RTX 3060 (driver 615.71.09), the main
loop frozen at `CoreStarted` (an earlier milestone than the reports of the sibling item).

- Same class as `crash-wayland-explicit-sync-missing-buffer-freezes-startup-b45f19`: the compositor reports
  `wp_linux_drm_syncobj_surface_v1#<n>: error <n>: Release or Acquire point set but no buffer attached` (another wording
  of the same explicit-sync protocol error), the swap-chain then gets `VK_ERROR_SURFACE_LOST_KHR` and the main loop
  stops cycling.
- **Expected on that release**: the Linux engine it shipped predates the fixes of 2026-11-14 — its symbols contain
  neither `Window::refuseDispatchOnLostDisplay` nor `Window::deliverRecordedEvents` (checked 2026-11-19 in the
  release's Breakpad symbols). Not a fix that failed.

## Why

<!-- crash-reports:begin -->
| | |
|---|---|
| Machines | 1 — per version: 8.0.5 ×1 |
| Reports | 1 (first 2026-11-19, last 2026-11-19) |
| Versions | 8.0.5 ×1 |
| Platforms | linux ×1 |
| OS | Fedora Linux 44 (Workstation Edition) ×1 |
| GPU (selected) | NVIDIA GeForce RTX 3060 (driver 615.71.09) ×1 |
| ANGLE backend | vulkan ×1 |
| Died at milestone | CoreStarted ×1 |
| Single-instance lock | Acquired ×1 |

- **Cause (normalized):** `main loop frozen at CoreStarted on linux`
- **Where:** no source location
- **Signature:** `81a754c6a3e9d022`

Last error before the death (else the last trace), by number of reports:

- ×1 `VulkanSwapChain: Error from the swap-chain : VK_ERROR_SURFACE_LOST_KHR !`

Traces of the main journal (normalized, latest report; around the Fatal trace when there is one):

```
[a line of the downstream application, redacted] (×4)
[Wayland][Error] (vendors/emeraude-engine/src/Window.linux.cpp `{anonymous}::waylandLogHandler`) libwayland: wp_linux_drm_syncobj_surface_v1#<n>: error <n>: Release or Acquire point set but no buffer attached
[VulkanSwapChain][Error] (vendors/emeraude-engine/src/Vulkan/SwapChain.cpp `EmEn::Vulkan::SwapChain::acquireNextImage`) Error from the swap-chain : VK_ERROR_SURFACE_LOST_KHR !
[VulkanSwapChain][Warning] (vendors/emeraude-engine/src/Vulkan/SwapChain.cpp `EmEn::Vulkan::SwapChain::acquireNextImage`) The WINDOWING SURFACE is gone, not the GPU — do not look for a device fault. On Wayland this is usually a compositor protocol error that killed the display connection, printed on stderr by libwayland 
[a line of the downstream application, redacted]
```

Archives:

- 1 archive(s), in the downstream application's crash-report mirror.
<!-- crash-reports:end -->

## What remains

- Close it together with `b45f19`, on the reports of the first release built after 2026-11-14.

## References

- The downstream application's crash-report documentation — what an archive holds, the flight recorder milestones.
- The downstream application's crash-report tool documentation — how this item was produced, how the cause is chosen.
