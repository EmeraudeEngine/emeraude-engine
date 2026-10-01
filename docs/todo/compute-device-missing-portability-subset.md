---
id: compute-device-missing-portability-subset
title: The physics compute device is created without VK_KHR_portability_subset on MoltenVK
status: open
priority: unranked
scope: Vulkan/Instance (getComputeDevice), Physics/Manager
opened: 2026-10-01
tags: [physics, vulkan, macos, moltenvk, validation]
---

# The physics compute device is created without VK_KHR_portability_subset on MoltenVK

## Why

Found by the macOS peer during the triad 9a validation (2026-10-01, Apple M2, bundled MoltenVK), with
`Core/Physics/EnableAcceleration = true` (it is off by default). The logical compute device fails to be created:

- `VUID-VkDeviceCreateInfo-pProperties-04451`: "VK_KHR_portability_subset must be enabled because physical device …
  supports it".
- `[Fatal][VulkanDevice] Unable to create a logical device : VK_ERROR_VALIDATION_FAILED_EXT !`
- `[Fatal][PhysicsManagerService] Unable to find a suitable compute device !`
- "No physics acceleration available!"

The engine keeps running and exits cleanly.

`Instance::getComputeDevice()` passes an EMPTY `requiredExtensions` vector to `Device::create()`. The
portability-subset handling (enable the extension when advertised, plus its features) exists only on the graphics
device path. The defect is pre-existing: 9a only made that vector `const`.

On Linux and Windows the same option hits a different failure first: engine item
`physics-acceleration-transfer-pool-on-missing-family`.

## What remains

- [ ] Share the portability-subset handling between the graphics and the compute device creation, so that one device
  creation path enables what the physical device requires.
- [ ] Re-run on macOS with `EnableAcceleration = true`: 0 VUID, then check the transfer-pool item there.

## References

- `src/Vulkan/Instance.cpp`: `getComputeDevice()` and the graphics device's portability handling.
- `src/Vulkan/Device.cpp`: `create()`.
