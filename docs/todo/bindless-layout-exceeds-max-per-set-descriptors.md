---
id: bindless-layout-exceeds-max-per-set-descriptors
title: The bindless texture layout ignores maxPerSetDescriptors — refused by the 1.4.363 validation layer on MoltenVK
status: in-progress
priority: high
scope: Graphics/BindlessTextureManager (budget), Vulkan/DescriptorSetLayout
opened: 2026-10-11
tags: [vulkan, validation, macos, moltenvk, defect]
---

# The bindless texture layout ignores maxPerSetDescriptors — refused by the 1.4.363 validation layer on MoltenVK

## Why

macOS-PA (Mac mini M6, MoltenVK 1.4.2, validation layer 1.4.363 from the external dependencies, 2026-10-11):
`VUID-vkCreateDescriptorSetLayout-support-09582` "pCreateInfo was passed to vkGetDescriptorSetLayoutSupport which reported
this layout is not supported. The layout has 4928 total descriptors which is more than maxPerSetDescriptors (1212)"
(1D 256 + 2D 4096 + 3D 256 + Cube 256 + CubeArray 64). `BindlessTexturesLayout` is not created, the
BindlessTextureManager service fails, the Renderer service fails, the application stops. Every demo, with validation.

The engine budgets the bindless arrays on the update-after-bind limits only ("budget 500000 … perStageSamplers"), never
on `VkPhysicalDeviceMaintenance3Properties::maxPerSetDescriptors`, and never asks `vkGetDescriptorSetLayoutSupport`.
MoltenVK reported 1212 already with the 1.4.1 driver (round 1 vulkaninfo); the 1.4.357 layer did not check it, the
1.4.363 one does. Without the layer MoltenVK accepts the layout and renders (it tolerates the violation): a spec
violation, not a MoltenVK regression.

## Fix (2026-10-11, waiting for the macOS proof)

`BindlessTextureManager::computeCapacities()` keeps choosing the profile on the update-after-bind budget
(`applyBudget()`), then asks `vkGetDescriptorSetLayoutSupport` about that exact layout (`isLayoutSupported()`: same
bindings, binding flags, layout flags). Refused: the profile is chosen again with `min(budget, maxPerSetDescriptors)`
and checked again; still refused: explicit failure. On the M6 (1212) this gives the reduced profile, 1180
descriptors. Linux NVIDIA unchanged (desired profile; `maxPerSetDescriptors` = 4294967295, the fallback path forced by
fault injection runs and keeps it). To close: the macOS run with the layer shows the reduced profile and 0 VUID.

## What remains

- Size the layout so `vkGetDescriptorSetLayoutSupport` (with `VkDescriptorSetVariableDescriptorCountLayoutSupport` for
  the variable-count binding) says supported: shrink the per-type arrays within the reported limits (bounded loop), and
  trace the final budget. Keep the M2 working (same MoltenVK family).
- Re-run on macOS with `--set-vk-layers=VK_LAYER_KHRONOS_validation`: 0 VUID; check the large scenes still find their
  textures (sponza, citadel) with the smaller 2D array.
