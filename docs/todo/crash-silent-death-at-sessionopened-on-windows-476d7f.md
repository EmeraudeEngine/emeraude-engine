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
| Reports | 44 (first 2026-10-15, last 2026-10-22) |
| Versions | 8.0.2 ×44 |
| Platforms | windows ×44 |
| OS | Windows 11 Home ×30, Windows 10 Pro ×6, Windows 11 Pro ×5, Windows 10 Home ×1, Windows 10 Home Single Language ×1, Windows 11 Home Single Language ×1 |
| GPU (selected) | unknown ×44 |
| ANGLE backend | d3d11 ×32, default ×12 |
| Died at milestone | SessionOpened ×44 |
| Single-instance lock | not recorded ×44 |

- **Cause (normalized):** `died at SessionOpened on windows`
- **Where:** no source location
- **Signature:** `476d7fc761290d14`

Last error before the death (else the last trace), by number of reports:

- ×10 `CrashReporter: Session token '<path>' written.`
- ×6 `ShaderManagerService: The shader 'OverlayRenderingFragmentShader' (<n> bytes) loaded from binary cache !`
- ×5 `RendererService: GraphicsRecorderService service failed or disabled at startup!`
- ×4 `NetAPIClientService: 'https:<path>' (ticket #<n>) answered HTTP <n> (<n> bytes).`
- ×4 `VulkanInstanceService: The physical device 'Intel(R) UHD Graphics <n>' is missing 'descriptorIndexing' feature !`
- ×3 `Core: PlatformManagerService service up!`
- ×3 `no journal`
- ×2 `Application: [CEF-ACCEL] DirectComposition disabled (the windowed DevTools present path is incompatible with the process-wide shared-texture state).`
- ×2 `VulkanInstanceService: The physical device 'Microsoft Direct3D12 (Microsoft Basic Render Driver)' is missing 'descriptorIndexing' feature !`
- ×1 `Application: Crash report question in 'english' (preferred language: 'en-US').`
- … 4 other(s), 4 report(s)

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

Archives (local mirror under `tools/crash-reports/data/`, extracted next to them in `extracted/`):

- … 24 older archive(s), see `crash_reports.py list`
- `reports/8.0.2/3ef76c9dbb164b7f438831bf3be41304-ce4dbd50-6241-445b-af49-bf00fc06db06.zip` → `archives/8.0.2/3ef76c9dbb164b7f438831bf3be41304-ce4dbd50-6241-445b-af49-bf00fc06db06.zip`
- `reports/8.0.2/6b167dd28974f89cedf4270641d15dc7-bc07fb1f-1a68-4223-9b11-72f2b802fdc9.zip` → `archives/8.0.2/6b167dd28974f89cedf4270641d15dc7-bc07fb1f-1a68-4223-9b11-72f2b802fdc9.zip`
- `reports/8.0.2/2da5cd91b99d4b5e2858f6f6fbdd02ac-a2084ac4-0fe1-4dfb-8e78-f6c0a761a3c2.zip` → `archives/8.0.2/2da5cd91b99d4b5e2858f6f6fbdd02ac-a2084ac4-0fe1-4dfb-8e78-f6c0a761a3c2.zip`
- `reports/8.0.2/a6be903a5eb54a226249595bab9bde18-06f04268-67da-4517-899b-52ac9a27409a.zip` → `archives/8.0.2/a6be903a5eb54a226249595bab9bde18-06f04268-67da-4517-899b-52ac9a27409a.zip`
- `reports/8.0.2/2224056a17efae3a8bdbbfffb9d426b7-cde975c8-d673-406f-99a8-9930a5a60e10.zip` → `archives/8.0.2/2224056a17efae3a8bdbbfffb9d426b7-cde975c8-d673-406f-99a8-9930a5a60e10.zip`
- `reports/8.0.2/4933581d33420ef178ae18016414abab-807c2633-e943-4787-bbf5-17c0c92c9149.zip` → `archives/8.0.2/4933581d33420ef178ae18016414abab-807c2633-e943-4787-bbf5-17c0c92c9149.zip`
- `reports/8.0.2/2f7fc657332e60d20e52dbfd8f112208-e5c4aa7d-60fa-416c-9b5b-096c0abd4cc6.zip` → `archives/8.0.2/2f7fc657332e60d20e52dbfd8f112208-e5c4aa7d-60fa-416c-9b5b-096c0abd4cc6.zip`
- `reports/8.0.2/09a3158c651989726b1a74fb5d27a1ba-be1d8b6d-8a26-466d-966f-77a5787ea722.zip` → `archives/8.0.2/09a3158c651989726b1a74fb5d27a1ba-be1d8b6d-8a26-466d-966f-77a5787ea722.zip`
- `reports/8.0.2/73e4f0292cb634d88029074767dc799a-4b8afdf0-9106-45d9-bc8a-968301a92671.zip` → `archives/8.0.2/73e4f0292cb634d88029074767dc799a-4b8afdf0-9106-45d9-bc8a-968301a92671.zip`
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
