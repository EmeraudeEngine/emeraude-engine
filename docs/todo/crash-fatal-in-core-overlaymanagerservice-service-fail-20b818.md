---
id: crash-fatal-in-core-overlaymanagerservice-service-fail-20b818
title: "Crash reports: Fatal in Core — OverlayManagerService service failed to execute!"
status: open
priority: unranked
scope: src/Overlay/Manager.cpp
opened: 2026-10-21
tags: [crash, crash-report-auto]
---

# Crash reports: Fatal in Core — OverlayManagerService service failed to execute!

> Dropped by app_system's `tools/crash-reports/crash_reports.py`: a cause never seen before in the production crash reports.
> **Triage**: confirm it is a defect, give the file a better name, move it to the repository that owns the defect
> (an engine defect goes to `vendors/emeraude-engine/docs/todo/`, …) and rank it. The block between the
> `crash-reports` markers is rewritten at every run, wherever the item went (it is found by its signature line);
> write outside it. Deleting the item closes the cause: a new report of it is then flagged as a regression.

## Why

<!-- crash-reports:begin -->
| | |
|---|---|
| Reports | 36 (first 2026-10-15, last 2026-10-20) |
| Versions | 8.0.2 ×36 |
| Platforms | windows ×36 |
| OS | Windows 10 Pro ×18, Windows 11 Home Single Language ×8, Windows 11 Pro ×4, Windows 11 Home ×3, Windows 10 Home ×2, Windows 10 Education ×1 |
| GPU (selected) | unknown ×36 |
| ANGLE backend | d3d11 ×36 |
| Died at milestone | SessionOpened ×36 |

- **Cause (normalized):** `Core: OverlayManagerService service failed to execute!`
- **Where:** `vendors/emeraude-engine/src/Core.cpp` — `EmEn::Core::initializeCoreLevel`
- **Signature:** `20b818a3ad4d0d52`

Last error before the death (else the last trace), by number of reports:

- ×36 `died at SessionOpened`

Traces of the main journal (normalized, latest report; around the Fatal trace when there is one):

```
[VulkanShaderModule][Error] (vendors/emeraude-engine/src/Vulkan/ShaderModule.cpp `EmEn::Vulkan::ShaderModule::createOnHardware`) Unable to create a shader module : VK_ERROR_INITIALIZATION_FAILED !
[ShaderManagerService][Error] (vendors/emeraude-engine/src/Saphir/ShaderManager.cpp `EmEn::Saphir::ShaderManager::getShaderModuleFromGeneratedShader`) Unable to create a shader module !
[ShaderManagerService][Error] (vendors/emeraude-engine/src/Saphir/ShaderManager.cpp `EmEn::Saphir::ShaderManager::getShaderModules`) Unable to create the shader module from the shader 'OverlayRenderingVertexShader' source code !
[ShaderGenerator][Error] (vendors/emeraude-engine/src/Saphir/Generator/Abstract.cpp `EmEn::Saphir::Generator::Abstract::createGraphicsPipeline`) Unable to configure the graphics pipeline with shader stages !
[OverlayManagerService][Error] (vendors/emeraude-engine/src/Overlay/Manager.cpp `EmEn::Overlay::Manager::generateShaderProgram`) Unable to generate the overlay manager shader program !
[OverlayManagerService][Error] (vendors/emeraude-engine/src/Overlay/Manager.cpp `EmEn::Overlay::Manager::onInitialize`) Unable to generate an overlay manager shader program !
[Core][Fatal] (vendors/emeraude-engine/src/Core.cpp `EmEn::Core::initializeCoreLevel`) OverlayManagerService service failed to execute!
[SystemNotificationService][Info] (vendors/emeraude-engine/src/SystemNotification.cpp `EmEn::SystemNotification::onTerminate`) System notification service terminated.
```

Archives (local mirror under `tools/crash-reports/data/`, extracted next to them in `extracted/`):

- … 16 older archive(s), see `crash_reports.py list`
- `reports/8.0.2/0e80f210fd5678b13d489fdb0bbfdd12-f141af1a-fe5e-455d-8501-d047ff40c123.zip` → `archives/8.0.2/0e80f210fd5678b13d489fdb0bbfdd12-f141af1a-fe5e-455d-8501-d047ff40c123.zip`
- `reports/8.0.2/f7b4ce33df8ac7c032e06b2e151341b4-2bc639c3-15cd-4d2e-b3a3-9c6dcdc1e655.zip` → `archives/8.0.2/f7b4ce33df8ac7c032e06b2e151341b4-2bc639c3-15cd-4d2e-b3a3-9c6dcdc1e655.zip`
- `reports/8.0.2/91b59bad5414e19b55e6afa22cc7db6b-70f2e1e6-16c8-4780-93cf-332bad2f6865.zip` → `archives/8.0.2/91b59bad5414e19b55e6afa22cc7db6b-70f2e1e6-16c8-4780-93cf-332bad2f6865.zip`
- `reports/8.0.2/242c20b42c61b1324757d418b18f8fd2-2220857a-05f3-4839-87f3-94109702d9b5.zip` → `archives/8.0.2/242c20b42c61b1324757d418b18f8fd2-2220857a-05f3-4839-87f3-94109702d9b5.zip`
- `reports/8.0.2/210185f2a77e222c081624b344d5916b-a992d088-fa16-4d31-a40f-72238fe3d2d6.zip` → `archives/8.0.2/210185f2a77e222c081624b344d5916b-a992d088-fa16-4d31-a40f-72238fe3d2d6.zip`
- `reports/8.0.2/2f88a8176bf389de46708a2c7f71f3af-0358cb8a-1ce0-46dd-9b7f-4e3e8bce854f.zip` → `archives/8.0.2/2f88a8176bf389de46708a2c7f71f3af-0358cb8a-1ce0-46dd-9b7f-4e3e8bce854f.zip`
- `reports/8.0.2/dcf5a17f001da0704afd6e00f661ca27-5bc3c6d4-a41f-4d03-a9ac-b5be1af2a859.zip` → `archives/8.0.2/dcf5a17f001da0704afd6e00f661ca27-5bc3c6d4-a41f-4d03-a9ac-b5be1af2a859.zip`
- `reports/8.0.2/a78b734095e173b8aede9476961eeafc-e1e79d48-f0f1-46f2-92a7-f24ad85a323b.zip` → `archives/8.0.2/a78b734095e173b8aede9476961eeafc-e1e79d48-f0f1-46f2-92a7-f24ad85a323b.zip`
- `reports/8.0.2/131a8c06232172d0f0d04e71116f5388-c52ba741-2d77-43c1-acc2-be0c6279296e.zip` → `archives/8.0.2/131a8c06232172d0f0d04e71116f5388-c52ba741-2d77-43c1-acc2-be0c6279296e.zip`
- `reports/8.0.2/74075a79501e12df01f607b3ecc36032-c1c42492-0d2c-42d2-8ccf-8871ec06fee7.zip` → `archives/8.0.2/74075a79501e12df01f607b3ecc36032-c1c42492-0d2c-42d2-8ccf-8871ec06fee7.zip`
- `reports/8.0.2/eb7a93f880e97f87db9fd797fdccdde2-04066b0f-0f50-49a9-b8d7-f974e7d9a7bc.zip` → `archives/8.0.2/eb7a93f880e97f87db9fd797fdccdde2-04066b0f-0f50-49a9-b8d7-f974e7d9a7bc.zip`
- `reports/8.0.2/39350f9c0a8b2ed35a7bcfaef9502969-5c4e4259-5606-4ec1-87d7-145e3be71d69.zip` → `archives/8.0.2/39350f9c0a8b2ed35a7bcfaef9502969-5c4e4259-5606-4ec1-87d7-145e3be71d69.zip`
- `reports/8.0.2/1303b052b4ecbeed4178b8775b94f447-a63a9594-c5a5-4b3b-91af-3662e2575aab.zip` → `archives/8.0.2/1303b052b4ecbeed4178b8775b94f447-a63a9594-c5a5-4b3b-91af-3662e2575aab.zip`
- `reports/8.0.2/1601b7273656cca2a99aa8e35526c3ad-9b1df563-229e-495a-a3ea-517986be8b23.zip` → `archives/8.0.2/1601b7273656cca2a99aa8e35526c3ad-9b1df563-229e-495a-a3ea-517986be8b23.zip`
- `reports/8.0.2/924130cec403163214425a69e3cadb7c-d87f592c-2a64-4aa7-a360-216ecfbb5671.zip` → `archives/8.0.2/924130cec403163214425a69e3cadb7c-d87f592c-2a64-4aa7-a360-216ecfbb5671.zip`
- `reports/8.0.2/74cea01a9ce434ff7fc51ae940013227-68cd11b1-c96f-47d8-803a-f18737671079.zip` → `archives/8.0.2/74cea01a9ce434ff7fc51ae940013227-68cd11b1-c96f-47d8-803a-f18737671079.zip`
- `reports/8.0.2/3924c1d988c3f9c13bf02360dd8d8bbc-6a69a8bb-2333-46f0-b9db-a13fa7d73ba3.zip` → `archives/8.0.2/3924c1d988c3f9c13bf02360dd8d8bbc-6a69a8bb-2333-46f0-b9db-a13fa7d73ba3.zip`
- `reports/8.0.2/addc78103d48aee684fc49c44a8e03ec-d55434d2-771e-469d-8eb8-5c925dd319a5.zip` → `archives/8.0.2/addc78103d48aee684fc49c44a8e03ec-d55434d2-771e-469d-8eb8-5c925dd319a5.zip`
- `reports/8.0.2/a6c496f68320c3ad54dd86dc89b4ad9e-aaf2e551-6421-4736-9832-0fe9190dd7a8.zip` → `archives/8.0.2/a6c496f68320c3ad54dd86dc89b4ad9e-aaf2e551-6421-4736-9832-0fe9190dd7a8.zip`
- `reports/8.0.2/28c961e5249b84e3149932dbb7833964-fc0dfe98-b791-4cbc-b03a-2d0cef1879ee.zip` → `archives/8.0.2/28c961e5249b84e3149932dbb7833964-fc0dfe98-b791-4cbc-b03a-2d0cef1879ee.zip`
<!-- crash-reports:end -->

## Triage (2026-10-21)

- **Owner**: the engine.
- **Machines**: ≈7 (reports grouped by CPU + RAM + OS fingerprint on 2026-10-21; several reports per machine
  when every launch fails).
- **Reading**: `OverlayManagerService` fails to initialize at startup (7 machines, up to 20 reports each): the overlay is the layer that composites the CEF views — without it nothing can be shown.
- **Next step**: Read the journals for the first error before the Fatal (accelerated paint / D3D11 shared textures?). Fix in the overlay manager, or make it fall back to the non-accelerated path instead of failing.
- **Plan**: `CrashReport Solucius`, wave W1 — app_system's `docs/plans/crashreport-solucius.md`.

## What remains

- Read the journals of the archives above (extracted under `tools/crash-reports/data/extracted/`).
- Find the defect, reproduce it, fix it where it lives.

## References

- app_system's `docs/crash-report.md` — what an archive holds, the flight recorder milestones.
- app_system's `tools/crash-reports/README.md` — how this item was produced, how the cause is chosen.
