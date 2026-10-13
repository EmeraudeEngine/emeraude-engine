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
log line is still missing by default.

Current state (2026-10-13): `PhysicalDevice::getPhysicalDeviceInformation()` (`src/Vulkan/PhysicalDevice.cpp:722-732`)
already prints the API driver and the decoded driver version, but `Instance.cpp:846` traces it only when
`m_showInformation` is set (`Core/Video/ShowInformation`, `--show-video-infos` / `--show-all-infos`; default `false`
unless `EMERAUDE_INFOS_DEFAULTS`, `src/SettingKeys.hpp:310-315`, read at `src/Vulkan/Instance.cpp:72-75`). The three
"Graphics capable physical device selected" lines (`src/Vulkan/Instance.cpp:1011`, `:1015`, `:1021`) print the name
only. `VkPhysicalDeviceDriverProperties` is never queried.

## What remains

- [ ] One unconditional line at device selection: name, type, driver name + info, API version, driver version.
- [ ] Query `VkPhysicalDeviceDriverProperties` (driverName, driverInfo) for it.
