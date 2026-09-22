---
id: log-the-gpu-driver-version
title: The startup log never states the GPU driver version
status: open
priority: low
scope: Vulkan/Instance, Vulkan/PhysicalDevice
opened: 2026-09-22
tags: [diagnostics, cross-platform-bench]
---

# The startup log never states the GPU driver version

## Why

Windows peer session, 2026-09-22: the engine log prints no driver line, so a report from another machine has
to fall back on `vulkaninfo`. Every cross-platform comparison needs it: `VkPhysicalDeviceDriverProperties`
(driverName, driverInfo) and `VkPhysicalDeviceProperties::driverVersion`, decoded per vendor (NVIDIA packs it
differently from the Vulkan version macro).

## What remains

- [ ] One line at device selection: name, type, driver name + info, API version.
