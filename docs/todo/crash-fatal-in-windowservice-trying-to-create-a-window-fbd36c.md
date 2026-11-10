---
id: crash-fatal-in-windowservice-trying-to-create-a-window-fbd36c
title: "Crash reports: Fatal in WindowService — Trying to create a window with invalid size (<hex>) !"
status: open
priority: unranked
scope: src/Window.cpp
opened: 2026-10-21
tags: [crash, crash-report-auto]
---

# Crash reports: Fatal in WindowService — Trying to create a window with invalid size (<hex>) !

> Dropped by the downstream application's crash-report tool: a cause never seen before in the production crash reports.
> **Triage**: confirm it is a defect, give the file a better name, move it to the repository that owns the defect
> (an engine defect goes to `vendors/emeraude-engine/docs/todo/`, …) and rank it. The block between the
> `crash-reports` markers is rewritten at every run, wherever the item went (it is found by its signature line);
> write outside it. Deleting the item closes the cause: a new report of it is then flagged as a regression.

## Why

<!-- crash-reports:begin -->
| | |
|---|---|
| Machines | 9 — per version: 8.0.2 ×1, 8.0.3 ×5, 8.0.4 ×3 |
| Reports | 11 (first 2026-10-16, last 2026-11-09) |
| Versions | 8.0.3 ×7, 8.0.4 ×3, 8.0.2 ×1 |
| Platforms | windows ×11 |
| OS | Windows 11 Home Single Language ×4, Windows 11 Pro ×4, Windows 11 Home ×3 |
| GPU (selected) | unknown ×11 |
| ANGLE backend | d3d11 ×11 |
| Died at milestone | SessionOpened ×11 |
| Single-instance lock | not recorded ×8, Acquired ×3 |

- **Cause (normalized):** `WindowService: Trying to create a window with invalid size (<hex>) !`
- **Where:** `vendors/emeraude-engine/src/Window.cpp` — `EmEn::Window::create`
- **Signature:** `fbd36c1550151939`

Last error before the death (else the last trace), by number of reports:

- ×11 `died at SessionOpened`

Traces of the main journal (normalized, latest report; around the Fatal trace when there is one):

```
[PlatformManagerService][Warning] (vendors/emeraude-engine/src/PlatformManager.cpp `EmEn::PlatformManager::onInitialize`) The process code page is <n>, not UTF-<n> (<n>) : the application manifest lacks <activeCodePage>UTF-<n><<path> Third-party code using the ANSI file APIs (the Vulkan loader) cannot open a path outside
[a line of the downstream application, redacted] (×2)
[Core][Success] (vendors/emeraude-engine/src/Core.cpp `EmEn::Core::initializeCoreLevel`) PlatformManagerService service up!
[VulkanInstanceService][Info] (vendors/emeraude-engine/src/Vulkan/Instance.cpp `EmEn::Vulkan::Instance::preparePhysicalDevices`) <n> physical device(s) found.
[Core][Success] (vendors/emeraude-engine/src/Core.cpp `EmEn::Core::initializeCoreLevel`) VulkanInstanceService service up!
[WindowService][Fatal] (vendors/emeraude-engine/src/Window.cpp `EmEn::Window::create`) Trying to create a window with invalid size (<hex>) !
[WindowService][Fatal] (vendors/emeraude-engine/src/Window.cpp `EmEn::Window::onInitialize`) Unable to get a valid window !
```

Archives:

- 11 archive(s), in the downstream application's crash-report mirror.
<!-- crash-reports:end -->

## Triage (2026-10-21)

- **Owner**: the engine.
- **Machines**: ≈1 (reports grouped by CPU + RAM + OS fingerprint on 2026-10-21; several reports per machine
  when every launch fails).
- **Reading**: Windows: window creation with an invalid size (hex value = negative or zero).
- **Next step**: Clamp the restored `Core/Video/Window/Width|Height` to the monitor at creation (a saved size from an unplugged monitor?).
- **Plan**: `CrashReport Solucius`, wave W6 — a plan of the downstream application.

## 8.0.3 status (checked 2026-10-21)

**Not addressed.** Nothing in 8.0.3 points at it.

Checked against the changes between 8.0.2 and 8.0.3 in the four repositories (the application 1.1.1 → 1.1.2, engine 1.0.0 → 1.0.1, its kernel 1.3.1 → 1.3.2, its frontend `47cbe7cd9` → `c3ab7b390`); nothing relevant after 8.0.3. Proof only comes from the 8.0.3 reports (plan § 5).

## What remains

- Read the journals of the archives counted above (in the downstream application's crash-report mirror).
- Find the defect, reproduce it, fix it where it lives.

## References

- The downstream application's crash-report documentation — what an archive holds, the flight recorder milestones.
- The downstream application's crash-report tool documentation — how this item was produced, how the cause is chosen.
