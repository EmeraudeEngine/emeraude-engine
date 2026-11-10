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
| Machines | 2 — per version: 8.0.4 ×2 |
| Reports | 2 (first 2026-11-06, last 2026-11-09) |
| Versions | 8.0.4 ×2 |
| Platforms | windows ×2 |
| OS | Windows 10 Pro ×1, Windows 11 Pro ×1 |
| GPU (selected) | Intel(R) UHD Graphics 770 (driver 101.6129) ×1, NVIDIA GeForce RTX 3060 (driver 610.62) ×1 |
| ANGLE backend | d3d11 ×2 |
| Died at milestone | ApplicationStarted ×2 |
| Single-instance lock | Acquired ×2 |

- **Cause (normalized):** `EXCEPTION_ACCESS_VIOLATION_READ in the browser process: nvoglv64.dll`
- **Where:** `nvoglv64.dll` (no symbols: a driver or a system library)
- **Signature:** `3890c27630f0c7de`

Last error before the death (else the last trace), by number of reports:

- ×2 `browser process, then the run died at ApplicationStarted`

Traces of the main journal (normalized, latest report; around the Fatal trace when there is one):

```
[GLFW][Error] Win32: Failed to query display settings (errno:<n>)
[GLFW][Error] Win32: Failed to query monitor DPI (errno:<n>)
[WindowService][Info] (vendors/emeraude-engine/src/Window.cpp `EmEn::Window::monitorConfigurationChanged`) New monitor removed from the configuration !
[WindowService][Error] (vendors/emeraude-engine/src/Window.cpp `EmEn::Window::getDesktopSize`) Unable to query the primary monitor ! (×5)
```

Native stacks of the latest report (minidumps, crashing thread; `?` = guessed by stack scanning):

```
EXCEPTION_ACCESS_VIOLATION_READ in the browser process
 0   nvoglv64.dll + 0x0000000000ba5336
 1 ? nvoglv64.dll + 0x0000000000ba4ba1
 2 ? nvoglv64.dll + 0x0000000000aa2afb
 3 ? nvoglv64.dll + 0x0000000000a7ebaf
 4 ? nvoglv64.dll + 0x0000000000af04fc
 5 ? nvoglv64.dll + 0x0000000000aefc5d
 6 ? nvoglv64.dll + 0x0000000000aeffd3
 7 ? nvoglv64.dll + 0x0000000000af670a
 8 ? nvoglv64.dll + 0x0000000000a6310f
 9 ? nvoglv64.dll + 0x0000000000af224a
10 ? nvoglv64.dll + 0x0000000002c3352f
11 ? nvoglv64.dll + 0x0000000000aa2fb5
```

Archives:

- 2 archive(s), in the downstream application's crash-report mirror.
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
