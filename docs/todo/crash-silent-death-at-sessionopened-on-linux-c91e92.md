---
id: crash-silent-death-at-sessionopened-on-linux-c91e92
title: "Crash reports: Silent death at SessionOpened on linux"
status: open
priority: unranked
scope: src/Core.cpp, src/Vulkan
opened: 2026-10-21
tags: [crash, crash-report-auto]
---

# Crash reports: Silent death at SessionOpened on linux

> Dropped by app_system's `tools/crash-reports/crash_reports.py`: a cause never seen before in the production crash reports.
> **Triage**: confirm it is a defect, give the file a better name, move it to the repository that owns the defect
> (an engine defect goes to `vendors/emeraude-engine/docs/todo/`, …) and rank it. The block between the
> `crash-reports` markers is rewritten at every run, wherever the item went (it is found by its signature line);
> write outside it. Deleting the item closes the cause: a new report of it is then flagged as a regression.

## Why

<!-- crash-reports:begin -->
| | |
|---|---|
| Reports | 1 (first 2026-10-15, last 2026-10-15) |
| Versions | 8.0.2 ×1 |
| Platforms | linux ×1 |
| OS | Ubuntu 26.04.1 LTS ×1 |
| GPU (selected) | unknown ×1 |
| ANGLE backend | vulkan ×1 |
| Died at milestone | SessionOpened ×1 |
| Single-instance lock | not recorded ×1 |

- **Cause (normalized):** `died at SessionOpened on linux`
- **Where:** no source location
- **Signature:** `c91e92f8acfe6106`

Last error before the death (else the last trace), by number of reports:

- ×1 `VulkanInstanceService: Vulkan Video H.<n> encode extensions detected and enabled (hardware RushMaker path).`

Traces of the main journal (normalized, latest report; around the Fatal trace when there is one):

```
[VulkanInstanceService][Warning] (vendors/emeraude-engine/src/Vulkan/Instance.cpp `EmEn::Vulkan::Instance::checkDevicesFeaturesForGraphics`) The physical device 'llvmpipe (LLVM <n>, <n> bits)' is missing 'sparseResidency4Samples' feature !
[VulkanInstanceService][Warning] (vendors/emeraude-engine/src/Vulkan/Instance.cpp `EmEn::Vulkan::Instance::checkDevicesFeaturesForGraphics`) The physical device 'llvmpipe (LLVM <n>, <n> bits)' is missing 'sparseResidency8Samples' feature !
[VulkanInstanceService][Warning] (vendors/emeraude-engine/src/Vulkan/Instance.cpp `EmEn::Vulkan::Instance::checkDevicesFeaturesForGraphics`) The physical device 'llvmpipe (LLVM <n>, <n> bits)' is missing 'sparseResidency16Samples' feature !
[VulkanInstanceService][Warning] (vendors/emeraude-engine/src/Vulkan/Instance.cpp `EmEn::Vulkan::Instance::checkDevicesFeaturesForGraphics`) The physical device 'llvmpipe (LLVM <n>, <n> bits)' is missing 'inheritedQueries' feature !
[VulkanInstanceService][Success] (vendors/emeraude-engine/src/Vulkan/Instance.cpp `EmEn::Vulkan::Instance::getGraphicsDevice`) Graphics capable physical device selected: NVIDIA GeForce RTX <n> Ti (Performance mode)
[VulkanInstanceService][Info] (vendors/emeraude-engine/src/Vulkan/Instance.cpp `EmEn::Vulkan::Instance::getGraphicsDevice`) Ray tracing extensions detected and enabled (VK_KHR_acceleration_structure + VK_KHR_ray_query).
[VulkanInstanceService][Info] (vendors/emeraude-engine/src/Vulkan/Instance.cpp `EmEn::Vulkan::Instance::getGraphicsDevice`) Mesh shaders detected and enabled (VK_EXT_mesh_shader): task stage yes, multiview yes, at most <n> vertices and <n> primitives per mesh workgroup.
[VulkanInstanceService][Info] (vendors/emeraude-engine/src/Vulkan/Instance.cpp `EmEn::Vulkan::Instance::getGraphicsDevice`) Vulkan Video H.<n> encode extensions detected and enabled (hardware RushMaker path).
```

Archives (local mirror under `tools/crash-reports/data/`, extracted next to them in `extracted/`):

- `reports/8.0.2/3f08d8631995e8514da72607b823955e-4c6298fd-2391-4ae4-acc6-abdb5f9c0e32.zip` → `archives/8.0.2/3f08d8631995e8514da72607b823955e-4c6298fd-2391-4ae4-acc6-abdb5f9c0e32.zip`
<!-- crash-reports:end -->

## Triage (2026-10-21)

- **Owner**: the engine.
- **Machines**: ≈2 (reports grouped by CPU + RAM + OS fingerprint on 2026-10-21; several reports per machine
  when every launch fails).
- **Reading**: Linux, dies before `CoreStarted` without a Fatal (one report ends on the Vulkan Video H.264 extensions).
- **Next step**: Low volume: read the two journals; wait for the minidumps (W0) if nothing stands out.
- **Plan**: `CrashReport Solucius`, wave W3 — app_system's `docs/plans/crashreport-solucius.md`.

## 8.0.3 status (checked 2026-10-21)

**Not addressed.** Nothing in 8.0.3 points at it.

Checked against the changes between 8.0.2 and 8.0.3 in the four repositories (app_system 1.1.1 → 1.1.2, engine 1.0.0 → 1.0.1, app_kernel 1.3.1 → 1.3.2, core `47cbe7cd9` → `c3ab7b390`); nothing relevant after 8.0.3. Proof only comes from the 8.0.3 reports (plan § 5).

## What remains

- Read the journals of the archives above (extracted under `tools/crash-reports/data/extracted/`).
- Find the defect, reproduce it, fix it where it lives.

## References

- app_system's `docs/crash-report.md` — what an archive holds, the flight recorder milestones.
- app_system's `tools/crash-reports/README.md` — how this item was produced, how the cause is chosen.
