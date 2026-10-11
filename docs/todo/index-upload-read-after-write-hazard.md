---
id: index-upload-read-after-write-hazard
title: Index buffers are drawn after their upload copy without a transfer → index-input dependency (SYNC-HAZARD-READ-AFTER-WRITE)
status: open
priority: high
scope: Vulkan/BufferTransferOperation, Vulkan/TransferManager, geometry upload, shadow maps
opened: 2026-09-03
tags: [vulkan, synchronisation, upload, measured]
---

# Index buffers drawn after their upload copy without a dependency

## Why

Seen twice on macOS with Synchronization Validation on. Each time the layer rejects the submit
(`VK_ERROR_VALIDATION_FAILED_EXT`, "Unable to submit command buffer"), so a frame is dropped under validation. Without
the layer the hazard is real on every platform: the first draw of a freshly uploaded index buffer may read before the
transfer lands.

- **2026-09-03, `animation-debug`** (`VK_LAYER_ENABLES=VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT`):
  one `SYNC-HAZARD-READ-AFTER-WRITE` at load time, one-shot and not recurring. "vkCmdDrawIndexed reads VkBuffer, which
  was previously written by vkCmdCopyBuffer (another command buffer, same queue) … INDEX_READ at INDEX_INPUT vs
  TRANSFER_WRITE at COPY". Found while hunting `moltenvk-skinning-ssbo-shared-block-collapse`, and unrelated to it.
- **2026-10-04, Sponza on the M2** (`VK_KHRONOS_VALIDATION_VALIDATE_SYNC=true`, engine 8c461d12): three of the same.
  The draw was in the render target `AnimatedSunShadowMapSampler_…`. Two came during the load and one in daylight, all
  BEFORE any deferred-resolve toggle, so the defect predates the resolve.

The M2 has ONE queue for everything. The copy and the draw are ordered by submission, but nothing makes the transfer
write available to the index fetch.

**In the code (audit 2026-10-11):** the single-family path of `BufferTransferOperation.cpp` (~129-150) copies and submits
with a fence only, and records no TRANSFER_WRITE → INDEX_READ barrier. Only the two-family path has an acquire barrier
(~196-212).

## What remains

- Reproduce on Linux with Synchronization Validation on: sponza with `--demo-options 1,…` (the animated sun's shadow
  map), and `animation-debug`.
- Give the single-family upload path its dependency:
  - a buffer memory barrier TRANSFER_WRITE → INDEX_READ (and VERTEX_ATTRIBUTE_READ for the vertices) at the release;
  - or a semaphore when the copy is on another queue.
- Check the vertex buffers and the shadow-pass path the same way.
- Re-run under Synchronization Validation: zero `SYNC-HAZARD` on both demos.

## References

`src/Vulkan/BufferTransferOperation.cpp`, `docs/caution-points.md` § Synchronization Validation. Absorbs the former item
`transfer-write-index-read-missing-barrier` (2026-09-03, merged 2026-10-11).
