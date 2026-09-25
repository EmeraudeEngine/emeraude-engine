---
id: renderer-fail-fast-on-device-loss
title: A lost device waits 60 s on a fence and aborts — fail fast, dump, shut down cleanly
status: open
priority: unranked
scope: Graphics/Renderer (beginFrame, renderFrame, discardAcquiredImage), Vulkan/Queue
opened: 2026-09-25
tags: [vulkan, device-lost, robustness, diagnostics]
---

# A lost device waits 60 s on a fence and aborts — fail fast, dump, shut down cleanly

## Why

By reading (2026-09-25, the terrain hang diagnosis, confirmed by an adversarial check): a failed submit logs
`Unable to submit work into the queue` and dumps the device-lost diagnostics (`Queue.cpp` ~103-110), but the
renderer then resets the in-flight fence, submits a drain carrying it (`discardAcquiredImage(..., true)`,
`Renderer.cpp` ~1945, ~2723-2731) — which fails too on a lost device — and the next `beginFrame()` waits that fence
for `m_timeout` = 60 s (`Renderer.hpp` ~1776) before `std::abort()` (`Renderer.cpp` ~1547, ~1564-1568). On the AMD
iGPU the process survived the loss and kept failing every submit.
⚠️ Not every 60 s timeout is a loss: by the spec, `vkWaitForFences` on a lost device returns in finite time, and the
NVIDIA-Windows `VK_TIMEOUT` of the same day meant a frame still executing (or paging), not a killed GPU.

## What remains

- On `VK_ERROR_DEVICE_LOST` from any submit or wait: mark the renderer lost, report once with the dump, stop
  recording, and request a clean shutdown (the user application may save its data) instead of resetting fences
  and waiting on them.
- Keep the 60 s timeout for a frame that is merely slow, but say which command buffers were in flight.
