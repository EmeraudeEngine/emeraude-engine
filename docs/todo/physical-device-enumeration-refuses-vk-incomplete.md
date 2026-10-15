---
id: physical-device-enumeration-refuses-vk-incomplete
title: The physical device enumeration treats VK_INCOMPLETE as a failure and stops the engine
status: open
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

## What remains

- [ ] Enumerate with the standard loop: query the count, size the array, fill; repeat while the answer is
      `VK_INCOMPLETE` (bounded number of attempts, the last answer kept), accept `VK_SUCCESS` and a final
      `VK_INCOMPLETE` as valid data; only a negative `VkResult` is a failure.
- [ ] Log the number of physical devices found and the number of retries when there was one.
- [ ] Check every other two-call enumeration of the engine for the same mistake (extensions, layers, queue families,
      surface formats / present modes, swap-chain images) and fix them the same way.
- [ ] Runtime verification recorded in the topic doc (no reproduction machine here: at least a forced
      `VK_INCOMPLETE` path, e.g. a fill array one element short).

## References

- Vulkan specification, § "Return Codes" (`VK_INCOMPLETE` is a success code) and § "Physical Devices"
  (`vkEnumeratePhysicalDevices`: fewer handles written than available → `VK_INCOMPLETE`).