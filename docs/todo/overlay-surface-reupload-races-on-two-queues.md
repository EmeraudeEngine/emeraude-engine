---
id: overlay-surface-reupload-races-on-two-queues
title: A re-upload of an overlay surface races the previous upload under synchronization validation
status: open
priority: high
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

Not reproduced on the peers the same day: macOS exposes a single graphics queue (MoltenVK), so no
two-queue race can exist there; Windows (RTX 3060 Laptop, SDK 1.4.357) reported no hazard at all.

What the message says:
- The **same transfer command buffer** recorded two uploads into the **same image**, and the two submissions
  went to **two different queues**. `ImageTransferOperation::transferToGPU()` (step 1/2) takes its queue from
  `Device::getPrimaryTransferQueue()` (`src/Vulkan/Device.hpp:797-799`, since engine `4ee503a2`), which still
  forwards to `getGraphicsTransferQueue()` on a graphics device and round-robins over the family
  (`docs/caution-points.md` § the round-robined transfer queues).
- The second upload's first barrier (`UNDEFINED → TRANSFER_DST`, source `TOP_OF_PIPE` / `NONE`) is a layout
  transition, hence a write, and nothing the layer can see orders it after the first upload's copy.

## State (2026-10-13)

- Since engine `b15b1346`, a PARTIAL update (`ImageTransferOperation::transferRegion()`, `Image::writeDataRegion()`,
  the dirty-rectangle bands of the staging overlay upload) runs entirely on the primary (graphics) family in one
  command buffer (`docs/caution-points.md` § A partial image update ran on the transfer-only family). The scope left
  is the FULL upload: `Image::writeData()` (`src/Overlay/Surface.cpp:335`) → `transferToGPU()` +
  `finalizeForGPU()`.
- `transferToGPU()` is `src/Vulkan/ImageTransferOperation.cpp:123-228` (submit l.209), `finalizeForGPU()` l.231-379
  (submit l.360). Both record `Image::recordPendingSubmission()`, but the pending submissions are consumed only at
  destruction (`Image::destroyAfterPendingSubmissions()`, `src/Vulkan/Image.cpp:543`), never before a re-upload.

## State (2026-11-03): a real race, measured on screen

- The race was seen without any validation layer, on a downstream application (Windows): a row band covering every
  row went through the FULL upload (transfer queue) right after a region upload (graphics queue) of the previous
  frame; the full one landed first, the older region overwrote it, and its rows stayed stale on screen until the next
  paint touched them (a translucent band of a faded-out modal backdrop, cut at the bottom of the next small repaint).
  One trace per paint and per upload gave the sequence `partial 101-1009` (old) → `full 0-1009` (new) →
  `partial 32-166` (new).
- Fix: `Overlay::Surface::uploadActiveBuffer()` pins every upload of a resident image (`SHADER_READ_ONLY_OPTIMAL`)
  to the region path, whole image included. `Image::writeData()` now only uploads a fresh image, which has no earlier
  upload to race. `docs/caution-points.md` § Vulkan Validation.

## What remains

- Re-run the same launch with synchronization validation: no `SYNC-HAZARD` from `ImageTransferOperation`,
  and the notifier still draws. The notifier's startup hazard above was two uploads of the same image on two
  queues; the second one now goes through the region path, so the hazard should be gone — to be measured.
- The general case (any `Image` re-uploaded through `writeData()` twice, outside the overlay) is untouched:
  `recordPendingSubmission()` still only defers destruction.

## ⚠️ Traps

- The layer **rejects** the offending submit: under synchronization validation the notifier looks broken.
  That is the measurement, not a second defect.
- Its message IDs are `SYNC-HAZARD-*`, not `VUID-*`: a `grep VUID-` reports a clean log.

## References

- `src/Vulkan/ImageTransferOperation.cpp` (`transferToGPU()` l.123-228, `finalizeForGPU()` l.231-379).
- `docs/caution-points.md` § *The Synchronization Validation layer is NOT enabled* (how to enable it) and
  the round-robined transfer queues fix (`Device::waitTransferQueuesIdle()`).
