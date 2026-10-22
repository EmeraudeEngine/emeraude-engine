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

> Dropped by app_system's `tools/crash-reports/crash_reports.py`: a cause never seen before in the production crash reports.
> **Triage**: confirm it is a defect, give the file a better name, move it to the repository that owns the defect
> (an engine defect goes to `vendors/emeraude-engine/docs/todo/`, …) and rank it. The block between the
> `crash-reports` markers is rewritten at every run, wherever the item went (it is found by its signature line);
> write outside it. Deleting the item closes the cause: a new report of it is then flagged as a regression.

## Why

<!-- crash-reports:begin -->
| | |
|---|---|
| Machines | 1 — per version: 8.0.2 ×1 |
| Reports | 1 (first 2026-10-16, last 2026-10-16) |
| Versions | 8.0.2 ×1 |
| Platforms | windows ×1 |
| OS | Windows 11 Pro ×1 |
| GPU (selected) | unknown ×1 |
| ANGLE backend | d3d11 ×1 |
| Died at milestone | SessionOpened ×1 |
| Single-instance lock | not recorded ×1 |

- **Cause (normalized):** `WindowService: Trying to create a window with invalid size (<hex>) !`
- **Where:** `vendors/emeraude-engine/src/Window.cpp` — `EmEn::Window::create`
- **Signature:** `fbd36c1550151939`

Last error before the death (else the last trace), by number of reports:

- ×1 `died at SessionOpened`

Traces of the main journal (normalized, latest report; around the Fatal trace when there is one):

```
[Application][Info] (src/Application.CefBrowserProcessHandler.cpp `Mango3D::System::Application::OnRegisterCustomPreferences`) A request context preference has been changed !
[Application][Success] (src/Application.CefBrowserProcessHandler.cpp `Mango3D::System::Application::OnContextInitialized`) Chromium context initialized !
[PlatformManagerService][Info] (vendors/emeraude-engine/src/PlatformManager.cpp `EmEn::PlatformManager::onInitialize`) Vulkan layer search path : '<path> Files\Lychee\vulkan\explicit_layer.d'.
[VulkanLoader][Info] (vendors/emeraude-engine/src/Vulkan/Loader.cpp `EmEn::Vulkan::Loader::open`) Vulkan loader '<path> Files\Lychee\vulkan\vulkan-<n>.dll' opened (instance version <n>).
[Core][Success] (vendors/emeraude-engine/src/Core.cpp `EmEn::Core::initializeCoreLevel`) PlatformManagerService service up!
[Core][Success] (vendors/emeraude-engine/src/Core.cpp `EmEn::Core::initializeCoreLevel`) VulkanInstanceService service up!
[WindowService][Fatal] (vendors/emeraude-engine/src/Window.cpp `EmEn::Window::create`) Trying to create a window with invalid size (<hex>) !
[WindowService][Fatal] (vendors/emeraude-engine/src/Window.cpp `EmEn::Window::onInitialize`) Unable to get a valid window !
```

Archives (local mirror under `tools/crash-reports/data/`, extracted next to them in `extracted/`):

- `reports/8.0.2/7b59706d409371644c9570f426321369-7b6acc46-ffd1-4ab9-a47c-35b095b6ea0e.zip` → `archives/8.0.2/7b59706d409371644c9570f426321369-7b6acc46-ffd1-4ab9-a47c-35b095b6ea0e.zip`
<!-- crash-reports:end -->

## Triage (2026-10-21)

- **Owner**: the engine.
- **Machines**: ≈1 (reports grouped by CPU + RAM + OS fingerprint on 2026-10-21; several reports per machine
  when every launch fails).
- **Reading**: Windows: window creation with an invalid size (hex value = negative or zero).
- **Next step**: Clamp the restored `Core/Video/Window/Width|Height` to the monitor at creation (a saved size from an unplugged monitor?).
- **Plan**: `CrashReport Solucius`, wave W6 — app_system's `docs/plans/crashreport-solucius.md`.

## 8.0.3 status (checked 2026-10-21)

**Not addressed.** Nothing in 8.0.3 points at it.

Checked against the changes between 8.0.2 and 8.0.3 in the four repositories (app_system 1.1.1 → 1.1.2, engine 1.0.0 → 1.0.1, app_kernel 1.3.1 → 1.3.2, core `47cbe7cd9` → `c3ab7b390`); nothing relevant after 8.0.3. Proof only comes from the 8.0.3 reports (plan § 5).

## What remains

- Read the journals of the archives above (extracted under `tools/crash-reports/data/extracted/`).
- Find the defect, reproduce it, fix it where it lives.

## References

- app_system's `docs/crash-report.md` — what an archive holds, the flight recorder milestones.
- app_system's `tools/crash-reports/README.md` — how this item was produced, how the cause is chosen.
