---
id: crash-fatal-in-vulkaninstanceservice-incompatible-driv-48a307
title: "Crash reports: Fatal in VulkanInstanceService — Incompatible driver !"
status: open
priority: unranked
scope: src/Vulkan/Instance.cpp
opened: 2026-10-21
tags: [crash, crash-report-auto]
---

# Crash reports: Fatal in VulkanInstanceService — Incompatible driver !

> Dropped by app_system's `tools/crash-reports/crash_reports.py`: a cause never seen before in the production crash reports.
> **Triage**: confirm it is a defect, give the file a better name, move it to the repository that owns the defect
> (an engine defect goes to `vendors/emeraude-engine/docs/todo/`, …) and rank it. The block between the
> `crash-reports` markers is rewritten at every run, wherever the item went (it is found by its signature line);
> write outside it. Deleting the item closes the cause: a new report of it is then flagged as a regression.

## Why

<!-- crash-reports:begin -->
| | |
|---|---|
| Machines | 65 — per version: 8.0.2 ×65 |
| Reports | 221 (first 2026-10-15, last 2026-10-21) |
| Versions | 8.0.2 ×221 |
| Platforms | windows ×221 |
| OS | Windows 10 Pro ×83, Windows 11 Pro ×59, Windows 10 Home ×49, Windows 11 Home ×10, Windows 10 Enterprise ×9, Windows 10 Home Single Language ×7, … (3 more) |
| GPU (selected) | unknown ×221 |
| ANGLE backend | d3d11 ×221 |
| Died at milestone | SessionOpened ×221 |
| Single-instance lock | not recorded ×221 |

- **Cause (normalized):** `VulkanInstanceService: Incompatible driver !`
- **Where:** `vendors/emeraude-engine/src/Vulkan/Instance.cpp` — `EmEn::Vulkan::Instance::onInitialize`
- **Signature:** `48a307142a440ca6`

Last error before the death (else the last trace), by number of reports:

- ×221 `died at SessionOpened`

Traces of the main journal (normalized, latest report; around the Fatal trace when there is one):

```
[Application][Info] (src/Application.CefBrowserProcessHandler.cpp `Mango3D::System::Application::OnRegisterCustomPreferences`) A global preference has been changed !
[Application][Info] (src/Application.CefBrowserProcessHandler.cpp `Mango3D::System::Application::OnRegisterCustomPreferences`) A request context preference has been changed !
[Application][Success] (src/Application.CefBrowserProcessHandler.cpp `Mango3D::System::Application::OnContextInitialized`) Chromium context initialized !
[PlatformManagerService][Info] (vendors/emeraude-engine/src/PlatformManager.cpp `EmEn::PlatformManager::onInitialize`) Vulkan layer search path : '<path> Files\Lychee\vulkan\explicit_layer.d'.
[VulkanLoader][Info] (vendors/emeraude-engine/src/Vulkan/Loader.cpp `EmEn::Vulkan::Loader::open`) Vulkan loader '<path> Files\Lychee\vulkan\vulkan-<n>.dll' opened (instance version <n>).
[Core][Success] (vendors/emeraude-engine/src/Core.cpp `EmEn::Core::initializeCoreLevel`) PlatformManagerService service up!
[VulkanInstanceService][Fatal] (vendors/emeraude-engine/src/Vulkan/Instance.cpp `EmEn::Vulkan::Instance::onInitialize`) Incompatible driver !
[Core][Fatal] (vendors/emeraude-engine/src/Core.cpp `EmEn::Core::initializeCoreLevel`) VulkanInstanceService service failed to execute!
```

Archives (local mirror under `tools/crash-reports/data/`, extracted next to them in `extracted/`):

- … 201 older archive(s), see `crash_reports.py list`
- `reports/8.0.2/295a1b96c9faf7aab27639d9bb69653c-5f11b2bf-8740-43d6-8215-d87b479d368a.zip` → `archives/8.0.2/295a1b96c9faf7aab27639d9bb69653c-5f11b2bf-8740-43d6-8215-d87b479d368a.zip`
- `reports/8.0.2/b5225637903a77aa3e023e3bcd128604-28f4cdab-e916-4b84-bddf-328fbde86b19.zip` → `archives/8.0.2/b5225637903a77aa3e023e3bcd128604-28f4cdab-e916-4b84-bddf-328fbde86b19.zip`
- `reports/8.0.2/eafdcc96daf54867c52b19f150b96b32-bab45e51-7bb8-4abf-af38-218d6d8f14bf.zip` → `archives/8.0.2/eafdcc96daf54867c52b19f150b96b32-bab45e51-7bb8-4abf-af38-218d6d8f14bf.zip`
- `reports/8.0.2/ffc6b56bad4d7540ad3aa0bda0f880a5-8dd8d228-cf1c-4efd-93cf-b5923182817c.zip` → `archives/8.0.2/ffc6b56bad4d7540ad3aa0bda0f880a5-8dd8d228-cf1c-4efd-93cf-b5923182817c.zip`
- `reports/8.0.2/eb9530cb172eb1d4845bdf3ebb6fd3a0-3a6789b8-6109-4bd3-b577-be04eb772331.zip` → `archives/8.0.2/eb9530cb172eb1d4845bdf3ebb6fd3a0-3a6789b8-6109-4bd3-b577-be04eb772331.zip`
- `reports/8.0.2/d499bd6f1fed41b67d8d7f10cf8bea78-690d2944-f059-435f-ac44-0d345a7db853.zip` → `archives/8.0.2/d499bd6f1fed41b67d8d7f10cf8bea78-690d2944-f059-435f-ac44-0d345a7db853.zip`
- `reports/8.0.2/3ec793613ff56f78c35678303207b6ef-ef922fca-bb1c-420a-a4f4-dca587c05d32.zip` → `archives/8.0.2/3ec793613ff56f78c35678303207b6ef-ef922fca-bb1c-420a-a4f4-dca587c05d32.zip`
- `reports/8.0.2/e07c18d2d41c7434bd1d780ffc8d2d9b-9290e75c-fc5c-455e-a825-431f3ee240a3.zip` → `archives/8.0.2/e07c18d2d41c7434bd1d780ffc8d2d9b-9290e75c-fc5c-455e-a825-431f3ee240a3.zip`
- `reports/8.0.2/3e54863e3a5bb30d8c2fb538721062e8-bd8f4ef4-1851-4b52-8c0e-345f38d31e48.zip` → `archives/8.0.2/3e54863e3a5bb30d8c2fb538721062e8-bd8f4ef4-1851-4b52-8c0e-345f38d31e48.zip`
- `reports/8.0.2/62e470242d1cb13183487d55cde0aea0-30dd1b82-fe08-4097-a141-a0d190697767.zip` → `archives/8.0.2/62e470242d1cb13183487d55cde0aea0-30dd1b82-fe08-4097-a141-a0d190697767.zip`
- `reports/8.0.2/06fb2152069e39a7aafc1e8c4d89e2d5-ce01c095-2b5d-49ea-a180-ff1ebd088321.zip` → `archives/8.0.2/06fb2152069e39a7aafc1e8c4d89e2d5-ce01c095-2b5d-49ea-a180-ff1ebd088321.zip`
- `reports/8.0.2/5ff19acebe63a3e9bd5898d760cb5057-eba2ed8b-14de-492c-b9b7-f246fd01c0f2.zip` → `archives/8.0.2/5ff19acebe63a3e9bd5898d760cb5057-eba2ed8b-14de-492c-b9b7-f246fd01c0f2.zip`
- `reports/8.0.2/516ab1ec5117fa18dd0cc9d940fe14ba-0754fe37-b6da-423e-87d8-28003004a96f.zip` → `archives/8.0.2/516ab1ec5117fa18dd0cc9d940fe14ba-0754fe37-b6da-423e-87d8-28003004a96f.zip`
- `reports/8.0.2/ff1803f315d717bc8d05d9a5a6f9a268-ce5a87a0-f75b-4663-8298-24bf3df4f96d.zip` → `archives/8.0.2/ff1803f315d717bc8d05d9a5a6f9a268-ce5a87a0-f75b-4663-8298-24bf3df4f96d.zip`
- `reports/8.0.2/866b69cc721fb8289c6263b7261af571-5e71489e-9765-439e-be7a-1d00c453de5f.zip` → `archives/8.0.2/866b69cc721fb8289c6263b7261af571-5e71489e-9765-439e-be7a-1d00c453de5f.zip`
- `reports/8.0.2/26a86697e82a3190dce001b1a8059b82-a415bda8-3ecd-4517-b6ea-fe721c57fea0.zip` → `archives/8.0.2/26a86697e82a3190dce001b1a8059b82-a415bda8-3ecd-4517-b6ea-fe721c57fea0.zip`
- `reports/8.0.2/a11715e1d3e8012dc5142523f8b97463-45a174af-1daa-491c-bdb5-85477daf8f73.zip` → `archives/8.0.2/a11715e1d3e8012dc5142523f8b97463-45a174af-1daa-491c-bdb5-85477daf8f73.zip`
- `reports/8.0.2/b789fbfa6f1b369b21036dcd3e9e3056-bb41e8c1-cab9-4755-afdc-3c429f07b129.zip` → `archives/8.0.2/b789fbfa6f1b369b21036dcd3e9e3056-bb41e8c1-cab9-4755-afdc-3c429f07b129.zip`
- `reports/8.0.2/b4b9deadea9060b829f67c54992c3432-884cc812-88df-4d6f-a59e-b6bb271f82c8.zip` → `archives/8.0.2/b4b9deadea9060b829f67c54992c3432-884cc812-88df-4d6f-a59e-b6bb271f82c8.zip`
- `reports/8.0.2/cca48fd232a584a7650dfefa4336a649-430c1a25-a104-48b4-872c-6249eac2acca.zip` → `archives/8.0.2/cca48fd232a584a7650dfefa4336a649-430c1a25-a104-48b4-872c-6249eac2acca.zip`
<!-- crash-reports:end -->

## Triage (2026-10-21)

- **Owner**: the engine.
- **Machines**: ≈61 (reports grouped by CPU + RAM + OS fingerprint on 2026-10-21; several reports per machine
  when every launch fails).
- **Reading**: `vkCreateInstance()` answers `VK_ERROR_INCOMPATIBLE_DRIVER`: no Vulkan driver usable. All 8.0.2 / engine 1.0.0, every report dies at `SessionOpened`, and each new launch reports the previous one: up to 15 reports per machine.
- **Next step**: Engine 1.0.1 (shipped with 8.0.3) classifies it as `NoVulkanDriver`: a dialog, exit code 97 (`docs/subsystems/vulkan/22-startup-refusals-told-to-the-user.md`). Confirm on the 8.0.3 reports that it is no longer a Fatal; what remains is app_system's: an exit 97 is still reported as a crash at every launch (app_system item `crash-report-incompatible-system-is-not-a-crash`).
- **Plan**: `CrashReport Solucius`, wave W1 — app_system's `docs/plans/crashreport-solucius.md`.

## 8.0.3 status (checked 2026-10-21)

**Changed.** The machines have no usable Vulkan driver: the bundled loader (1.4.363) opens, then `vkCreateInstance()` answers `VK_ERROR_INCOMPATIBLE_DRIVER` — no ICD at all; the Vulkan 1.1 target of 8.0.3 does not change that. Engine 1.0.1 tells it instead of a Fatal: `NoVulkanDriver` dialog with the GPUs listed by the OS (`d166a93c`, `c4b115f8`), exit 97. These machines still cannot run Lychee, and app_system still reports exit 97 at every launch (`crash-report-incompatible-system-is-not-a-crash`).

Checked against the changes between 8.0.2 and 8.0.3 in the four repositories (app_system 1.1.1 → 1.1.2, engine 1.0.0 → 1.0.1, app_kernel 1.3.1 → 1.3.2, core `47cbe7cd9` → `c3ab7b390`); nothing relevant after 8.0.3. Proof only comes from the 8.0.3 reports (plan § 5).

## What remains

- Read the journals of the archives above (extracted under `tools/crash-reports/data/extracted/`).
- Find the defect, reproduce it, fix it where it lives.

## References

- app_system's `docs/crash-report.md` — what an archive holds, the flight recorder milestones.
- app_system's `tools/crash-reports/README.md` — how this item was produced, how the cause is chosen.
