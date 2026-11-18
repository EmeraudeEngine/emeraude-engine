---
id: crash-wayland-explicit-sync-missing-buffer-freezes-startup-b45f19
title: "Crash reports: Wayland explicit sync error 'Missing buffer' freezes the main loop at startup on Linux"
status: open
priority: high
scope: src/Window.linux.cpp (Wayland), the swap-chain presentation on Wayland
opened: 2026-11-18
tags: [crash, crash-report-auto, linux, wayland, freeze, startup]
---

# Crash reports: Wayland explicit sync error 'Missing buffer' freezes the main loop at startup on Linux

Triaged 2026-11-18. 2 machines, 3 reports, Linux, the main loop freezes at `ApplicationStarted`.

**Evidence:** in 2 of 3 reports the last error is the compositor's
`libwayland: wp_linux_drm_syncobj_surface_v1#<n>: error <n>: Missing buffer` — a protocol error of Wayland **explicit
synchronization** (`linux-drm-syncobj-v1`): a surface commit carried an acquire/release point without an attached
buffer. A Wayland protocol error is fatal for the connection, which matches the freeze that follows.

**Next.** Check the presentation path on Wayland (WSI / the driver's explicit-sync support, the first commits of the
window before the swap-chain presents), the compositors and drivers of the reports, and whether forcing implicit sync or
X11 avoids it.

## Why

<!-- crash-reports:begin -->
| | |
|---|---|
| Machines | 2 — per version: 8.0.5 ×2 |
| Reports | 3 (first 2026-11-17, last 2026-11-18) |
| Versions | 8.0.5 ×3 |
| Platforms | linux ×3 |
| OS | Arch Linux ×2, Zorin OS 18.1 ×1 |
| GPU (selected) | AMD Radeon RX 6900 XT (RADV NAVI21) (driver 26.2.7) ×2, NVIDIA GeForce GTX 960 (driver 580.178.04) ×1 |
| ANGLE backend | vulkan ×3 |
| Died at milestone | ApplicationStarted ×3 |
| Single-instance lock | Acquired ×3 |

- **Cause (normalized):** `main loop frozen at ApplicationStarted on linux`
- **Where:** no source location
- **Signature:** `b45f19778506a683`

Last error before the death (else the last trace), by number of reports:

- ×2 `Wayland: libwayland: wp_linux_drm_syncobj_surface_v1#<n>: error <n>: Missing buffer`
- ×1 `Time::Elapsed::PrintScopeRealTime: logicsTask : <n> ms`

Traces of the main journal (normalized, latest report; around the Fatal trace when there is one):

```
[VulkanTransferManagerService][Info] (vendors/emeraude-engine/src/Vulkan/TransferManager.cpp `EmEn::Vulkan::TransferManager::releaseIdleStagingBuffers`) <n> staging buffers released (<n> MiB), <n> buffer and <n> image transfer operations kept. (×5)
[VulkanTransferManagerService][Info] (vendors/emeraude-engine/src/Vulkan/TransferManager.cpp `EmEn::Vulkan::TransferManager::releaseIdleStagingBuffers`) <n> staging buffer released (<n> MiB), <n> buffer and <n> image transfer operations kept.
[Wayland][Error] (vendors/emeraude-engine/src/Window.linux.cpp `{anonymous}::waylandLogHandler`) libwayland: wp_linux_drm_syncobj_surface_v1#<n>: error <n>: Missing buffer
[a line of the downstream application, redacted]
```

Archives:

- 3 archive(s), in the downstream application's crash-report mirror.
<!-- crash-reports:end -->

## What remains

- Read the journals of the archives counted above (in the downstream application's crash-report mirror).
- Find the defect, reproduce it, fix it where it lives.

## References

- The downstream application's crash-report documentation — what an archive holds, the flight recorder milestones.
- The downstream application's crash-report tool documentation — how this item was produced, how the cause is chosen.
