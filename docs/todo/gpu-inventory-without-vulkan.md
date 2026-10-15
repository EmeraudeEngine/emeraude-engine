---
id: gpu-inventory-without-vulkan
title: When Vulkan fails, nothing says which GPUs the machine has
status: open
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
discrete GPU whose driver is missing or broken" (update the driver). The user-facing message of
`incompatible-gpu-is-never-told-to-the-user` needs that distinction.

Measured in user crash reports of a downstream application, 2026-10-15: a Windows 10 desktop (Intel Core i5-4460)
fails with `VK_ERROR_INCOMPATIBLE_DRIVER` twice; two Ryzen laptops fail at the enumeration. None of the reports can
name the GPUs.

## What remains

- [ ] A Vulkan-independent adapter list, logged at startup next to the system information: Windows through DXGI
      (`IDXGIFactory1::EnumAdapters1`: description, vendor / device id, dedicated memory, LUID, driver version through
      `CheckInterfaceSupport` or the registry), Linux through `/sys/class/drm/*/device` (vendor, device, driver),
      macOS through IOKit / Metal device names.
- [ ] Exposed through `SystemInfo` so a downstream application can put it in its reports.
- [ ] Used by the Vulkan failure classification (`incompatible-gpu-is-never-told-to-the-user`).
- [ ] Runtime verification on the three OS, recorded in the topic doc.

## ⚠️ Traps

- Windows also lists the "Microsoft Basic Render Driver" (software adapter): mark it, do not count it as a GPU.
- No COM exception / throwing API: every call checks its `HRESULT`.