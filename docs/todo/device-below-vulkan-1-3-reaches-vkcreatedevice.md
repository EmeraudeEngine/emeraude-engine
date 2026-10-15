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

## What remains

- [ ] macOS: confirm the API version MoltenVK reports (the 1.2 minimum there) on the M6.
- [ ] Windows: a run (no outdated driver at hand: the forced path as on Linux).

## ⚠️ Traps

- macOS / MoltenVK reports its own API version: check what the minimum means there before refusing (the macOS path
  compiles for Vulkan 1.2 / SPIR-V 1.5).
- `maintenance4` absent is a symptom of a < 1.3 device, not the cause; the version is the test.