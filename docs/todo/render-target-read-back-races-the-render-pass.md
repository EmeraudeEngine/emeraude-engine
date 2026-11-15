---
id: render-target-read-back-races-the-render-pass
title: A render target read back from the console is copied while the render thread draws it
status: open
priority: high
scope: Vulkan/TransferManager (downloadImage: its queue), Scenes/Manager.console (dumpRenderTarget), Graphics/RenderTarget (ShadowMap::capture, Texture::capture), Graphics/Renderer (the queue it renders on)
opened: 2026-11-15
tags: [render-target, read-back, queue, synchronization, console]
---

# A render target read back from the console is copied while the render thread draws it

## Why

`dumpRenderTarget` runs on the console thread: `Device::waitIdle()`, then `TransferManager::downloadImage()`. The
copy is submitted on `Device::getPrimaryQueue(High)` — which hands out the queues of the graphics family IN ROTATION
— while the renderer submits its passes on its own queue (`Renderer::m_graphicsQueue`). The barrier the read-back
records (old layout to TRANSFER_SRC) orders it against nothing: two queues are not ordered with each other, and the
render thread is free to submit the next frame between the `waitIdle()` and the copy. The comment in the command
says "full device sync first; brutal, but correct": it is not.

**Measured 2026-11-15** (Linux, RTX 3070 Ti, `light-and-shadow-debug`, `--enable-depth-readback`, the bulb's shadow
cube: one multiview pass for six views; 0 `VUID-` everywhere; "empty" = the 1 048 576 pixels of a face at the far
plane):
- 3 launches of 24 in Release with the three lights on read faces 4 and 5 EMPTY at every read, faces 0 to 3 right;
  the other launches read the six faces right. Debug gave both kinds of launches too. The Windows peer read face 2
  empty once (RTX 3500 Ada).
- In such a launch, switching the BULB off — its map is then suspended, no pass writes the image any more — makes
  faces 4 and 5 read RIGHT (631 244 and 542 859 pixels at the far plane, twice, seconds apart); switching it on again
  makes them read empty again. The image always held six right faces: the read lands INSIDE the pass, after the
  driver wrote views 0 to 3 and before views 4 and 5 (it draws the six views of this pass in two groups).
- A RenderDoc capture of a frame of such a launch, replayed: the six matrices of the view UBO are right, the vertex
  shader's output is right for the six views, the six layers hold their geometry from the pass's last draw to the
  present. What is submitted is correct.
- Screenshots of a "bad" launch and of good ones do not differ beyond the animated sprite: the frame samples the
  map after its semaphore, complete.

So the shadow maps are right and the INSTRUMENT is wrong — for three weeks the solid blue dump of
`offscreen-rendering` was suspected of exactly this race and turned out to be a real picture; this one looked like a
real hole in a shadow map for a few hours and is the race. Two wrong conclusions were written in this very item
before the proof (in commits `5f39f8b5` and `210cceeb`): "mostly in Debug", then "the map, not the read" — the second
from the fact that switching the two OTHER lights off made the faces read right. It only moved the race.

The same call serves `ShadowMap::capture()`, `Texture::capture()`, `SceneRenderTarget`'s capture and the imposter
atlas' read-back: each is exposed when called off the render thread on a target still being drawn.

## What remains

- [ ] Owner decision, the fix:
  1. submit the read-back on the queue that RENDERS the target (one queue's submissions are ordered: the existing
     barrier then means something, in both directions — the copy after the last pass, the next pass after the
     copy); `waitIdle()` goes;
  2. or order it across queues with the mechanism the uploads already use (`Image::recordPendingSubmission()` /
     `pendingWrites()`, `SynchInfo::waitsFor()`): the pass records itself on the images it writes, the read-back
     waits for them AND records itself so that the next pass waits for it;
  3. or serve the read-back on the render thread at a frame boundary, as `Graphics::FrameCapture` does for a
     screenshot (the console command posts a request and waits for its result).
- [ ] Re-test: in 24 launches, faces 4 and 5 of the bulb's cube right at every read with the three lights on
  (classify with `dumpRenderTarget(BulbShadowMap, 5)`; a launch is bad for all its reads or for none), Debug and
  Release; the Windows peer on its face 2.
- [ ] The other callers of `downloadImage()` listed above: which thread, which target.

## ⚠️ Traps

- A read that changes after the SCENE changed proves nothing about the content: it may only have moved the race.
  Suspend the target (switch its light off: `isRenderingSuspended()`), then read — an image nobody draws cannot be
  read wrong.
- A launch is "bad" or "good" as a whole: one launch proves nothing, classify several.
- With the bulb alone the faces read right in 12 reads of 12: fewer submissions per frame, another timing.
- Each depth dump leaves a 4.2 MB `.pfm` in the captures folder; two dumps of one layer within the same second: the
  second is refused (item `dump-render-target-same-second-refused`).

## References

- `src/Scenes/Manager.console.cpp` (`dumpRenderTarget`: `waitIdle()` then the download), `src/Vulkan/TransferManager.cpp`
  (the copy and its queue), `src/Vulkan/ImageTransferOperation.cpp` (the note on the rotating queues, the pending
  writes of an image), `src/Graphics/Renderer.cpp` (`renderShadowMaps()`: one submission per map).
- `docs/render-targets.md` (the read-back, its measurements and its two lessons).
- Related: `queue-waitidle-holds-the-device-lock`.
