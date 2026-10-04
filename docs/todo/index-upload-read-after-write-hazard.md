---
id: index-upload-read-after-write-hazard
title: Index buffers are drawn after their upload copy without a transfer → index-input dependency (SYNC-HAZARD-READ-AFTER-WRITE)
status: open
priority: unranked
scope: Vulkan/TransferManager, geometry upload, shadow maps
opened: 2026-10-04
tags: [vulkan, synchronisation, upload]
---

# Index buffers drawn after their upload copy without a dependency

## Why

macOS-PA, Sponza on the M2 with the synchronisation validation (`VK_KHRONOS_VALIDATION_VALIDATE_SYNC=true`,
2026-10-04, engine 8c461d12): 3 × `SYNC-HAZARD-READ-AFTER-WRITE` — "vkCmdDrawIndexed … reads VkBuffer …, which was
previously written by vkCmdCopyBuffer (another command buffer, same VkQueue) … INDEX_READ at INDEX_INPUT vs
TRANSFER_WRITE at COPY", the draw in the render target `AnimatedSunShadowMapSampler_…`; two during the load, one in
daylight, all BEFORE any deferred-resolve toggle — it predates the resolve. The M2 has ONE queue for everything: the
copy and the draw are ordered by submission, but nothing makes the transfer write available to the index fetch.
With the sync layer on, the layer fails those submits (`VK_ERROR_VALIDATION_FAILED_EXT`, "Unable to submit command
buffer").

## What remains

- Reproduce on Linux with the sync validation on (sponza, `--demo-options 1,…` for the animated sun's shadow map).
- Find the upload path of the index buffers and give it its dependency: a buffer memory barrier TRANSFER_WRITE →
  INDEX_READ (and VERTEX_ATTRIBUTE_READ for the vertices) at the release, or a semaphore when the copy is on another
  queue; check the vertex buffers and the shadow-pass path alike.
