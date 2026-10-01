## 10. Video Recording (Graphics::Recorder — "RushMaker" video track)

Studio-quality video recording of the Vulkan swap-chain framebuffer, encoded VP9/IVF.
Part of the RushMaker studio workflow: separate tracks (video IVF + game audio WAV + voice-over WAV)
assembled by an auto-generated ffmpeg script (`Core::startAudioVideoRecording()`).

### Encoder selection (hardware H.265 vs software VP9)
ONE decision point: `Recorder::hardwarePath()`. Hardware when the device exposes Vulkan
Video H.265 encode, software VP9 otherwise (the fallback is the cross-hardware guarantee:
AMD/Intel/older GPUs record without any configuration). Everything downstream follows the
choice automatically — file extension (`.h265` / `.ivf`), container and audio codec in the
assemble script (MP4/AAC / WebM/Opus).
**`Core/RushMaker/ForceCPUEncoding`** (default `false`) forces the software path on a
hardware-capable device: A/B comparison of the two encoders, and royalty-free WebM/VP9 on
demand. The startup log states which one is active — `Encoder: hardware H.265` /
`software VP9 (forced by settings)` / `software VP9 (no hardware support)`.

### ONE mode: studio CFR (owner decision, Aug 2026)
The RushMaker produces promotion rushes — **image quality is the only metric**. There is no
realtime/quality mode duality: one quality-first VBR configuration (16-frame lookahead,
complexity AQ, `VPX_DL_GOOD_QUALITY`, bitrate ladder via `QualityPreset`), and the output is
**constant frame rate on the wall clock** — the video timeline is real time, so the separately
recorded audio tracks stay in sync and the engine remains interactive during capture.
Should a low-latency capture need ever arise, it will be a **separate concept ("Streamer")**,
not a mode of this recorder.

### Pipeline
1. **The copy is recorded INSIDE the frame** (2026-09-25) — `Renderer::renderFrame()` asks
   `Recorder::wantsFrame()` right after the screenshot hook (§ 9b: last pass done, overlay included,
   image still ACQUIRED) and `recordFrameCopy()` records the swap-chain copy into the frame's own
   command buffer: to a GPU snapshot slot (hardware path) or to a staging / device-local buffer
   (software path), 4 slots each. `confirmSubmit()` follows the frame's submit, and the frame slot's
   in-flight fence releases the copy: `onFrameSlotRetired()` hands the snapshot to the encoding thread,
   or reads the staging buffer back into the grab buffer. The copy therefore belongs to the batch that
   rendered the frame and can see no other one.
   ⚠️⚠️ **It used to be a separate submit made AFTER the present** (`Core::renderingTask()` called
   `captureAndSubmitFrame()` once `renderFrame()` returned), on the queue
   `Device::getGraphicsQueue(High)` handed out — and that accessor **ROTATES over every queue of the
   family** (16 on NVIDIA) while the renderer keeps one. Nothing ordered the copy after the frame:
   whenever the GPU ran behind, it read the image BEFORE the frame was drawn into it, i.e. the frame
   presented 3-4 images earlier (the swap-chain image count). The owner saw a video stepping back like a
   laggy online shooter while the screen stayed perfect; measured on the owner's 8.7 s rush: **19 of 260
   frames bit-identical to frame k-3 or k-4**, periodic. After: 0 on a 328-frame hardware rush (owner,
   by hand), 0 on a 358-frame VP9 rush, 0 VUID. It was also the ownership defect `screenshot()` had
   (a presented image belongs to the presentation engine until re-acquired). `docs/caution-points.md`
   § *RushMaker stepped back 3-4 frames*.
   **Stop is deferred while a copy is in flight**: `stopRecording()` records nothing more; the session
   closes at once if no copy is on the GPU, otherwise on the rendering thread when the last one lands
   (framesInFlight() frames later). `startRecording()` refuses a new rush meanwhile. A swap-chain
   recreation completes every submitted copy (`onDeviceIdle()`, after its `waitIdle`: the frame slots
   may come back with another count), and so does the shutdown. The recording size is locked at
   start: a resized swap-chain is not copied (traced once), its slots become CFR duplicates.
   **Pacing**: the capture is paced at the target FPS on the wall clock (`shouldCaptureFrame()`): the
   wall clock is cut into CFR slots (`cfrSlotAt()`, THE single timeline formula, also every PTS), and
   the first rendered frame inside a slot not served yet is captured.
   ⚠️⚠️ **Never pace on "one frame duration since the last capture"** — what it did until 2026-09-24.
   The capture then lags the slot grid by up to one render interval each time and drifts to a lower
   rate: `forest` rendering at 48 FPS gave ~24 captures/s for a 30 FPS timeline, every fourth slot
   empty, **132 CFR fillers out of 626 frames**, seen by the owner as hiccups. On the slot grid: 0
   fillers at the same load. A filler is legitimate ONLY when no frame was rendered inside a slot
   (renderer below the target FPS, or a backpressure skip); the finalisation line
   `N CFR filler frames` is the counter to read — a decoded-frame hash undercounts them on the
   hardware path, whose rate control re-encodes a duplicate slightly differently.
2. **Bounded grab buffer** (`Core/RushMaker/MaxQueuedFrames`, default 90, range 3-240) — gives the encoder time
   to write the file; above the bound, captures are **skipped and counted** (backpressure) so a
   slow encode cannot balloon RAM
3. **Dedicated encoding thread** — BGRA→I420 conversion (SIMD dispatched: scalar/SSE4.1/AVX2),
   VP9 encoding, IVF writing at **constant frame rate**: every missing capture slot (renderer
   slower than the target FPS, backpressure skip) is filled by re-encoding the PREVIOUS image
   (`EncodingSession::encodeImageAt()` called before the next conversion overwrites the planes —
   a static VP9 frame costs almost nothing). The timeline never judders and never drifts.
   Encoder threads are capped at **hardware_concurrency/4**: the runtime stays interactive —
   half the cores at cpuUsed=1 measurably starved the logic and rendering threads (Aug 2026).

### Time model (owner rule, Aug 2026): the ONLY realtime element is the capture
The capture runs at the target FPS on the wall clock; the **encoder is free to take its
time** — it works in libvpx's quality path (`VPX_DL_GOOD_QUALITY` on every encode, including
the flush) and keeps draining in background after the recording stops. Never reintroduce
`VPX_DL_REALTIME` (a libvpx deadline constant, not a recorder mode — the realtime notion was
removed from this recorder entirely).

### Adaptive encoder speed (real frames beat encoding effort)
Software VP9 at maximum effort cannot hold 30 FPS at high resolution (measured Aug 2026:
cpuUsed=1 at 2880×1620 ≈ 6 real FPS → 4 frames out of 5 were CFR duplicates — unusable rush).
The encoding thread therefore **adapts `VP8E_SET_CPUUSED` live** (allowed mid-stream by libvpx),
**inside the good-quality mode only** (speeds 1..5 — 6+ belongs to the realtime path, unused):
once per second, if the grab buffer sits at its bound (captures being skipped), speed goes up
one notch (real frames beat per-frame effort); when the buffer drains below a quarter, speed
eases back toward the preset value. The next session **warm-starts** from the converged speed
(`m_adaptedCpuUsed`) instead of replaying the ramp. The periodic stats print `Speed:` — watch
it to know what the CPU actually sustains; the definitive fix for encoding at full effort and
full rate is the Vulkan Video hardware chantier.
Buffer knob: `Core/RushMaker/MaxQueuedFrames` (default 90 ≈ 3 s of elasticity at 30 FPS;
one buffered frame = width×height×4 bytes ≈ 1.6 GB at 2880×1620 — raise it for short takes
if RAM allows: zero skip, the encoder finishes in background). ⚠️ `getOrSetDefault` persists
the first-seen value: an older settings.json may still carry 32.
Ranges (owner ruling 2026-10-01, `SettingKeys.hpp`): `MaxQueuedFrames` 3-240 (240 = 8 s at 30 FPS, ~6.7 GB at
2880×1620), `VideoFramerate` 1-240; a value outside its range (0 included) warns and takes the default.

### When the file can no longer be written (owner ruling 2026-10-01)
Every write goes through `writeOutput()` (`Recorder.cpp`, anonymous namespace): the IVF / Annex-B header, every
packet, the flush; the final `fclose()` counts too (a failed final flush). The FIRST failure — a full disk, a removed
drive, a file-size limit — is traced with the OS reason and latched in the session (`writeFailed`): nothing more is
written, the software session stops encoding what is still queued (dropped, not encoded for nothing), and the
recorder STOPS the recording at its next capture, on the render thread (`recordFrameCopy()` →
`stopRecordingLocked()`, at a frame boundary). The session ends with `… TRUNCATED …` instead of `… finalized …`; the
file holds the frames written before the failure and stays playable (an IVF's frame count is left at 0, the
decoders count the frames). The output files are owned by a `std::unique_ptr< std::FILE, FileCloser >`.
The rush itself (audio, voice-over) keeps running: `Core::rushRecording()` (video OR audio OR voice-over) is what the
toggle (`Core.toggleRecording()`, Shift+Ctrl+F12) tests, so the next toggle STOPS the rest of the rush instead of
starting a new one (which the "audio recorder is still active" guard would refuse).
Measured 2026-10-01 (Linux, RTX 3070 Ti, 2880×1620, `RLIMIT_FSIZE` + `SIGXFSZ` ignored to fake a full disk):
hardware H.265 at 4 MiB → "Unable to write … (File too large)", stopped after ~65 frames, TRUNCATED, the file decodes
(ffprobe: 66 HEVC frames); VP9 at 2 MiB → the same, 64 VP9 frames; with audio ON the next toggle saved the WAV; 0 VUID,
exit 0. Without a limit: 181 frames in 6 s on both paths.

### Colorimetry (BT.709 — do not regress to BT.601)
The BGRA→I420 conversion uses **BT.709 limited-range** coefficients — single source of truth:
**`VideoColorConversion.hpp`** (`VideoColor::YCoef*/UCoef*/VCoef*`), consumed by the CPU
scalar/SIMD paths (`Recorder.cpp`) AND by the GPU compute converter (the GLSL receives them
through `VideoColor::glslDefines()`). The matrix is signalled in the VP9 bitstream
(`VP9E_SET_COLOR_SPACE` = BT.709, studio range) and tagged at container level by the generated
ffmpeg script (`-colorspace bt709 ...`). HD players assume BT.709; BT.601 coefficients on HD
content shift hues on playback.

### Hardware encode chantier (Vulkan Video H.265 — in progress, Aug 2026)
- **M1 done** — device plumbing: `Vulkan/Instance.cpp` enables `VK_KHR_video_queue` +
  `VK_KHR_video_encode_queue` + `VK_KHR_video_encode_h265` when present and logs the H.265
  encode capabilities; `Vulkan/Device` configures the dedicated VIDEO_ENCODE queue family
  (`videoEncodeH265Enabled()`, `getVideoEncodeQueue()`). Validated on the RTX 3070 Ti:
  8192×8192 max, 16 DPB slots, 120 Mbps, 7 quality levels, queue family #4.
- **M2 done** — `Graphics/VideoFrameConverter`: compute BGRA→NV12 planes (R8 luma full-res +
  R8G8 chroma half-res), **integer math identical to the CPU converters** — validated
  byte-for-byte via the console command `Core.RendererService.testVideoFrameConverter()`
  (procedural hash pattern generated identically in GLSL and C++, no upload involved).
- **M3 done** — `Vulkan/VideoEncoderH265`: session + memory binding, std VPS/SPS/PPS (driver
  returns the encoded Annex-B header via `vkGetEncodedVideoSessionParametersKHR`), two-slot
  DPB (separate images), VBR rate control from the presets, IDR+P GOP, encode-feedback query,
  Annex-B packets. Validated end-to-end via `Core.RendererService.testVideoEncoderH265()`:
  90 frames / 3 GOPs, ffmpeg decode with ZERO errors, decoded content matches the pattern.
  **Three NVIDIA lessons paid for in blood (do not regress):**
  1. `VkVideoEncodeH265RateControlLayerInfoKHR` MUST be chained to the rate-control layer —
     without it `vkCmdControlVideoCoding` invalidates the command buffer
     (`vkEndCommandBuffer` → `VK_ERROR_INITIALIZATION_FAILED`, validation layer silent).
  2. Picture images (src + DPB) MUST be allocated aligned on
     `pictureAccessGranularity` (32×32 on NVIDIA; we align on 64) — an under-aligned image
     HANGS the encode engine (infinite `waitIdle`). The logical `codedExtent` stays exact.
  3. Do NOT force `cu_qp_delta_enabled_flag` in the std PPS — the driver manages it; forcing
     it desynchronises the P-slice entropy (decoder reads out-of-range `cu_qp_delta`).
  Also required: the `synchronization2` device feature (video barriers are sync2-only) —
  enabled with the video extensions in `Instance.cpp`.
- **M4 done** — Recorder integration, validated pixel-exact against a live framebuffer
  screenshot (animation-debug @ 2880×1620 Ultra). Hardware path when
  `videoEncodeH265Enabled()`: swap-chain → GPU snapshot slots (image copy, NO CPU readback)
  → `VideoFrameConverter::convertFrom()` (sampler variant) → `VideoEncoderH265` on a
  dedicated encoding thread (CFR fillers by re-encoding the still-loaded planes). Software
  VP9 path untouched as the cross-hardware fallback. `Core.toggleRecording()` console
  command (= Shift+Ctrl+F12). The assemble script muxes `.h265`+WAV → MP4/AAC with BT.709
  tags (`-framerate` input option is mandatory for a raw elementary stream).
  **v1 is ALL-INTRA (idrPeriod 1)** — the professional mezzanine layout (frame-exact seek,
  no error propagation, ideal for editing); hardware preset bitrates are raised accordingly
  (8/16/30/60 Mbps).
  ⚠️ **Open issue — P frames**: with references enabled, P frames predict from an empty
  reconstruction (green frames) whatever the DPB organisation (separate images, layered
  array, per-frame availability barriers, slot-index conventions, SPS minCB 8/16,
  codedExtent variants — all bisected on a deterministic 2880×1620 structured-pattern
  bench, `Core.RendererService.testVideoEncoderH265()`). IDR frames are pixel-perfect.
  Next lead: trace the nvpro reference encoder with gfxreconstruct and diff the API
  streams, or try explicit quality-level session parameters. All-intra sidesteps it.
  More NVIDIA lessons (in addition to the M3 three): the bitstream buffer SIZE must be
  aligned on minBitstreamBufferSizeAlignment (a misaligned dstBufferRange corrupts the
  stream — 1280×720 was aligned by luck, 2880×1620 was not) and a bench with a STRUCTURED
  test pattern is mandatory — corruption is invisible in noise (the M3 "clean" validation
  was noise-blind).
- **HDR10 lookahead (owner request)**: keep the profile/bit-depth parametric — Main 10 +
  P010 planes (16-bit containers) + BT.2020/PQ shader variant sourcing the PRE-tonemap HDR
  buffer + mastering-display SEI. Nothing in M2/M3 may hardcode 8-bit assumptions in the API.

### Symmetric API

| Method | Purpose |
|--------|---------|
| `startRecording(path)` | Begin recording to IVF file at given path (refused while the previous rush is closing) |
| `stopRecording()` | Stop capturing; close the session now, or when the last in-flight copy lands |
| `isRecording()` | Check if recording is active |
| `shouldCaptureFrame()` | Frame pacing check (target FPS, CFR slot grid) |
| `wantsFrame()` | Renderer: should THIS frame carry a copy (recording + pacing) |
| `recordFrameCopy()` / `confirmSubmit()` | Renderer: record the copy in the frame's command buffer, then report its submit |
| `onFrameSlotRetired()` / `onDeviceIdle()` | Renderer: the frame fence (or the whole device) completed the copies |

Path generation is owned by `Core::startAudioVideoRecording()` — see `src/AGENTS.md` Core section.

### Transfer Queue Optimization
When a dedicated transfer queue family is available, the software path copies in two steps:
1. In the frame's command buffer: swap-chain image → device-local buffer (with layout transitions)
2. Once the frame's fence was waited (`onFrameSlotRetired()`): transfer queue, device-local →
   host-visible staging (DMA, its own fence, polled every frame — the host wait orders it after the
   frame, no semaphore)

### Code References
- `Recorder.hpp` — Full class with Doxygen documentation
- `Recorder.cpp` (top) — Shared BT.709 conversion coefficients (single source of truth)
- `Recorder.cpp:startRecording()` — VP9 init (incl. colour-space signalling), async resource creation, thread start
- `Recorder.cpp:recordFrameCopy()` — Extent check + slot pick + in-frame copy (`recordHardwareCopy()` / `recordReadbackCopy()`, the latter with the backpressure gate)
- `Recorder.cpp:retireCopies()` / `harvestTransfers()` — Frame fence retirement: queue the snapshot, read back, or start the DMA
- `Recorder.cpp:stopRecording()` / `stopRecordingLocked()` / `closeSession()` — Deferred stop (also taken when `outputWriteFailed()`)
- `Recorder.cpp:writeOutput()` / `closeOutput()` — Checked writes, the first failure latched
- `Recorder.cpp:encodingThreadFunc()` — CFR filler loop + BGRA→I420 + VP9 encode
- `Recorder.cpp:EncodingSession::encodeImageAt()` — Encode current image at a given PTS (also duplicates into empty CFR slots)
- `Renderer.cpp:renderFrame()` — The call sites (after the screenshot hook, the three submit outcomes, the fence wait)
- `cmake/SetupLibVPX.cmake` — Build configuration for libvpx
