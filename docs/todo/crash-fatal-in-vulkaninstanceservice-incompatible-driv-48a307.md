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
| Reports | 204 (first 2026-10-15, last 2026-10-21) |
| Versions | 8.0.2 ×204 |
| Platforms | windows ×204 |
| OS | Windows 10 Pro ×81, Windows 11 Pro ×52, Windows 10 Home ×44, Windows 11 Home ×10, Windows 10 Enterprise ×9, Windows 10 Home Single Language ×4, … (3 more) |
| GPU (selected) | unknown ×204 |
| ANGLE backend | d3d11 ×204 |
| Died at milestone | SessionOpened ×204 |

- **Cause (normalized):** `VulkanInstanceService: Incompatible driver !`
- **Where:** `vendors/emeraude-engine/src/Vulkan/Instance.cpp` — `EmEn::Vulkan::Instance::onInitialize`
- **Signature:** `48a307142a440ca6`

Last error before the death (else the last trace), by number of reports:

- ×204 `died at SessionOpened`

Traces of the main journal (normalized, latest report; around the Fatal trace when there is one):

```
[Application][Info] (src/Application.CefBrowserProcessHandler.cpp `Mango3D::System::Application::OnRegisterCustomPreferences`) A request context preference has been changed !
[NetAPIClientService][Success] (vendors/emeraude-engine/src/Net/APIClient.cpp `EmEn::Net::APIClient::performRequest`) 'https:<path>' (ticket #<n>) answered HTTP <n> (<n> bytes).
[Application][Success] (src/Application.CefBrowserProcessHandler.cpp `Mango3D::System::Application::OnContextInitialized`) Chromium context initialized !
[PlatformManagerService][Info] (vendors/emeraude-engine/src/PlatformManager.cpp `EmEn::PlatformManager::onInitialize`) Vulkan layer search path : '<path> Files\Lychee\vulkan\explicit_layer.d'.
[VulkanLoader][Info] (vendors/emeraude-engine/src/Vulkan/Loader.cpp `EmEn::Vulkan::Loader::open`) Vulkan loader '<path> Files\Lychee\vulkan\vulkan-<n>.dll' opened (instance version <n>).
[Core][Success] (vendors/emeraude-engine/src/Core.cpp `EmEn::Core::initializeCoreLevel`) PlatformManagerService service up!
[VulkanInstanceService][Fatal] (vendors/emeraude-engine/src/Vulkan/Instance.cpp `EmEn::Vulkan::Instance::onInitialize`) Incompatible driver !
[Core][Fatal] (vendors/emeraude-engine/src/Core.cpp `EmEn::Core::initializeCoreLevel`) VulkanInstanceService service failed to execute!
```

Archives (local mirror under `tools/crash-reports/data/`, extracted next to them in `extracted/`):

- … 184 older archive(s), see `crash_reports.py list`
- `reports/8.0.2/5c54f08152ca252308321fab6594291f-b2c15364-442e-49bd-9df1-b6cbe83f7e77.zip` → `archives/8.0.2/5c54f08152ca252308321fab6594291f-b2c15364-442e-49bd-9df1-b6cbe83f7e77.zip`
- `reports/8.0.2/3de8a4c925af45af61c7ef165a22f228-82bfdcad-2cc3-49e5-89cb-cab40f81ae93.zip` → `archives/8.0.2/3de8a4c925af45af61c7ef165a22f228-82bfdcad-2cc3-49e5-89cb-cab40f81ae93.zip`
- `reports/8.0.2/208326056f30203f8760921da615638e-329cdba1-ff22-4fb2-9af1-9dd781ea40b1.zip` → `archives/8.0.2/208326056f30203f8760921da615638e-329cdba1-ff22-4fb2-9af1-9dd781ea40b1.zip`
- `reports/8.0.2/8a971ac47c84e5f3c303d55c60fc3846-971da2c0-9b90-4599-b0aa-897eedca4d83.zip` → `archives/8.0.2/8a971ac47c84e5f3c303d55c60fc3846-971da2c0-9b90-4599-b0aa-897eedca4d83.zip`
- `reports/8.0.2/4944c0de35aa6b93f8d47cdd63c40c69-2b1db6b7-5b80-4f14-9f19-9730dea1862d.zip` → `archives/8.0.2/4944c0de35aa6b93f8d47cdd63c40c69-2b1db6b7-5b80-4f14-9f19-9730dea1862d.zip`
- `reports/8.0.2/b4cc8c5b0870ebf525b2d02e6c12f63f-31f90da9-4ecf-4fb0-90ee-525c6dacc624.zip` → `archives/8.0.2/b4cc8c5b0870ebf525b2d02e6c12f63f-31f90da9-4ecf-4fb0-90ee-525c6dacc624.zip`
- `reports/8.0.2/5fefbff2c81f7129d59fe0213cdc57cf-8f33dc4c-755d-4302-add6-5cc7cc55532f.zip` → `archives/8.0.2/5fefbff2c81f7129d59fe0213cdc57cf-8f33dc4c-755d-4302-add6-5cc7cc55532f.zip`
- `reports/8.0.2/c0bf17e0959194e5946a5dbfa7389be6-879d979a-b854-40af-bebe-e504c7dade23.zip` → `archives/8.0.2/c0bf17e0959194e5946a5dbfa7389be6-879d979a-b854-40af-bebe-e504c7dade23.zip`
- `reports/8.0.2/73e4f0292cb634d88029074767dc799a-4b8afdf0-9106-45d9-bc8a-968301a92671.zip` → `archives/8.0.2/73e4f0292cb634d88029074767dc799a-4b8afdf0-9106-45d9-bc8a-968301a92671.zip`
- `reports/8.0.2/5e97cac5c4ad6a848b0397c0e205c027-5eb0e3b9-37dc-49e3-a451-7ee49122f63f.zip` → `archives/8.0.2/5e97cac5c4ad6a848b0397c0e205c027-5eb0e3b9-37dc-49e3-a451-7ee49122f63f.zip`
- `reports/8.0.2/d7b362b8b91d3280ed1967599c8c7729-2e5bb737-bcb7-4322-902c-1371c54606a0.zip` → `archives/8.0.2/d7b362b8b91d3280ed1967599c8c7729-2e5bb737-bcb7-4322-902c-1371c54606a0.zip`
- `reports/8.0.2/4bbdedb5e156c95f01c03b9a1faf0899-4056fe37-6f9c-4826-a57b-54bb489fed14.zip` → `archives/8.0.2/4bbdedb5e156c95f01c03b9a1faf0899-4056fe37-6f9c-4826-a57b-54bb489fed14.zip`
- `reports/8.0.2/d6e207d7b13ad6a4534ae312bdc38703-200966ea-7ed6-4398-8f42-3fc311690804.zip` → `archives/8.0.2/d6e207d7b13ad6a4534ae312bdc38703-200966ea-7ed6-4398-8f42-3fc311690804.zip`
- `reports/8.0.2/2f1f3ad401e1456763fdb02f76df9fb5-f56c9518-ab88-4e97-84e1-4d7e3fd149b9.zip` → `archives/8.0.2/2f1f3ad401e1456763fdb02f76df9fb5-f56c9518-ab88-4e97-84e1-4d7e3fd149b9.zip`
- `reports/8.0.2/f02384236b7f5438cc20ddee7e476cce-5fcf4284-6d10-44e2-a787-ed24bee83bb9.zip` → `archives/8.0.2/f02384236b7f5438cc20ddee7e476cce-5fcf4284-6d10-44e2-a787-ed24bee83bb9.zip`
- `reports/8.0.2/edb57531adaf8254c1b65a63e5e0ef18-75ae0538-db14-4bf2-b762-a45c5ba8a3f2.zip` → `archives/8.0.2/edb57531adaf8254c1b65a63e5e0ef18-75ae0538-db14-4bf2-b762-a45c5ba8a3f2.zip`
- `reports/8.0.2/49a46a21a67f020ac6a7512409b29b2a-9b4f7579-c08b-4ce4-a64b-09c79426c97b.zip` → `archives/8.0.2/49a46a21a67f020ac6a7512409b29b2a-9b4f7579-c08b-4ce4-a64b-09c79426c97b.zip`
- `reports/8.0.2/295a1b96c9faf7aab27639d9bb69653c-5f11b2bf-8740-43d6-8215-d87b479d368a.zip` → `archives/8.0.2/295a1b96c9faf7aab27639d9bb69653c-5f11b2bf-8740-43d6-8215-d87b479d368a.zip`
- `reports/8.0.2/b5225637903a77aa3e023e3bcd128604-28f4cdab-e916-4b84-bddf-328fbde86b19.zip` → `archives/8.0.2/b5225637903a77aa3e023e3bcd128604-28f4cdab-e916-4b84-bddf-328fbde86b19.zip`
- `reports/8.0.2/eafdcc96daf54867c52b19f150b96b32-bab45e51-7bb8-4abf-af38-218d6d8f14bf.zip` → `archives/8.0.2/eafdcc96daf54867c52b19f150b96b32-bab45e51-7bb8-4abf-af38-218d6d8f14bf.zip`
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
