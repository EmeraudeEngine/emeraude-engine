---
id: crash-window-created-with-an-invalid-size-22658a
title: "Window — created with an invalid size (0x0, 144x0)"
status: open
priority: high
scope: src/Window.cpp
opened: 2026-10-21
tags: [crash, crash-report-auto]
---

# Window — created with an invalid size (0x0, 144x0)

> Dropped by the downstream application's crash-report tool: a cause seen in its production crash reports.
> The block between the `crash-reports` markers is rewritten at every run of that tool (found by its signature line,
> names of the application redacted); write outside it. Deleting the item closes the cause: a new report of it from a
> newer version is then flagged as a regression.

## Why

<!-- crash-reports:begin -->
| | |
|---|---|
| Machines | 10 — per version: 8.0.2 ×1, 8.0.3 ×6, 8.0.4 ×3 |
| Reports | 12 (first 2026-10-16, last 2026-11-09) |
| Versions | 8.0.3 ×8, 8.0.4 ×3, 8.0.2 ×1 |
| Platforms | windows ×12 |
| OS | Windows 11 Home Single Language ×4, Windows 11 Pro ×4, Windows 11 Home ×3, Windows 10 Pro ×1 |
| GPU (selected) | unknown ×12 |
| ANGLE backend | d3d11 ×12 |
| Died at milestone | SessionOpened ×12 |
| Single-instance lock | not recorded ×9, Acquired ×3 |

- **Cause (normalized):** `WindowService: Trying to create a window with invalid size (<n>x<n>) !`
- **Where:** `vendors/emeraude-engine/src/Window.cpp` — `EmEn::Window::create`
- **Signature:** `22658af7d92ffff6`

Last error before the death (else the last trace), by number of reports:

- ×12 `died at SessionOpened`

Traces of the main journal (normalized, latest report; around the Fatal trace when there is one):

```
[PlatformManagerService][Warning] (vendors/emeraude-engine/src/PlatformManager.cpp `EmEn::PlatformManager::onInitialize`) The process code page is <n>, not UTF-<n> (<n>) : the application manifest lacks <activeCodePage>UTF-<n><<path> Third-party code using the ANSI file APIs (the Vulkan loader) cannot open a path outside
[a line of the downstream application, redacted] (×2)
[Core][Success] (vendors/emeraude-engine/src/Core.cpp `EmEn::Core::initializeCoreLevel`) PlatformManagerService service up!
[VulkanInstanceService][Info] (vendors/emeraude-engine/src/Vulkan/Instance.cpp `EmEn::Vulkan::Instance::preparePhysicalDevices`) <n> physical device(s) found.
[Core][Success] (vendors/emeraude-engine/src/Core.cpp `EmEn::Core::initializeCoreLevel`) VulkanInstanceService service up!
[WindowService][Fatal] (vendors/emeraude-engine/src/Window.cpp `EmEn::Window::create`) Trying to create a window with invalid size (<n>x<n>) !
[WindowService][Fatal] (vendors/emeraude-engine/src/Window.cpp `EmEn::Window::onInitialize`) Unable to get a valid window !
```

Archives:

- 12 archive(s), in the downstream application's crash-report mirror.
<!-- crash-reports:end -->

## Triage

- **Owner**: the engine. A defect (priority `high` by default).
- **2026-10-21**: first seen; Windows only, every report dies at `SessionOpened`, the window service fails its
  initialization ("Unable to get a valid window !").
- **2026-11-10, merged**: the reports said "(0x0)" ×11 and "(144x0)" ×1. The tool read "0x0" as a hex value and left
  "144x0" as is, so the same defect had two items (signatures `fbd36c1550151939` and `ca516a885d3381b5`). The tool
  now normalizes a size "(WxH)" to "(<n>x<n>)": one signature, this item.
- **Reading**: the HEIGHT is 0 in all 12 reports, the width 0 or 144. A size restored from the settings or computed
  from a monitor that reports nothing usable (an unplugged or virtual monitor, a minimized window saved as is?) —
  not confirmed.

## What remains

- Read the journals of the archives counted above (in the downstream application's crash-report mirror): where the
  requested size comes from (the saved `Core/Video/Window/Width|Height`, or the monitor's work area).
- Fix it where it lives: clamp the size to the monitor at creation (fall back to a default size when either side is
  0), then reproduce with a settings file holding a zero height.

## References

- `src/Window.cpp` — `Window::create()` refuses the size, `Window::onInitialize()` fails the service.
- The downstream application's crash-report documentation — what an archive holds, the flight recorder milestones.
- The downstream application's crash-report tool documentation — how this item was produced, how the cause is chosen.
