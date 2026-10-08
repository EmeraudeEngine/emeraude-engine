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

Since 2026-10-05 `PhysicalDevice::DriverVersionString()` decodes the vendor packing (NVIDIA 10.8.8.6 bits, Intel on
Windows 18.14 — measured: NVIDIA Linux 615.71.09, Windows 616.92); a downstream application's crash-report token uses it. The startup
log line is still missing.

## What remains

- [ ] One line at device selection: name, type, driver name + info, API version.
