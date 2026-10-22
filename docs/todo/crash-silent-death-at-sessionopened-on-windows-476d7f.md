---
id: crash-silent-death-at-sessionopened-on-windows-476d7f
title: "Crash reports: Silent death at SessionOpened on windows"
status: open
priority: unranked
scope: src/Core.cpp, src/Vulkan
opened: 2026-10-21
tags: [crash, crash-report-auto]
---

# Crash reports: Silent death at SessionOpened on windows

> Dropped by app_system's `tools/crash-reports/crash_reports.py`: a cause never seen before in the production crash reports.
> **Triage**: confirm it is a defect, give the file a better name, move it to the repository that owns the defect
> (an engine defect goes to `vendors/emeraude-engine/docs/todo/`, …) and rank it. The block between the
> `crash-reports` markers is rewritten at every run, wherever the item went (it is found by its signature line);
> write outside it. Deleting the item closes the cause: a new report of it is then flagged as a regression.

## Why

<!-- crash-reports:begin -->
| | |
|---|---|
| Machines | 34 — per version: 8.0.2 ×34 |
| Reports | 53 (first 2026-10-15, last 2026-10-22) |
| Versions | 8.0.2 ×53 |
| Platforms | windows ×53 |
| OS | Windows 11 Home ×30, Windows 11 Pro ×9, Windows 10 Home ×6, Windows 10 Pro ×6, Windows 10 Home Single Language ×1, Windows 11 Home Single Language ×1 |
| GPU (selected) | unknown ×53 |
| ANGLE backend | d3d11 ×41, default ×12 |
| Died at milestone | SessionOpened ×53 |
| Single-instance lock | not recorded ×53 |

- **Cause (normalized):** `died at SessionOpened on windows`
- **Where:** no source location
- **Signature:** `476d7fc761290d14`

Last error before the death (else the last trace), by number of reports:

- ×11 `ShaderManagerService: The shader 'OverlayRenderingFragmentShader' (<n> bytes) loaded from binary cache !`
- ×10 `CrashReporter: Session token '<path>' written.`
- ×7 `NetAPIClientService: 'https:<path>' (ticket #<n>) answered HTTP <n> (<n> bytes).`
- ×5 `RendererService: GraphicsRecorderService service failed or disabled at startup!`
- ×4 `VulkanInstanceService: The physical device 'Intel(R) UHD Graphics <n>' is missing 'descriptorIndexing' feature !`
- ×3 `Core: PlatformManagerService service up!`
- ×3 `no journal`
- ×2 `Application: [CEF-ACCEL] DirectComposition disabled (the windowed DevTools present path is incompatible with the process-wide shared-texture state).`
- ×2 `VulkanInstanceService: The physical device 'Microsoft Direct3D12 (Microsoft Basic Render Driver)' is missing 'descriptorIndexing' feature !`
- ×1 `Application: Crash report question in 'english' (preferred language: 'en-US').`
- … 5 other(s), 5 report(s)

Traces of the main journal (normalized, latest report; around the Fatal trace when there is one):

```
[Application][Info] (src/Application.CrashReport.cpp `Mango3D::System::Application::offerPendingCrashReport`) The user declined to send the crash report, it stays at '<path>'.
[Network::HTTPSClient][Error] (vendors/emeraude-engine/src/Tracer.cpp `EmEn::Tracer::earlySetup::<lambda_1>::operator`) performHop(), the total time budget expired.
[NetAPIClientService][Error] (vendors/emeraude-engine/src/Net/APIClient.cpp `EmEn::Net::APIClient::performRequest`) 'https:<path>' (ticket #<n>) failed: Timeout.
[Application][Info] (src/Application.CefApp.cpp `Mango3D::System::Application::OnBeforeCommandLineProcessing`) Forcing the high performance GPU in chromium ...
[Application][Info] (src/Application.CefApp.cpp `Mango3D::System::Application::OnBeforeCommandLineProcessing`) [CEF-ACCEL] Overriding the ANGLE backend 'vulkan' to 'd3d11' (required by the shared-texture path).
[Application][Info] (src/Application.CefApp.cpp `Mango3D::System::Application::OnBeforeCommandLineProcessing`) [CEF-ACCEL] Pinning the Chromium GPU process to the adapter LUID <n>,<n> (mirroring the engine GPU selection).
[Application][Info] (src/Application.CefApp.cpp `Mango3D::System::Application::OnBeforeCommandLineProcessing`) [CEF-ACCEL] DirectComposition disabled (the windowed DevTools present path is incompatible with the process-wide shared-texture state).
[Application][Info] (src/Application.CefBrowserProcessHandler.cpp `Mango3D::System::Application::OnRegisterCustomPreferences`) A global preference has been changed !
```

Archives (local mirror under `tools/crash-reports/data/`, extracted next to them in `extracted/`):

- … 33 older archive(s), see `crash_reports.py list`
- `reports/8.0.2/34ae55d271599c8d50521129327bacb7-3b685493-f512-4216-b0b7-a4eec226c454.zip` → `archives/8.0.2/34ae55d271599c8d50521129327bacb7-3b685493-f512-4216-b0b7-a4eec226c454.zip`
- `reports/8.0.2/22bf555b8f60b4a012ebc6e618620753-f93b0668-693b-4ecd-be05-e8b206b827ab.zip` → `archives/8.0.2/22bf555b8f60b4a012ebc6e618620753-f93b0668-693b-4ecd-be05-e8b206b827ab.zip`
- `reports/8.0.2/69b4e99efc17aa1f379aaed6f53c663e-dd4a3a47-5858-47df-9ad3-14bc1f8ca77f.zip` → `archives/8.0.2/69b4e99efc17aa1f379aaed6f53c663e-dd4a3a47-5858-47df-9ad3-14bc1f8ca77f.zip`
- `reports/8.0.2/ef673ca669b77ab65d5c640a6ab74018-b8c54b77-9102-454c-ae05-555594709d0b.zip` → `archives/8.0.2/ef673ca669b77ab65d5c640a6ab74018-b8c54b77-9102-454c-ae05-555594709d0b.zip`
- `reports/8.0.2/2248dfecad8f780a05ae3a27bbb3f464-1838a208-f774-4e07-bcb5-0f4af255353e.zip` → `archives/8.0.2/2248dfecad8f780a05ae3a27bbb3f464-1838a208-f774-4e07-bcb5-0f4af255353e.zip`
- `reports/8.0.2/478f1c98538b7250a246a09ef3aa3594-481c4f6e-137c-4287-bfec-2ee06e09f76b.zip` → `archives/8.0.2/478f1c98538b7250a246a09ef3aa3594-481c4f6e-137c-4287-bfec-2ee06e09f76b.zip`
- `reports/8.0.2/941ba077235816ecc3bff153cb113ade-8b112a28-2db4-4de8-952d-e765cc6d44fc.zip` → `archives/8.0.2/941ba077235816ecc3bff153cb113ade-8b112a28-2db4-4de8-952d-e765cc6d44fc.zip`
- `reports/8.0.2/9ef1374915cc1bf9953d3e3786a7e20c-5cbf4ba6-c7e3-44ae-8a2e-70409d7ee83d.zip` → `archives/8.0.2/9ef1374915cc1bf9953d3e3786a7e20c-5cbf4ba6-c7e3-44ae-8a2e-70409d7ee83d.zip`
- `reports/8.0.2/ff1fa5a69ed4fa0091f99d6ee87dce8e-a88b170a-b798-4de4-b49e-3156757f56a7.zip` → `archives/8.0.2/ff1fa5a69ed4fa0091f99d6ee87dce8e-a88b170a-b798-4de4-b49e-3156757f56a7.zip`
- `reports/8.0.2/ff70436ca2c026636e7dcfb1589cf0c6-1fdd5dce-869f-4992-8d81-173e68323205.zip` → `archives/8.0.2/ff70436ca2c026636e7dcfb1589cf0c6-1fdd5dce-869f-4992-8d81-173e68323205.zip`
- `reports/8.0.2/a10e94acb08d52a9f9a8361d2144b59f-a99f524f-823f-43c7-a2b6-d434295ad1d4.zip` → `archives/8.0.2/a10e94acb08d52a9f9a8361d2144b59f-a99f524f-823f-43c7-a2b6-d434295ad1d4.zip`
- `reports/8.0.2/8594c879b6b662419a227bd3191a76dc-eeb52599-c77e-475e-8359-b96464634313.zip` → `archives/8.0.2/8594c879b6b662419a227bd3191a76dc-eeb52599-c77e-475e-8359-b96464634313.zip`
- `reports/8.0.2/941b2543dd50f62aa8cc086a37ea13bc-089c0b69-ee08-4bad-941b-9891c5dd4223.zip` → `archives/8.0.2/941b2543dd50f62aa8cc086a37ea13bc-089c0b69-ee08-4bad-941b-9891c5dd4223.zip`
- `reports/8.0.2/97dc4f1464c93222439ab1ee8d468b81-2ccf835c-f5a8-45f3-bfae-badae3062087.zip` → `archives/8.0.2/97dc4f1464c93222439ab1ee8d468b81-2ccf835c-f5a8-45f3-bfae-badae3062087.zip`
- `reports/8.0.3/8a81b6303ef0830e281afe22a5316222-5ae5ec4c-c320-46c7-abdf-2afc330ec4d8.zip` → `archives/8.0.3/8a81b6303ef0830e281afe22a5316222-5ae5ec4c-c320-46c7-abdf-2afc330ec4d8.zip`
- `reports/8.0.2/2ab95f7eebe3714b1cc8700f4ab838df-96104445-3882-402c-8a9a-1e17fc377d05.zip` → `archives/8.0.2/2ab95f7eebe3714b1cc8700f4ab838df-96104445-3882-402c-8a9a-1e17fc377d05.zip`
- `reports/8.0.2/4631270825309d7fd63ff16408b008c0-bcaf328d-aeec-4c4a-a95c-5d63be27b65d.zip` → `archives/8.0.2/4631270825309d7fd63ff16408b008c0-bcaf328d-aeec-4c4a-a95c-5d63be27b65d.zip`
- `reports/8.0.3/42c6a21da6c1cf409b0b529e6ee513ce-86ec8975-c760-457c-855c-5c705049c9e7.zip` → `archives/8.0.3/42c6a21da6c1cf409b0b529e6ee513ce-86ec8975-c760-457c-855c-5c705049c9e7.zip`
- `reports/8.0.2/86f9a98857af1b66b8020ea40cd473fe-8c9de12a-2fba-49e1-aacc-7be315175ab3.zip` → `archives/8.0.2/86f9a98857af1b66b8020ea40cd473fe-8c9de12a-2fba-49e1-aacc-7be315175ab3.zip`
- `reports/8.0.2/916bcf74ce4d15c16318d6b85374c7d8-8619c562-5225-488f-bcec-f739cf7bbeb1.zip` → `archives/8.0.2/916bcf74ce4d15c16318d6b85374c7d8-8619c562-5225-488f-bcec-f739cf7bbeb1.zip`
<!-- crash-reports:end -->

## Triage (2026-10-21)

- **Owner**: the engine.
- **Machines**: ≈26 (reports grouped by CPU + RAM + OS fingerprint on 2026-10-21; several reports per machine
  when every launch fails).
- **Reading**: The run dies before `CoreStarted` without any Fatal: inside the engine's startup (Vulkan instance/device creation, shader cache, overlay), most likely in the driver. Last traces vary (session token written, `GraphicsRecorderService` disabled, shader binary cache…).
- **Next step**: Needs a native dump to go further (plan wave W0, minidumps). Meanwhile: compare the last trace of each report with the engine's startup order to bound the step that dies.
- **Plan**: `CrashReport Solucius`, wave W3 — app_system's `docs/plans/crashreport-solucius.md`.

## 8.0.3 status (checked 2026-10-21)

**Possibly affected.** Engine `12566475` (1.0.1) removed the startup aborts (0xC0000409) on a path outside the ANSI code page — install directory, Vulkan loader logs, `BaseInformation`. Unverified link. ⚠️ The last traces lag the death: the tracer writes asynchronously, a crash loses the last queued entries — 10 journals stop at `Core level created`, which precedes the session token that made the report: the run went further.

Checked against the changes between 8.0.2 and 8.0.3 in the four repositories (app_system 1.1.1 → 1.1.2, engine 1.0.0 → 1.0.1, app_kernel 1.3.1 → 1.3.2, core `47cbe7cd9` → `c3ab7b390`); nothing relevant after 8.0.3. Proof only comes from the 8.0.3 reports (plan § 5).

## What remains

- Read the journals of the archives above (extracted under `tools/crash-reports/data/extracted/`).
- Find the defect, reproduce it, fix it where it lives.

## References

- app_system's `docs/crash-report.md` — what an archive holds, the flight recorder milestones.
- app_system's `tools/crash-reports/README.md` — how this item was produced, how the cause is chosen.
