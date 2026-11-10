---
id: crash-silent-death-at-sessionopened-on-linux-c91e92
title: "Crash reports: Silent death at SessionOpened on linux"
status: open
priority: unranked
scope: src/Core.cpp, src/Vulkan
opened: 2026-10-21
tags: [crash, crash-report-auto]
---

# Crash reports: Silent death at SessionOpened on linux

> Dropped by the downstream application's crash-report tool: a cause never seen before in the production crash reports.
> **Triage**: confirm it is a defect, give the file a better name, move it to the repository that owns the defect
> (an engine defect goes to `vendors/emeraude-engine/docs/todo/`, …) and rank it. The block between the
> `crash-reports` markers is rewritten at every run, wherever the item went (it is found by its signature line);
> write outside it. Deleting the item closes the cause: a new report of it is then flagged as a regression.

## Why

<!-- crash-reports:begin -->
| | |
|---|---|
| Machines | 6 — per version: 8.0.2 ×1, 8.0.3 ×2, 8.0.4 ×3 |
| Reports | 6 (first 2026-10-15, last 2026-11-10) |
| Versions | 8.0.4 ×3, 8.0.3 ×2, 8.0.2 ×1 |
| Platforms | linux ×6 |
| OS | Ubuntu 26.04.1 LTS ×3, Pop!_OS 24.04 LTS ×1, Ubuntu 24.04.4 LTS ×1, Ubuntu 25.10 ×1 |
| GPU (selected) | unknown ×6 |
| ANGLE backend | default ×5, vulkan ×1 |
| Died at milestone | SessionOpened ×6 |
| Single-instance lock | Acquired ×3, not recorded ×3 |

- **Cause (normalized):** `died at SessionOpened on linux`
- **Where:** no source location
- **Signature:** `c91e92f8acfe6106`

Last error before the death (else the last trace), by number of reports:

- ×2 `CrashReporter: Requesting an upload URL for the crash report 'crash-report-<n>-<n>.zip' from 'https:<path>' (attempt <n>/<n>, ticket #<n>).`
- ×1 `Application: Crash report question in 'portuguese' (preferred language: 'pt-BR').`
- ×1 `CrashReporter: Session token '<path>' written.`
- ×1 `VulkanInstanceService: Vulkan Video H.<n> encode extensions detected and enabled (hardware RushMaker path).`
- ×1 `no journal`

Traces of the main journal (normalized, latest report; around the Fatal trace when there is one):

```
[a line of the downstream application, redacted] (×6)
[NetAPIClientService][Info] (vendors/emeraude-engine/src/Net/APIClient.cpp `EmEn::Net::APIClient::performRequest`) Calling POST 'https:<path>' (ticket #<n>) ...
[a line of the downstream application, redacted]
```

Archives:

- 6 archive(s), in the downstream application's crash-report mirror.
<!-- crash-reports:end -->

## Triage (2026-10-21)

- **Owner**: the engine.
- **Machines**: ≈2 (reports grouped by CPU + RAM + OS fingerprint on 2026-10-21; several reports per machine
  when every launch fails).
- **Reading**: Linux, dies before `CoreStarted` without a Fatal (one report ends on the Vulkan Video H.264 extensions).
- **Next step**: Low volume: read the two journals; wait for the minidumps (W0) if nothing stands out.
- **Plan**: `CrashReport Solucius`, wave W3 — a plan of the downstream application.

## 8.0.3 status (checked 2026-10-21)

**Not addressed.** Nothing in 8.0.3 points at it.

Checked against the changes between 8.0.2 and 8.0.3 in the four repositories (the application 1.1.1 → 1.1.2, engine 1.0.0 → 1.0.1, its kernel 1.3.1 → 1.3.2, its frontend `47cbe7cd9` → `c3ab7b390`); nothing relevant after 8.0.3. Proof only comes from the 8.0.3 reports (plan § 5).

## What remains

- Read the journals of the archives counted above (in the downstream application's crash-report mirror).
- Find the defect, reproduce it, fix it where it lives.

## References

- The downstream application's crash-report documentation — what an archive holds, the flight recorder milestones.
- The downstream application's crash-report tool documentation — how this item was produced, how the cause is chosen.
