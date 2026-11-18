---
id: crash-vulkan-loader-fast-fail-during-physical-device-enumeration-1cfc0a
title: "Crash reports: the Vulkan loader fast-fails while the physical devices are enumerated (the device list changes during enumeration)"
status: open
priority: high
scope: src/Vulkan/Instance.cpp (preparePhysicalDevices), src/Vulkan/PhysicalDevice.cpp
opened: 2026-11-18
tags: [crash, crash-report-auto, vulkan, loader, startup, windows]
---

# Crash reports: the Vulkan loader fast-fails while the physical devices are enumerated (the device list changes during enumeration)

Triaged 2026-11-18. 1 machine, 7 reports (every launch), Windows, at `SessionOpened`.

**Evidence:** `EXCEPTION_STACK_BUFFER_OVERRUN / FAST_FAIL_FATAL_APP_EXIT` inside `vulkan-1.dll`, reached from
`PhysicalDevice::PhysicalDevice` (`PhysicalDevice.cpp:58`) during `Instance::preparePhysicalDevices`. Just before, the
engine traced "The physical device list changed during its enumeration (… retries, last answer VK_INCOMPLETE)": the
loader or a driver changes the device list between two calls, and a query on one of those devices aborts in the loader.

**Next.** Read the report's GPUs and loader version; check what `PhysicalDevice.cpp:58` queries first and whether a
device that disappeared between the enumeration and the query can be skipped instead of queried. One machine: likely a
driver / layer installation issue, but the engine should survive it.

## Why

<!-- crash-reports:begin -->
| | |
|---|---|
| Machines | 1 — per version: 8.0.5 ×1 |
| Reports | 7 (first 2026-11-17, last 2026-11-18) |
| Versions | 8.0.5 ×7 |
| Platforms | windows ×7 |
| OS | Windows 11 Home ×7 |
| GPU (selected) | unknown ×7 |
| ANGLE backend | d3d11 ×7 |
| Died at milestone | SessionOpened ×7 |
| Single-instance lock | Acquired ×7 |

- **Cause (normalized):** `EXCEPTION_STACK_BUFFER_OVERRUN / FAST_FAIL_FATAL_APP_EXIT in the browser process: vulkan-1.dll`
- **Where:** `vulkan-1.dll` (no symbols: a driver or a system library)
- **Signature:** `1cfc0ac10a02b89a`

Last error before the death (else the last trace), by number of reports:

- ×7 `browser process, then the run died at SessionOpened`

Traces of the main journal (normalized, latest report; around the Fatal trace when there is one):

```
[a line of the downstream application, redacted] (×5)
[Core][Success] (vendors/emeraude-engine/src/Core.cpp `EmEn::Core::initializeCoreLevel`) PlatformManagerService service up!
[VulkanInstanceService][Warning] (vendors/emeraude-engine/src/Vulkan/Instance.cpp `EmEn::Vulkan::Instance::preparePhysicalDevices`) The physical device list changed during its enumeration (<n> retries, last answer VK_INCOMPLETE) : the last list is used.
[VulkanInstanceService][Info] (vendors/emeraude-engine/src/Vulkan/Instance.cpp `EmEn::Vulkan::Instance::preparePhysicalDevices`) <n> physical device(s) found.
```

Native stacks of the latest report (minidumps, crashing thread; `?` = guessed by stack scanning):

```
EXCEPTION_STACK_BUFFER_OVERRUN / FAST_FAIL_FATAL_APP_EXIT in the browser process
 0   vulkan-1.dll + 0x00000000000eae75
 1 ? vulkan-1.dll + 0x000000000007a641
 2 ? vulkan-1.dll + 0x00000000001642ef
 3 ? Emeraude.dll!EmEn::Vulkan::PhysicalDevice::PhysicalDevice(VkPhysicalDevice_T* const&, unsigned int) [vendors/emeraude-engine/src/Vulkan/PhysicalDevice.cpp:58]
 4 ? Emeraude.dll!EmEn::Vulkan::Instance::preparePhysicalDevices() [vendors/emeraude-engine/src/Vulkan/Instance.cpp:438]
 5 ? Emeraude.dll!EmEn::Vulkan::Instance::onInitialize() [vendors/emeraude-engine/src/Vulkan/Instance.cpp:332]
 6 ? Emeraude.dll!EmEn::ServiceInterface::initialize() [vendors/emeraude-engine/src/ServiceInterface.hpp:122]
 7 ? Emeraude.dll!EmEn::ServiceInterface::initialize(std::vector<EmEn::ServiceInterface *,std::allocator<EmEn::ServiceInterface *> >&) [vendors/emeraude-engine/src/ServiceInterface.hpp:142]
 8 ? Emeraude.dll!EmEn::Core::initializeCoreLevel() [vendors/emeraude-engine/src/Core.cpp:1225]
 9 ? Emeraude.dll!EmEn::Core::run() [vendors/emeraude-engine/src/Core.cpp:653]
10 ? <a frame of the downstream application>
11 ? <a frame of the downstream application>
```

Archives:

- 7 archive(s), in the downstream application's crash-report mirror.
<!-- crash-reports:end -->

## What remains

- Read the journals of the archives counted above (in the downstream application's crash-report mirror).
- Find the defect, reproduce it, fix it where it lives.

## References

- The downstream application's crash-report documentation — what an archive holds, the flight recorder milestones.
- The downstream application's crash-report tool documentation — how this item was produced, how the cause is chosen.
