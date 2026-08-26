---
id: ubo-max-capacity-detection
title: Detect the real UBO maximum capacity
status: open
priority: low
scope: Vulkan
opened: unknown
tags: [vulkan, limits]
---

# Detect the real UBO maximum capacity

## Why

The limit is hard-coded to **65,536 bytes** today.

## What remains

- [ ] Read it from the device (`maxUniformBufferRange`) and honour it, instead of assuming the
  spec floor.

## Origin

Inherited from the historical root `TODO.md`.
