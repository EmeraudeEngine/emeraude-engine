---
id: failed-frame-submit-crashes
title: A rejected frame submission is followed by a SIGSEGV
status: open
priority: high
scope: Graphics/Renderer (frame submission failure path), Vulkan/Queue
opened: 2026-11-11
tags: [vulkan, crash, failure-path, defect]
---

# A rejected frame submission is followed by a SIGSEGV

## Why

Linux, Intel RPL-S (`ForceGPU`), `sponza --demo-options 1 --window-less`, Synchronization Validation on, engine
21081ebe WITHOUT the upload ordering lot (vulkan doc 25), 3 runs out of 3: the layer rejects a submit
(`SYNC-HAZARD-READ-AFTER-WRITE` → `[Error][VulkanQueue] Unable to submit work into the queue :
VK_ERROR_VALIDATION_FAILED_EXT !`, `Queue.cpp:173`), and the process dies right after (exit 139, the log ends on that
line). The hazard itself is fixed; the crash on a failed submit is not. A failed `vkQueueSubmit()` is a legal outcome
(`VK_ERROR_OUT_OF_HOST_MEMORY`, `VK_ERROR_OUT_OF_DEVICE_MEMORY`, `VK_ERROR_DEVICE_LOST`, a layer rejection): every
failure path must exit cleanly (Ave Robustus II).

## What remains

- Reproduce with a backtrace: the old code (`git apply -R` of the upload ordering lot in a scratch tree) on the Intel
  iGPU as above, under gdb; or a fault-injection switch that fails the frame submission (`src/FaultInjection.*`), which
  also makes it a regression test of the runner (`tools/stress/StartupFaultInjection.cpp`).
- Find which submission failed (the frame, a shadow map, a render-to-texture) and what its failure path leaves behind
  (a fence never signalled then waited, a binary semaphore signalled but never waited, a frame-capture or recorder
  state). Fix it so the frame is dropped cleanly and the next one renders, or the engine stops with a clear message.

## References

- `src/Graphics/Renderer.cpp` (`renderFrame()`, `renderShadowMaps()`, `renderRenderToTextures()`,
  `discardAcquiredImage()`), `src/Vulkan/Queue.cpp` (`submit()`).
- Engine `docs/subsystems/vulkan/25-uploads-ordered-before-their-readers.md` (the hazard that triggered it).
