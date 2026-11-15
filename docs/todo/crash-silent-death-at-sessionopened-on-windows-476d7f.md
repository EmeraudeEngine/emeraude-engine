---
id: crash-silent-death-at-sessionopened-on-windows-476d7f
title: "Crash reports: Silent death at SessionOpened on windows"
status: open
priority: high
scope: src/Core.cpp, src/Vulkan
opened: 2026-10-21
tags: [crash, crash-report-auto]
---

# Crash reports: Silent death at SessionOpened on windows

> Dropped by the downstream application's crash-report tool: a cause never seen before in the production crash reports.
> **Triage**: confirm it is a defect, give the file a better name, move it to the repository that owns the defect
> (an engine defect goes to `vendors/emeraude-engine/docs/todo/`, …) and rank it. The block between the
> `crash-reports` markers is rewritten at every run, wherever the item went (it is found by its signature line);
> write outside it. Deleting the item closes the cause: a new report of it is then flagged as a regression.

## Why

<!-- crash-reports:begin -->
| | |
|---|---|
| Machines | 84 — per version: 8.0.2 ×44, 8.0.3 ×25, 8.0.4 ×15 |
| Reports | 122 (first 2026-10-15, last 2026-11-10) |
| Versions | 8.0.2 ×73, 8.0.3 ×30, 8.0.4 ×19 |
| Platforms | windows ×122 |
| OS | Windows 11 Home ×45, Windows 11 Pro ×44, Windows 10 Pro ×13, Windows 10 Home ×10, Windows 11 Home Single Language ×6, Windows 10 Home Single Language ×2, … (2 more) |
| GPU (selected) | unknown ×122 |
| ANGLE backend | d3d11 ×80, default ×42 |
| Died at milestone | SessionOpened ×122 |
| Single-instance lock | not recorded ×103, Acquired ×19 |

- **Cause (normalized):** `died at SessionOpened on windows`
- **Where:** no source location
- **Signature:** `476d7fc761290d14`

Last error before the death (else the last trace), by number of reports:

- ×30 `CrashReporter: Session token '<path>' written.`
- ×18 `NetAPIClientService: 'https:<path>' (ticket #<n>) answered HTTP <n> (<n> bytes).`
- ×15 `ShaderManagerService: The shader 'OverlayRenderingFragmentShader' (<n> bytes) loaded from binary cache !`
- ×9 `RendererService: GraphicsRecorderService service failed or disabled at startup!`
- ×5 `Application: [CEF-ACCEL] DirectComposition disabled (the windowed DevTools present path is incompatible with the process-wide shared-texture state).`
- ×5 `Core: WindowService service up!`
- ×4 `Core: PlatformManagerService service up!`
- ×4 `VulkanInstanceService: <n> physical device(s) found.`
- ×4 `VulkanInstanceService: The physical device 'Intel(R) UHD Graphics <n>' is missing 'descriptorIndexing' feature !`
- ×3 `Application: Crash report question in 'english' (preferred language: 'en-US').`
- … 16 other(s), 25 report(s)

Traces of the main journal (normalized, latest report; around the Fatal trace when there is one):

```
[PhysicsManagerService][Warning] (vendors/emeraude-engine/src/Physics/Manager.cpp `EmEn::Physics::Manager::onInitialize`) Physics acceleration disabled at startup.
[Core][Success] (vendors/emeraude-engine/src/Core.cpp `EmEn::Core::initializeCoreLevel`) PhysicsManagerService service up!
[AudioManagerService][Warning] (vendors/emeraude-engine/src/Audio/Manager.cpp `EmEn::Audio::Manager::onInitialize`) Audio manager disabled at startup.
[Core][Success] (vendors/emeraude-engine/src/Core.cpp `EmEn::Core::initializeCoreLevel`) AudioManagerService service up!
[SystemNotificationService][Info] (vendors/emeraude-engine/src/SystemNotification.cpp `EmEn::SystemNotification::onInitialize`) System notification service initialized.
[Core][Success] (vendors/emeraude-engine/src/Core.cpp `EmEn::Core::initializeCoreLevel`) SystemNotificationService service up!
[ShaderManagerService][Success] (vendors/emeraude-engine/src/Saphir/ShaderManager.cpp `EmEn::Saphir::ShaderManager::getShaderModuleFromGeneratedShader`) The shader 'OverlayRenderingVertexShader' (<n> bytes) loaded from binary cache !
[ShaderManagerService][Success] (vendors/emeraude-engine/src/Saphir/ShaderManager.cpp `EmEn::Saphir::ShaderManager::getShaderModuleFromGeneratedShader`) The shader 'OverlayRenderingFragmentShader' (<n> bytes) loaded from binary cache !
```

Archives:

- 122 archive(s), in the downstream application's crash-report mirror.
<!-- crash-reports:end -->

## Triage (2026-10-21)

- **Owner**: the engine.
- **Machines**: ≈26 (reports grouped by CPU + RAM + OS fingerprint on 2026-10-21; several reports per machine
  when every launch fails).
- **Reading**: The run dies before `CoreStarted` without any Fatal: inside the engine's startup (Vulkan instance/device creation, shader cache, overlay), most likely in the driver. Last traces vary (session token written, `GraphicsRecorderService` disabled, shader binary cache…).
- **Next step**: Needs a native dump to go further (plan wave W0, minidumps). Meanwhile: compare the last trace of each report with the engine's startup order to bound the step that dies.
- **Plan**: `CrashReport Solucius`, wave W3 — a plan of the downstream application.

## 8.0.3 status (checked 2026-10-21)

**Possibly affected.** Engine `12566475` (1.0.1) removed the startup aborts (0xC0000409) on a path outside the ANSI code page — install directory, Vulkan loader logs, `BaseInformation`. Unverified link. ⚠️ The last traces lag the death: the tracer writes asynchronously, a crash loses the last queued entries — 10 journals stop at `Core level created`, which precedes the session token that made the report: the run went further.

Checked against the changes between 8.0.2 and 8.0.3 in the four repositories (the application 1.1.1 → 1.1.2, engine 1.0.0 → 1.0.1, its kernel 1.3.1 → 1.3.2, its frontend `47cbe7cd9` → `c3ab7b390`); nothing relevant after 8.0.3. Proof only comes from the 8.0.3 reports (plan § 5).

## Journals read (2026-11-11, 55 of 122: 22 of 8.0.4, 33 of 8.0.3, 46 machines)

- The journal is never closed and the report names no GPU. Last ENGINE line: "*** Core level created ***" 27; an HTTP
  request of the API client at startup (answered, or "Calling POST …") 12; "WindowService service up!" 4; "N physical
  device(s) found" 4; a shader loaded from the binary cache 2; the recorder service disabled 2; single others.
- **In 33 of 55 the very last line is the downstream application's own** (its crash reporter 22, its application object
  11), written AFTER the engine's last line: those deaths happen after the engine handed control back — maybe not an
  engine death at all.
- 12 of 55 carry the code-page warning (1252, not UTF-8) and the Vulkan loader opened right after.

- **2026-11-15, one of these reports explained by its sequel**: on one machine the silent death is the FIRST run of
  a new version — it performed the version guard's settings reset, rewrote the file (which kept a stored window size
  of 0x0 from an old engine) and died before any window line; its second run, 24 s later, is a report of
  `crash-window-created-with-an-invalid-size-22658a`. One case, read in the downstream application's crash-report
  mirror; it does not say why the first run died silently instead of failing loudly as the second did.

## The 122 reports, counted (2026-11-15)

Read in the downstream application's crash-report mirror by that application's own session: all 122 reports, 84
machines, counts only.

- **Not the version guard**: the dying run performed the settings reset in 6 reports of 122 (about 5 %), against 2 to
  6 % among the reports of every other signature on the same versions. The case above is one case.
- **No dump, in any of them**: no minidump, no native exception record. What is left under this signature is exactly
  the deaths nothing caught. The downstream application's next version collects the system's local dumps on Windows;
  none of these versions could.
- **They die early**: from the session's start to the last journal line, under 1 s in 44 reports, 1 to 5 s in 34,
  5 to 30 s in 37, longer in 7 (median 2.9 s). On the newest version: under 1 s in 13 of 19.
- **A hang ended by the user and a crash look the same**: the report is made by the NEXT launch (median 87 s after
  the last line), no journal has its closing line, and no exit code, termination reason or "killed by" exists in a
  report (an exit code in 2 of 122 only: both 1, with "Core level terminated" as last line — orderly failures,
  misfiled here).
- **The last line**: the downstream application's session token 30, an ANSWERED request of the API client 21, a
  shader loaded from the binary cache 15, the recorder service disabled 11, "WindowService service up!" 5, a
  downstream line about its web view 5, "PlatformManagerService service up!" 4, "N physical device(s) found" 4.
  No report ends on a request left unanswered (0 of 122). 10 runs died while handling an EARLIER crash report (its
  question shown 5, its upload 3, "user declined" 2).
- **Second tries**: for 89 reports the next launch came within five minutes — the same silent death again 22,
  another signature 4, nothing recorded 63 (a normal session leaves no trace). Per machine: 1 report on 63
  machines, 2 on 10, 3 on 6, 4 on 4, 5 on 1.

So: no dump and no exit code. Nothing here tells WHICH step dies; the dumps the downstream application's next
version brings are what will.

## Does the journal stop short of the death? Measured: no (2026-11-15, Linux)

The note of 2026-10-21 above ("the tracer writes asynchronously, a crash loses the last queued entries") made a
synchronous journal during the startup look useful, and the owner had retained it. It was MEASURED before being
written, and dropped (owner decision, the same day):
- a test instance killed (SIGKILL) at 13 moments of its startup, from 40 to 400 ms after its launch, 45 kills
  (Release, RTX 3070 Ti, `default`, the output line-buffered into a file as the reference): the journal lacks 3 lines
  in 44 kills and 4 in one. The 3 are always the same — the very first lines, traced before the logger exists (it
  waits for the settings service): never queued, not lost. So the writing thread lost ONE line, once in 45;
- the last line of the journal is the last line printed (the same source line), up to 14 636 lines into the startup;
- a synchronous startup would cost about 34 500 writes with a flush each here (the lines a startup traces before
  "The application successfully started.", 1073 ms median) for a gain which cannot be measured.

⚠️ Not measured on Windows, where these reports come from: its console output is not flushed line by line when
redirected, so the same reference does not exist there. If the question comes back, measure it there first (a tool
which pushes lines then dies at once, and counts what the file holds).

## What remains

- Read the dumps of the first reports which carry one (the downstream application's next version).
- Then find the defect, reproduce it, fix it where it lives.

## References

- The downstream application's crash-report documentation — what an archive holds, the flight recorder milestones.
- The downstream application's crash-report tool documentation — how this item was produced, how the cause is chosen.
