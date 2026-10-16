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

## After the SPS fix: still non-conformant on AMD, the reference decoder says so (2026-10-15, LinuxB-PA)

AMD rushes with the depth fix (engine 7bd7cebf, then 283602aa; the withholding bypassed locally, test-only):
- libav decodes 177/177 pictures with 0 error and plausible images.
- The hardware decoders stop at the same CU-row band of every picture (flat-green concealment): VA-API `vah265dec` on
  the same VCN, and the NVIDIA Vulkan decoder.
- Per CTB, libav and the hardware already differ slightly from CTB 0 (row means 0.1-0.4/765). On the radeonsi and NVIDIA
  streams the same decoders agree bit-exactly.
- One VCL NAL per picture (IDR_N_LP, no dependent slice segment, no entry points).
- Every slice NAL ends with exactly 193 zero bytes, whatever its size (23-79 KB).

**The HEVC reference decoder (HM, master 9c1f298) is the arbiter:**
- It decodes our NVIDIA stream and the radeonsi stream.
- It aborts on picture 0 of EVERY AMD stream: CABAC reads past the end of the slice data
  (`TComInputBitstream::readByte: m_fifo_idx < m_fifo.size()`); before the depth fix, the slice did not end on its
  trailing bits.
- The trace build runs out of data inside CU 119, where `vah265dec` conceals from CTB 100.
- So the stream is non-conformant: the hardware decoders are right, and libav is lenient. libav's plausible bottom half
  is prediction over padding.

Ruled out:
- the trailing zeros (stripped: same failure);
- the encode-to-host visibility: 283602aa's VIDEO_ENCODE → HOST barrier changes nothing;
- RADV reports no query result status ("reports no query result status"), so a failed encode cannot be seen.

Environment:
- Radeon iGPU 0x1638 = Cezanne ("RADV RENOIR"; firmware green_sardine_vcn), Mesa 26.0.8, kernel 7.0.0-38,
  linux-firmware 20260319.
- RADV exposes the encode extensions WITHOUT RADV_PERFTEST: they are on by default.
- Context: RADV enables Vulkan encode by default only on recent enough encoder firmware; Mesa closed a broken-HEVC
  report on VCN3 (Steam Deck) as a firmware problem.

## Root cause: RADV misreports the bitstream offset (2026-10-16, LinuxB-PA, experiment (a))

- Firmware (kernel log, boot 2026-10-16): `[VCN instance 0] Found VCN firmware Version ENC: 1.24 DEC: 8 VEP: 0
  Revision: 9`; Mesa 26.0.8, RADV RENOIR (Cezanne iGPU 0x1638).
- Method: the whole host-coherent bitstream buffer set to a 0xA5 sentinel before each submit; after the feedback query,
  the offset, the byte count and the first / last non-sentinel byte logged; 8 raw buffers dumped.
- All 364 pictures of 2 rushes: the feedback OFFSET is 0, the first byte written is at **192** (`00 00 00 01 26 01`,
  the IDR_N_LP start code), bytes [0, 192) keep the sentinel. The BYTES_WRITTEN value is right: the slice ends at
  192 + bytesWritten on its rbsp stop bit, then 0-7 zero bytes pad to 8.
- The engine (`dstBufferOffset = 0`, both feedback flags requested, `VideoEncoderH265.cpp` reads
  `[feedback[0], feedback[0] + feedback[1])`) therefore read 192 never-written bytes and lost the slice's last 192:
  the "193 trailing zeros" (192 + the next start code's leading 0), HM reading past the slice data, the hardware
  decoders concealing from a CTB row on — all explained.
- Read from byte 192 (test-only): HM 181 / 181 pictures, exit 0; libav 181 frames, 0 error; `vah265dec` 181; green
  census 0 / 181; the YUV of HM, libav and VA bit-exact on 181 / 181. The SPS depth fix (`0c83af85`) stays needed.
- The engine is conformant: VkVideoEncodeFeedbackFlagBitsKHR — the reported offset is relative to `dstBufferOffset`,
  and is always 0 for the first encode operation, so the data must start at `dstBufferOffset`. A RADV defect. Mesa
  already fixed a neighbouring one ("Fix encode bitstream buffer offset and alignment", 25.1.7 / 25.2.0-rc3); this one
  is present in 26.0.8. LinuxB-PA's reading of the source (not verified on hardware): `radv_query.c` writes
  `offset = src32[5]` and `bytes = src32[6] - src32[8]`; radeonsi places its slice at `enc->bs_offset` — the slice
  may start at `src32[8]` or after a VCN2 header reserve.
- Owner decision 2026-10-16: no workaround in the engine; the hardware H.265 stays withheld on AMD
  (`Recorder::hardwareEncodeWithheld()`, VP9 instead) until Mesa is fixed; a Mesa report, written by Linux-PA, filed
  by the owner.

## What remains

- [ ] The owner files the Mesa report (gitlab.freedesktop.org/mesa/mesa), with the streams from LinuxB.
- [ ] Once a fixed Mesa ships: re-test on the AMD machine (HM on a rush), then lift the withholding for the fixed
      versions only (driver version check), and delete this item after the knowledge moves to
      `docs/subsystems/graphics/11-10-video-recording.md`.

Evidence on the AMD test machine (LinuxB, not this one): `~/amd-h265-evidence-2026-10-15/` (52 MB, README inside,
section 2026-10-16): the AMD streams (depth fix; read as-is and read from 192), the radeonsi VA reference and the NVIDIA
control, the HM version and every TAppDecoder log, the per-picture feedback logs, the 8 raw buffer dumps, the firmware
line, the test patch and the launcher.

## ⚠️ Traps

- The stream is ALL-INTRA on purpose (`idrPeriod = 1`, `Recorder.cpp`: a mezzanine layout with frame-exact seeking,
  and it avoids an NVIDIA P-frame DPB issue). "IDR every 1 frames" in the log is not a bug.
- AMD reports the padded coded extent: every size derived from the record extent must be checked against it.

## References

- `src/Vulkan/VideoEncoderH265.cpp`: the NV12 upload and the encode submit.
- `docs/subsystems/graphics/11-10-video-recording.md` § Hardware encode chantier.
