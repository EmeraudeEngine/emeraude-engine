---
id: crash-window-no-monitor-on-the-system-fff0f1
title: "Window — \"There is no monitor on the system\" is a Fatal"
status: open
priority: unranked
scope: Window
opened: 2026-11-10
tags: [crash, crash-report-auto]
---

# Window — "There is no monitor on the system" is a Fatal

> Dropped by the downstream application's crash-report tool: a cause never seen in its production crash reports.
> The block between the `crash-reports` markers is rewritten at every run of that tool (found by its signature line, names of the application redacted); write outside it.

## Why

<!-- crash-reports:begin -->
| | |
|---|---|
| Machines | 1 — per version: 8.0.3 ×1 |
| Reports | 3 (first 2026-11-02, last 2026-11-02) |
| Versions | 8.0.3 ×3 |
| Platforms | windows ×3 |
| OS | Windows 11 Pro ×3 |
| GPU (selected) | unknown ×3 |
| ANGLE backend | d3d11 ×3 |
| Died at milestone | SessionOpened ×3 |
| Single-instance lock | not recorded ×3 |

- **Cause (normalized):** `WindowService: There is no monitor on the system !`
- **Where:** `vendors/emeraude-engine/src/Window.cpp` — `EmEn::Window::checkMonitors`
- **Signature:** `fff0f154db101ffe`

Last error before the death (else the last trace), by number of reports:

- ×3 `died at SessionOpened`

Traces of the main journal (normalized, latest report; around the Fatal trace when there is one):

```
[PlatformManagerService][Warning] (vendors/emeraude-engine/src/PlatformManager.cpp `EmEn::PlatformManager::onInitialize`) The process code page is <n>, not UTF-<n> (<n>) : the application manifest lacks <activeCodePage>UTF-<n><<path> Third-party code using the ANSI file APIs (the Vulkan loader) cannot open a path outside
[a line of the downstream application, redacted] (×2)
[Core][Success] (vendors/emeraude-engine/src/Core.cpp `EmEn::Core::initializeCoreLevel`) PlatformManagerService service up!
[VulkanInstanceService][Info] (vendors/emeraude-engine/src/Vulkan/Instance.cpp `EmEn::Vulkan::Instance::preparePhysicalDevices`) <n> physical device(s) found.
[Core][Success] (vendors/emeraude-engine/src/Core.cpp `EmEn::Core::initializeCoreLevel`) VulkanInstanceService service up!
[WindowService][Fatal] (vendors/emeraude-engine/src/Window.cpp `EmEn::Window::checkMonitors`) There is no monitor on the system !
[WindowService][Fatal] (vendors/emeraude-engine/src/Window.cpp `EmEn::Window::onInitialize`) There is no monitor connected to the system !
```

Archives:

- 3 archive(s), in the downstream application's crash-report mirror.
<!-- crash-reports:end -->

## Triage (2026-11-10)

- **Owner**: the engine. 1 Windows machine, 8.0.3, 3 reports at `SessionOpened` (a remote session, a display switched off?).
- **Question for the owner**: a defect (a Fatal where a told refusal would do) or an environment to refuse cleanly? Left `unranked` until ranked.

## What remains

- See the triage above; the journals stay in the downstream application's crash-report mirror.

## References

- The downstream application's crash-report documentation — what an archive holds, the flight recorder milestones.
- The downstream application's crash-report tool documentation — how this item was produced, how the cause is chosen.
