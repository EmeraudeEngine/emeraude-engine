---
id: crash-intel-vulkan-driver-write-access-violation-feb826
title: "Intel Vulkan driver — write access violation in a driver thread"
status: open
priority: high
scope: Vulkan driver
opened: 2026-11-10
tags: [crash, crash-report-auto]
---

# Intel Vulkan driver — write access violation in a driver thread

> Dropped by the downstream application's crash-report tool: a cause seen in its production crash reports.
> The block between the `crash-reports` markers is rewritten at every run of that tool (found by its signature line,
> names of the application redacted); write outside it. Deleting the item closes the cause: a new report of it from a
> newer version is then flagged as a regression.

## Why

<!-- crash-reports:begin -->
| | |
|---|---|
| Machines | 1 — per version: 8.0.4 ×1 |
| Reports | 1 (first 2026-11-04, last 2026-11-04) |
| Versions | 8.0.4 ×1 |
| Platforms | windows ×1 |
| OS | Windows 11 Home ×1 |
| GPU (selected) | Intel(R) Iris(R) Xe Graphics (driver 101.4502) ×1 |
| ANGLE backend | d3d11 ×1 |
| Died at milestone | ApplicationStarted ×1 |
| Single-instance lock | Acquired ×1 |

- **Cause (normalized):** `EXCEPTION_ACCESS_VIOLATION_WRITE in the browser process: igvk64.dll`
- **Where:** `igvk64.dll` (no symbols: a driver or a system library)
- **Signature:** `feb8269e564c269c`

Last error before the death (else the last trace), by number of reports:

- ×1 `browser process, then the run died at ApplicationStarted`

Traces of the main journal (normalized, latest report; around the Fatal trace when there is one):

```
[SystemNotificationService][Info] (vendors/emeraude-engine/src/SystemNotification.cpp `EmEn::SystemNotification::requestPermission`) User granted notification permission.
[VulkanTransferManagerService][Info] (vendors/emeraude-engine/src/Vulkan/TransferManager.cpp `EmEn::Vulkan::TransferManager::releaseIdleStagingBuffers`) <n> staging buffer released (<n> MiB), <n> buffer and <n> image transfer operations kept. (×6)
[VulkanTransferManagerService][Info] (vendors/emeraude-engine/src/Vulkan/TransferManager.cpp `EmEn::Vulkan::TransferManager::releaseIdleStagingBuffers`) <n> staging buffer released (<n>.28882e-<n> MiB), <n> buffer and <n> image transfer operations kept.
```

Native stacks of the latest report (minidumps, crashing thread; `?` = guessed by stack scanning):

```
EXCEPTION_ACCESS_VIOLATION_WRITE in the browser process
 0   igvk64.dll + 0x0000000000360c55
 1 ? igvk64.dll + 0x00000000003ef37b
 2 ? igvk64.dll + 0x00000000009686d8
 3 ? igvk64.dll + 0x000000000090ae68
 4 ? RPCRT4.dll + 0x00000000000333ec
 5 ? igvk64.dll + 0x000000000090e22f
 6 ? igvk64.dll + 0x00000000009686bf
 7 ? igvk64.dll + 0x000000000090d464
 8 ? igvk64.dll + 0x00000000009686bf
 9 ? igvk64.dll + 0x00000000009686bf
10 ? igvk64.dll + 0x000000000090d34f
11 ? ntdll.dll + 0x0000000000123895
```

Archives:

- 1 archive(s), in the downstream application's crash-report mirror.
<!-- crash-reports:end -->

## Triage (2026-11-10)

- **Owner**: the engine (a driver crash in its process). 1 Windows machine (Intel Iris Xe, driver 101.4502), 1 report
  in 8.0.4, during use.
- **Stack**: only `igvk64.dll` frames (and `RPCRT4.dll`): a thread of the driver itself, no engine frame. The driver
  version is old (101.4502); a newer one may fix it.
- Found on 2026-11-10 once the crash-report tool named the causes of driver crashes without symbols.

## What remains

- Watch the count on later reports; with more machines, compare the driver versions (an old-driver warning at
  startup may be the only lever).

## References

- The downstream application's crash-report documentation — what an archive holds, the minidumps.
- The downstream application's crash-report tool documentation — how this item was produced, how the cause is chosen.
