---
id: video-encoder-h265-transfer-on-encode-only-queue
title: The H.265 hardware encoder records copies and transfer barriers on an encode-only queue (AMD: 55 VUIDs per rush)
status: open
priority: unranked
scope: Vulkan/VideoEncoderH265
opened: 2026-10-01
tags: [vulkan, video, amd, rushmaker, validation]
---

# The H.265 hardware encoder records copies and transfer barriers on an encode-only queue

## Why

Found by the Windows peer on 2026-10-01 while validating triad 7d (engine `9cc3659d`). The cause predates the triad:
`VideoEncoderH265.cpp` was last changed in `d1186709` / `888d100c`.

A 6 s hardware rush on the AMD Radeon iGPU produces the file (180 pictures), but with **55 VUIDs**. Each VUID is
capped at 10 by the layer's duplicate limit, so the errors are probably raised on every frame:

- `VUID-vkCmdPipelineBarrier-dstStageMask-06462`: ALL_TRANSFER is not compatible with `VK_QUEUE_VIDEO_ENCODE_BIT_KHR`.
- `VUID-vkCmdCopyBufferToImage-commandBuffer-cmdpool`: the pool's family 3 is encode-only, and the copy requires
  GRAPHICS, COMPUTE or TRANSFER.
- `VUID-vkCmdCopyBufferToImage-imageOffset-07738`: AMD pads the coded picture to 1280x768, so a 1280x720 copy on a
  queue with a `minImageTransferGranularity` of (0,0,0) is refused.
- `VUID-vkCmdDraw-None-09600`: PLANE_0 is used as TRANSFER_DST_OPTIMAL while its current layout is UNDEFINED.
- `VUID-vkCmdPipelineBarrier2-srcStageMask-09675`: COPY is not compatible with the video-encode family.
- `VUID-vkCmdEncodeVideoKHR-pEncodeInfo-10811`: the source picture is in TRANSFER_DST_OPTIMAL, not
  VIDEO_ENCODE_SRC_KHR.

`VideoEncoderH265.cpp` (~ lines 678-720) uploads the NV12 planes "ON THE VIDEO QUEUE": a `vkCmdCopyBufferToImage` and
TRANSFER/COPY barriers recorded in a command buffer from `m_videoCommandPool`. NVIDIA's encode family (#4)
apparently also exposes TRANSFER, which is why it passes there (181 pictures, 0 VUID). AMD's family #3 does not.

## What remains

- [ ] Choose the upload path (an owner decision):
  - record the plane upload on a TRANSFER-, COMPUTE- or GRAPHICS-capable family, and hand the picture to the encode
    family with a queue-family ownership transfer (release + acquire, ordered by a semaphore);
  - or check the encode family's flags at creation, keep the current path when it has TRANSFER, and use the other
    path otherwise.
- [ ] Copy the padded coded extent (1280x768 here) or respect the granularity, not the record extent.
- [ ] Re-validate on the AMD iGPU (Windows peer, forced) and on NVIDIA: 0 VUID, the file decodes.

## ⚠️ Traps

- The stream is ALL-INTRA on purpose (`idrPeriod = 1`, `Recorder.cpp`: a mezzanine layout with frame-exact seeking,
  and it avoids an NVIDIA P-frame DPB issue). "IDR every 1 frames" in the log is not a bug.
- AMD reports the padded coded extent: every size derived from the record extent must be checked against it.

## References

- `src/Vulkan/VideoEncoderH265.cpp`: the NV12 upload and the encode submit.
- `docs/subsystems/graphics/11-10-video-recording.md` § Hardware encode chantier.
