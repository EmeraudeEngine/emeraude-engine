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
| Reports | 75 (first 2026-10-15, last 2026-10-22) |
| Versions | 8.0.2 ×75 |
| Platforms | windows ×75 |
| OS | Windows 10 Home ×32, Windows 10 Pro ×15, Windows 11 Pro ×15, Windows 11 Home ×7, Windows 10 Enterprise LTSC 2019 ×2, Windows 11 Enterprise ×2, … (2 more) |
| GPU (selected) | unknown ×75 |
| ANGLE backend | d3d11 ×75 |
| Died at milestone | SessionOpened ×75 |

- **Cause (normalized):** `RendererService: Unable to find a suitable graphics device!`
- **Where:** `vendors/emeraude-engine/src/Graphics/Renderer.cpp` — `EmEn::Graphics::Renderer::initializeRenderingStack`
- **Signature:** `d44787f2ff78e6ee`

Last error before the death (else the last trace), by number of reports:

- ×75 `died at SessionOpened`

Traces of the main journal (normalized, latest report; around the Fatal trace when there is one):

```
[VulkanInstanceService][Success] (vendors/emeraude-engine/src/Vulkan/Instance.cpp `EmEn::Vulkan::Instance::checkDeviceCompatibility`) 'Intel(R) UHD Graphics <n>' support presentation!
[VulkanInstanceService][Warning] (vendors/emeraude-engine/src/Vulkan/Instance.cpp `EmEn::Vulkan::Instance::checkDevicesFeaturesForGraphics`) The physical device 'Intel(R) UHD Graphics <n>' is missing 'depthBounds' feature !
[VulkanInstanceService][Warning] (vendors/emeraude-engine/src/Vulkan/Instance.cpp `EmEn::Vulkan::Instance::checkDevicesFeaturesForGraphics`) The physical device 'Intel(R) UHD Graphics <n>' is missing 'shaderStorageImageReadWithoutFormat' feature !
[VulkanInstanceService][Warning] (vendors/emeraude-engine/src/Vulkan/Instance.cpp `EmEn::Vulkan::Instance::checkDevicesFeaturesForGraphics`) The physical device 'Intel(R) UHD Graphics <n>' is missing 'shaderResourceMinLod' feature !
[VulkanInstanceService][Error] (vendors/emeraude-engine/src/Vulkan/Instance.cpp `EmEn::Vulkan::Instance::checkDevicesFeaturesForGraphics`) The physical device 'Intel(R) UHD Graphics <n>' is missing 'descriptorIndexing' feature !
[VulkanInstanceService][Error] (vendors/emeraude-engine/src/Vulkan/Instance.cpp `EmEn::Vulkan::Instance::getGraphicsDevice`) There is no physical device (Graphics) compatible with Vulkan.
[RendererService][Fatal] (vendors/emeraude-engine/src/Graphics/Renderer.cpp `EmEn::Graphics::Renderer::initializeRenderingStack`) Unable to find a suitable graphics device!
[Core][Fatal] (vendors/emeraude-engine/src/Core.cpp `EmEn::Core::initializeCoreLevel`) RendererService service failed to execute!
```

Archives (local mirror under `tools/crash-reports/data/`, extracted next to them in `extracted/`):

- … 55 older archive(s), see `crash_reports.py list`
- `reports/8.0.2/c9b36d3635e9a50182a361b97146b929-1fa4a8e8-b9ee-4ea1-a768-293edd708b90.zip` → `archives/8.0.2/c9b36d3635e9a50182a361b97146b929-1fa4a8e8-b9ee-4ea1-a768-293edd708b90.zip`
- `reports/8.0.2/be6164ffe2663f241c1b76af5e685f1d-b5e532cf-a1d2-4adf-91e0-dc7038fe49ed.zip` → `archives/8.0.2/be6164ffe2663f241c1b76af5e685f1d-b5e532cf-a1d2-4adf-91e0-dc7038fe49ed.zip`
- `reports/8.0.2/dbd52caeb15a94e3d41d2e81cf6b4218-b899cb15-5965-4c10-953f-280b119ecfc5.zip` → `archives/8.0.2/dbd52caeb15a94e3d41d2e81cf6b4218-b899cb15-5965-4c10-953f-280b119ecfc5.zip`
- `reports/8.0.2/60609640045f37a357c786b3ace043ad-f423db22-8c1b-4c95-b336-86f3e02e7528.zip` → `archives/8.0.2/60609640045f37a357c786b3ace043ad-f423db22-8c1b-4c95-b336-86f3e02e7528.zip`
- `reports/8.0.2/704871ca796031e6ba7d2433304ea173-a3194f13-c9d4-41e1-962a-f1a4a0a52ee8.zip` → `archives/8.0.2/704871ca796031e6ba7d2433304ea173-a3194f13-c9d4-41e1-962a-f1a4a0a52ee8.zip`
- `reports/8.0.2/6d69de9526bfff24c7a93898f99c720f-6e51c82d-336b-4df1-8ea8-572b795e57c2.zip` → `archives/8.0.2/6d69de9526bfff24c7a93898f99c720f-6e51c82d-336b-4df1-8ea8-572b795e57c2.zip`
- `reports/8.0.2/a4b83ddde25ebc9bda6c858af1acd4df-81fc101c-099a-42c3-8fab-f7725c3c5e6b.zip` → `archives/8.0.2/a4b83ddde25ebc9bda6c858af1acd4df-81fc101c-099a-42c3-8fab-f7725c3c5e6b.zip`
- `reports/8.0.2/baa79cdb55ceffbf154d9a634c4e9cdb-d59d3db6-9e87-4420-902b-ffae8a443bec.zip` → `archives/8.0.2/baa79cdb55ceffbf154d9a634c4e9cdb-d59d3db6-9e87-4420-902b-ffae8a443bec.zip`
- `reports/8.0.2/16693d1354ca1099c3be6cf23011c6d5-8c8bed67-9c49-44f7-98b7-b81e0a934bb8.zip` → `archives/8.0.2/16693d1354ca1099c3be6cf23011c6d5-8c8bed67-9c49-44f7-98b7-b81e0a934bb8.zip`
- `reports/8.0.2/a2512703cad1e908ea68c39a2169632f-774e126e-8a88-4850-95b5-c5e864d1e6dc.zip` → `archives/8.0.2/a2512703cad1e908ea68c39a2169632f-774e126e-8a88-4850-95b5-c5e864d1e6dc.zip`
- `reports/8.0.2/bff6263297fc5b0ce4237b458a93c24f-7ea19da5-ef2e-4f27-81df-ea66c6ff3ac2.zip` → `archives/8.0.2/bff6263297fc5b0ce4237b458a93c24f-7ea19da5-ef2e-4f27-81df-ea66c6ff3ac2.zip`
- `reports/8.0.2/af0e98450b111e2bcc577be47179838b-1747e26a-fa09-40ce-93a9-77a78feff676.zip` → `archives/8.0.2/af0e98450b111e2bcc577be47179838b-1747e26a-fa09-40ce-93a9-77a78feff676.zip`
- `reports/8.0.2/82b0f4eda2e6fa2a716973955e9ab83b-ec5fc49d-8f50-4f92-99e4-cf66a9724c3d.zip` → `archives/8.0.2/82b0f4eda2e6fa2a716973955e9ab83b-ec5fc49d-8f50-4f92-99e4-cf66a9724c3d.zip`
- `reports/8.0.2/f6354640950f18afe2fb9c9889c31a9a-ef0876b0-ca02-400c-b1f1-2bcb40979b27.zip` → `archives/8.0.2/f6354640950f18afe2fb9c9889c31a9a-ef0876b0-ca02-400c-b1f1-2bcb40979b27.zip`
- `reports/8.0.2/b51e363fd646957ccf2bdd72d61acfa2-629a8e0c-24ce-4f44-be33-c1fd1e97eee2.zip` → `archives/8.0.2/b51e363fd646957ccf2bdd72d61acfa2-629a8e0c-24ce-4f44-be33-c1fd1e97eee2.zip`
- `reports/8.0.2/b42047fea87d6365147a81b5675a24cd-dfdd5b7d-ab51-4c2d-8573-305da9317500.zip` → `archives/8.0.2/b42047fea87d6365147a81b5675a24cd-dfdd5b7d-ab51-4c2d-8573-305da9317500.zip`
- `reports/8.0.2/51f661997eaa8a8d98ffc9a2c6a50e82-d49505ed-512d-46cc-8104-7d08c49fc044.zip` → `archives/8.0.2/51f661997eaa8a8d98ffc9a2c6a50e82-d49505ed-512d-46cc-8104-7d08c49fc044.zip`
- `reports/8.0.2/80a11e952ecc73a05ec28262b455808a-76849569-cad5-484e-9936-46ccc6cf499d.zip` → `archives/8.0.2/80a11e952ecc73a05ec28262b455808a-76849569-cad5-484e-9936-46ccc6cf499d.zip`
- `reports/8.0.2/a99921e3fc13a255198429ce39aa6477-d63eb139-29ca-4ad7-803b-0286a6477ced.zip` → `archives/8.0.2/a99921e3fc13a255198429ce39aa6477-d63eb139-29ca-4ad7-803b-0286a6477ced.zip`
- `reports/8.0.2/f6f92201748364e138990084421277f1-b577bc92-b239-466b-b374-73012286441b.zip` → `archives/8.0.2/f6f92201748364e138990084421277f1-b577bc92-b239-466b-b374-73012286441b.zip`
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
