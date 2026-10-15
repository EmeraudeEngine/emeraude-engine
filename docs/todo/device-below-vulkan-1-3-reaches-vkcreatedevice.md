---
id: device-below-vulkan-1-3-reaches-vkcreatedevice
title: A device below Vulkan 1.3 (outdated driver) is selected and dies at vkCreateDevice, with no fallback
status: open
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

## What remains

- [ ] `checkDeviceCompatibility()`: refuse a device whose `apiVersion` is below the engine's minimum (one named
      constant, shared with the instance `apiVersion`), with an explicit log line: device name, its API version,
      the required version, the driver version (decoded, `PhysicalDevice::DriverVersionString()`), and "update the
      graphics driver".
- [ ] Keep the reason of each refused device so the user-facing message can use it
      (`incompatible-gpu-is-never-told-to-the-user`).
- [ ] When `Device::create()` fails on the selected device, try the next compatible scored device before giving up
      (preferred / forced selection included: log that it was abandoned and why).
- [ ] On `VK_ERROR_FEATURE_NOT_PRESENT`, log which requested features the device does not advertise (compare the
      requested chain with the queried one) — the code is otherwise silent on the culprit.
- [ ] The unconditional driver line of `log-the-gpu-driver-version` belongs to the same diagnosis: do both.
- [ ] Runtime verification recorded in the topic doc (no 1.2 driver here: force the minimum above the local device's
      version to exercise the refusal and the fallback).

## ⚠️ Traps

- macOS / MoltenVK reports its own API version: check what the minimum means there before refusing (the macOS path
  compiles for Vulkan 1.2 / SPIR-V 1.5).
- `maintenance4` absent is a symptom of a < 1.3 device, not the cause; the version is the test.