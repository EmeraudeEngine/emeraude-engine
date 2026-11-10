---
id: crash-exit-code-98-after-stopping-80d5c2
title: "Crash reports: Exit code 98 after Stopping"
status: open
priority: unranked
scope: src/Core.cpp, src/Vulkan/Device.cpp
opened: 2026-10-21
tags: [crash, crash-report-auto]
---

# Crash reports: Exit code 98 after Stopping

> Dropped by the downstream application's crash-report tool: a cause never seen before in the production crash reports.
> **Triage**: confirm it is a defect, give the file a better name, move it to the repository that owns the defect
> (an engine defect goes to `vendors/emeraude-engine/docs/todo/`, …) and rank it. The block between the
> `crash-reports` markers is rewritten at every run, wherever the item went (it is found by its signature line);
> write outside it. Deleting the item closes the cause: a new report of it is then flagged as a regression.

## Why

<!-- crash-reports:begin -->
| | |
|---|---|
| Machines | 67 — per version: 8.0.2 ×28, 8.0.3 ×28, 8.0.4 ×11 |
| Reports | 100 (first 2026-10-16, last 2026-11-10) |
| Versions | 8.0.3 ×48, 8.0.2 ×41, 8.0.4 ×11 |
| Platforms | windows ×91, macos ×7, linux ×2 |
| OS | Windows 11 Pro ×44, Windows 11 Home ×23, Windows 10 Home Single Language ×10, Windows 10 Home ×8, Apple macOS 26.5 ×5, Windows 10 Pro ×4, … (5 more) |
| GPU (selected) | NVIDIA GeForce GTX 1050 Ti (driver 581.80) ×8, Apple M1 (driver MoltenVK 1.4.2) ×7, Intel(R) Arc(TM) 140V GPU (16GB) (driver 101.8132) ×4, NVIDIA GeForce MX150 (driver 511.69) ×4, NVIDIA GeForce RTX 3050 Laptop GPU (driver 592.82) ×4, NVIDIA GeForce RTX 5050 Laptop GPU (driver 573.13) ×4, … (53 more) |
| ANGLE backend | d3d11 ×91, metal ×7, vulkan ×2 |
| Died at milestone | Stopping ×100 |
| Single-instance lock | not recorded ×89, Acquired ×11 |

- **Cause (normalized):** `exit code 98 after Stopping`
- **Where:** no source location
- **Signature:** `80d5c2cb7a36e62d`

Last error before the death (else the last trace), by number of reports:

- ×60 `Core: The graphics device is lost or the renderer is no longer usable : the engine stops now (exit code <n>).`
- ×10 `OverlayManagerService: Failed to process screen 'ApplicationUI' updates!`
- ×10 `UIManagerService: Failed to create the startup error overlay web view !`
- ×9 `WebView: [CEF-ACCEL] Unable to import the accelerated frame for 'View3D' (<n><hex>px) !`
- ×2 `VulkanDevice: DEVICE LOST (Queue::present) — GPU diagnostics follow: [checkpoints] last GPU markers reached per queue: - queue <hex> stage=<hex> marker='t`
- ×2 `WebView: [CEF-ACCEL] Unable to import the accelerated frame for 'ViewFile' (<n><hex>px) !`
- ×1 `UIManagerService: The browser of the web-view 'ViewLib' did not complete its close within <n> ms : left to CEF.`
- ×1 `VulkanDevice: DEVICE LOST (Fence::wait) — GPU diagnostics follow: [device_fault] extension not available on this driver. [checkpoints] last GPU markers re`
- ×1 `WebView: [CEF-ACCEL] Unable to import the accelerated frame for 'View3D' (1835x1042px) !`
- ×1 `WebView: [CEF-ACCEL] Unable to import the accelerated frame for 'View3D' (2561x1560px) !`
- … 3 other(s), 3 report(s)

Traces of the main journal (normalized, latest report; around the Fatal trace when there is one):

```
[PrimaryServices][Success] (vendors/emeraude-engine/src/PrimaryServices.cpp `EmEn::PrimaryServices::Impl::terminate`) NetAPIClientService primary service terminated gracefully! [main]
[PrimaryServices][Success] (vendors/emeraude-engine/src/PrimaryServices.cpp `EmEn::PrimaryServices::Impl::terminate`) SystemInfoService primary service terminated gracefully! [main]
[SettingsService][Success] (vendors/emeraude-engine/src/Settings.cpp `EmEn::Settings::onTerminate`) Settings file saved to "<path>" !
[PrimaryServices][Success] (vendors/emeraude-engine/src/PrimaryServices.cpp `EmEn::PrimaryServices::Impl::terminate`) SettingsService primary service terminated gracefully! [main]
[PrimaryServices][Success] (vendors/emeraude-engine/src/PrimaryServices.cpp `EmEn::PrimaryServices::Impl::terminate`) FileSystemService primary service terminated gracefully! [main]
[PrimaryServices][Success] (vendors/emeraude-engine/src/PrimaryServices.cpp `EmEn::PrimaryServices::Impl::terminate`) UserInfoService primary service terminated gracefully! [main]
[PrimaryServices][Success] (vendors/emeraude-engine/src/PrimaryServices.cpp `EmEn::PrimaryServices::Impl::terminate`) ArgumentsService primary service terminated gracefully! [main]
[Core][Success] (vendors/emeraude-engine/src/Core.cpp `EmEn::Core::~Core`) *** Core level terminated ***
```

Archives:

- 100 archive(s), in the downstream application's crash-report mirror.
<!-- crash-reports:end -->

## Triage (2026-10-21)

- **Owner**: the engine.
- **Machines**: ≈9 (reports grouped by CPU + RAM + OS fingerprint on 2026-10-21; several reports per machine
  when every launch fails).
- **Reading**: Exit code 98 = `Core::DeviceLostExitCode`: the GPU device was lost and the engine stopped cleanly (`stopOnDeviceLoss()`). A real incident (driver reset, TDR), reported on purpose.
- **Next step**: Read the `DEVICE LOST` diagnostics (checkpoints) in the journals: one recurring GPU marker = an engine command to fix; scattered = driver / hardware. Linked: `crash-exit-code-98-after-applicationready-39d5f0`.
- **Plan**: `CrashReport Solucius`, wave W5 — a plan of the downstream application.

## 8.0.3 status (checked 2026-10-21)

**Not addressed.** A device loss is an incident, kept on purpose (exit 98). 8.0.3 changed nothing on its causes.

Checked against the changes between 8.0.2 and 8.0.3 in the four repositories (the application 1.1.1 → 1.1.2, engine 1.0.0 → 1.0.1, its kernel 1.3.1 → 1.3.2, its frontend `47cbe7cd9` → `c3ab7b390`); nothing relevant after 8.0.3. Proof only comes from the 8.0.3 reports (plan § 5).

## What remains

- Read the journals of the archives counted above (in the downstream application's crash-report mirror).
- Find the defect, reproduce it, fix it where it lives.

## References

- The downstream application's crash-report documentation — what an archive holds, the flight recorder milestones.
- The downstream application's crash-report tool documentation — how this item was produced, how the cause is chosen.
