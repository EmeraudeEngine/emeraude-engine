---
id: overlay-surface-reupload-races-on-two-queues
title: A re-upload of an overlay surface races the previous upload under synchronization validation
status: open
priority: unranked
scope: Vulkan/ImageTransferOperation, Vulkan/TransferManager, Overlay/Surface
opened: 2026-09-26
tags: [vulkan, synchronization, validation, overlay, queues]
---

# A re-upload of an overlay surface races the previous upload under synchronization validation

## Why

Seen once, at startup, in the synchronization-validation run of the overflow census (engine
`a42a7410` plus the C1 working tree, Linux, `light-and-shadow-debug --demo-options 0,1`, launched with
`VK_LAYER_ENABLES=VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT`):

```
SYNC-HAZARD-WRITE-RACING-WRITE: vkQueueSubmit(): vkCmdPipelineBarrier (from VkCommandBuffer 0x55bc84bddc10
submitted on the current VkQueue 0x55bc8134b730) writes to VkImage 0xe200000000e2, which was previously written
by vkCmdCopyBufferToImage (from VkCommandBuffer 0x55bc84bddc10 submitted on VkQueue 0x55bc8250f4c0).
No sufficient synchronization is present to ensure that a layout transition does not conflict with a prior
write (VK_ACCESS_2_TRANSFER_WRITE_BIT) at VK_PIPELINE_STAGE_2_COPY_BIT.
```

The layer then rejects the submit (`VK_ERROR_VALIDATION_FAILED_EXT`), and the chain of errors names the
victim: `Unable to transfer an image (1/2)` → `Unable to update the content of surface 'Notifier'` →
the `NotifierScreen` is disabled for the session. Without synchronization validation nothing is reported
and the notifier works.

What the message says:
- The **same transfer command buffer** recorded two uploads into the **same image**, and the two submissions
  went to **two different queues**. `ImageTransferOperation::transferToGPU()` (step 1/2) takes its queue from
  `getGraphicsTransferQueue()`, which round-robins over the family (`docs/caution-points.md` § the
  round-robined transfer queues).
- The second upload's first barrier (`UNDEFINED → TRANSFER_DST`, source `TOP_OF_PIPE` / `NONE`) is a layout
  transition, hence a write, and nothing the layer can see orders it after the first upload's copy.

## What remains

- Decide first whether this is a **real race or a tracking gap of the layer**. 0 VUID in the same run means
  the command buffer was not pending when re-recorded, so the host knew the previous operation had finished
  through some wait. Find which one (`TransferManager` / `Overlay::Surface::uploadActiveBuffer()` →
  `Image::writeData()`), and whether it covers the step-1 submission on the other queue, or only step 2's
  fence on the graphics queue.
- If real: order each upload of an image after the previous one (wait the operation's own fence before
  reuse, or pin one image's uploads to one queue), in line with the rule of the round-robin caution:
  "never assume a `waitIdle()` on a round-robined queue covers a previously submitted operation".
- Re-run the same launch with synchronization validation: no `SYNC-HAZARD` from `ImageTransferOperation`,
  and the notifier still draws.

## ⚠️ Traps

- The layer **rejects** the offending submit: under synchronization validation the notifier looks broken.
  That is the measurement, not a second defect.
- Its message IDs are `SYNC-HAZARD-*`, not `VUID-*`: a `grep VUID-` reports a clean log.

## References

- `src/Vulkan/ImageTransferOperation.cpp` (`transferToGPU()`, steps 1/2 and 2/2, about lines 120-360).
- `docs/caution-points.md` § *The Synchronization Validation layer is NOT enabled* (how to enable it) and
  the round-robined transfer queues fix (`Device::waitTransferQueuesIdle()`).
