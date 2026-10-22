---
id: physical-device-enumeration-refuses-vk-incomplete
title: The physical device enumeration treats VK_INCOMPLETE as a failure and stops the engine
status: in-progress
priority: high
scope: Vulkan/Instance
opened: 2026-10-15
tags: [vulkan, startup, windows, defect]
---

# The physical device enumeration treats VK_INCOMPLETE as a failure and stops the engine

## Why

`Instance::preparePhysicalDevices()` (`src/Vulkan/Instance.cpp:345`) calls `vkEnumeratePhysicalDevices()` twice
(count, then fill) and accepts only `VK_SUCCESS` from each. `VK_INCOMPLETE` is a **success code** (positive
`VkResult`): the returned data is valid, the list simply changed or did not fit, and the caller is expected to query
again. The engine logs it as an error, reports "No physical device available !" and the Vulkan instance service fails:
the application exits with code 1 before a window is shown.

Measured in user crash reports of a downstream application, 2026-10-15 (Windows 10, engine 1.0.0, bundled loader
1.4.363), two machines, three sessions, both AMD Ryzen "H" laptop CPUs (Radeon iGPU, very likely a discrete GPU too —
the log cannot tell, see `gpu-inventory-without-vulkan`):

- on the FIRST call (count query): `Unable to get physical device count : VK_INCOMPLETE !`;
- on the SECOND call (fill): `Unable to get physical devices : VK_INCOMPLETE !` — the device count grew between the
  two calls (a hybrid laptop whose discrete GPU appears late, or an ICD answering differently).

These machines very likely have a perfectly capable GPU: the engine refuses them for nothing.

## Done (2026-10-15, engine 7dfbe4c0)

- `Vulkan::enumerate()` (`src/Vulkan/Utility.hpp`): the standard loop (count, size, fill; again while `VK_INCOMPLETE`,
  8 attempts at most, the last answer kept); only a negative `VkResult` fails. `preparePhysicalDevices()` uses it, logs
  the number of devices found and warns on a retry.
- Every other two-call enumeration goes through it: instance / device layers and extensions, tool properties, surface
  formats, present modes, present rectangles, video session memory requirements; `PhysicalDeviceProbe` accepts any
  non-negative answer. The swap-chain image query keeps `VK_SUCCESS` only, on purpose (its count is the swap-chain's
  own: a partial list would be missing images — commented there).

## Production reports (2026-10-21, crash-report tool)

56 reports (≈17 machines, Windows, 8.0.2 / engine 1.0.0) end on `No physical device available !`; 34 of them carry `VK_INCOMPLETE` — this item's defect, at scale (the others: a real absence of device, to read). Engine 1.0.1 (shipped with 8.0.3) holds the fix; **the 8.0.3 production reports are the Windows run of "What remains"**.

Plan: `CrashReport Solucius` (app_system `docs/plans/crashreport-solucius.md`).

<!-- crash-reports:begin -->
| | |
|---|---|
| Reports | 58 (first 2026-10-15, last 2026-10-22) |
| Versions | 8.0.2 ×58 |
| Platforms | windows ×58 |
| OS | Windows 10 Pro ×25, Windows 10 Home ×16, Windows 11 Home ×15, Windows 11 Pro ×2 |
| GPU (selected) | unknown ×58 |
| ANGLE backend | d3d11 ×58 |
| Died at milestone | SessionOpened ×58 |

- **Cause (normalized):** `VulkanInstanceService: No physical device available !`
- **Where:** `vendors/emeraude-engine/src/Vulkan/Instance.cpp` — `EmEn::Vulkan::Instance::onInitialize`
- **Signature:** `065c7bb001149082`

Last error before the death (else the last trace), by number of reports:

- ×58 `died at SessionOpened`

Traces of the main journal (normalized, latest report; around the Fatal trace when there is one):

```
[Application][Info] (src/Application.CefBrowserProcessHandler.cpp `Mango3D::System::Application::OnRegisterCustomPreferences`) A request context preference has been changed !
[Application][Success] (src/Application.CefBrowserProcessHandler.cpp `Mango3D::System::Application::OnContextInitialized`) Chromium context initialized !
[PlatformManagerService][Info] (vendors/emeraude-engine/src/PlatformManager.cpp `EmEn::PlatformManager::onInitialize`) Vulkan layer search path : '<path> Files\Lychee\vulkan\explicit_layer.d'.
[VulkanLoader][Info] (vendors/emeraude-engine/src/Vulkan/Loader.cpp `EmEn::Vulkan::Loader::open`) Vulkan loader '<path> Files\Lychee\vulkan\vulkan-<n>.dll' opened (instance version <n>).
[Core][Success] (vendors/emeraude-engine/src/Core.cpp `EmEn::Core::initializeCoreLevel`) PlatformManagerService service up!
[VulkanInstanceService][Error] (vendors/emeraude-engine/src/Vulkan/Instance.cpp `EmEn::Vulkan::Instance::preparePhysicalDevices`) Unable to get physical device count : VK_ERROR_INITIALIZATION_FAILED !
[VulkanInstanceService][Fatal] (vendors/emeraude-engine/src/Vulkan/Instance.cpp `EmEn::Vulkan::Instance::onInitialize`) No physical device available !
[Core][Fatal] (vendors/emeraude-engine/src/Core.cpp `EmEn::Core::initializeCoreLevel`) VulkanInstanceService service failed to execute!
```

Archives (local mirror under `tools/crash-reports/data/`, extracted next to them in `extracted/`):

- … 38 older archive(s), see `crash_reports.py list`
- `reports/8.0.2/ef17a668da06779c2c8676eec70938e7-2d76c41c-dc8f-4418-a61c-7795de56dc48.zip` → `archives/8.0.2/ef17a668da06779c2c8676eec70938e7-2d76c41c-dc8f-4418-a61c-7795de56dc48.zip`
- `reports/8.0.2/b5115e87eab991143413f728b339dd8b-2326ddd6-7d71-4a12-b19c-64baeb380dc8.zip` → `archives/8.0.2/b5115e87eab991143413f728b339dd8b-2326ddd6-7d71-4a12-b19c-64baeb380dc8.zip`
- `reports/8.0.2/a0f06246859f348e122eacf6a246408c-bc1d9b67-bb3c-4d28-9d8c-78cd1bd6e88b.zip` → `archives/8.0.2/a0f06246859f348e122eacf6a246408c-bc1d9b67-bb3c-4d28-9d8c-78cd1bd6e88b.zip`
- `reports/8.0.2/ed454557d83296853e3b3eb64732021f-daf89069-62a2-4865-8c58-428f7cb40945.zip` → `archives/8.0.2/ed454557d83296853e3b3eb64732021f-daf89069-62a2-4865-8c58-428f7cb40945.zip`
- `reports/8.0.2/049b1ff7505518c1ff408d9f01bea7e3-ffdd960a-e40c-4104-bd14-682cbb4f4a15.zip` → `archives/8.0.2/049b1ff7505518c1ff408d9f01bea7e3-ffdd960a-e40c-4104-bd14-682cbb4f4a15.zip`
- `reports/8.0.2/ded252c5d9402ff82dcb8ce09873b317-02634dfb-5c01-4625-999b-83f37fd7c4af.zip` → `archives/8.0.2/ded252c5d9402ff82dcb8ce09873b317-02634dfb-5c01-4625-999b-83f37fd7c4af.zip`
- `reports/8.0.2/08cc3dde3bd8c04319945aeb2d913a17-dd834ab4-9682-4e6b-8ea8-a2153bf6e9d4.zip` → `archives/8.0.2/08cc3dde3bd8c04319945aeb2d913a17-dd834ab4-9682-4e6b-8ea8-a2153bf6e9d4.zip`
- `reports/8.0.2/361fe9b24d6ecee834f438679023f662-cc2affec-828d-4987-8909-9adb76ac95eb.zip` → `archives/8.0.2/361fe9b24d6ecee834f438679023f662-cc2affec-828d-4987-8909-9adb76ac95eb.zip`
- `reports/8.0.2/d332179a4f4f03b28e04f5b731797543-03856748-4219-47ba-b490-3e0073ec795b.zip` → `archives/8.0.2/d332179a4f4f03b28e04f5b731797543-03856748-4219-47ba-b490-3e0073ec795b.zip`
- `reports/8.0.2/47b44234bc94661f63e0c4f608b91f26-aa3124d5-4f8b-4b96-b43e-6cfa7e714b90.zip` → `archives/8.0.2/47b44234bc94661f63e0c4f608b91f26-aa3124d5-4f8b-4b96-b43e-6cfa7e714b90.zip`
- `reports/8.0.2/e16a01270a66fc7d02a266ed385d7dc7-c955cdc1-56dc-4124-a8af-98f0aff61810.zip` → `archives/8.0.2/e16a01270a66fc7d02a266ed385d7dc7-c955cdc1-56dc-4124-a8af-98f0aff61810.zip`
- `reports/8.0.2/ca7da69e897d50a62f3fb0bc6f3b011c-4a410dee-f3f6-456a-a5d5-e0cb19b4c511.zip` → `archives/8.0.2/ca7da69e897d50a62f3fb0bc6f3b011c-4a410dee-f3f6-456a-a5d5-e0cb19b4c511.zip`
- `reports/8.0.2/1c00afcb628c6d57a90110dce97f1700-1723556e-eef5-458b-83e5-8f5bada69e7b.zip` → `archives/8.0.2/1c00afcb628c6d57a90110dce97f1700-1723556e-eef5-458b-83e5-8f5bada69e7b.zip`
- `reports/8.0.2/12385dc4bdb07d680ccf6eb94a6f24d3-5e293d41-ae30-4f09-8368-358e39827dc9.zip` → `archives/8.0.2/12385dc4bdb07d680ccf6eb94a6f24d3-5e293d41-ae30-4f09-8368-358e39827dc9.zip`
- `reports/8.0.2/2da2d9ccbe3b8043ee38549a10b3e494-9a8a3092-dce7-40d8-bee0-437bece277a5.zip` → `archives/8.0.2/2da2d9ccbe3b8043ee38549a10b3e494-9a8a3092-dce7-40d8-bee0-437bece277a5.zip`
- `reports/8.0.2/eabb9355ccf3da277ca54e5f423cb3f0-debe9672-7277-498a-b8b5-c96b1dd5f969.zip` → `archives/8.0.2/eabb9355ccf3da277ca54e5f423cb3f0-debe9672-7277-498a-b8b5-c96b1dd5f969.zip`
- `reports/8.0.2/7b3cded70076145c2371bf3263edcead-471e207b-504d-45ae-bb00-23dfe293f207.zip` → `archives/8.0.2/7b3cded70076145c2371bf3263edcead-471e207b-504d-45ae-bb00-23dfe293f207.zip`
- `reports/8.0.2/d58ebc717921f627434c40fa9e93d9c3-c9872d3f-8cee-4079-9c08-5bc369971118.zip` → `archives/8.0.2/d58ebc717921f627434c40fa9e93d9c3-c9872d3f-8cee-4079-9c08-5bc369971118.zip`
- `reports/8.0.2/6676ca9a13bec9b2a5a39c447cafa5f6-1367fc7e-cea5-46f6-a133-0235353261b6.zip` → `archives/8.0.2/6676ca9a13bec9b2a5a39c447cafa5f6-1367fc7e-cea5-46f6-a133-0235353261b6.zip`
- `reports/8.0.2/dddcc2b7eb2138db8f2acf664cf5fb61-6744b492-6976-4c42-94ac-ef0f8a097944.zip` → `archives/8.0.2/dddcc2b7eb2138db8f2acf664cf5fb61-6744b492-6976-4c42-94ac-ef0f8a097944.zip`
<!-- crash-reports:end -->

## 8.0.3 status (checked 2026-10-21)

**Fixed.** 34 of the 56 reports carry `VK_INCOMPLETE`, accepted as a success since `7dfbe4c0` (engine 1.0.1): those machines should start. The 22 others had no device at all: 8.0.3 tells them (`NoVulkanDriver`, exit 97) — Changed, not fixed.

Checked against the changes between 8.0.2 and 8.0.3 in the four repositories (app_system 1.1.1 → 1.1.2, engine 1.0.0 → 1.0.1, app_kernel 1.3.1 → 1.3.2, core `47cbe7cd9` → `c3ab7b390`); nothing relevant after 8.0.3. Proof only comes from the 8.0.3 reports (plan § 5).

## What remains

- [ ] A run on a machine that answers `VK_INCOMPLETE` (a Windows AMD hybrid laptop, as in the reports): no reproduction
      machine here; the Linux path is verified unchanged (3 devices found).

## References

- Vulkan specification, § "Return Codes" (`VK_INCOMPLETE` is a success code) and § "Physical Devices"
  (`vkEnumeratePhysicalDevices`: fewer handles written than available → `VK_INCOMPLETE`).