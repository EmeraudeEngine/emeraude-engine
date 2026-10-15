---
id: gpu-inventory-without-vulkan
title: When Vulkan fails, nothing says which GPUs the machine has
status: in-progress
priority: high
scope: PlatformSpecific, SystemInfo, Vulkan/Instance
opened: 2026-10-15
tags: [diagnostics, startup, drivers]
---

# When Vulkan fails, nothing says which GPUs the machine has

## Why

The only GPU inventory of the engine comes from Vulkan (`vkEnumeratePhysicalDevices`). When the instance cannot be
created (`VK_ERROR_INCOMPATIBLE_DRIVER`) or the enumeration fails, the log and any downstream report contain no GPU
at all: impossible to tell "only an old integrated GPU without a Vulkan driver" (not compatible) from "a capable
discrete GPU whose driver is missing or broken" (update the driver). The user-facing refusal message
(`docs/subsystems/vulkan/22-startup-refusals-told-to-the-user.md`) needs that distinction.

Measured in user crash reports of a downstream application, 2026-10-15: a Windows 10 desktop (Intel Core i5-4460)
fails with `VK_ERROR_INCOMPATIBLE_DRIVER` twice; two Ryzen laptops fail at the enumeration. None of the reports can
name the GPUs.

## Done (2026-10-15)

- `SystemInfo::fetchGPUInformation()` fills `GPUDevice` (driver name / version, `software` added) and logs one line at
  startup ("Graphics adapters (operating system): N | …"); the startup refusal message uses it (names, vendor pages).
  Linux (sysfs + `pci.ids`, integrated vs discrete by the root PCI bus, a stated heuristic); Windows by Window-PA (DXGI
  `EnumAdapters1`, the UMD version, the LUID as the bus, the Basic Render Driver flagged software, integrated at
  <= 512 MiB dedicated — stated heuristic); macOS by macOS-PA (IOKit `IOAccelerator`, owner decision: no Metal, which
  would load the very drivers being diagnosed). Verified on the three OS (Linux RTX 3070 Ti + Intel UHD 770; Windows RTX
  3500 Ada + Intel UHD + Basic Render; macOS Apple M6).

## What remains

- [ ] The Intel-Mac PCI path of the macOS implementation is untested (no Intel Mac).
- [x] The CEF helper processes logged the inventory too (3 lines per launch, macOS and Linux): a child process
      (`Settings::isChildProcess()`) no longer takes it (2026-10-15; Linux: 1 line with 2 helpers started).

## ⚠️ Traps

- Windows also lists the "Microsoft Basic Render Driver" (software adapter): mark it, do not count it as a GPU.
- No COM exception / throwing API: every call checks its `HRESULT`.