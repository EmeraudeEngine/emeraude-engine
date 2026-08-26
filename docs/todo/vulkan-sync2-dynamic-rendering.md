---
id: vulkan-sync2-dynamic-rendering
title: Implement VK_KHR_synchronization2 and VK_KHR_dynamic_rendering
status: open
priority: medium
scope: Vulkan
opened: unknown
tags: [vulkan, performance]
---

# Implement VK_KHR_synchronization2 and VK_KHR_dynamic_rendering

## Why

Vulkan 1.3 core features. Beyond the modernisation, dynamic rendering is the lever for ordering
draws by pipeline layout and cutting binding cost.

## What remains

- [ ] Implement `VK_KHR_synchronization2`.
- [ ] Implement `VK_KHR_dynamic_rendering`.
- [ ] Then leverage dynamic rendering to order draws by pipeline layout.

## Origin

Inherited from the historical root `TODO.md`.
