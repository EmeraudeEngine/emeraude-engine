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


**A second path, measured 2026-10-01** (macOS M2, engine `734de41d`, triad 13 peer validation): `Core.WindowService.resize(17000,
1200)` gives a 34000×1870 framebuffer. The overlay refuses its surfaces past 16384 px (triad 13), and the scene target
is clamped to 16384×1870 RGBA16F. But the post-process targets at that size exhaust the GPU memory:
`VK_ERROR_OUT_OF_DEVICE_MEMORY` (MoltenVK "Insufficient Memory"), then `VK_ERROR_DEVICE_LOST` on a fence wait. The
next resize cascades on the lost device:
- every post-process target fails;
- 9 × `VUID-vkResetFences-pFences-01123` (resetting a fence still pending on the lost device);
- then a **SIGSEGV** in `vkQueueSubmit` on the render thread, from
  `Overlay::Manager::updateVideoMemory → UIScreen::processSurfaceUpdates → Surface::uploadActiveBuffer →
  Image::writeData → ImageTransferOperation::transferToGPU → Queue::submit`.

The overlay upload, like the rest, keeps submitting after the device is lost. The unbounded `Window.resize` console
command is the trigger: triad section 15 (`Window.cpp`).

## What remains

- On `VK_ERROR_DEVICE_LOST` from any submit or wait: mark the renderer lost, report once with the dump, stop
  recording, and request a clean shutdown (the user application may save its data) instead of resetting fences
  and waiting on them.
- Keep the 60 s timeout for a frame that is merely slow, but say which command buffers were in flight.
