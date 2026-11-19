---
id: crash-windows-open-dialog-retried-then-quit-stalls-the-main-loop-de5bbb
title: "Crash reports: Windows open-file dialog retried seven times, then the quit stalls the main loop"
status: open
priority: high
scope: src/PlatformSpecific/Desktop/Dialog/OpenFile.windows.cpp, PlatformSpecific/Helpers.hpp (runFileDialogOnDedicatedThread)
opened: 2026-11-19
tags: [crash, crash-report-auto, windows, dialog, freeze, quit]
---

# Crash reports: Windows open-file dialog retried seven times, then the quit stalls the main loop

## Triage

Triaged 2026-11-19. 1 report, Windows 11 Pro, RTX 4090 (driver 617.14) + AMD integrated GPU, the main loop
stalled at `QuitRequested`, then the run ended.

Timeline of the main journal (uptime; the journal's clock is in nanoseconds):

- 10 s `ApplicationStarted`. About 32 s: an open-file dialog is requested, `OpenFile::showDialog()` traces
  `SHCreateItemFromParsingName failed for the default directory !`; the main loop then does not cycle for 18 s.
- Six more requests within 18 s, the last three less than a second apart, each with the same error; five
  `Stop requested by the window system (close request)` in between; `QuitRequested` at 74 s; the main loop stalls
  11 s; the run ends without a clean exit.

Reading (from the code, not reproduced): `OpenFile::execute()` runs the dialog on a dedicated STA thread and the main
thread waits in a message-pumping wait (`runFileDialogOnDedicatedThread`), so the main loop does not cycle while a
dialog is open — the downstream application's main-loop watchdog counts an open dialog as a stall. The user's
behaviour (seven openings, five close attempts) suggests the dialog was not visible to them.

Open questions:

- Is the dialog shown (on screen, in front of its owner) when `SetFolder()` was skipped because the default directory
  was rejected?
- How do new dialog requests arrive while the main thread waits on one (re-entrance through the pumping wait)?
- A close request pumped during the wait cannot complete before the dialog returns: should it cancel the dialog?
- The error trace names neither the directory nor the `HRESULT`: add both.

## Why

<!-- crash-reports:begin -->
| | |
|---|---|
| Machines | 1 — per version: 8.0.5 ×1 |
| Reports | 1 (first 2026-11-19, last 2026-11-19) |
| Versions | 8.0.5 ×1 |
| Platforms | windows ×1 |
| OS | Windows 11 Pro ×1 |
| GPU (selected) | NVIDIA GeForce RTX 4090 (driver 617.14) ×1 |
| ANGLE backend | d3d11 ×1 |
| Died at milestone | QuitRequested ×1 |
| Single-instance lock | Acquired ×1 |

- **Cause (normalized):** `main loop frozen at QuitRequested on windows`
- **Where:** no source location
- **Signature:** `de5bbbdbd5512681`

Last error before the death (else the last trace), by number of reports:

- ×1 `OpenFile: SHCreateItemFromParsingName failed for the default directory !`

Traces of the main journal (normalized, latest report; around the Fatal trace when there is one):

```
[a line of the downstream application, redacted]
[OpenFile][Error] (vendors/emeraude-engine/src/PlatformSpecific/Desktop/Dialog/OpenFile.windows.cpp `EmEn::PlatformSpecific::Desktop::Dialog::OpenFile::showDialog`) SHCreateItemFromParsingName failed for the default directory !
[Core][Info] (vendors/emeraude-engine/src/Core.cpp `EmEn::Core::onNotification`) Stop requested by the window system (close request).
[OpenFile][Error] (vendors/emeraude-engine/src/PlatformSpecific/Desktop/Dialog/OpenFile.windows.cpp `EmEn::PlatformSpecific::Desktop::Dialog::OpenFile::showDialog`) SHCreateItemFromParsingName failed for the default directory !
[VulkanTransferManagerService][Info] (vendors/emeraude-engine/src/Vulkan/TransferManager.cpp `EmEn::Vulkan::TransferManager::releaseIdleStagingBuffers`) <n> staging buffer released (<n> MiB), <n> buffer and <n> image transfer operations kept.
[Core][Info] (vendors/emeraude-engine/src/Core.cpp `EmEn::Core::onNotification`) Stop requested by the window system (close request). (×2)
[a line of the downstream application, redacted]
```

Archives:

- 1 archive(s), in the downstream application's crash-report mirror.
<!-- crash-reports:end -->

## What remains

- Answer the questions above on Windows (a default directory that does not exist, a forward-slash path, a
  UNC path), then fix where it lives.

## References

- The downstream application's crash-report documentation — what an archive holds, the flight recorder milestones.
- The downstream application's crash-report tool documentation — how this item was produced, how the cause is chosen.
