---
id: renderer-init-failure-leaks-transfer-manager
title: After a Renderer initialisation failure, the TransferManager outlives the device and aborts in its destructor
status: in-progress
priority: high
scope: Graphics/Renderer (failure path), Vulkan/TransferManager, Core teardown
opened: 2026-10-11
tags: [ave-robustus-ii, lifetime, failure-path, defect]
---

# After a Renderer initialisation failure, the TransferManager outlives the device and aborts in its destructor

## Why

macOS-PA (M6, 2026-10-11), when the BindlessTextureManager service failed (item
`bindless-layout-exceeds-max-per-set-descriptors`) and the Renderer service failed with it:
`VUID-vkDestroyDevice-device-05137` "4 leaked objects: VkCommandBuffer, VkFence, VkCommandPool ×2", "The Vulkan selected
graphics device '' smart pointer still have 10 uses !", then after "*** Core level terminated ***": "No device to destroy
the descriptor set layout", "No device to destroy the fence", `VUID-vkFreeCommandBuffers-device-parameter`, SIGABRT:
`vkFreeCommandBuffers < CommandPool::freeCommandBuffer < CommandBuffer::~CommandBuffer < TransferManager::~TransferManager
< Renderer::~Renderer < Core::~Core`. The Renderer's TransferManager keeps a command buffer, two pools and a fence alive
past the device's destruction. Ave Robustus II: every `initialize*()` step must exit cleanly (P3, fault injection).

## Fix (2026-10-11, waiting for the macOS proof)

`Renderer::onInitialize()` runs `initializeRenderingStack()` and, on failure after the device was acquired, calls
`onTerminate()` before returning (sub-services, pools, swap-chain, command buffers released while the device exists);
`BindlessTextureManager::onInitialize()` releases its partial state the same way. Linux fault injection (the bindless
layout creation forced to fail): exit 1, 0 VUID, no leaked object, no "No device to destroy", no abort. Since volk the
macOS symptom had become a SIGSEGV (a null `vkFreeCommandBuffers`: volk finalized before `~Renderer`) — the same root.
To close: the macOS layer run (which still hits the bindless item until that fix lands) exits cleanly.

## What remains

- The Renderer (and every service that owns Vulkan objects) releases them in its own `onTerminate()` / failure path,
  before the device goes; the TransferManager's pools and fence are terminated when its initialisation is abandoned.
- Fault injection: force the BindlessTextureManager (and each Renderer sub-service) to fail in turn — exit with a
  clean code, 0 leaked object, no abort.
