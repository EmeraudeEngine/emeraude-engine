---
id: crash-fatal-in-rendererservice-unable-to-find-a-suitab-d44787
title: "Crash reports: Fatal in RendererService — Unable to find a suitable graphics device!"
status: open
priority: unranked
scope: src/Graphics/Renderer.cpp
opened: 2026-10-21
tags: [crash, crash-report-auto]
---

# Crash reports: Fatal in RendererService — Unable to find a suitable graphics device!

> Dropped by app_system's `tools/crash-reports/crash_reports.py`: a cause never seen before in the production crash reports.
> **Triage**: confirm it is a defect, give the file a better name, move it to the repository that owns the defect
> (an engine defect goes to `vendors/emeraude-engine/docs/todo/`, …) and rank it. The block between the
> `crash-reports` markers is rewritten at every run, wherever the item went (it is found by its signature line);
> write outside it. Deleting the item closes the cause: a new report of it is then flagged as a regression.

## Why

<!-- crash-reports:begin -->
| | |
|---|---|
| Reports | 63 (first 2026-10-15, last 2026-10-21) |
| Versions | 8.0.2 ×63 |
| Platforms | windows ×63 |
| OS | Windows 10 Home ×32, Windows 10 Pro ×14, Windows 11 Home ×7, Windows 11 Pro ×4, Windows 10 Enterprise LTSC 2019 ×2, Windows 11 Enterprise ×2, … (2 more) |
| GPU (selected) | unknown ×63 |
| ANGLE backend | d3d11 ×63 |
| Died at milestone | SessionOpened ×63 |

- **Cause (normalized):** `RendererService: Unable to find a suitable graphics device!`
- **Where:** `vendors/emeraude-engine/src/Graphics/Renderer.cpp` — `EmEn::Graphics::Renderer::initializeRenderingStack`
- **Signature:** `d44787f2ff78e6ee`

Last error before the death (else the last trace), by number of reports:

- ×63 `died at SessionOpened`

Traces of the main journal (normalized, latest report; around the Fatal trace when there is one):

```
[VulkanInstanceService][Warning] (vendors/emeraude-engine/src/Vulkan/Instance.cpp `EmEn::Vulkan::Instance::checkDevicesFeaturesForGraphics`) The physical device '840M' is missing 'sparseResidency4Samples' feature !
[VulkanInstanceService][Warning] (vendors/emeraude-engine/src/Vulkan/Instance.cpp `EmEn::Vulkan::Instance::checkDevicesFeaturesForGraphics`) The physical device '840M' is missing 'sparseResidency8Samples' feature !
[VulkanInstanceService][Warning] (vendors/emeraude-engine/src/Vulkan/Instance.cpp `EmEn::Vulkan::Instance::checkDevicesFeaturesForGraphics`) The physical device '840M' is missing 'sparseResidency16Samples' feature !
[VulkanInstanceService][Warning] (vendors/emeraude-engine/src/Vulkan/Instance.cpp `EmEn::Vulkan::Instance::checkDevicesFeaturesForGraphics`) The physical device '840M' is missing 'sparseResidencyAliased' feature !
[VulkanInstanceService][Error] (vendors/emeraude-engine/src/Vulkan/Instance.cpp `EmEn::Vulkan::Instance::checkDevicesFeaturesForGraphics`) The physical device '840M' is missing 'descriptorIndexing' feature !
[VulkanInstanceService][Error] (vendors/emeraude-engine/src/Vulkan/Instance.cpp `EmEn::Vulkan::Instance::getGraphicsDevice`) There is no physical device (Graphics) compatible with Vulkan.
[RendererService][Fatal] (vendors/emeraude-engine/src/Graphics/Renderer.cpp `EmEn::Graphics::Renderer::initializeRenderingStack`) Unable to find a suitable graphics device!
[Core][Fatal] (vendors/emeraude-engine/src/Core.cpp `EmEn::Core::initializeCoreLevel`) RendererService service failed to execute!
```

Archives (local mirror under `tools/crash-reports/data/`, extracted next to them in `extracted/`):

- … 43 older archive(s), see `crash_reports.py list`
- `reports/8.0.2/5a29f4752863c65d463dbb354ecd5f9f-fcf80c77-ff7a-4155-94e7-76cbf3b92469.zip` → `archives/8.0.2/5a29f4752863c65d463dbb354ecd5f9f-fcf80c77-ff7a-4155-94e7-76cbf3b92469.zip`
- `reports/8.0.2/2707beecec2c5c2d6779dd2fc43ce994-bac94a21-c663-4408-9be7-71c20cdbf644.zip` → `archives/8.0.2/2707beecec2c5c2d6779dd2fc43ce994-bac94a21-c663-4408-9be7-71c20cdbf644.zip`
- `reports/8.0.2/18d4b524ca31185bcf363f8318e9598a-f5add297-0bb4-4f20-b264-00e7770bea35.zip` → `archives/8.0.2/18d4b524ca31185bcf363f8318e9598a-f5add297-0bb4-4f20-b264-00e7770bea35.zip`
- `reports/8.0.2/293f98eec79163cac759122b93dc3a0b-685aa166-aad2-4ef8-970f-f31c2f0a015f.zip` → `archives/8.0.2/293f98eec79163cac759122b93dc3a0b-685aa166-aad2-4ef8-970f-f31c2f0a015f.zip`
- `reports/8.0.2/65b2463573be16074b2d68115cb86f1b-8ae4be3b-2d71-4a12-9664-45eb549b4606.zip` → `archives/8.0.2/65b2463573be16074b2d68115cb86f1b-8ae4be3b-2d71-4a12-9664-45eb549b4606.zip`
- `reports/8.0.2/ef6128879787430ad05a0d51156a4413-e8259fbc-7f84-422e-94a5-9453d162d795.zip` → `archives/8.0.2/ef6128879787430ad05a0d51156a4413-e8259fbc-7f84-422e-94a5-9453d162d795.zip`
- `reports/8.0.2/96991ca2c2b514bb69329a1317763ef2-5c8ed2bd-f75f-4222-a3f9-f4eb971726b1.zip` → `archives/8.0.2/96991ca2c2b514bb69329a1317763ef2-5c8ed2bd-f75f-4222-a3f9-f4eb971726b1.zip`
- `reports/8.0.2/41f1c967e3cef4cc918537b1acf0b086-eada1795-066b-472e-9f6b-f1058609000c.zip` → `archives/8.0.2/41f1c967e3cef4cc918537b1acf0b086-eada1795-066b-472e-9f6b-f1058609000c.zip`
- `reports/8.0.2/22ecce5c83973a3df9b4ddda8dee5881-ea62c7c9-e1e8-47c0-ae2b-b76ee17078cb.zip` → `archives/8.0.2/22ecce5c83973a3df9b4ddda8dee5881-ea62c7c9-e1e8-47c0-ae2b-b76ee17078cb.zip`
- `reports/8.0.2/1f376becc7104ae3c0b3652fbfe5fbcf-6252c80b-be12-42aa-b116-6bfd61fd2dc2.zip` → `archives/8.0.2/1f376becc7104ae3c0b3652fbfe5fbcf-6252c80b-be12-42aa-b116-6bfd61fd2dc2.zip`
- `reports/8.0.2/699a3481261ba81c6417fbce8bf47f20-8bd78b97-e4f0-4a81-8770-1ec7473eca92.zip` → `archives/8.0.2/699a3481261ba81c6417fbce8bf47f20-8bd78b97-e4f0-4a81-8770-1ec7473eca92.zip`
- `reports/8.0.2/ecf33a0b83e155589ddf53efa3c6fa03-90ef4ac7-3dfc-4e9e-a647-7a2e1e21e531.zip` → `archives/8.0.2/ecf33a0b83e155589ddf53efa3c6fa03-90ef4ac7-3dfc-4e9e-a647-7a2e1e21e531.zip`
- `reports/8.0.2/c9b36d3635e9a50182a361b97146b929-1fa4a8e8-b9ee-4ea1-a768-293edd708b90.zip` → `archives/8.0.2/c9b36d3635e9a50182a361b97146b929-1fa4a8e8-b9ee-4ea1-a768-293edd708b90.zip`
- `reports/8.0.2/be6164ffe2663f241c1b76af5e685f1d-b5e532cf-a1d2-4adf-91e0-dc7038fe49ed.zip` → `archives/8.0.2/be6164ffe2663f241c1b76af5e685f1d-b5e532cf-a1d2-4adf-91e0-dc7038fe49ed.zip`
- `reports/8.0.2/dbd52caeb15a94e3d41d2e81cf6b4218-b899cb15-5965-4c10-953f-280b119ecfc5.zip` → `archives/8.0.2/dbd52caeb15a94e3d41d2e81cf6b4218-b899cb15-5965-4c10-953f-280b119ecfc5.zip`
- `reports/8.0.2/60609640045f37a357c786b3ace043ad-f423db22-8c1b-4c95-b336-86f3e02e7528.zip` → `archives/8.0.2/60609640045f37a357c786b3ace043ad-f423db22-8c1b-4c95-b336-86f3e02e7528.zip`
- `reports/8.0.2/704871ca796031e6ba7d2433304ea173-a3194f13-c9d4-41e1-962a-f1a4a0a52ee8.zip` → `archives/8.0.2/704871ca796031e6ba7d2433304ea173-a3194f13-c9d4-41e1-962a-f1a4a0a52ee8.zip`
- `reports/8.0.2/6d69de9526bfff24c7a93898f99c720f-6e51c82d-336b-4df1-8ea8-572b795e57c2.zip` → `archives/8.0.2/6d69de9526bfff24c7a93898f99c720f-6e51c82d-336b-4df1-8ea8-572b795e57c2.zip`
- `reports/8.0.2/a4b83ddde25ebc9bda6c858af1acd4df-81fc101c-099a-42c3-8fab-f7725c3c5e6b.zip` → `archives/8.0.2/a4b83ddde25ebc9bda6c858af1acd4df-81fc101c-099a-42c3-8fab-f7725c3c5e6b.zip`
- `reports/8.0.2/baa79cdb55ceffbf154d9a634c4e9cdb-d59d3db6-9e87-4420-902b-ffae8a443bec.zip` → `archives/8.0.2/baa79cdb55ceffbf154d9a634c4e9cdb-d59d3db6-9e87-4420-902b-ffae8a443bec.zip`
<!-- crash-reports:end -->

## Triage (2026-10-21)

- **Owner**: the engine.
- **Machines**: ≈21 (reports grouped by CPU + RAM + OS fingerprint on 2026-10-21; several reports per machine
  when every launch fails).
- **Reading**: Devices exist, none passes the renderer's selection. The trace does not say which requirement each one failed.
- **Next step**: Engine 1.0.1 should classify it as `OutdatedDriver` / `IncompatibleHardware` (exit 97, dialog naming the GPU and what it lacks). Confirm on the 8.0.3 reports; if it is still `Other`, make the selection say why each device was refused.
- **Plan**: `CrashReport Solucius`, wave W1 — app_system's `docs/plans/crashreport-solucius.md`.

## 8.0.3 status (checked 2026-10-21)

**Fixed.** The devices were refused for scene-rendering features (`descriptorIndexing` and the other bindless ones, logged per device). At the Vulkan 1.1 target of 8.0.3 the engine draws the overlay only and checks none of them (`Instance::checkDeviceCompatibility()`, `sceneRenderingEnabled()`, engine 1.0.1): these GPUs should start, provided their driver has timeline semaphores (else exit 97, `OutdatedDriver`).

Checked against the changes between 8.0.2 and 8.0.3 in the four repositories (app_system 1.1.1 → 1.1.2, engine 1.0.0 → 1.0.1, app_kernel 1.3.1 → 1.3.2, core `47cbe7cd9` → `c3ab7b390`); nothing relevant after 8.0.3. Proof only comes from the 8.0.3 reports (plan § 5).

## What remains

- Read the journals of the archives above (extracted under `tools/crash-reports/data/extracted/`).
- Find the defect, reproduce it, fix it where it lives.

## References

- app_system's `docs/crash-report.md` — what an archive holds, the flight recorder milestones.
- app_system's `tools/crash-reports/README.md` — how this item was produced, how the cause is chosen.
