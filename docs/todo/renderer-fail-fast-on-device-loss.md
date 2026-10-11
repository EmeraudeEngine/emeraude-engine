---
id: renderer-fail-fast-on-device-loss
title: On a genuine 60 s fence timeout, name the command buffers in flight (the device-loss fail-fast is done)
status: open
priority: high
scope: Graphics/Renderer (beginFrame), Vulkan/Queue, Vulkan/Device (dumpDeviceLostDiagnostics)
opened: 2026-09-25
tags: [vulkan, device-lost, robustness, diagnostics]
---

# On a genuine 60 s fence timeout, name the command buffers in flight

## Why

A fence that does not signal within `m_timeout` (60 s) is treated as a device loss: the engine stops with exit code 98.
That stop gives the diagnosis nothing to work with. Only the frame slot is traced, plus `dumpDeviceLostDiagnostics()`,
which on many drivers has no `VK_EXT_device_fault` to report.

⚠️ Not every 60 s timeout is a loss. By the spec, `vkWaitForFences` on a lost device returns in finite time. The
NVIDIA-Windows `VK_TIMEOUT` of 2026-09-25 meant a frame still executing (or paging), not a killed GPU. When the engine
gives up on such a timeout, the log should say what the GPU was busy with.

## Done (2026-10-11)

- The fail-fast this item was opened for is in place: the `Device::isLost()` latch, and every submit / wait / reset
  refused after a loss. `beginFrame()` no longer calls `std::abort()`. `Core::stopOnDeviceLoss()` stops the engine
  once, with exit code 98.
  - Commits: df1b8158, plus the three-OS proof in 6669299f and 995d465f.
  - Contract and measurements: `docs/caution-points.md` § "A lost device stops the engine at once and cleanly".
- The three paths that led here are now covered by that stop:
  - a failed submit followed by a 60 s wait (terrain, 2026-09-25);
  - `VK_ERROR_OUT_OF_DEVICE_MEMORY` then `DEVICE_LOST` after `Window.resize(17000, 1200)` (M2, 2026-10-01). The resize
    itself is bounded by triad section 15;
  - an NVK / nouveau kernel fault on the first shadow-map submit (the owner's laptop, 2026-10-02), which ended in
    exit 134.

## What remains

- When a fence wait times out (60 s), trace the command buffers recorded into the frame that timed out, BEFORE the
  device is treated as lost. Give each one's owner (render target / pass / transfer) and its queue.
- Make the timeout path distinguish "still executing" from "lost" in the log, so a slow frame is never reported as a
  dead GPU.

## References

- `src/Graphics/Renderer.cpp` (`beginFrame()`, ~1676-1688), `src/Vulkan/Queue.cpp`, `src/Vulkan/Device.cpp`.
- `jungle-ruins-fence-timeout-abort.md`: the workload that hit the 60 s timeout.
