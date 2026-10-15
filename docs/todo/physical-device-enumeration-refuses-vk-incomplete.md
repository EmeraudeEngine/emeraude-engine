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

## What remains

- [ ] A run on a machine that answers `VK_INCOMPLETE` (a Windows AMD hybrid laptop, as in the reports): no reproduction
      machine here; the Linux path is verified unchanged (3 devices found).

## References

- Vulkan specification, § "Return Codes" (`VK_INCOMPLETE` is a success code) and § "Physical Devices"
  (`vkEnumeratePhysicalDevices`: fewer handles written than available → `VK_INCOMPLETE`).