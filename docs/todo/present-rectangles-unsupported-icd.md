---
id: present-rectangles-unsupported-icd
title: The surface queries the present rectangles unconditionally; MoltenVK's ICD lacks them and the loader reports an ERROR
status: open
priority: high
scope: engine Vulkan (PhysicalDevice::getPresentRectangles(), Surface)
opened: 2026-11-09
tags: [vulkan, macos, moltenvk, loader, validation]
---

# The surface queries the present rectangles unconditionally; MoltenVK's ICD lacks them and the loader reports an ERROR

## Why

On macOS (M6, MoltenVK), every test instance shows three Vulkan loader ERROR messages (macOS-PA, 2026-11-09):

```
[Error][VulkanDebugMessenger] [VK][GENERAL][ERROR][ID:Loader Message #0]
[MESSAGE:ICD associated with VkPhysicalDevice does not support GetPhysicalDevicePresentRectanglesKHX]
```

`Surface.cpp` (lines 97 and 117) calls `PhysicalDevice::getPresentRectangles()`, which calls
`vkGetPhysicalDevicePresentRectanglesKHR` with no capability check. That entry point belongs to device groups
(`VK_KHR_device_group` with `VK_KHR_swapchain`, core in Vulkan 1.1); MoltenVK's ICD does not implement it, so the loader
refuses the call with an ERROR.

The messages are not new: they were printed raw on stderr before the "no direct stream access" lot (engine
`b3fe29ff`) routed the debug messenger through the Tracer, where they now count as `[Error]` and break the
"0 `[Error]`" bar of a macOS test instance.

## What remains

- Read the use: `Surface::presentRectangles()` (`Surface.hpp:169`) has **no caller** in the engine or projet-alpha
  (`/usr/bin/grep -rn "presentRectangles()"`, 2026-11-09; check the downstream consumers read-only). If nothing needs the
  rectangles, the query may simply go — an owner decision.
- Otherwise, query them only where the device supports the entry point (device-group support / API version /
  portability subset), and document the condition at the call.
- Verify on macOS-PA: a default-demo test instance with 0 `[Error]` from `VulkanDebugMessenger`, and on Linux and
  Windows that nothing changed (0 VUID).

## ⚠️ Traps

- Linux (NVIDIA) and Windows never show the message: only a peer run on macOS proves the fix.
- The loader message names the `KHX` (pre-1.1) entry point; the engine calls the `KHR` one.

## References

- `src/Vulkan/PhysicalDevice.cpp` (`getPresentRectangles()`), `src/Vulkan/Surface.cpp`, `src/Vulkan/Surface.hpp`.
- `src/Vulkan/DebugMessenger.cpp`: the message severity maps onto the Tracer's since 2026-11-09.
