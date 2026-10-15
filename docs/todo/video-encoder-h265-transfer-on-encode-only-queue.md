---
id: video-encoder-h265-transfer-on-encode-only-queue
title: The H.265 hardware encoder records copies and transfer barriers on an encode-only queue (AMD: 55 VUIDs per rush)
status: in-progress
priority: high
scope: Vulkan/VideoEncoderH265
opened: 2026-10-01
tags: [vulkan, video, amd, rushmaker, validation]
---

# The H.265 hardware encoder records copies and transfer barriers on an encode-only queue (AMD: 55 VUIDs per rush)

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

`src/Vulkan/VideoEncoderH265.cpp:670-710` uploads the NV12 planes "ON THE VIDEO QUEUE" (comment `:681`): a
`vkCmdPipelineBarrier` (`:695`), two `vkCmdCopyBufferToImage` (`:704`, `:710`) recorded in a command buffer from
`m_videoCommandPool` (created on the encode family, `:151`; the command buffer at `:670`). NVIDIA's encode family (#4)
apparently also exposes TRANSFER, which is why it passes there (181 pictures, 0 VUID). AMD's family #3 does not.

## Done (2026-10-15, engine b53c63b5)

- Owner decision 2026-10-15, by family: `VideoEncoderH265::create()` checks the encode family's flags; when it cannot
  copy (no TRANSFER / COMPUTE / GRAPHICS), `recordPlaneUploadWithRelease()` uploads the NV12 planes on the graphics
  queue (in the plane-readback command buffer) and RELEASES the picture to the encode family; the encode queue only
  ACQUIRES it (same layouts TRANSFER_DST -> VIDEO_ENCODE_SRC, no source stage). NVIDIA keeps its path.
- The copy runs on a family with a (1,1,1) granularity, so the record extent (1280x720) is valid there (07738).
- Reproduced on Linux 2026-10-15 (LinuxB-PA, Radeon RENOIR, RADV Mesa 26.0.8, encode family #3 encode-only): 21 VUID
  messages (06462, cmdpool, 09675) and a SIGSEGV inside RADV at the 6th frame, 0 picture written — a crash, not only
  VUIDs. NVIDIA after the fix: a 6 s rush, 181 pictures, decodes, 0 VUID.

## AMD stream corruption (2026-10-15, LinuxB-PA, RADV RENOIR / Mesa 26.0.8)

After the upload fix (no crash, 0 VUID) the AMD stream is UNDECODABLE: well-formed VPS / SPS / PPS (RADV overrides
SPS and PPS: CTB 64, cu_qp_delta on, dependent slices on) and well-formed slice headers (RADV builds them itself), but
the slice DATA desyncs within the first 1-3 CTB rows (libav: invalid cu_qp_delta, CABAC_MAX_BIN), in every variant
tried: VBR; rate control off + constant QP 24 (half the errors); a 64-aligned picture extent; explicit slice SAO flags;
SAO off in the SPS (honoured). NVIDIA decodes clean on the same machine and build. Engine 7cbefbbc derives the CTB,
transform-block sizes and SAO from `VkVideoEncodeH265CapabilitiesKHR` (correct per the specification; no effect on AMD,
whose driver already overrode them). Owner decision 2026-10-15: `Recorder::hardwareEncodeWithheld()` withholds the
hardware H.265 encode on AMD (vendor 0x1002) — the software VP9 path makes the rushes — until the cause is settled.

## The reference encodes point at our SPS (2026-10-15, LinuxB-PA)

GStreamer 1.28.2 has no `vulkanh265enc` (only `vulkanh264enc`). Instead: `vah265enc` (VA-API HEVC on the SAME VCN2
through Mesa radeonsi) and `vulkanh264enc` on RADV, 180 frames 1280x720 NV12 each, decoded by `avdec`: ALL clean (0
error, 180/180) — default CBR, VBR, all-intra VBR (zone-plate, snow 12 MB like our broken streams), CQP 24, and the
Vulkan H.264 encode on RADV. Our AMD stream cross-decoded by VA-API (`vah265dec`) and the NVIDIA Vulkan decoder gives
the same garbage as libav: the stream itself is invalid.
Parameters, the clean VA stream against ours: `max_transform_hierarchy_depth_inter / intra` 4 / 4 against **0 / 0**
(never set: `m_stdSPS = {}`); the rest is identical or harmless (amp 1/0, loop filter across slices 1/0, POC lsb
4/8). A depth-0 SPS declares no `split_transform_flag` below the implicit split; VCN still splits its transform trees
and writes that syntax → CABAC desync after the first CTBs, our exact symptom. NVENC honours depth 0, hence clean.
Fix (this item): both depths = `CtbLog2SizeY - 2`, the maximum H.265 allows (7.4.3.2.1), as radeonsi / vah265enc
write it. NVIDIA after it (Linux, RTX 3070 Ti, CTB 32 → depth 3, read back with `trace_headers`): 181 pictures,
0 decode error, 0 VUID, picture 100 clean.

## What remains

- [ ] AMD: a rush with the SPS fix and `hardwareEncodeWithheld()` bypassed LOCALLY (test-only, never committed),
      decoded with libav / VA-API: clean → lift the withholding; still corrupt → a RADV/VCN2 report upstream.

## ⚠️ Traps

- The stream is ALL-INTRA on purpose (`idrPeriod = 1`, `Recorder.cpp`: a mezzanine layout with frame-exact seeking,
  and it avoids an NVIDIA P-frame DPB issue). "IDR every 1 frames" in the log is not a bug.
- AMD reports the padded coded extent: every size derived from the record extent must be checked against it.

## References

- `src/Vulkan/VideoEncoderH265.cpp`: the NV12 upload and the encode submit.
- `docs/subsystems/graphics/11-10-video-recording.md` § Hardware encode chantier.
