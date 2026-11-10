---
id: crash-fatal-in-vulkaninstanceservice-incompatible-driv-48a307
title: "Crash reports: Fatal in VulkanInstanceService — Incompatible driver !"
status: open
priority: unranked
scope: src/Vulkan/Instance.cpp
opened: 2026-10-21
tags: [crash, crash-report-auto]
---

# Crash reports: Fatal in VulkanInstanceService — Incompatible driver !

> Dropped by the downstream application's crash-report tool: a cause never seen before in the production crash reports.
> **Triage**: confirm it is a defect, give the file a better name, move it to the repository that owns the defect
> (an engine defect goes to `vendors/emeraude-engine/docs/todo/`, …) and rank it. The block between the
> `crash-reports` markers is rewritten at every run, wherever the item went (it is found by its signature line);
> write outside it. Deleting the item closes the cause: a new report of it is then flagged as a regression.

## Why

<!-- crash-reports:begin -->
| | |
|---|---|
| Machines | 68 — per version: 8.0.2 ×68 |
| Reports | 237 (first 2026-10-15, last 2026-11-08) |
| Versions | 8.0.2 ×237 |
| Platforms | windows ×237 |
| OS | Windows 10 Pro ×91, Windows 11 Pro ×60, Windows 10 Home ×50, Windows 10 Home Single Language ×10, Windows 11 Home ×10, Windows 10 Enterprise ×9, … (4 more) |
| GPU (selected) | unknown ×237 |
| ANGLE backend | d3d11 ×237 |
| Died at milestone | SessionOpened ×237 |
| Single-instance lock | not recorded ×237 |

- **Cause (normalized):** `VulkanInstanceService: Incompatible driver !`
- **Where:** `vendors/emeraude-engine/src/Vulkan/Instance.cpp` — `EmEn::Vulkan::Instance::onInitialize`
- **Signature:** `48a307142a440ca6`

Last error before the death (else the last trace), by number of reports:

- ×237 `died at SessionOpened`

Traces of the main journal (normalized, latest report; around the Fatal trace when there is one):

```
[a line of the downstream application, redacted] (×5)
[Core][Success] (vendors/emeraude-engine/src/Core.cpp `EmEn::Core::initializeCoreLevel`) PlatformManagerService service up!
[VulkanInstanceService][Fatal] (vendors/emeraude-engine/src/Vulkan/Instance.cpp `EmEn::Vulkan::Instance::onInitialize`) Incompatible driver !
[Core][Fatal] (vendors/emeraude-engine/src/Core.cpp `EmEn::Core::initializeCoreLevel`) VulkanInstanceService service failed to execute!
```

Archives:

- 237 archive(s), in the downstream application's crash-report mirror.
<!-- crash-reports:end -->

## Triage (2026-10-21)

- **Owner**: the engine.
- **Machines**: ≈61 (reports grouped by CPU + RAM + OS fingerprint on 2026-10-21; several reports per machine
  when every launch fails).
- **Reading**: `vkCreateInstance()` answers `VK_ERROR_INCOMPATIBLE_DRIVER`: no Vulkan driver usable. All 8.0.2 / engine 1.0.0, every report dies at `SessionOpened`, and each new launch reports the previous one: up to 15 reports per machine.
- **Next step**: Engine 1.0.1 (shipped with 8.0.3) classifies it as `NoVulkanDriver`: a dialog, exit code 97 (`docs/subsystems/vulkan/22-startup-refusals-told-to-the-user.md`). Confirm on the 8.0.3 reports that it is no longer a Fatal; what remains belongs to the downstream application: its crash reporter still reported an exit 97 as a crash at every launch.
- **Plan**: `CrashReport Solucius`, wave W1 — a plan of the downstream application.

## 8.0.3 status (checked 2026-10-21)

**Changed.** The machines have no usable Vulkan driver: the bundled loader (1.4.363) opens, then `vkCreateInstance()` answers `VK_ERROR_INCOMPATIBLE_DRIVER` — no ICD at all; the Vulkan 1.1 target of 8.0.3 does not change that. Engine 1.0.1 tells it instead of a Fatal: `NoVulkanDriver` dialog with the GPUs listed by the OS (`d166a93c`, `c4b115f8`), exit 97. These machines still cannot run the application, and its crash reporter still reported exit 97 at every launch.

Checked against the changes between 8.0.2 and 8.0.3 in the four repositories (the application 1.1.1 → 1.1.2, engine 1.0.0 → 1.0.1, its kernel 1.3.1 → 1.3.2, its frontend `47cbe7cd9` → `c3ab7b390`); nothing relevant after 8.0.3. Proof only comes from the 8.0.3 reports (plan § 5).

## What remains

- Read the journals of the archives counted above (in the downstream application's crash-report mirror).
- Find the defect, reproduce it, fix it where it lives.

## References

- The downstream application's crash-report documentation — what an archive holds, the flight recorder milestones.
- The downstream application's crash-report tool documentation — how this item was produced, how the cause is chosen.
