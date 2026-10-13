---
id: vulkan-sync2-dynamic-rendering
title: Implement VK_KHR_synchronization2 and VK_KHR_dynamic_rendering
status: open
priority: low
scope: Vulkan
opened: unknown
tags: [vulkan, performance]
---

# Implement VK_KHR_synchronization2 and VK_KHR_dynamic_rendering

## Why

Vulkan 1.3 core features. Beyond the modernisation, dynamic rendering is the lever for ordering
draws by pipeline layout and cutting binding cost.

## State (2026-10-13)

- `synchronization2` is enabled ONLY when the H.265 video-encode extensions are present
  (`src/Vulkan/Instance.cpp:1265`) and used only by the encoder's layout transitions
  (`src/Vulkan/VideoEncoderH265.cpp:764`, `vkCmdPipelineBarrier2`); every other barrier is sync1.
- Dynamic rendering is absent (the ImGui overlay sets `UseDynamicRendering = false`,
  `src/Overlay/Manager.cpp:1023`).

## What remains

Owner decision (2026-10-13): do it only if it improves performance; otherwise the item waits for a later decision.

- [ ] **Measure first.** Estimate, then measure, the gain before any migration: the current binding cost (descriptor
  set / pipeline binds per frame on a heavy scene) and what ordering the draws by pipeline layout would save. Adopt
  only on a measured gain, by the Ave Performus rule: ≥ 5 % and outside the noise, median of ≥ 5 runs, recorded
  either way (projet-alpha `docs/plans/ave-performus.md`). No gain → record the numbers and stop here.
- [ ] Implement `VK_KHR_synchronization2` everywhere (enabled unconditionally, the barriers migrated).
- [ ] Implement `VK_KHR_dynamic_rendering`.
- [ ] Then leverage dynamic rendering to order draws by pipeline layout.

## Origin

Inherited from the historical root `TODO.md`.
