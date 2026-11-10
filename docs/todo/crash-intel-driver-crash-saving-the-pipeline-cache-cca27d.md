---
id: crash-intel-driver-crash-saving-the-pipeline-cache-cca27d
title: "Intel Vulkan driver — crash in vkGetPipelineCacheData while the pipeline cache is saved at exit"
status: open
priority: high
scope: src/Graphics/Renderer.cpp
opened: 2026-11-10
tags: [crash, crash-report-auto]
---

# Intel Vulkan driver — crash in vkGetPipelineCacheData while the pipeline cache is saved at exit

> Dropped by the downstream application's crash-report tool: a cause seen in its production crash reports.
> The block between the `crash-reports` markers is rewritten at every run of that tool (found by its signature line,
> names of the application redacted); write outside it. Deleting the item closes the cause: a new report of it from a
> newer version is then flagged as a regression.

## Why

<!-- crash-reports:begin -->
| | |
|---|---|
| Machines | 1 — per version: 8.0.4 ×1 |
| Reports | 4 (first 2026-11-09, last 2026-11-10) |
| Versions | 8.0.4 ×4 |
| Platforms | windows ×4 |
| OS | Windows 11 Home ×4 |
| GPU (selected) | Intel(R) UHD Graphics 630 (driver 100.7925) ×4 |
| ANGLE backend | d3d11 ×4 |
| Died at milestone | Stopping ×4 |
| Single-instance lock | Acquired ×4 |

- **Cause (normalized):** `EXCEPTION_ACCESS_VIOLATION_READ in the browser process: igvk64.dll`
- **Where:** `igvk64.dll` (no symbols: a driver or a system library)
- **Signature:** `cca27d1de81bb90a`

Last error before the death (else the last trace), by number of reports:

- ×4 `browser process, then the run died at Stopping`

Traces of the main journal (normalized, latest report; around the Fatal trace when there is one):

```
[Core][Success] (vendors/emeraude-engine/src/Core.cpp `EmEn::Core::terminate`) OverlayManagerService secondary service terminated gracefully!
[SystemNotificationService][Info] (vendors/emeraude-engine/src/SystemNotification.cpp `EmEn::SystemNotification::onTerminate`) System notification service terminated.
[Core][Success] (vendors/emeraude-engine/src/Core.cpp `EmEn::Core::terminate`) SystemNotificationService secondary service terminated gracefully!
[Core][Success] (vendors/emeraude-engine/src/Core.cpp `EmEn::Core::terminate`) AudioManagerService secondary service terminated gracefully!
[Core][Success] (vendors/emeraude-engine/src/Core.cpp `EmEn::Core::terminate`) PhysicsManagerService secondary service terminated gracefully!
[RendererService][Info] (vendors/emeraude-engine/src/Graphics/Renderer.cpp `EmEn::Graphics::Renderer::requestShutdown`) Graceful shutdown requested. Waiting for frames in-flight to complete...
[RendererService][Info] (vendors/emeraude-engine/src/Graphics/Renderer.cpp `EmEn::Graphics::Renderer::requestShutdown`) All frames in-flight have completed.
[VulkanDevice][Info] (vendors/emeraude-engine/src/Vulkan/Device.cpp `EmEn::Vulkan::Device::getPipelineCacheData`) The restored and live pipeline caches merged in <n> ms.
```

Native stacks of the latest report (minidumps, crashing thread; `?` = guessed by stack scanning):

```
EXCEPTION_ACCESS_VIOLATION_READ in the browser process
 0   igvk64.dll + 0x0000000000818517
 1 ? igvk64.dll + 0x00000000006552b9
 2 ? Emeraude.dll!_malloc_base(unsigned long long) [src/appcrt/heap/malloc_base.cpp:34]
 3 ? ? + 0x0000028ebd9b85e7
 4 ? igvk64.dll + 0x0000000000653cbe
 5 ? Emeraude.dll!std::vector<unsigned char,std::allocator<unsigned char> >::assign(const unsigned long long, unsigned char const&) [vector:1362]
 6 ? Emeraude.dll!EmEn::Vulkan::(anonymous namespace)::readPipelineCacheData(VkDevice_T*, VkPipelineCache_T*, std::vector<unsigned char,std::allocator<unsigned char> >&) [vendors/emeraude-engine/src/Vulkan/Device.cpp:1320]
 7 ? Emeraude.dll!EmEn::Vulkan::Device::getPipelineCacheData(std::vector<unsigned char,std::allocator<unsigned char> >&) const [vendors/emeraude-engine/src/Vulkan/Device.cpp:1392]
 8 ? Emeraude.dll!EmEn::Graphics::Renderer::savePipelineCache() const [vendors/emeraude-engine/src/Graphics/Renderer.cpp:3910]
 9 ? Emeraude.dll!EmEn::Graphics::Renderer::onTerminate() [vendors/emeraude-engine/src/Graphics/Renderer.cpp:1042]
10 ? Emeraude.dll!EmEn::Core::terminate() [vendors/emeraude-engine/src/Core.cpp:2019]
11 ? Emeraude.dll!EmEn::Core::run() [vendors/emeraude-engine/src/Core.cpp:823]
```

Archives:

- 4 archive(s), in the downstream application's crash-report mirror.
<!-- crash-reports:end -->

## Triage (2026-11-10)

- **Owner**: the engine. 1 Windows machine (Intel UHD Graphics 630, driver 100.7925), 4 reports in 8.0.4, every one
  at `Stopping`.
- **Stack** (symbolized): `Core::run()` → `Core::terminate()` → `Renderer::onTerminate()` →
  `Renderer::savePipelineCache()` → `Device::getPipelineCacheData()` → `readPipelineCacheData()` (`Device.cpp`, the
  `vkGetPipelineCacheData` call) → an access violation inside `igvk64.dll`. The driver crashes when asked for its
  cache data; the run had worked, only the exit fails (the crash is reported, the user's next launch starts).
- Found on 2026-11-10 once the crash-report tool named the causes of driver crashes without symbols (they were
  counted as silent deaths).

## What remains

- Decide the mitigation: the engine cannot catch an access violation in the driver. Candidates: skip the save on
  this driver family / version (a known-bad list), or save the cache earlier (after the startup's pipelines, not at
  exit) so a crash there loses less. Check whether the same driver crashes on the size query (first call) or on the
  data call.
- Confirm on later reports.

## References

- The downstream application's crash-report documentation — what an archive holds, the minidumps.
- The downstream application's crash-report tool documentation — how this item was produced, how the cause is chosen.
