---
id: vulkan-instance-has-no-destructor
title: Vulkan::Instance releases the VkInstance only in onTerminate()
status: open
priority: high
scope: src/Vulkan/Instance.hpp, src/Vulkan/Instance.cpp
opened: 2026-10-08
tags: [ave-robustus-ii, raii, defect]
---

# Vulkan::Instance releases the VkInstance only in onTerminate()

## Why
`Vulkan/Instance.cpp:246`: `vkDestroyInstance()` is called only from `onTerminate()`; the class has no destructor
(checked 2026-10-08). Any path that skips `terminate()` leaks the instance (and its debug messenger).

## What remains
- The destructor releases what is still held (decision D3-a); the debug messenger and the instance in that order.
- Covered by the fault-injection test of `core-init-failure-skips-terminate`.

## References
- projet-alpha `docs/plans/ave-robustus-ii.md` § 3.1 H3, § 3.3.
