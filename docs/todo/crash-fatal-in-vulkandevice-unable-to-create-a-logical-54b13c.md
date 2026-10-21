---
id: crash-fatal-in-vulkandevice-unable-to-create-a-logical-54b13c
title: "Crash reports: Fatal in VulkanDevice — Unable to create a logical device : VK_ERROR_OUT_OF_HOST_MEMORY !"
status: open
priority: unranked
scope: src/Vulkan/Device.cpp
opened: 2026-10-21
tags: [crash, crash-report-auto]
---

# Crash reports: Fatal in VulkanDevice — Unable to create a logical device : VK_ERROR_OUT_OF_HOST_MEMORY !

> Dropped by app_system's `tools/crash-reports/crash_reports.py`: a cause never seen before in the production crash reports.
> **Triage**: confirm it is a defect, give the file a better name, move it to the repository that owns the defect
> (an engine defect goes to `vendors/emeraude-engine/docs/todo/`, …) and rank it. The block between the
> `crash-reports` markers is rewritten at every run, wherever the item went (it is found by its signature line);
> write outside it. Deleting the item closes the cause: a new report of it is then flagged as a regression.

## Why

<!-- crash-reports:begin -->
| | |
|---|---|
| Reports | 25 (first 2026-10-15, last 2026-10-21) |
| Versions | 8.0.2 ×25 |
| Platforms | windows ×25 |
| OS | Windows 11 Home ×10, Windows 10 Home ×6, Windows 11 Pro ×5, Windows 11 Enterprise ×2, Windows 11 Home Single Language ×2 |
| GPU (selected) | unknown ×25 |
| ANGLE backend | d3d11 ×25 |
| Died at milestone | SessionOpened ×25 |

- **Cause (normalized):** `VulkanDevice: Unable to create a logical device : VK_ERROR_OUT_OF_HOST_MEMORY !`
- **Where:** `vendors/emeraude-engine/src/Vulkan/Device.cpp` — `EmEn::Vulkan::Device::createDevice`
- **Signature:** `54b13c6f5a110162`

Last error before the death (else the last trace), by number of reports:

- ×25 `died at SessionOpened`

Traces of the main journal (normalized, latest report; around the Fatal trace when there is one):

```
[VulkanInstanceService][Info] (vendors/emeraude-engine/src/Vulkan/Instance.cpp `EmEn::Vulkan::Instance::getGraphicsDevice`) Mesh shaders not supported by this device.
[VulkanInstanceService][Info] (vendors/emeraude-engine/src/Vulkan/Instance.cpp `EmEn::Vulkan::Instance::getGraphicsDevice`) Vulkan Video H.<n> encode not supported by this device (RushMaker will use software VP9).
[VulkanInstanceService][Info] (vendors/emeraude-engine/src/Vulkan/Instance.cpp `EmEn::Vulkan::Instance::getGraphicsDevice`) VK_KHR_external_memory_win32 detected and enabled (external D3D11 texture import).
[VulkanDevice][Info] (vendors/emeraude-engine/src/Vulkan/Device.cpp `EmEn::Vulkan::Device::create`) Create a device requiring graphics capabilities !
[VulkanDevice][Success] (vendors/emeraude-engine/src/Vulkan/Device.cpp `EmEn::Vulkan::Device::searchGraphicsQueueConfiguration`) Graphics configured with queue family index #<n> (queue count: <n>).
[VulkanDevice][Success] (vendors/emeraude-engine/src/Vulkan/Device.cpp `EmEn::Vulkan::Device::searchTransferOnlyQueueConfiguration`) Transfer-only configured with queue family index #<n> (queue count: <n>).
[VulkanDevice][Fatal] (vendors/emeraude-engine/src/Vulkan/Device.cpp `EmEn::Vulkan::Device::createDevice`) Unable to create a logical device : VK_ERROR_OUT_OF_HOST_MEMORY !
[VulkanDevice][Error] (vendors/emeraude-engine/src/Vulkan/Device.cpp `EmEn::Vulkan::Device::create`) Logical device creation failed!
```

Archives (local mirror under `tools/crash-reports/data/`, extracted next to them in `extracted/`):

- … 5 older archive(s), see `crash_reports.py list`
- `reports/8.0.2/211f5a36f477ce4586d2c253e58d8962-f09154e6-87e4-49d7-85b6-290a6655dc8e.zip` → `archives/8.0.2/211f5a36f477ce4586d2c253e58d8962-f09154e6-87e4-49d7-85b6-290a6655dc8e.zip`
- `reports/8.0.2/e88e825488a82d17c511944e4e3ac4ec-33a83143-f18f-4b3b-84e8-aac0bc5543fa.zip` → `archives/8.0.2/e88e825488a82d17c511944e4e3ac4ec-33a83143-f18f-4b3b-84e8-aac0bc5543fa.zip`
- `reports/8.0.2/b1fd82cb2719a411e0a08c8df9e656c9-282e8c90-ed85-4699-a91f-96574ed83713.zip` → `archives/8.0.2/b1fd82cb2719a411e0a08c8df9e656c9-282e8c90-ed85-4699-a91f-96574ed83713.zip`
- `reports/8.0.2/2652ccd11d4e393b96995f946d09516d-4619e04c-fc1e-41a9-8ef2-e4625b2530a4.zip` → `archives/8.0.2/2652ccd11d4e393b96995f946d09516d-4619e04c-fc1e-41a9-8ef2-e4625b2530a4.zip`
- `reports/8.0.2/1e9676951f5deeb1b0cbb5412244ccb8-11913838-c500-4145-aca6-71fd7844f3cf.zip` → `archives/8.0.2/1e9676951f5deeb1b0cbb5412244ccb8-11913838-c500-4145-aca6-71fd7844f3cf.zip`
- `reports/8.0.2/b7f208c931ab62d8dea0d1af9bdde2da-ceb05fae-7b8b-433c-aa0c-e57bd4514e76.zip` → `archives/8.0.2/b7f208c931ab62d8dea0d1af9bdde2da-ceb05fae-7b8b-433c-aa0c-e57bd4514e76.zip`
- `reports/8.0.2/993490cfebc0e8a41b572dc77917315a-b4f0b7a6-44bc-4a68-ba36-1e515bad144c.zip` → `archives/8.0.2/993490cfebc0e8a41b572dc77917315a-b4f0b7a6-44bc-4a68-ba36-1e515bad144c.zip`
- `reports/8.0.2/036f8564eb7ac0961ec6bf8c029cd2a6-f4a94c2e-3cd9-4205-a6c0-542de248c968.zip` → `archives/8.0.2/036f8564eb7ac0961ec6bf8c029cd2a6-f4a94c2e-3cd9-4205-a6c0-542de248c968.zip`
- `reports/8.0.2/2411ea4693afe76e75642e8a455425fd-f28d8f3f-6051-4cb8-842f-b772c9b90938.zip` → `archives/8.0.2/2411ea4693afe76e75642e8a455425fd-f28d8f3f-6051-4cb8-842f-b772c9b90938.zip`
- `reports/8.0.2/1f20633902302c8ef4c916be70618903-87bca860-372b-4cf5-aff6-8c359357c2fe.zip` → `archives/8.0.2/1f20633902302c8ef4c916be70618903-87bca860-372b-4cf5-aff6-8c359357c2fe.zip`
- `reports/8.0.2/29b54d2c7944090c82aafaae125f72a4-445b10f0-c4da-45e9-9fc1-d4e5b0bb8d32.zip` → `archives/8.0.2/29b54d2c7944090c82aafaae125f72a4-445b10f0-c4da-45e9-9fc1-d4e5b0bb8d32.zip`
- `reports/8.0.2/2ebc1dd3cbf9a5befc58c0bea097b15e-227fd85f-7ba8-4bbb-9024-ff3d3051ef6f.zip` → `archives/8.0.2/2ebc1dd3cbf9a5befc58c0bea097b15e-227fd85f-7ba8-4bbb-9024-ff3d3051ef6f.zip`
- `reports/8.0.2/08fabd205a3443e372ea9707b225f09d-488d74ca-1cb5-4efd-89c0-bc7b22a81836.zip` → `archives/8.0.2/08fabd205a3443e372ea9707b225f09d-488d74ca-1cb5-4efd-89c0-bc7b22a81836.zip`
- `reports/8.0.2/4163fcfe3c3f191939b4b98a469fa664-3e466a15-706c-4ea2-8ba7-beb903d2a966.zip` → `archives/8.0.2/4163fcfe3c3f191939b4b98a469fa664-3e466a15-706c-4ea2-8ba7-beb903d2a966.zip`
- `reports/8.0.2/2af0f53187dfa203ccf4006c9ebbde85-06477654-3515-4fc6-98a7-6f0c3554c59f.zip` → `archives/8.0.2/2af0f53187dfa203ccf4006c9ebbde85-06477654-3515-4fc6-98a7-6f0c3554c59f.zip`
- `reports/8.0.2/5e3757b98a4467d7112b307baec38b3e-c97c3678-2327-4328-b2fc-3c093a683a59.zip` → `archives/8.0.2/5e3757b98a4467d7112b307baec38b3e-c97c3678-2327-4328-b2fc-3c093a683a59.zip`
- `reports/8.0.2/c9a36eee7b5c254a5e3d576ec65addd2-c8702b07-c5ce-48d5-8ccb-879b92ec4fa8.zip` → `archives/8.0.2/c9a36eee7b5c254a5e3d576ec65addd2-c8702b07-c5ce-48d5-8ccb-879b92ec4fa8.zip`
- `reports/8.0.2/ec4f7e255d4940d96a11dd94227aaa74-aa93013f-a37b-4aa3-bf24-d80083fd0cc7.zip` → `archives/8.0.2/ec4f7e255d4940d96a11dd94227aaa74-aa93013f-a37b-4aa3-bf24-d80083fd0cc7.zip`
- `reports/8.0.2/ba79c713be97cf898a7768c4e4e152b4-dcdecb8b-db1b-45f4-9418-3ee95fea1286.zip` → `archives/8.0.2/ba79c713be97cf898a7768c4e4e152b4-dcdecb8b-db1b-45f4-9418-3ee95fea1286.zip`
- `reports/8.0.2/18c7b1e26b277a24f7ec7eac21bd011b-452ac3ea-15d7-4714-a96f-f707bb393387.zip` → `archives/8.0.2/18c7b1e26b277a24f7ec7eac21bd011b-452ac3ea-15d7-4714-a96f-f707bb393387.zip`
<!-- crash-reports:end -->

## Triage (2026-10-21)

- **Owner**: the engine.
- **Machines**: ≈8 (reports grouped by CPU + RAM + OS fingerprint on 2026-10-21; several reports per machine
  when every launch fails).
- **Reading**: `vkCreateDevice()` answers `VK_ERROR_OUT_OF_HOST_MEMORY` — on a desktop, usually a driver refusing the requested queues/features rather than a real RAM shortage.
- **Next step**: Read the journals (GPU, driver, queues requested). Engine 1.0.1 classifies it as `Other` (still a Fatal): decide whether it is a refusal to tell the user, or a retry with fewer queues/features.
- **Plan**: `CrashReport Solucius`, wave W1 — app_system's `docs/plans/crashreport-solucius.md`.

## 8.0.3 status (checked 2026-10-21)

**Likely fixed.** AMD Radeon iGPUs: `vkCreateDevice()` answered `VK_ERROR_OUT_OF_HOST_MEMORY` to the 1.3 device request. At the 1.1 target only the structures valid for the device's version are chained (`DeviceRequirements::linkFeatureChain()`, `aa8b01e1`), and a failed creation moves to the next device (`7dfbe4c0`).

Checked against the changes between 8.0.2 and 8.0.3 in the four repositories (app_system 1.1.1 → 1.1.2, engine 1.0.0 → 1.0.1, app_kernel 1.3.1 → 1.3.2, core `47cbe7cd9` → `c3ab7b390`); nothing relevant after 8.0.3. Proof only comes from the 8.0.3 reports (plan § 5).

## What remains

- Read the journals of the archives above (extracted under `tools/crash-reports/data/extracted/`).
- Find the defect, reproduce it, fix it where it lives.

## References

- app_system's `docs/crash-report.md` — what an archive holds, the flight recorder milestones.
- app_system's `tools/crash-reports/README.md` — how this item was produced, how the cause is chosen.
