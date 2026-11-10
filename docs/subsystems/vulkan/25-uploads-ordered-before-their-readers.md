## Critical: an upload is ordered before its readers and after its previous writes, on the GPU (Nov 2026)

> [!CRITICAL]
> **An upload returns as soon as it is SUBMITTED.** It runs on a queue picked round-robin
> (`Device::getPrimaryTransferQueue()` / `getPrimaryQueue()` rotate over a family: 16 graphics queues on NVIDIA), while
> the renderer submits on ONE fixed graphics queue (`Renderer::m_graphicsQueue`). A fence wait or a timeline value read
> on the host orders the HOST only: it never makes a write visible to another queue, nor orders two queues. The only
> orderings that count are on the GPU: a barrier on the same queue, or a semaphore wait.

### What was wrong (fixed 2026-11-11)

Until then, nothing ordered an upload before the first draw that read it, nor two uploads of the same object:

- **Read after write.** `BufferTransferOperation` (single-family path) and `ImageTransferOperation` submitted the copy
  with a fence only; `Image::recordPendingSubmission()` / `Buffer::recordPendingSubmission()` only deferred the
  object's DESTRUCTION. On a device with one queue (Intel iGPU, Apple M-series through MoltenVK), the copy and the draw
  share the queue but no barrier makes the transfer write visible to the index fetch. Measured on Linux, Intel RPL-S
  (one family, one queue), `sponza --demo-options 1`, window-less, Synchronization Validation on: **3 runs out of 3**
  report `SYNC-HAZARD-READ-AFTER-WRITE` — `vkCmdDrawIndexed` reads an index buffer of 10 016 880 bytes written by
  `vkCmdCopyBuffer` on the same queue — the layer rejects the submit (`VK_ERROR_VALIDATION_FAILED_EXT`), then the
  process dies (SIGSEGV, exit 139: engine item `failed-frame-submit-crashes`). The same hazard was seen on the Apple M2
  (2026-09-03, 2026-10-04) and on the Windows Intel iGPU (2026-10-25). Across queues (NVIDIA) there was not even an
  execution order: it only held because the copy usually finished first.
- **Write after write.** Two uploads of one image on two queues were unordered: the older one could land last. A
  downstream application showed a stale band of a web view (the full-versus-region race, fixed in `b3745cc1` by keeping
  every re-upload of a resident overlay image on the region path). The region path still rotated over the graphics
  queues: **10 `SYNC-HAZARD-WRITE-RACING-WRITE`** between two region uploads of a web-view surface (Linux, RTX 3070 Ti,
  2026-11-10), and the layer's rejection made 4 of them fall back to the full upload.

### The mechanism

1. **`SynchInfo::waitsFor(points)`** — a submission waits, on the GPU, for completion points (`SubmissionPoint`: a
   queue and a value of its timeline, from `SynchInfo::tracksCompletion()`). `Queue::submit()` merges them with the
   binary waits in one `VkSubmitInfo` (`VkTimelineSemaphoreSubmitInfo` carries a value for every wait, 0 for a binary
   one), at `VK_PIPELINE_STAGE_ALL_COMMANDS_BIT`, two points on one queue keeping the highest value. A semaphore wait is
   also a memory dependency: the writes become visible to the batch, from any queue — the submitting queue included.
   Waiting a reached value costs the GPU nothing; there is no CPU wait.
2. **Every write of a buffer or an image waits for the previous writes of that object** (`Buffer::pendingWrites()`,
   `Image::pendingWrites()`, read from its `PendingSubmissions` without consuming them): the full, compressed and region
   image uploads, the buffer uploads (both paths), and `TransferManager`'s layout transitions and clears.
3. **Every such write publishes its completion point** (`Device::publishUploadPoint()`): the device keeps the latest
   value per queue (`Device::uploadFrontier()`, at most `Device::MaxQueues` = 32 points). For a two-step upload, the
   point published is the last step (the graphics-family finalize or acquire, which itself waits for the transfer step).
4. **Every batch that may read uploaded data waits for the frontier**, taken AFTER recording, right before the submit
   (a resource found ready while recording published its point before it became ready, so the batch covers it): the
   renderer's sky atmosphere, shadow-map and render-to-texture submissions and the frame itself
   (`Renderer.cpp`), and the acceleration-structure builds (`AccelerationStructureBuilder::submitOneShot()`). A point is
   never dropped once reached: only a GPU wait makes a write visible to another queue.

### Measured (2026-11-11, engine 21081ebe + this lot)

| Device | Scenario | Before | After |
|---|---|---|---|
| Intel RPL-S, Linux (one queue) | `sponza --demo-options 1`, window-less, Synchronization Validation | 3/3 runs: 2 `SYNC-HAZARD-READ-AFTER-WRITE`, rejected submit, SIGSEGV | 4/4 runs: 0 `SYNC-HAZARD`, 0 VUID, 0 `[Error]`, exit 0 |
| Intel RPL-S, Linux | `animation-debug`, `labyrinth`, `reflexion-debug`, `sky-lab` | 9 runs: 0 hazard (Linux's iGPU did not reproduce the Windows iGPU's intermittent ones) | 4 runs: 0 hazard, 0 VUID, 0 `[Error]`, exit 0 |
| RTX 3070 Ti, Linux | `light-and-shadow-debug 0,1` (the overlay notifier), `animation-debug`, `sponza 1` | — | 0 `SYNC-HAZARD`, 0 VUID, 0 `[Error]` |
| RTX 3070 Ti, Linux | a downstream application's web views (login modal, backdrop fade, menus) | 10 `SYNC-HAZARD-WRITE-RACING-WRITE`, 4 fallbacks to the full upload | 0 `SYNC-HAZARD`, 0 fallback, 0 VUID, no stale band |

Synchronization Validation was proven on in every run (the layer's `Current Validation Enabled` list includes
`Synchronization`; `VK_KHRONOS_VALIDATION_VALIDATE_SYNC=true`).

### ⚠️ Traps

- **A new writer of GPU data publishes its point** (`Device::publishUploadPoint()`) and waits for the previous writes of
  its object; **a new reader submission waits for the frontier** (`waitsFor(m_device->uploadFrontier())`). Forgetting
  either reopens the race silently: no validation message without Synchronization Validation.
- **Take the frontier after recording**, never at the start of the frame: an upload finishing during the recording
  would be drawn without its wait.
- **A semaphore wait covers its own batch only**, not the later batches on the same queue: each reader batch of the
  frame waits for the frontier, not only the first one.
- **Not covered yet:** the compute submissions that wait their own queue on the host (`IBLBaker`, `OceanWaves`,
  `SkyAtmosphere`, `VideoFrameConverter`, `XRayAnalyzer`, `VideoEncoderH265`): a host wait orders nothing on another
  queue. They read data they produced themselves on the same queue, or textures uploaded long before; move them to
  `waitsFor()` when one of them reads a fresh upload.
- **Write after READ is not covered**: a re-upload does not wait for the frames in flight that still sample the old
  content (engine item `gpu-data-rewritten-under-frames-in-flight`).

### Code references

- `src/Vulkan/Queue.hpp` (`SynchInfo::waitsFor()`, `Queue::timelineHandle()`, `MaxWaitSemaphores`), `Queue.cpp`
  (`submit()`, the merge).
- `src/Vulkan/Device.hpp` / `.cpp` (`publishUploadPoint()`, `uploadFrontier()`).
- `src/Vulkan/PendingSubmissions.hpp` (`points()`), `Buffer.hpp` / `Image.hpp` (`pendingWrites()`).
- `src/Vulkan/BufferTransferOperation.cpp`, `ImageTransferOperation.cpp`, `TransferManager.cpp` (the writers).
- `src/Graphics/Renderer.cpp`, `src/Vulkan/AccelerationStructureBuilder.cpp` (the readers).
