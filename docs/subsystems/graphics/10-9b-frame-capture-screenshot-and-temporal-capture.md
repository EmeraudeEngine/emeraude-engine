## 9b. Frame capture — screenshot and TEMPORAL capture (Graphics::FrameCapture, Sep 2026)

What is PRESENTED is captured INSIDE its own frame: `Renderer::renderFrame()` records, after the
last pass that writes the swap-chain image (UI overlay included) and before `commandBuffer->end()`,
a barrier final layout → `TRANSFER_SRC`, a copy into a staging buffer, a barrier back and a
buffer barrier `TRANSFER_WRITE → HOST_READ`. The copy rides the frame's own submit and fence; the
slot's next fence wait (`onFrameSlotRetired()`) proves it complete. It replaced
`SwapChain::capture()`, which downloaded an image the presentation engine owned
(`UNASSIGNED-non-acquired-swapchain-image-used` on MoltenVK and the Windows layers; the method now
refuses) and `Renderer::captureFramebuffer()` (deleted).
- **Screenshot**: `Renderer::captureFrames(1, false, timeout)` — console `screenshot()` and F12
  WAIT for the file (owner decision 2026-09-23: the console contract "Screenshot saved: <path>"
  holds). `<unix seconds>.png`.
- **Temporal capture** (DEV tool, owner request 2026-09-23, against shimmer and temporal artefacts):
  console `Core.RendererService.temporalCapture([N = 5])` — N CONSECUTIVE presented frames,
  `<unix seconds>-<n>.png` (n from 0) plus `<unix seconds>.json`: per frame the renderer frame
  serial, CPU record time and delta, TAA jitter (NDC and pixels), camera position, view matrix and
  exposure triad (+ auto flag). The N staging buffers are allocated at ARM time and nothing is
  read on the CPU before the last frame's fence (owner decision): the captured frames cost one GPU
  copy each, so they are the frames of a normal run. Read-back, PNG encoding (`parallelFor`) and
  the JSON run on the thread pool afterwards. Budget `FrameCapture::MaxCaptureBytes` = 1 GiB (57
  frames at 2880×1620); a larger request is refused with the maximum.
- Analysis: `tools/temporal-analysis.py <stem|json>` — the protocol of projet-alpha's
  `docs/temporal-stability-measurement.md` on the consecutive frames (flatness, per-pixel
  peak-to-peak and its tails, bands, worst tiles, gradient/Laplacian signature, `-ptp.png` and
  `-heat.png` maps).
- ⚠️ The TAA jitter is a Halton (2,3) cycle of **8** frames (verified in the JSON: the offsets repeat
  from frame 8): capture ≥ 8 frames to see a whole cycle; the default 5 does not.
- ⚠️ A swap-chain recreation (resize) abandons a running capture (its frames must share one extent
  and have no hole); so does an abandoned frame (a failed submit).
- ⚠️⚠️ **The stem is UNIQUE** (fixed 2026-09-24): still an integer `<unix seconds>`, but a second
  already used — by this process or by a file on disk — moves it to the next free one. Two
  screenshots in the same second used to share a name and the second failed ("Unable to write …
  (does it already exist?)"), reproduced by two `screenshot()` 0.3 s apart. A stem may therefore run
  a few seconds ahead of the clock in a burst: read the path the command answers, never rebuild it.
- ⚠️⚠️ **A timed-out capture is CANCELLED** (fixed 2026-09-24): `waitForCompletion()` stops asking
  for frames, lets the copies already RECORDED drain through their fences (a recorded, not yet
  submitted copy is in flight too: its command buffer references the staging buffer) and goes back
  to idle, writing and publishing nothing (`A timed-out capture was cancelled and its frames
  released.`). It used to stay ARMED: the next request was refused "A capture is already in
  progress" and its late result went to nobody — seen on Windows right after a runtime scene switch
  (item `screenshot-lost-across-scene-switch`: WHY no frame reached the capture in 5 s there is not
  reproduced on Linux). Stress-tested with a 20 ms timeout: 20 cancellations in a row, 20 releases,
  0 VUID. A request while a cancelled capture drains answers "retry in a moment".
- ⚠️ With MAILBOX presentation a submitted image may be replaced before it is displayed: the capture
  holds what the renderer produced, frame after frame.
- RushMaker records its video copy on the same hook since 2026-09-25 (§ 10, *Pipeline*): it used to
  copy AFTER the present, on another queue, and the video jumped back 3-4 frames.
