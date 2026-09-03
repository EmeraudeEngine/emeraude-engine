---
id: transfer-write-index-read-missing-barrier
title: A geometry upload has no barrier between vkCmdCopyBuffer and the first indexed draw
status: open
priority: medium
scope: Vulkan/BufferTransferOperation, Graphics/Geometry
opened: 2026-09-03
tags: [synchronization, measured]
---

# A geometry upload has no barrier between vkCmdCopyBuffer and the first indexed draw

## Why

Synchronization Validation (`VK_LAYER_ENABLES=VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT`)
on `animation-debug` (macOS, 2026-09-03) reports once, at load time:

```
SYNC-HAZARD-READ-AFTER-WRITE: vkQueueSubmit(): vkCmdDrawIndexed reads VkBuffer, which was
previously written by vkCmdCopyBuffer (another command buffer, same queue).
No sufficient synchronization is present to ensure that a read (VK_ACCESS_2_INDEX_READ_BIT) at
VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT does not conflict with a prior write
(VK_ACCESS_2_TRANSFER_WRITE_BIT) at VK_PIPELINE_STAGE_2_COPY_BIT.
```

The layer rejects that submit (`VK_ERROR_VALIDATION_FAILED_EXT`), so one frame is dropped under
validation. Without the layer the hazard is real on every platform: the first draw of a freshly
uploaded index buffer may read before the transfer lands. Found while hunting
`moltenvk-skinning-ssbo-shared-block-collapse` (unrelated to it — one-shot, not recurring).

## What remains

- [ ] Identify the upload path (staged transfer of an IBO — `Vulkan::BufferTransferOperation` /
  the transfer manager) and add the `TRANSFER_WRITE → INDEX_READ` barrier (or a semaphore between
  the transfer submit and the first graphics submit).
- [ ] Re-run under Synchronization Validation: zero `SYNC-HAZARD` on `animation-debug`.

## References

`src/Vulkan/BufferTransferOperation.cpp`, `docs/caution-points.md` § Synchronization Validation.
