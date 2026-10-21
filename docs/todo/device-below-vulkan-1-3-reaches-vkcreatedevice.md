---
id: device-below-vulkan-1-3-reaches-vkcreatedevice
title: A device below Vulkan 1.3 (outdated driver) is selected and dies at vkCreateDevice, with no fallback
status: in-progress
priority: high
scope: Vulkan/Instance
opened: 2026-10-15
tags: [vulkan, startup, drivers, defect]
---

# A device below Vulkan 1.3 (outdated driver) is selected and dies at vkCreateDevice, with no fallback

## Why

The engine requires Vulkan 1.3: the instance asks `VK_API_VERSION_1_3` (`src/Vulkan/Instance.cpp:157-162`), the
shaders are compiled for Vulkan 1.3 / SPIR-V 1.6 outside macOS (`src/Saphir/ShaderManager.cpp:972-973`), and
`getGraphicsDevice()` requests `VkPhysicalDeviceVulkan13Features::shaderDemoteToHelperInvocation` unconditionally
(`src/Vulkan/Instance.cpp:1158`). Nothing compares the device's `VkPhysicalDeviceProperties::apiVersion` with that
minimum: `checkDeviceCompatibility()` (`src/Vulkan/Instance.cpp:1700`) lets a 1.2 device through, it is selected, and
`vkCreateDevice()` answers `VK_ERROR_FEATURE_NOT_PRESENT` for the whole feature set without naming the culprit. The
log ends on "Unable to find a suitable graphics device!" — nothing says "driver too old". Chaining
`VkPhysicalDeviceVulkan13Features` on a 1.2 device is itself invalid usage.

`getGraphicsDevice()` then returns `{}` (`src/Vulkan/Instance.cpp:1373`): the next scored device (an iGPU that may be
1.3) is never tried.

Measured in user crash reports of a downstream application, 2026-10-15 (Windows 11, one machine, five sessions):
GeForce GTX 1050 Ti on an outdated NVIDIA driver (device name without the "NVIDIA" prefix, `maintenance4` — mandatory
in 1.3 — not advertised, i.e. a 1.2 driver) + Intel UHD Graphics 630. The same GPU model on driver 572.83 (another
user, same day) reports Vulkan 1.4.303 and runs normally: the hardware is fine, the driver is not. NVIDIA exposes
Vulkan 1.3 on Pascal since the R510/R511 branch (early 2022).

## Done (2026-10-15, engine 7dfbe4c0)

- `Instance::MinimumDeviceAPIVersion` (1.3; 1.2 on macOS, whose shaders target 1.2 / SPIR-V 1.5) and
  `RequestedAPIVersion` for the instance; `checkDeviceAPIVersion()` refuses a device below it (graphics and compute
  selection) with one line: device, driver version, the reported and required versions, "update the graphics driver".
- The required features are checked BEFORE `vkCreateDevice()` (`requireVK10/11/12/13`): the missing ones are named and
  the device is not created.
- `getGraphicsDevice()` = the selection, then `createGraphicsDevice()` on each compatible device from the best: a
  device that cannot be created no longer ends the startup.
- Every refusal is recorded: `Instance::deviceRefusals()` (device, driver, API version, reason), for the
  user-facing refusal (`docs/subsystems/vulkan/22-startup-refusals-told-to-the-user.md`).
- Verified (Linux, RTX 3070 Ti + Intel RPL-S + llvmpipe, validation layer): a test-only build reporting llvmpipe as 1.2
  and the NVIDIA GPU without a required feature refused both and started on the Intel iGPU, 0 VUID, clean exit.

## Production reports (2026-10-21, crash-report tool)

All 85 reports (≈28 machines, Windows, 8.0.2 / engine 1.0.0) carry `maintenance4` and `VK_ERROR_FEATURE_NOT_PRESENT`: this item's defect, at scale. Engine 1.0.1 (shipped with 8.0.3) holds the fix; **the 8.0.3 production reports are the Windows run of "What remains"** — the signature below must stop growing on 8.0.3, the refused machines turning into a startup refusal (exit 97) or starting on another device.

Plan: `CrashReport Solucius` (app_system `docs/plans/crashreport-solucius.md`).

<!-- crash-reports:begin -->
| | |
|---|---|
| Reports | 85 (first 2026-10-15, last 2026-10-21) |
| Versions | 8.0.2 ×85 |
| Platforms | windows ×85 |
| OS | Windows 11 Home ×46, Windows 10 Pro ×19, Windows 10 Home ×8, Windows 11 Pro ×5, Windows 11 Home Single Language ×4, Windows 11 Education ×3 |
| GPU (selected) | unknown ×85 |
| ANGLE backend | d3d11 ×85 |
| Died at milestone | SessionOpened ×85 |

- **Cause (normalized):** `VulkanDevice: Unable to create a logical device : VK_ERROR_FEATURE_NOT_PRESENT !`
- **Where:** `vendors/emeraude-engine/src/Vulkan/Device.cpp` — `EmEn::Vulkan::Device::createDevice`
- **Signature:** `8fa61633bfbba055`

Last error before the death (else the last trace), by number of reports:

- ×85 `died at SessionOpened`

Traces of the main journal (normalized, latest report; around the Fatal trace when there is one):

```
[VulkanInstanceService][Info] (vendors/emeraude-engine/src/Vulkan/Instance.cpp `EmEn::Vulkan::Instance::getGraphicsDevice`) Mesh shaders not supported by this device.
[VulkanInstanceService][Info] (vendors/emeraude-engine/src/Vulkan/Instance.cpp `EmEn::Vulkan::Instance::getGraphicsDevice`) Vulkan Video H.<n> encode not supported by this device (RushMaker will use software VP9).
[VulkanInstanceService][Info] (vendors/emeraude-engine/src/Vulkan/Instance.cpp `EmEn::Vulkan::Instance::getGraphicsDevice`) VK_KHR_external_memory_win32 detected and enabled (external D3D11 texture import).
[VulkanDevice][Info] (vendors/emeraude-engine/src/Vulkan/Device.cpp `EmEn::Vulkan::Device::create`) Create a device requiring graphics capabilities !
[VulkanDevice][Success] (vendors/emeraude-engine/src/Vulkan/Device.cpp `EmEn::Vulkan::Device::searchGraphicsQueueConfiguration`) Graphics configured with queue family index #<n> (queue count: <n>).
[VulkanDevice][Success] (vendors/emeraude-engine/src/Vulkan/Device.cpp `EmEn::Vulkan::Device::searchTransferOnlyQueueConfiguration`) Transfer-only configured with queue family index #<n> (queue count: <n>).
[VulkanDevice][Fatal] (vendors/emeraude-engine/src/Vulkan/Device.cpp `EmEn::Vulkan::Device::createDevice`) Unable to create a logical device : VK_ERROR_FEATURE_NOT_PRESENT !
[VulkanDevice][Error] (vendors/emeraude-engine/src/Vulkan/Device.cpp `EmEn::Vulkan::Device::create`) Logical device creation failed!
```

Archives (local mirror under `tools/crash-reports/data/`, extracted next to them in `extracted/`):

- … 65 older archive(s), see `crash_reports.py list`
- `reports/8.0.2/190e8784b0bd26c2b8978758501aea8b-e984c64a-fac1-4ba2-aa53-3115ecd1b3d2.zip` → `archives/8.0.2/190e8784b0bd26c2b8978758501aea8b-e984c64a-fac1-4ba2-aa53-3115ecd1b3d2.zip`
- `reports/8.0.2/69be9e14fbc24105219c6317c5569668-14dac5f3-f382-4a73-9dfc-22b9b37320ac.zip` → `archives/8.0.2/69be9e14fbc24105219c6317c5569668-14dac5f3-f382-4a73-9dfc-22b9b37320ac.zip`
- `reports/8.0.2/385ee036df9b2fe78774c17dea8ef90e-46c874ed-1610-4f79-ba7a-df661005bbe5.zip` → `archives/8.0.2/385ee036df9b2fe78774c17dea8ef90e-46c874ed-1610-4f79-ba7a-df661005bbe5.zip`
- `reports/8.0.2/8a3a64102892e792451c0dae6b68f2f0-e9ae6f38-d4af-48a0-b543-1540e1d5b6e3.zip` → `archives/8.0.2/8a3a64102892e792451c0dae6b68f2f0-e9ae6f38-d4af-48a0-b543-1540e1d5b6e3.zip`
- `reports/8.0.2/4d97e96bfda7a46dfdc02d9011a26496-098a9c13-27e6-47f0-b1c3-f4ebe6fb12c1.zip` → `archives/8.0.2/4d97e96bfda7a46dfdc02d9011a26496-098a9c13-27e6-47f0-b1c3-f4ebe6fb12c1.zip`
- `reports/8.0.2/a8fdc3603d6b83598fb48f8599a5a5b4-480d5ddb-1eaf-4194-a859-8b386a23ccb8.zip` → `archives/8.0.2/a8fdc3603d6b83598fb48f8599a5a5b4-480d5ddb-1eaf-4194-a859-8b386a23ccb8.zip`
- `reports/8.0.2/c824a1d0035cdfe451265fbe99011630-2112e007-18a2-4775-ad5c-f2fd2ea7cb48.zip` → `archives/8.0.2/c824a1d0035cdfe451265fbe99011630-2112e007-18a2-4775-ad5c-f2fd2ea7cb48.zip`
- `reports/8.0.2/a700035adcee754228d3318fbef4d2f2-9efc5534-5109-4bc3-8860-7bb6c94c3a9c.zip` → `archives/8.0.2/a700035adcee754228d3318fbef4d2f2-9efc5534-5109-4bc3-8860-7bb6c94c3a9c.zip`
- `reports/8.0.2/a1eb6da9853ba57d04b99113c8752d82-d3ef2496-b028-452c-b094-758cca83ac4f.zip` → `archives/8.0.2/a1eb6da9853ba57d04b99113c8752d82-d3ef2496-b028-452c-b094-758cca83ac4f.zip`
- `reports/8.0.2/dc4a3ddf5155fe9aba463e360db73aa7-58c2eb2c-8d67-4ae3-808b-1562f2088ec2.zip` → `archives/8.0.2/dc4a3ddf5155fe9aba463e360db73aa7-58c2eb2c-8d67-4ae3-808b-1562f2088ec2.zip`
- `reports/8.0.2/7c411a4996c31270a73d4d331b600070-39bb76dc-9c2d-4b30-abd6-ab97e9fd8ea2.zip` → `archives/8.0.2/7c411a4996c31270a73d4d331b600070-39bb76dc-9c2d-4b30-abd6-ab97e9fd8ea2.zip`
- `reports/8.0.2/a4f92b72bb5aee36d12bdfc9b3f6bc6e-37abdf04-cbd3-4021-9b4e-8bdef98ed6b8.zip` → `archives/8.0.2/a4f92b72bb5aee36d12bdfc9b3f6bc6e-37abdf04-cbd3-4021-9b4e-8bdef98ed6b8.zip`
- `reports/8.0.2/61cde6925535657b84e543157526c07b-c5dc7368-217c-40b2-a1d6-9c10a7f535fc.zip` → `archives/8.0.2/61cde6925535657b84e543157526c07b-c5dc7368-217c-40b2-a1d6-9c10a7f535fc.zip`
- `reports/8.0.2/f5831b6e5a5c5a162f08c16aca1f42dc-774a592c-b639-42d2-9551-6daa57977e51.zip` → `archives/8.0.2/f5831b6e5a5c5a162f08c16aca1f42dc-774a592c-b639-42d2-9551-6daa57977e51.zip`
- `reports/8.0.2/eafcefaafd3629cee80ce0b08b11f76b-15fe7835-9038-463f-ba90-81b93d64f871.zip` → `archives/8.0.2/eafcefaafd3629cee80ce0b08b11f76b-15fe7835-9038-463f-ba90-81b93d64f871.zip`
- `reports/8.0.2/dbd56ff851390645ae8ea849d20fde31-da21b6eb-2e3c-4eef-80df-9619d5452080.zip` → `archives/8.0.2/dbd56ff851390645ae8ea849d20fde31-da21b6eb-2e3c-4eef-80df-9619d5452080.zip`
- `reports/8.0.2/6cbdbcf204187f2a92b255f65138b864-a0c2bafd-8c53-4901-8a7f-7a8a5ca61ba2.zip` → `archives/8.0.2/6cbdbcf204187f2a92b255f65138b864-a0c2bafd-8c53-4901-8a7f-7a8a5ca61ba2.zip`
- `reports/8.0.2/81eb4d4718273990e2f4e4714aae5cc4-ceb5adb2-eac6-47b6-806e-5f1600c51b7a.zip` → `archives/8.0.2/81eb4d4718273990e2f4e4714aae5cc4-ceb5adb2-eac6-47b6-806e-5f1600c51b7a.zip`
- `reports/8.0.2/f980c2ebcd3e95a825af973256791830-c1d52764-aa98-4fb0-b010-8e17723cbb04.zip` → `archives/8.0.2/f980c2ebcd3e95a825af973256791830-c1d52764-aa98-4fb0-b010-8e17723cbb04.zip`
- `reports/8.0.2/690864c339058be7ea469d9987d85339-4170183d-24af-44cc-a83a-c487db26f382.zip` → `archives/8.0.2/690864c339058be7ea469d9987d85339-4170183d-24af-44cc-a83a-c487db26f382.zip`
<!-- crash-reports:end -->

## 8.0.3 status (checked 2026-10-21)

**Fixed.** 8.0.3 targets Vulkan 1.1 (app_system `9447f346`, engine `aa8b01e1`): a driver below 1.3 is no longer driven at 1.3, its feature chain stops at its version, and a device that still cannot be created makes the engine try the next one (`7dfbe4c0`). These GPUs should start.

Checked against the changes between 8.0.2 and 8.0.3 in the four repositories (app_system 1.1.1 → 1.1.2, engine 1.0.0 → 1.0.1, app_kernel 1.3.1 → 1.3.2, core `47cbe7cd9` → `c3ab7b390`); nothing relevant after 8.0.3. Proof only comes from the 8.0.3 reports (plan § 5).

## What remains

- [ ] macOS: confirm the API version MoltenVK reports (the 1.2 minimum there) on the M6.
- [ ] Windows: a run (no outdated driver at hand: the forced path as on Linux).

## ⚠️ Traps

- macOS / MoltenVK reports its own API version: check what the minimum means there before refusing (the macOS path
  compiles for Vulkan 1.2 / SPIR-V 1.5).
- `maintenance4` absent is a symptom of a < 1.3 device, not the cause; the version is the test.