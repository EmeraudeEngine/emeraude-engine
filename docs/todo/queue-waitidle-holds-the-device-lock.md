---
id: queue-waitidle-holds-the-device-lock
title: Queue::waitIdle() holds the device-wide mutex for the whole GPU wait, blocking every submit and present
status: open
priority: unranked
scope: Vulkan/Queue, Vulkan/Device, Graphics/VideoFrameConverter, Vulkan/VideoEncoderH265, Graphics/Compute
opened: 2026-09-25
tags: [vulkan, threading, stall, video]
---

# Queue::waitIdle() holds the device-wide mutex for the whole GPU wait, blocking every submit and present

## Why

`Queue::submit()`, `Queue::present()` and `Queue::waitIdle()` (`src/Vulkan/Queue.cpp`) all take
`std::lock_guard< Device >`, i.e. `Device::m_logicalDeviceAccess`, ONE `std::mutex` for the whole device.
`waitIdle()` keeps it for the whole `vkQueueWaitIdle()`: while any thread waits for its queue to drain, no
other thread can submit or present on ANY queue — the rendering thread included.

Vulkan only requires the host to synchronise access to the SAME `VkQueue`: `vkQueueWaitIdle()` on queue A
does not have to exclude a `vkQueueSubmit()` on queue B.

Runtime callers that wait this way (read by grep on 2026-09-25, not measured):

- `VideoFrameConverter` (lines ~587 and ~662): every RushMaker frame, on the encoding thread;
- `VideoEncoderH265` (~658 the plane copies, ~945/988 the video queue): every RushMaker frame;
- `IBLBaker` (~618, ~941, ~1045): every bake;
- `XRayAnalyzer` (~827, ~963).

`Device::getGraphicsQueue()` ROTATES over the family's queues (16 on NVIDIA,
`DeviceQueueConfiguration::queue()`), so such a caller also lands on the renderer's own queue from time to
time and then waits for a whole in-flight frame, lock held.

## What remains

- Measure first: the render thread's time blocked in `Queue::submit()` / `present()` while RushMaker records
  (hardware path) and during an IBL bake. The owner saw no stutter on screen during a recording on
  2026-09-25, so the cost may be small — or hidden by the frame pacing.
- If it is real: lock per `VkQueue` (a mutex in `Queue`) for submit/present/waitIdle, and keep the device
  lock only for what truly needs the device-wide exclusion. Check every `std::lock_guard< Device >` user
  first — some may rely on the device-wide exclusion for something else.
- Or remove the waits from the per-frame paths (the converter and the encoder could wait on their own
  fence instead of the whole queue).

## ⚠️ Traps

- `vkQueueWaitIdle()` on a rotating queue waits for whatever ANOTHER subsystem submitted there, not only for
  the caller's own work.
- Found while fixing the RushMaker stepped-back frames (`docs/caution-points.md` § *RushMaker stepped back
  3-4 frames*); it is NOT their cause.
