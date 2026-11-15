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

- **2026-11-10, read side fixed**: the size comes from the stored `Core/Video/Window/Width|Height`; the save side
  already refused a minimised or empty window (since 2026-10-01, shipped in 8.0.4), yet 3 machines started 8.0.4 with
  0x0 — the writer was not found (neither the downstream application nor its frontend writes these keys). The window
  service now replaces a stored size with a side <= 0 by the default one, with a warning ("The stored window size 0x0
  is invalid : the default size 1280x720 is used."), and the launch goes on. Verified (Linux, the fault-injection
  runner with a settings file holding 0x0): the warning, exit 0, the settings repaired.
- **2026-11-11**: `Window::wasStoredSizeReplaced()` tells an application that the stored size was replaced, so it can
  give the window its own first-launch geometry (the owner's downstream application maximizes it) instead of 1280x720.

- **2026-11-15, the writer is found: an OLD engine, and the version guard carried its 0x0 over.** Read in the
  downstream application's crash-report mirror by that application's own session (counts and versions only):
  - in 9 of the 10 machines the crashing run is the one which performs the settings reset of the version guard —
    "Reset reason: engine version 0.9.8 older than 1.0.0 / 1.0.1 / 1.1.1" — and its journal lists
    `[KEPT] Core/Video/Window/Width` and `[KEPT] …/Height` right before "invalid size (0x0)". The file had last been
    written by engine 0.9.8 (tagged 2026-09-23): a build from before the save-side refusal (2026-10-01), on a machine
    which then skipped several versions. The window size is one of the keys EVERY reset keeps
    (`SettingsKeyRestoration::EngineKeys`), and nothing validated what was kept;
  - so the three machines reported on the newest version were never written a 0x0 by it: two crashed at its first
    run, the third at its second, 24 s after a first run which did the reset, rewrote the file — still 0x0 — and
    died silently before any window line (that first run is a report of the signature
    `crash-silent-death-at-sessionopened-on-windows-476d7f`);
  - the tenth machine (engine 1.0.1, 3 reports within two minutes) had a file already stamped 1.0.1 holding 0x0, no
    reset: where that one came from the mirror does not say. It failed at each launch — on that engine a failed
    creation did not put the defaults back; since 1.1.1 it does (one report per machine there, and the settings
    file archived after the crash holds 1280x720);
  - no archive holds a window position or a "minimised" marker: the old files have only the width, the height and
    the frameless flag. How 0.9.8 came to write 0x0 is therefore inferred, not read: a window minimised at exit
    (on Windows its size callback says 0x0) before the refusal existed; 144x0 is a window at the system's minimum
    size, whose client area has no height (120 px wide at 100 %, 144 at 120 %).
- **What it means for the fix**: the read side (2026-11-10) is the right place — whatever an old file holds and
  whatever a reset keeps, the stored size is checked where it is used. The save side could not have helped these
  machines.

## What remains

- Confirm on the reports of the first release that ships the read-side fix: this signature should disappear; then
  move the rule into the window documentation and delete the item.
- Done 2026-11-15 (owner decision: the same refusal): `Window::switchToFullscreenMode()` no longer stores the size of
  a minimised or empty window — it was the one writer left without it; and the settings documentation says that a
  key kept across a reset is not cleaned by it (source-tree doc 02). Run on Windows by the peer: a minimised window
  reads 0x0 at -32000,-32000, Shift+F11 warns and the stored size stays as it was.

## References

- `src/Window.cpp` — `Window::create()` refuses the size, `Window::onInitialize()` fails the service.
- The downstream application's crash-report documentation — what an archive holds, the flight recorder milestones.
- The downstream application's crash-report tool documentation — how this item was produced, how the cause is chosen.
