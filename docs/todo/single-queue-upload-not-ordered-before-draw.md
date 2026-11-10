---
id: single-queue-upload-not-ordered-before-draw
title: On a device with one queue, a buffer upload is not ordered before the draw that reads it (SYNC-HAZARD, Intel iGPU)
status: in-progress
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

## State (2026-11-11): fixed on Linux, awaiting macOS and Windows

Fixed by the upload ordering lot (engine `docs/subsystems/vulkan/25-uploads-ordered-before-their-readers.md`): every
upload publishes its completion point (`Device::publishUploadPoint()`) and every reader batch of the renderer and the
AS builds waits for `Device::uploadFrontier()` on the GPU. Measured on Linux, Intel RPL-S (one queue),
`sponza --demo-options 1`, window-less, Synchronization Validation on: before, 3/3 runs `SYNC-HAZARD-READ-AFTER-WRITE`
(a 10 016 880-byte index buffer); after, 4/4 runs 0 hazard, 0 VUID, 0 `[Error]`.

## What remains

- macOS-PA (one MoltenVK queue, where it was first seen): `animation-debug` and `sponza --demo-options 1` with
  Synchronization Validation, several runs: 0 `SYNC-HAZARD`.
- Windows-PA, Intel iGPU (`ForceGPU`), `sky-lab`, `reflexion-debug`, `labyrinth`: 11 runs, 0 `SYNC-HAZARD` (it was 3 in 11).
- Then delete this item (its knowledge is in vulkan doc 25 and `docs/caution-points.md`).

## References

- `src/Vulkan/TransferManager.*`, `src/Vulkan/DeviceQueueConfiguration.cpp`; graphics doc 42 (where it was met).
