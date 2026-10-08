---
id: max-sample-count-min-instead-of-and
title: PhysicalDevice::getMaxAvailableSampleCount() takes std::min of two bitmasks
status: open
priority: high
scope: src/Vulkan/PhysicalDevice.cpp
opened: 2026-10-08
tags: [ave-robustus-ii, vulkan, defect]
---

# PhysicalDevice::getMaxAvailableSampleCount() takes std::min of two bitmasks

## Why
`getMaxAvailableSampleCount()` combines the colour and depth `VkSampleCountFlags` with `std::min` instead of `&`. With
non-contiguous support the result can be a count the colour (or depth) attachment does not support → a VUID at
pipeline / image creation.

## What remains
- `colour & depth`, then the highest bit; a unit-level check with synthetic masks (e.g. 0b1011 & 0b0111).

## References
- Found by the Ave Robustus II warning pass (2026-10-08, projet-alpha `docs/plans/ave-robustus-ii.md`); not raised by a warning, so left for its own fix.
