## 16. Frame Synchronization — Double-Buffering (GPU) and the Logic Triple Buffer

> [!CRITICAL]
> **Read [`src/Scenes/AGENTS.md` → Frame Synchronization](../../../src/Scenes/AGENTS.md) BEFORE adding
> any GPU buffer (SSBO, UBO) that is updated per-frame, or any post-process effect that
> reconstructs world positions from the depth buffer.
> Also read [Section 15](#15-instance-local-program-cache-renderableinstance) for the
> instance-local cache that avoids per-draw hashtable lookups.**

The renderer uses **frames-in-flight** (`Renderer::framesInFlight()`, typically 2-3).
Each frame has its own command buffer, fence, and descriptor sets indexed by
`m_currentFrameIndex`. The logic thread runs concurrently with the GPU.

### Rule 1: Per-Frame GPU Buffers

Any GPU buffer written every frame **MUST** have one copy per frame-in-flight,
indexed by `m_currentFrameIndex`. Writing to a single shared buffer while the GPU reads
the previous frame causes race conditions (flickering, data corruption).

**Pattern:**
- `SceneMetaData::m_meshMetaDataSSBOs` — vector of SSBOs, one per frame-in-flight
- `Renderer::updateRTDescriptorSet()` — binds `meshMetaDataSSBO(m_currentFrameIndex)`
- `Scene::prepareRender()` — calls `rebuild(..., frameIndex)` with current frame index

### Rule 2: View Matrix State Index

Post-process effects that read the depth buffer to reconstruct world positions **MUST**
use `Renderer::currentReadStateIndex()` when calling `viewMatrix(readStateIndex, ...)`.
The default `viewMatrix(false, 0)` reads `m_logicState` which may have already advanced
to the next logic tick → **matrix/depth mismatch → flickering**. See `RTR.cpp:execute()`.

### Rule 3: Frame History ≠ State Indices (Temporal Effects)

The logic/render state slots (`readStateIndex`/`writeStateIndex`, a **triple buffer** since
2026-09-24, `RenderStateSlotCount`) track **logic ticks**, NOT rendered frames — if the logic
thread ticks twice between two frames, no slot index is "the previous frame". Temporal effects (RTGI reprojection, future TAA) must use the
**frame-history contract** instead:

- `ViewMatricesInterface::previousViewMatrix()` / `previousProjectionMatrix()` — the state
  consumed by the previously RENDERED frame (identity until first archive; consumers handle
  their own first-frame invalidation).
- `archiveStateAfterRendering(readStateIndex)` — called by `Renderer::renderFrame()` ONCE
  per rendered frame, on the render thread, AFTER the command buffer is recorded (so during
  the recording of frame N the archive still holds frame N-1). Real implementation in
  `ViewMatrices2DUBO` (the swap-chain camera UBO, which `SceneRenderTarget` delegates to);
  cubemap/CSM views keep the no-history default.

**Code references:**
- `Renderer.hpp:m_currentFrameIndex` — Current frame-in-flight index
- `Renderer.hpp:m_currentReadStateIndex` — read state slot latched by the current frame (triple buffer: see `src/Scenes/AGENTS.md` → Frame Synchronization)
- `Renderer.hpp:framesInFlight()` — Number of frames-in-flight
- `Scenes/SceneMetaData.hpp:initializePerFrameBuffers()` — Reference implementation
- `ViewMatricesInterface.hpp` — frame-history contract (previous view/projection + archive)
- `Renderer.cpp:renderFrame()` — the single `archiveStateAfterRendering()` call site

### Rule 4: The View UBO Is Single-Buffered — Never Put Frame-Varying Data In It

`ViewMatrices2DUBO` (and its 3D/Cascaded siblings) own **ONE** `UniformBufferObject`
(`ViewUBOSize`, one descriptor set) — **NOT** `framesInFlight()` copies. `updateVideoMemory()`
rewrites it once per cycle, on the render thread, while the GPU may still be reading it for
a frame that is still in flight.

This is safe **only** because everything the UBO holds is *view state*, which is identical
for every frame that observes the same camera. The instant a value inside it varies **per
rendered frame**, Rule 1 applies and the single buffer becomes a data race.

> [!CAUTION]
> **Lived example (Jul 2026).** The TAA sub-pixel projection jitter was written into this
> UBO's projection matrix. Scene vertex shaders on the advanced-matrices path build their
> MVP as `ubView.projectionMatrix * pcMatrices.viewMatrix * model`, so the raster read a
> jitter that could belong to frame N±1, while the jitter *removed* from the velocity
> outputs came from the per-frame `InstanceTransforms` SSBO — correctly frame N. Residual =
> `j_{N±1} - j_N`: a **constant in NDC space**, hence a velocity that is uniform across
> every depth in the frame (a real camera motion is depth-dependent through parallax — that
> uniformity is the diagnostic signature). Consequences: motion vectors wrong on a static
> camera, TAA history rejected by its variance clip (accumulation collapsed → the image
> vibrated at full jitter amplitude, from the very first frame), and RTGI reprojecting off
> by ~1 px with its history validation silently masking the error. The CPU-side matrices
> were exact the whole time — a CPU trace showed `maxAbs(A - B) == 0` over 683 consecutive
> frames — which is why static code review kept concluding "velocity must be zero".
>
> **Resolution (applied 2026-07-25):** frame-varying jitter belongs in a **per-draw push
> constant**, never in the shared UBO. Push constants are recorded per draw, so they are
> per-frame AND per-target by construction — shadow maps, cubemaps and render-to-texture
> targets push zero because only the main view ever has a jitter enabled.
> `updateVideoMemory()` now uploads the clean projection unconditionally and carries a
> `[CAUTION]` marker at the exact spot where the jittered write used to be. Measured effect
> on the static-camera protocol: temporal peak-to-peak mean `2.1`-`3.8` → `0.29`
> (baseline `0.11`). See `docs/caution-points.md` § "Sub-pixel projection jitter raced the
> single-buffered view UBO" and `src/Saphir/AGENTS.md` § "TAA Sub-Pixel Jitter".

**Checklist before adding a member to a view UBO:** does this value differ between two
frames that share the same camera state? If yes, it does not belong here.

### Rule 5: Frame-in-Flight Index ≠ Swap-Chain Image Index (Aug 2026)

`Renderer::createRenderingSystem()` sizes `m_rendererFrameScope` from the swap-chain **image
count**, so the two counts are equal — but the two **indices are not interchangeable**:

| Index | Advances how | Addresses |
|---|---|---|
| `m_currentFrameIndex` | `+1 % framesInFlight()`, strictly cyclic | frame scope: command pool, in-flight fence, image-available semaphore, per-frame SSBOs/descriptors |
| the value returned by `SwapChain::acquireNextImage()` | **arbitrary order**, decided by the presentation engine | framebuffer, colour image, **present semaphore** |

The two coincide under FIFO (Linux/Mesa) and diverge under MAILBOX (typical on Windows), which
is why an index mix-up can stay invisible for a long time on one platform.

**The synchronization primitives split along that line, and the reason is the completion proof:**

- **Image-available semaphore → per frame in flight.** The image index is unknown until
  `vkAcquireNextImageKHR()` returns, so it *cannot* be indexed by image. Reuse is safe because
  the frame's fence proves the submission that waited on it has completed.
- **Present semaphore → per swap-chain image** (`Renderer::m_presentSemaphores`). **No fence
  ever observes the completion of a `vkQueuePresentKHR()`.** The only proof that a present
  released its semaphore is the **re-acquisition of the image it presented**. Index it by frame
  and a binary semaphore gets re-signaled while a present still waits on it:
  `VUID-vkQueueSubmit-pSignalSemaphores-00067`.

These semaphores live with the **rendering system**, not with `SwapChain::Frame`, on purpose:
they must survive swap-chain recreation, because `vkDeviceWaitIdle()` does **not** retire
pending present operations — destroying them on resize would be a destruction-while-in-use.

**Any bail-out after a successful acquisition must drain, not return.** Every semaphore already
signaled for that frame (the acquisition, plus the shadow-map and render-to-texture submissions
that ran before the failure) must be waited on exactly once, or the next frame reusing them hits
the same VUID. `Renderer::discardAcquiredImage()` submits an empty synchronization batch
(`Queue::submit(const SynchInfo &)`, no command buffer) that drains them and, when the fence was
already reset, signals it back — then declares the swap-chain degraded, because its recreation is
the only thing that gives back an image that was acquired and never presented.

**Code references:**
- `Renderer.hpp:m_presentSemaphores` — the per-image array and its lifetime contract
- `Renderer.cpp:renderFrame()` — `imageIndex` (acquired) vs `m_currentFrameIndex` (frame slot)
- `Renderer.cpp:discardAcquiredImage()` — the drain
- `Vulkan/SwapChain.hpp:present()` — `@warning` stating the per-image requirement
- `docs/caution-points.md` § "Present semaphore was indexed by frame in flight"
