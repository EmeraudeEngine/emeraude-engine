---
id: crash-window-invalid-size-144x0-ca516a
title: "Window — created with an invalid size (144x0)"
status: open
priority: high
scope: Window
opened: 2026-11-10
tags: [crash, crash-report-auto]
---

# Window — created with an invalid size (144x0)

> Dropped by the downstream application's crash-report tool: a cause never seen in its production crash reports.
> The block between the `crash-reports` markers is rewritten at every run of that tool (found by its signature line, names of the application redacted); write outside it.

## Why

<!-- crash-reports:begin -->
| | |
|---|---|
| Machines | 1 — per version: 8.0.3 ×1 |
| Reports | 1 (first 2026-10-26, last 2026-10-26) |
| Versions | 8.0.3 ×1 |
| Platforms | windows ×1 |
| OS | Windows 10 Pro ×1 |
| GPU (selected) | unknown ×1 |
| ANGLE backend | d3d11 ×1 |
| Died at milestone | SessionOpened ×1 |
| Single-instance lock | not recorded ×1 |

- **Cause (normalized):** `WindowService: Trying to create a window with invalid size (144x0) !`
- **Where:** `vendors/emeraude-engine/src/Window.cpp` — `EmEn::Window::create`
- **Signature:** `ca516a885d3381b5`

Last error before the death (else the last trace), by number of reports:

- ×1 `died at SessionOpened`

Traces of the main journal (normalized, latest report; around the Fatal trace when there is one):

```
[PlatformManagerService][Warning] (vendors/emeraude-engine/src/PlatformManager.cpp `EmEn::PlatformManager::onInitialize`) The process code page is <n>, not UTF-<n> (<n>) : the application manifest lacks <activeCodePage>UTF-<n><<path> Third-party code using the ANSI file APIs (the Vulkan loader) cannot open a path outside
[a line of the downstream application, redacted] (×2)
[Core][Success] (vendors/emeraude-engine/src/Core.cpp `EmEn::Core::initializeCoreLevel`) PlatformManagerService service up!
[VulkanInstanceService][Info] (vendors/emeraude-engine/src/Vulkan/Instance.cpp `EmEn::Vulkan::Instance::preparePhysicalDevices`) <n> physical device(s) found.
[Core][Success] (vendors/emeraude-engine/src/Core.cpp `EmEn::Core::initializeCoreLevel`) VulkanInstanceService service up!
[WindowService][Fatal] (vendors/emeraude-engine/src/Window.cpp `EmEn::Window::create`) Trying to create a window with invalid size (144x0) !
[WindowService][Fatal] (vendors/emeraude-engine/src/Window.cpp `EmEn::Window::onInitialize`) Unable to get a valid window !
```

Archives:

- 1 archive(s), in the downstream application's crash-report mirror.
<!-- crash-reports:end -->

## Triage (2026-11-10)

- **Owner**: the engine. Same family as `crash-fatal-in-windowservice-trying-to-create-a-window-fbd36c`: only the size normalization differs ("144x0" is not rewritten, "0x…" is), hence a second signature. 1 Windows machine, 8.0.3.

## What remains

- See the triage above; the journals stay in the downstream application's crash-report mirror.

## References

- The downstream application's crash-report documentation — what an archive holds, the flight recorder milestones.
- The downstream application's crash-report tool documentation — how this item was produced, how the cause is chosen.
