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
| Reports | 35 (first 2026-10-15, last 2026-10-21) |
| Versions | 8.0.2 ×35 |
| Platforms | windows ×35 |
| OS | Windows 11 Home ×23, Windows 10 Pro ×6, Windows 11 Pro ×3, Windows 10 Home ×1, Windows 10 Home Single Language ×1, Windows 11 Home Single Language ×1 |
| GPU (selected) | unknown ×35 |
| ANGLE backend | d3d11 ×24, default ×11 |
| Died at milestone | SessionOpened ×35 |

- **Cause (normalized):** `died at SessionOpened on windows`
- **Where:** no source location
- **Signature:** `476d7fc761290d14`

Last error before the death (else the last trace), by number of reports:

- ×10 `CrashReporter: Session token '<path>' written.`
- ×5 `RendererService: GraphicsRecorderService service failed or disabled at startup!`
- ×5 `ShaderManagerService: The shader 'OverlayRenderingFragmentShader' (<n> bytes) loaded from binary cache !`
- ×3 `Core: PlatformManagerService service up!`
- ×3 `NetAPIClientService: 'https:<path>' (ticket #<n>) answered HTTP <n> (<n> bytes).`
- ×3 `no journal`
- ×2 `VulkanInstanceService: The physical device 'Microsoft Direct3D12 (Microsoft Basic Render Driver)' is missing 'descriptorIndexing' feature !`
- ×1 `Application: Crash report question in 'english' (preferred language: 'en-US').`
- ×1 `Application: [CEF-ACCEL] DirectComposition disabled (the windowed DevTools present path is incompatible with the process-wide shared-texture state).`
- ×1 `Core: SystemNotificationService service up!`
- … 1 other(s), 1 report(s)

Traces of the main journal (normalized, latest report; around the Fatal trace when there is one):

```
[ResourcesManagerService][Success] (vendors/emeraude-engine/src/Resources/Manager.cpp `EmEn::Resources::Manager::onInitialize`) Basic ground manager service up !
[ResourcesManagerService][Success] (vendors/emeraude-engine/src/Resources/Manager.cpp `EmEn::Resources::Manager::onInitialize`) Basic sea manager service up !
[ResourcesManagerService][Success] (vendors/emeraude-engine/src/Resources/Manager.cpp `EmEn::Resources::Manager::onInitialize`) Cloud shape manager service up !
[ResourcesManagerService][Success] (vendors/emeraude-engine/src/Resources/Manager.cpp `EmEn::Resources::Manager::onInitialize`) Scene definition manager service up !
[Core][Success] (vendors/emeraude-engine/src/Core.cpp `EmEn::Core::initializeBaseLevel`) ResourcesManagerService service up !
[Core][Success] (vendors/emeraude-engine/src/Core.cpp `EmEn::Core::initializeBaseLevel`) UserService service up !
[Core][Success] (vendors/emeraude-engine/src/Core.cpp `EmEn::Core::initializeBaseLevel`) *** Core level created ***
[CrashReporter][Success] (src/CrashReporter.cpp `Mango3D::System::CrashReporter::openSession`) Session token '<path>' written.
```

Archives (local mirror under `tools/crash-reports/data/`, extracted next to them in `extracted/`):

- … 15 older archive(s), see `crash_reports.py list`
- `reports/8.0.2/6ab1a5df1490ab296be156ab916d5d78-f0b88f4b-f694-4b88-8bb3-5c85c7ce20e0.zip` → `archives/8.0.2/6ab1a5df1490ab296be156ab916d5d78-f0b88f4b-f694-4b88-8bb3-5c85c7ce20e0.zip`
- `reports/8.0.2/bf88e4e5228fe1b2c60114c77077f1af-c9e615f4-c4a5-4b77-966a-7d3c577d8b01.zip` → `archives/8.0.2/bf88e4e5228fe1b2c60114c77077f1af-c9e615f4-c4a5-4b77-966a-7d3c577d8b01.zip`
- `reports/8.0.2/c4834236315e4fe0159bedede32d2167-0fcf18c8-126a-4821-a13a-a0929ccc6b81.zip` → `archives/8.0.2/c4834236315e4fe0159bedede32d2167-0fcf18c8-126a-4821-a13a-a0929ccc6b81.zip`
- `reports/8.0.2/22f433f37bf3c5f2f83053d0aa3a2c4a-372d46fe-19a3-4cdf-b0ef-b676a065148f.zip` → `archives/8.0.2/22f433f37bf3c5f2f83053d0aa3a2c4a-372d46fe-19a3-4cdf-b0ef-b676a065148f.zip`
- `reports/8.0.2/3340c41ffc40bcc25632f778c1e99d3a-aa159159-d738-4385-9ec0-71412902e23b.zip` → `archives/8.0.2/3340c41ffc40bcc25632f778c1e99d3a-aa159159-d738-4385-9ec0-71412902e23b.zip`
- `reports/8.0.2/3f6018a87594a557e188056b586b2913-441a7c01-ec1c-4d90-9e31-3d7dea9f8eba.zip` → `archives/8.0.2/3f6018a87594a557e188056b586b2913-441a7c01-ec1c-4d90-9e31-3d7dea9f8eba.zip`
- `reports/8.0.2/5562ad2822d2a70f2f9a7e90ed172c3a-bb24bb99-061b-4107-a081-3ca5828f0253.zip` → `archives/8.0.2/5562ad2822d2a70f2f9a7e90ed172c3a-bb24bb99-061b-4107-a081-3ca5828f0253.zip`
- `reports/8.0.2/63abf41d69c15e226a77d01b4ebea99c-bb64e46e-107e-4640-8261-93ae10c735b3.zip` → `archives/8.0.2/63abf41d69c15e226a77d01b4ebea99c-bb64e46e-107e-4640-8261-93ae10c735b3.zip`
- `reports/8.0.2/850e9ec2ec324da7befd0771dd5b815b-23afac2f-11fa-4699-ae76-36199f162392.zip` → `archives/8.0.2/850e9ec2ec324da7befd0771dd5b815b-23afac2f-11fa-4699-ae76-36199f162392.zip`
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
<!-- crash-reports:end -->

## Triage (2026-10-21)

- **Owner**: the engine.
- **Machines**: ≈26 (reports grouped by CPU + RAM + OS fingerprint on 2026-10-21; several reports per machine
  when every launch fails).
- **Reading**: The run dies before `CoreStarted` without any Fatal: inside the engine's startup (Vulkan instance/device creation, shader cache, overlay), most likely in the driver. Last traces vary (session token written, `GraphicsRecorderService` disabled, shader binary cache…).
- **Next step**: Needs a native dump to go further (plan wave W0, minidumps). Meanwhile: compare the last trace of each report with the engine's startup order to bound the step that dies.
- **Plan**: `CrashReport Solucius`, wave W3 — app_system's `docs/plans/crashreport-solucius.md`.

## What remains

- Read the journals of the archives above (extracted under `tools/crash-reports/data/extracted/`).
- Find the defect, reproduce it, fix it where it lives.

## References

- app_system's `docs/crash-report.md` — what an archive holds, the flight recorder milestones.
- app_system's `tools/crash-reports/README.md` — how this item was produced, how the cause is chosen.
