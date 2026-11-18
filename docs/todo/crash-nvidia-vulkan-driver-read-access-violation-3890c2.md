---
id: crash-nvidia-vulkan-driver-read-access-violation-3890c2
title: "NVIDIA Vulkan driver — read access violation inside the driver"
status: open
priority: high
scope: Vulkan driver
opened: 2026-11-10
tags: [crash, crash-report-auto]
---

# NVIDIA Vulkan driver — read access violation inside the driver

> Dropped by the downstream application's crash-report tool: a cause seen in its production crash reports.
> The block between the `crash-reports` markers is rewritten at every run of that tool (found by its signature line,
> names of the application redacted); write outside it. Deleting the item closes the cause: a new report of it from a
> newer version is then flagged as a regression.

## Why

<!-- crash-reports:begin -->
| | |
|---|---|
| Machines | 3 — per version: 8.0.4 ×3 |
| Reports | 3 (first 2026-11-06, last 2026-11-14) |
| Versions | 8.0.4 ×3 |
| Platforms | windows ×3 |
| OS | Windows 10 Home ×1, Windows 10 Pro ×1, Windows 11 Pro ×1 |
| GPU (selected) | Intel(R) UHD Graphics 770 (driver 101.6129) ×1, NVIDIA GeForce RTX 3060 (driver 610.62) ×1, unknown ×1 |
| ANGLE backend | d3d11 ×3 |
| Died at milestone | ApplicationStarted ×2, CoreStarted ×1 |
| Single-instance lock | Acquired ×3 |

- **Cause (normalized):** `EXCEPTION_ACCESS_VIOLATION_READ in the browser process: nvoglv64.dll`
- **Where:** `nvoglv64.dll` (no symbols: a driver or a system library)
- **Signature:** `3890c27630f0c7de`

Last error before the death (else the last trace), by number of reports:

- ×2 `browser process, then the run died at ApplicationStarted`
- ×1 `browser process, then the run died at CoreStarted`

Traces of the main journal (normalized, latest report; around the Fatal trace when there is one):

```
[SystemNotificationService][Info] (vendors/emeraude-engine/src/SystemNotification.cpp `EmEn::SystemNotification::onInitialize`) System notification service initialized.
[Core][Success] (vendors/emeraude-engine/src/Core.cpp `EmEn::Core::initializeCoreLevel`) SystemNotificationService service up!
[Core][Success] (vendors/emeraude-engine/src/Core.cpp `EmEn::Core::initializeCoreLevel`) OverlayManagerService service up!
[Core][Success] (vendors/emeraude-engine/src/Core.cpp `EmEn::Core::initializeCoreLevel`) SceneManagerService service up!
[Core][Success] (vendors/emeraude-engine/src/Core.cpp `EmEn::Core::initializeCoreLevel`) Engine at core level initialized ! Now initialization at the application level ...
[Core][Success] (vendors/emeraude-engine/src/Core.cpp `EmEn::Core::run`) Engine at application level initialized !
[Core][Info] (vendors/emeraude-engine/src/Core.cpp `EmEn::Core::updateNoScenePacing`) Without an active scene, the Continuous rendering loop is paced at <n> Hz (by the present itself when it blocks).
[Core][Success] (vendors/emeraude-engine/src/Core.cpp `EmEn::Core::run`) Core level execution started !
```

Native stacks of the latest report (minidumps, crashing thread; `?` = guessed by stack scanning):

```
EXCEPTION_ACCESS_VIOLATION_READ in the browser process
 0   nvoglv64.dll + 0x0000000000cb7b30
 1 ? ntdll.dll + 0x000000000002b44c
 2 ? nvoglv64.dll + 0x0000000000c49b2b
 3 ? nvoglv64.dll + 0x0000000000c48ed9
 4 ? nvoglv64.dll + 0x0000000000c7c008
 5 ? KERNELBASE.dll + 0x000000000005f212
 6 ? nvoglv64.dll + 0x0000000000c7b14b
```

Archives:

- 3 archive(s), in the downstream application's crash-report mirror.
<!-- crash-reports:end -->

## Triage (2026-11-10)

- **Owner**: the engine (a driver crash in its process). 2 Windows machines in 8.0.4, during use: an RTX 3060
  (driver 610.62) and a hybrid machine whose SELECTED device is an Intel UHD Graphics 770 — the NVIDIA driver
  (`nvoglv64.dll`, NVIDIA's Vulkan driver) is loaded anyway by the device enumeration and crashes in its own code.
- **Stack**: only `nvoglv64.dll` frames, no engine frame.
- Found on 2026-11-10 once the crash-report tool named the causes of driver crashes without symbols.

## What remains

- With more reports: compare the driver versions; on the hybrid machine, check what calls into the unselected
  NVIDIA driver (the enumeration, a second device for compute, the video encoder).

## References

- The downstream application's crash-report documentation — what an archive holds, the minidumps.
- The downstream application's crash-report tool documentation — how this item was produced, how the cause is chosen.
