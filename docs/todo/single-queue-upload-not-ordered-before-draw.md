---
id: single-queue-upload-not-ordered-before-draw
title: On a device with one queue, a buffer upload is not ordered before the draw that reads it (SYNC-HAZARD, Intel iGPU)
status: open
priority: high
scope: Vulkan/TransferManager, the geometry upload path, Graphics/Renderer (queue submission)
opened: 2026-10-25
tags: [vulkan, synchronization, transfer, intel, defect]
---

# On a device with one queue, a buffer upload is not ordered before the draw that reads it

## Why

Windows-PA, Intel Raptor Lake iGPU (one queue family, graphics #0 × 1 queue, no transfer-only family), sky-lab with
synchronization validation (`VK_KHRONOS_VALIDATION_VALIDATE_SYNC=true`), 2026-10-25 — 1 run in 5, at load, right after
the MetalBarrel model loaded:

- `SYNC-HAZARD-READ-AFTER-WRITE`: `vkCmdDrawIndexed` reads an index `VkBuffer` (offset 0, size 144: the 36 indices of a
  cube) previously written by `vkCmdCopyBuffer` on the SAME `VkQueue` — INDEX_READ / INDEX_INPUT against
  TRANSFER_WRITE / COPY, with nothing ordering them;
- then `[Error][VulkanQueue] Unable to submit work into the queue : VK_ERROR_VALIDATION_FAILED_EXT`: the frame was
  dropped under the layer.

The NVIDIA devices (a graphics family × 16 + a transfer family × 2) upload on the transfer queue and wait on its
semaphore: never seen there (0 in 6 runs). On a single-queue device the uploads share the graphics queue, and the copy
needs a barrier (TRANSFER_WRITE → INDEX_READ / VERTEX_ATTRIBUTE_READ / UNIFORM_READ) before the first read — or a
semaphore between the two submissions — which the single-queue path does not record. Probably as old as the transfer
path: synchronization validation had never run on an Intel device before. A latent wrong result on any single-queue
device (Intel, some mobile and Apple configurations through MoltenVK).

Seen again (Windows-PA, the same iGPU, 2026-10-25, the GI anti-lag A/B runs): 3 hazards in 11 runs, all the same
`SYNC-HAZARD-READ-AFTER-WRITE`. `vkCmdDrawIndexed` read an index buffer (offset 0, size **104 772**) written by a
`vkCmdCopyBuffer` on the same queue: two in `reflexion-debug`, one in `labyrinth`. A model's buffer, not only the
144-byte cube: any upload can race its first draw.

## What remains

- [ ] Reproduce on Linux by forcing the single-queue path (a device-queue configuration with the transfer queue off),
      under synchronization validation.
- [ ] Record the buffer barrier (or the semaphore) on the single-queue path; prove 0 SYNC-HAZARD on the iGPU (several
      runs: it is intermittent).

## References

- `src/Vulkan/TransferManager.*`, `src/Vulkan/DeviceQueueConfiguration.cpp`; graphics doc 42 (where it was met).
