---
id: crash-silent-death-at-sessionopened-on-macos-8e38ac
title: "Crash reports: Silent death at SessionOpened on macos"
status: open
priority: high
scope: unknown
opened: 2026-11-10
tags: [crash, crash-report-auto]
---

# Crash reports: Silent death at SessionOpened on macos

> Dropped by the downstream application's crash-report tool: a cause never seen in its production crash reports.
> The block between the `crash-reports` markers is rewritten at every run of that tool (found by its signature line, names of the application redacted); write outside it.

## Why

<!-- crash-reports:begin -->
| | |
|---|---|
| Machines | 1 — per version: 8.0.3 ×1 |
| Reports | 1 (first 2026-11-03, last 2026-11-03) |
| Versions | 8.0.3 ×1 |
| Platforms | macos ×1 |
| OS | Apple macOS 26.6 ×1 |
| GPU (selected) | unknown ×1 |
| ANGLE backend | metal ×1 |
| Died at milestone | SessionOpened ×1 |
| Single-instance lock | not recorded ×1 |

- **Cause (normalized):** `died at SessionOpened on macos`
- **Where:** no source location
- **Signature:** `8e38aca565b2976d`

Last error before the death (else the last trace), by number of reports:

- ×1 `Application: A request context preference has been changed !`

Traces of the main journal (normalized, latest report; around the Fatal trace when there is one):

```
[ResourcesManagerService][Success] (vendors/emeraude-engine/src/Resources/Manager.cpp `EmEn::Resources::Manager::onInitialize`) Skeleton manager service up !
[Core][Success] (vendors/emeraude-engine/src/Core.cpp `EmEn::Core::initializeBaseLevel`) ResourcesManagerService service up !
[Core][Success] (vendors/emeraude-engine/src/Core.cpp `EmEn::Core::initializeBaseLevel`) UserService service up !
[Core][Success] (vendors/emeraude-engine/src/Core.cpp `EmEn::Core::initializeBaseLevel`) *** Core level created ***
[a line of the downstream application, redacted] (×4)
```

Archives:

- 1 archive(s), in the downstream application's crash-report mirror.
<!-- crash-reports:end -->

## Triage (2026-11-10)

- **Owner**: the engine, like its Windows and Linux siblings (`476d7f`, `c91e92`). 1 machine, 8.0.3.

## What remains

- See the triage above; the journals stay in the downstream application's crash-report mirror.

## References

- The downstream application's crash-report documentation — what an archive holds, the flight recorder milestones.
- The downstream application's crash-report tool documentation — how this item was produced, how the cause is chosen.
