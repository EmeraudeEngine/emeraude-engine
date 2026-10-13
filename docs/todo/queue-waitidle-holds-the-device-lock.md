---
id: queue-waitidle-holds-the-device-lock
title: Queue::waitIdle() holds the device-wide mutex for the whole GPU wait, blocking every submit and present
status: open
priority: high
scope: Vulkan/Queue, Vulkan/Device, Graphics/VideoFrameConverter, Vulkan/VideoEncoderH265, Graphics/Compute
opened: 2026-09-25
tags: [vulkan, threading, stall, video]
---

# Queue::waitIdle() holds the device-wide mutex for the whole GPU wait, blocking every submit and present

## Why

`Queue::submit()`, `Queue::present()` and `Queue::waitIdle()` (`src/Vulkan/Queue.cpp`: submits l.59, 146, 216,
present l.243, wait l.298) all take `m_device.lockAccess()` (a guard since engine `09615d88`, which replaced the
former `std::lock_guard< Device >`), i.e. `Device::m_logicalDeviceAccess`, ONE `std::mutex` for the whole device.
`waitIdle()` keeps it for the whole `vkQueueWaitIdle()` (l.306): while any thread waits for its queue to drain, no
other thread can submit or present on ANY queue — the rendering thread included.

Vulkan only requires the host to synchronise access to the SAME `VkQueue`: `vkQueueWaitIdle()` on queue A
does not have to exclude a `vkQueueSubmit()` on queue B.

Runtime callers that wait this way (read by grep, lines checked 2026-10-13, not measured):

- `VideoFrameConverter` (`src/Graphics/VideoFrameConverter.cpp:590`, `:665`): every RushMaker frame, on the encoding thread;
- `VideoEncoderH265` (`src/Vulkan/VideoEncoderH265.cpp:661` the plane copies, `:948` / `:991` the video queue): every RushMaker frame;
- `IBLBaker` (`src/Graphics/Compute/IBLBaker.cpp:593`, `:916`, `:1020`): every bake;
- `XRayAnalyzer` (`src/Graphics/Compute/XRayAnalyzer.cpp:830`, `:966`).

`Device::getGraphicsQueue()` ROTATES over the family's queues (16 on NVIDIA,
`DeviceQueueConfiguration::queue()`), so such a caller also lands on the renderer's own queue from time to
time and then waits for a whole in-flight frame, lock held.

## What remains

- Measure first: the render thread's time blocked in `Queue::submit()` / `present()` while RushMaker records
  (hardware path) and during an IBL bake. The owner saw no stutter on screen during a recording on
  2026-09-25, so the cost may be small — or hidden by the frame pacing.
- If it is real: lock per `VkQueue` (a mutex in `Queue`) for submit/present/waitIdle, and keep the device
  lock only for what truly needs the device-wide exclusion. Check every `Device::lockAccess()` user
  first — some may rely on the device-wide exclusion for something else.
- Or remove the waits from the per-frame paths (the converter and the encoder could wait on their own
  fence instead of the whole queue).

## ⚠️ Traps

- `vkQueueWaitIdle()` on a rotating queue waits for whatever ANOTHER subsystem submitted there, not only for
  the caller's own work.
- Found while fixing the RushMaker stepped-back frames (`docs/caution-points.md` § *RushMaker stepped back
  3-4 frames*); it is NOT their cause.
