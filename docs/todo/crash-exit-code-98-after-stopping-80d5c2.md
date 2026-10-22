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

> Dropped by app_system's `tools/crash-reports/crash_reports.py`: a cause never seen before in the production crash reports.
> **Triage**: confirm it is a defect, give the file a better name, move it to the repository that owns the defect
> (an engine defect goes to `vendors/emeraude-engine/docs/todo/`, …) and rank it. The block between the
> `crash-reports` markers is rewritten at every run, wherever the item went (it is found by its signature line);
> write outside it. Deleting the item closes the cause: a new report of it is then flagged as a regression.

## Why

<!-- crash-reports:begin -->
| | |
|---|---|
| Reports | 10 (first 2026-10-16, last 2026-10-21) |
| Versions | 8.0.2 ×10 |
| Platforms | windows ×10 |
| OS | Windows 11 Pro ×7, Windows 11 Home ×3 |
| GPU (selected) | AMD Radeon RX 9060 XT (driver 2.0.395) ×2, Microsoft Direct3D12 (NVIDIA GeForce RTX 4060 Laptop GPU) (driver 26.0.129.35) ×1, NVIDIA GeForce RTX 2070 with Max-Q Design (driver 555.97) ×1, NVIDIA GeForce RTX 3070 (driver 610.88) ×1, NVIDIA GeForce RTX 4070 Ti SUPER (driver 610.88) ×1, NVIDIA GeForce RTX 5070 (driver 591.86) ×1, … (3 more) |
| ANGLE backend | d3d11 ×10 |
| Died at milestone | Stopping ×10 |
| Single-instance lock | not recorded ×10 |

- **Cause (normalized):** `exit code 98 after Stopping`
- **Where:** no source location
- **Signature:** `80d5c2cb7a36e62d`

Last error before the death (else the last trace), by number of reports:

- ×4 `Core: The graphics device is lost or the renderer is no longer usable : the engine stops now (exit code <n>).`
- ×3 `UIManagerService: Failed to create the startup error overlay web view !`
- ×1 `VulkanDevice: DEVICE LOST (Queue::present) — GPU diagnostics follow: [checkpoints] last GPU markers reached per queue: - queue <hex> stage=<hex> marker='t`
- ×1 `WebView: [CEF-ACCEL] Unable to import the accelerated frame for 'View3D' (2561x1566px) !`
- ×1 `WebView: [CEF-ACCEL] Unable to import the accelerated frame for 'View3D' (<n><hex>px) !`

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

Archives (local mirror under `tools/crash-reports/data/`, extracted next to them in `extracted/`):

- `reports/8.0.2/1ff2a88b0a72f9cd1dcb786f695edba5-fc655a2b-63f6-4539-aaa0-878807a0784b.zip` → `archives/8.0.2/1ff2a88b0a72f9cd1dcb786f695edba5-fc655a2b-63f6-4539-aaa0-878807a0784b.zip`
- `reports/8.0.2/adb0faab09fdc7c7bc36a6b219a18a47-f406d643-c4f8-4082-9c3e-5a6b2d45620d.zip` → `archives/8.0.2/adb0faab09fdc7c7bc36a6b219a18a47-f406d643-c4f8-4082-9c3e-5a6b2d45620d.zip`
- `reports/8.0.2/c9e1f4af8803dfb8d49e41eec47c96af-b26e3efc-c9a1-477f-8c82-d15017fe7358.zip` → `archives/8.0.2/c9e1f4af8803dfb8d49e41eec47c96af-b26e3efc-c9a1-477f-8c82-d15017fe7358.zip`
- `reports/8.0.2/003f5747fa5a01fc8f4e1f8047b8d47c-c8f6236c-c753-4b80-8f9a-14347fadfae3.zip` → `archives/8.0.2/003f5747fa5a01fc8f4e1f8047b8d47c-c8f6236c-c753-4b80-8f9a-14347fadfae3.zip`
- `reports/8.0.2/dd99392eacbe3bfab479706e0bcca01b-026445ab-5ac8-45b9-a448-7b78e9b823c3.zip` → `archives/8.0.2/dd99392eacbe3bfab479706e0bcca01b-026445ab-5ac8-45b9-a448-7b78e9b823c3.zip`
- `reports/8.0.2/f7270a45bf1e5310d795ae6166958bcd-e537448a-ea2d-45eb-84e1-65cd5179b1d5.zip` → `archives/8.0.2/f7270a45bf1e5310d795ae6166958bcd-e537448a-ea2d-45eb-84e1-65cd5179b1d5.zip`
- `reports/8.0.2/200b27600253a015039b4c4e7970c89b-3cf32c6d-cccf-4bdf-bdbb-dadaa5b4f827.zip` → `archives/8.0.2/200b27600253a015039b4c4e7970c89b-3cf32c6d-cccf-4bdf-bdbb-dadaa5b4f827.zip`
- `reports/8.0.2/51eeee3e6ef2d6cf2d629d7bfa3d16b4-bc81d7ea-5ae0-4679-92a5-58c4c62e7fd9.zip` → `archives/8.0.2/51eeee3e6ef2d6cf2d629d7bfa3d16b4-bc81d7ea-5ae0-4679-92a5-58c4c62e7fd9.zip`
- `reports/8.0.2/67d7b35e57dbb5903c0bb84a75782f1c-14d4757a-06d2-4fad-a148-9e6db78cda68.zip` → `archives/8.0.2/67d7b35e57dbb5903c0bb84a75782f1c-14d4757a-06d2-4fad-a148-9e6db78cda68.zip`
- `reports/8.0.2/8e7e20e87e5ac6a5e1f89e78b4b42db4-12fa3f6c-39db-49d5-9767-983e9c5b4c4d.zip` → `archives/8.0.2/8e7e20e87e5ac6a5e1f89e78b4b42db4-12fa3f6c-39db-49d5-9767-983e9c5b4c4d.zip`
<!-- crash-reports:end -->

## Triage (2026-10-21)

- **Owner**: the engine.
- **Machines**: ≈9 (reports grouped by CPU + RAM + OS fingerprint on 2026-10-21; several reports per machine
  when every launch fails).
- **Reading**: Exit code 98 = `Core::DeviceLostExitCode`: the GPU device was lost and the engine stopped cleanly (`stopOnDeviceLoss()`). A real incident (driver reset, TDR), reported on purpose.
- **Next step**: Read the `DEVICE LOST` diagnostics (checkpoints) in the journals: one recurring GPU marker = an engine command to fix; scattered = driver / hardware. Linked: `crash-exit-code-98-after-applicationready-39d5f0`.
- **Plan**: `CrashReport Solucius`, wave W5 — app_system's `docs/plans/crashreport-solucius.md`.

## 8.0.3 status (checked 2026-10-21)

**Not addressed.** A device loss is an incident, kept on purpose (exit 98). 8.0.3 changed nothing on its causes.

Checked against the changes between 8.0.2 and 8.0.3 in the four repositories (app_system 1.1.1 → 1.1.2, engine 1.0.0 → 1.0.1, app_kernel 1.3.1 → 1.3.2, core `47cbe7cd9` → `c3ab7b390`); nothing relevant after 8.0.3. Proof only comes from the 8.0.3 reports (plan § 5).

## What remains

- Read the journals of the archives above (extracted under `tools/crash-reports/data/extracted/`).
- Find the defect, reproduce it, fix it where it lives.

## References

- app_system's `docs/crash-report.md` — what an archive holds, the flight recorder milestones.
- app_system's `tools/crash-reports/README.md` — how this item was produced, how the cause is chosen.
