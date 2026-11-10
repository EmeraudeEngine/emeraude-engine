---
id: crash-platform-manager-failed-at-startup-e6b7c9
title: "PlatformManagerService fails at startup"
status: open
priority: high
scope: PlatformManager
opened: 2026-11-10
tags: [crash, crash-report-auto]
---

# PlatformManagerService fails at startup

> Dropped by the downstream application's crash-report tool: a cause never seen in its production crash reports.
> The block between the `crash-reports` markers is rewritten at every run of that tool (found by its signature line, names of the application redacted); write outside it.

## Why

<!-- crash-reports:begin -->
| | |
|---|---|
| Machines | 1 — per version: 8.0.3 ×1 |
| Reports | 1 (first 2026-11-03, last 2026-11-03) |
| Versions | 8.0.3 ×1 |
| Platforms | windows ×1 |
| OS | Windows 11 Pro ×1 |
| GPU (selected) | unknown ×1 |
| ANGLE backend | d3d11 ×1 |
| Died at milestone | SessionOpened ×1 |
| Single-instance lock | not recorded ×1 |

- **Cause (normalized):** `Core: PlatformManagerService service failed to execute!`
- **Where:** `vendors/emeraude-engine/src/Core.cpp` — `EmEn::Core::initializeCoreLevel`
- **Signature:** `e6b7c902af8ca54a`

Last error before the death (else the last trace), by number of reports:

- ×1 `died at SessionOpened`

Traces of the main journal (normalized, latest report; around the Fatal trace when there is one):

```
[a line of the downstream application, redacted] (×3)
[PlatformManagerService][Warning] (vendors/emeraude-engine/src/PlatformManager.cpp `EmEn::PlatformManager::onInitialize`) The process code page is <n>, not UTF-<n> (<n>) : the application manifest lacks <activeCodePage>UTF-<n><<path> Third-party code using the ANSI file APIs (the Vulkan loader) cannot open a path outside
[a line of the downstream application, redacted] (×2)
[Core][Fatal] (vendors/emeraude-engine/src/Core.cpp `EmEn::Core::initializeCoreLevel`) PlatformManagerService service failed to execute!
[ResourcesManagerService][Info] (vendors/emeraude-engine/src/Resources/Manager.cpp `EmEn::Resources::Manager::stopBackgroundWork`) stopBackgroundWork(): every resource job stopped in <n> ms.
```

Archives:

- 1 archive(s), in the downstream application's crash-report mirror.
<!-- crash-reports:end -->

## Triage (2026-11-10)

- **Owner**: the engine. 1 Windows machine, 8.0.3, at `SessionOpened`: read the journal for the platform manager's own error (loader not found, code page).

## What remains

- See the triage above; the journals stay in the downstream application's crash-report mirror.

## References

- The downstream application's crash-report documentation — what an archive holds, the flight recorder milestones.
- The downstream application's crash-report tool documentation — how this item was produced, how the cause is chosen.
