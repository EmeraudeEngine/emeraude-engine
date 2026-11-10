---
id: crash-exit-code-98-after-applicationready-39d5f0
title: "Crash reports: Exit code 98 after ApplicationReady"
status: open
priority: unranked
scope: src/Core.cpp, src/Overlay
opened: 2026-10-21
tags: [crash, crash-report-auto]
---

# Crash reports: Exit code 98 after ApplicationReady

> Dropped by the downstream application's crash-report tool: a cause never seen before in the production crash reports.
> **Triage**: confirm it is a defect, give the file a better name, move it to the repository that owns the defect
> (an engine defect goes to `vendors/emeraude-engine/docs/todo/`, …) and rank it. The block between the
> `crash-reports` markers is rewritten at every run, wherever the item went (it is found by its signature line);
> write outside it. Deleting the item closes the cause: a new report of it is then flagged as a regression.

## Why

<!-- crash-reports:begin -->
| | |
|---|---|
| Machines | 1 — per version: 8.0.2 ×1 |
| Reports | 1 (first 2026-10-20, last 2026-10-20) |
| Versions | 8.0.2 ×1 |
| Platforms | windows ×1 |
| OS | Windows 11 Pro ×1 |
| GPU (selected) | AMD Radeon RX 7700 XT (driver 2.0.416) ×1 |
| ANGLE backend | d3d11 ×1 |
| Died at milestone | ApplicationReady ×1 |
| Single-instance lock | not recorded ×1 |

- **Cause (normalized):** `exit code 98 after ApplicationReady`
- **Where:** no source location
- **Signature:** `39d5f08939755546`

Last error before the death (else the last trace), by number of reports:

- ×1 `WebView: [CEF-ACCEL] Unable to import the accelerated frame for 'ViewSplash' (<n>x<n>px) !`

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

- 1 archive(s), in the downstream application's crash-report mirror.
<!-- crash-reports:end -->

## Triage (2026-10-21)

- **Owner**: the engine.
- **Machines**: ≈1 (reports grouped by CPU + RAM + OS fingerprint on 2026-10-21; several reports per machine
  when every launch fails).
- **Reading**: Device lost (exit 98) right after `ApplicationReady`, last error a failed accelerated-frame import for the splash.
- **Next step**: Same family as `crash-exit-code-98-after-stopping-80d5c2`: treat them together.
- **Plan**: `CrashReport Solucius`, wave W5 — a plan of the downstream application.

## 8.0.3 status (checked 2026-10-21)

**Not addressed.** Same as `crash-exit-code-98-after-stopping-80d5c2`.

Checked against the changes between 8.0.2 and 8.0.3 in the four repositories (the application 1.1.1 → 1.1.2, engine 1.0.0 → 1.0.1, its kernel 1.3.1 → 1.3.2, its frontend `47cbe7cd9` → `c3ab7b390`); nothing relevant after 8.0.3. Proof only comes from the 8.0.3 reports (plan § 5).

## What remains

- Read the journals of the archives counted above (in the downstream application's crash-report mirror).
- Find the defect, reproduce it, fix it where it lives.

## References

- The downstream application's crash-report documentation — what an archive holds, the flight recorder milestones.
- The downstream application's crash-report tool documentation — how this item was produced, how the cause is chosen.
