---
id: recovered-device-creation-failure-traced-as-fatal
title: A vkCreateDevice() failure the engine recovers from is traced as Fatal
status: open
priority: high
scope: Vulkan/Device
opened: 2026-10-22
tags: [vulkan, startup, logging, crash-report, defect]
---

# A vkCreateDevice() failure the engine recovers from is traced as Fatal

## Why

Since `7dfbe4c0` (engine 1.0.1), `Instance::getGraphicsDevice()` (`src/Vulkan/Instance.cpp:1058-1090`) tries every
compatible physical device in turn: a device whose `vkCreateDevice()` fails is skipped ("could not be used for
graphics", an Error) and the next one is created. But `Device::createDevice()` (`src/Vulkan/Device.cpp:921`) still
traces that failure as **Fatal** ("Unable to create a logical device : <VkResult> !"), although the run goes on — a
lie in the log. The real end of the startup, when no device at all can be created, is already traced by its callers
("No compatible physical device could be created for graphics.", then the renderer's Fatal).

It matters downstream: app_system's crash-report tool classifies a report by the **first Fatal** of the journal
(`tools/crash-reports/README.md` § How the cause is chosen). A run that recovered on the second device and crashed
much later for another reason is filed under "Fatal in VulkanDevice — Unable to create a logical device", a cause
closed on 2026-10-22 (`VK_ERROR_FEATURE_NOT_PRESENT`, `VK_ERROR_OUT_OF_HOST_MEMORY`): a false regression, and the real
cause hidden.

## What remains

- Trace the `vkCreateDevice()` failure as an Error in `Device::createDevice()` (it is the caller that decides whether
  it is fatal); check the compute device path (`Instance::getComputeDevice()`) and any other caller for the same pattern.
- Look for other Fatal traces on paths 1.0.1 made recoverable (the device selection, the swap-chain retry of
  `8a5bfd58`: "Unable to create the swap-chain" is still a Fatal of `VulkanSwapChain` while the recreation is retried).

## ⚠️ Traps

- Changing a traced message or its level changes the downstream crash signature: the old items find their reports by
  signature (the downstream tool, plan `CrashReport Solucius`). Note the release in which the level changed.

## References

- `docs/subsystems/vulkan/22-startup-refusals-told-to-the-user.md` § What the production reports showed.
- app_system `docs/plans/crashreport-solucius.md` (wave W1).
