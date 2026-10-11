---
id: physics-acceleration-transfer-pool-on-missing-family
title: Physics acceleration fails at startup: its transfer manager uses a queue family the compute device does not have
status: open
priority: high
scope: Physics/Manager, Vulkan/TransferManager, Vulkan/Instance (getComputeDevice)
opened: 2026-10-01
tags: [physics, vulkan, compute, validation]
---

# Physics acceleration fails at startup: its transfer manager uses a queue family the compute device does not have

## Why

Found in triad 9a (2026-10-01, Linux, RTX 3070 Ti) while testing the compute device's `ForceGPU` selection. With
`Core/Physics/EnableAcceleration = true` (off by default), the compute device is created with the compute family
(#2, 8 queues) and the transfer-only family (#1). Its transfer manager then creates a command pool on family **0**,
which is not among the device's queue families:

- `VUID-vkCreateCommandPool-queueFamilyIndex-01937` ("queueFamilyIndex (0) is not one of the queue families given via
  VkDeviceQueueCreateInfo structures when the device was created"), followed by "VulkanTransferManagerService:
  Unable to create the specific command pool", "[Fatal] PhysicsManagerService: VulkanTransferManagerService service
  failed to execute" and "No physics acceleration available!".
- At shutdown: "The Vulkan selected compute device '' smart pointer still have 4 uses", a "Vulkan object is not
  correctly destroyed" error and two `VUID-vkDestroyDevice-device-05137`.

The engine keeps running without acceleration. The defect is pre-existing: triad 9a changed neither the compute
device's queue families nor the transfer manager. It reproduces with `ForceGPU` set or unset.

## 2026-10-10/11 — two more layers in front of it, both fixed (engine, not committed yet)

- Engine `3c4986549` (queue timelines) made every queue create a timeline semaphore but requested `timelineSemaphore`
  on the graphics device only: the compute device's first queue failed
  (`VUID-VkSemaphoreTypeCreateInfo-timelineSemaphore-03252`, "Unable to find a suitable compute device"). Fixed:
  `getComputeDevice()` requests it, `checkDevicesFeaturesForCompute()` requires it.
- The half-created compute device then leaked through the `Device ↔ Queue` `shared_ptr` cycle and the process
  crashed in the NVIDIA driver at exit (SIGSEGV, every demo). Fixed: `Queue` holds a `Device &`
  (`docs/subsystems/vulkan/19-critical-device-owns-its-queues.md`), proved by fault injection (exit 0).
- With both fixes the startup reaches THIS item again, unchanged: `VUID-vkCreateCommandPool-queueFamilyIndex-01937`,
  the 4 remaining uses, `VUID-vkDestroyDevice-device-05137`, exit 0. Cause confirmed by reading:
  `TransferManager::onInitialize()` takes the "specific" pool on `getGraphicsFamilyIndex()` when
  `!hasBasicSupport()`, and a compute-only device has no graphics family. The transfer manager is written for a
  graphics device; giving it a compute-device role is a design choice (owner).

## Owner decision (2026-10-11)

The TransferManager takes the ROLE of its device: on a compute device its pools go on the compute (or transfer) family and the image layout transitions on the compute queue.

## What remains

- [ ] Find where the physics transfer manager picks family 0: probably a graphics-family index used on a compute-only
  device. Use the compute device's own transfer (or compute) family.
- [ ] Release the compute device cleanly when the physics service fails (the 4 remaining uses). The Windows peer
  (2026-10-01, NVIDIA and AMD, the same failure) traced the tail: the command pool whose creation the validation layer
  refused keeps its handle and outlives the device (`VUID-vkDestroyDevice-device-05137` "1 leaked objects: VkCommandPool",
  then "No device to destroy this command pool").
- [ ] Re-run with `EnableAcceleration = true`: 0 VUID, "physics acceleration" up, a clean shutdown.

## References

- `src/Physics/Manager.cpp`: `onInitialize()` and the compute device's transfer manager.
- `src/Vulkan/Instance.cpp`: `getComputeDevice()`.
