## Frame Synchronization — Double-Buffering Contract

> [!CRITICAL]
> **ANY data that flows from the Logic thread to the Renderer MUST be double-buffered
> (one copy per frame-in-flight).** Failure to respect this causes GPU read / CPU write
> race conditions that manifest as flickering, tearing, or corrupted data.

### How It Works

The engine uses **frames-in-flight** (typically 2-3) to keep the GPU busy while the CPU
prepares the next frame. Each frame-in-flight has its own fence, command buffer, and
descriptor sets. The logic thread and render thread run concurrently.

**Synchronization mechanism — a lock-free TRIPLE buffer (since 2026-09-24):**
- Every published state (node and static-entity frames, the three `ViewMatrices*UBO`, the light
  emitters' uniform blocks) has **`RenderStateSlotCount` = 3** copies (`Constants.hpp`).
- Three slot indices are always disjoint: `m_writeSlot` (logic thread only), `m_frameReadStateIndex`
  (render thread only, latched ONCE per frame by `beginRenderFrame()`) and the middle one,
  `m_publishedSlot` (atomic, carries `FreshPublication` while the render thread has not taken it).
- `publishStateForRendering()` writes `m_writeSlot`, then **exchanges** it with the middle slot.
  `beginRenderFrame()` takes a fresh publication by **exchanging** its own slot with the middle one.
  Neither thread can ever reach the slot the other holds, however many ticks one frame lasts.

> [!CRITICAL]
> **Never go back to two slots, and never take the published index with a plain load.** Until
> 2026-09-24 the state had two slots and the logic alternated between them: its SECOND publication
> inside one frame landed on the slot the frame was reading. Every frame longer than a 60 Hz tick
> mixed two ticks — the draws recorded early used one camera, those recorded late the camera two
> ticks later — so heavy content visibly **slid** on the rest of the image while the camera turned
> (the `terrain` forest, Sponza's central tree), shadows desynchronised from their casters, and the
> view-state archive fed TAA/RTGI a "previous" matrix newer than the one rendered. Measured with
> `Core.SceneManagerService.getStateSyncStatistics(true)`: **41-66 % of the frames overwritten on
> `terrain`, 11-16 % on `sponza`; 0 % on both after the fix at the same load** (1.6 and 1.2 publications per frame).
> The comment in `ViewMatrices2DUBO::archiveStateAfterRendering()` claimed the latched slot was
> "stable for the whole frame" — true only below one tick of CPU recording.
> **The instrument stays**: `Scene::endRenderFrame()` closes a per-frame check (a write generation
> per slot, bumped BEFORE the logic writes it). Any value other than 0 is a regression.

**Code references:**
- `Scene.rendering.cpp:beginRenderFrame()` — the render-side exchange (the latch)
- `Scene.rendering.cpp:publishStateForRendering()` — the logic-side exchange
- `Scene.rendering.cpp:endRenderFrame()` / `Scene::stateSyncStatistics()` — the measurement
- `Scene.hpp` — `m_publishedSlot`, `m_writeSlot`, `m_frameReadStateIndex`, `m_preparedReadStateIndex`
- `Renderer.hpp` — `m_currentFrameIndex`, `framesInFlight()`

### Per-Frame GPU Resources

Any GPU buffer (SSBO, UBO) that is **updated every frame** must have one instance per
frame-in-flight. Otherwise, the CPU overwrites the buffer while the GPU is still reading
the previous frame's data.

**Already double-buffered:**
| Resource | Owner | Indexed by |
|----------|-------|------------|
| Entity world coordinates | `LocatableInterface` | `m_renderStateIndex` |
| RT mesh metadata SSBOs | `SceneMetaData` | `m_currentFrameIndex` |
| RT material data SSBOs | `SceneMetaData` | `m_currentFrameIndex` |
| RT descriptor sets | `Renderer` | `m_currentFrameIndex` |
| Instance transforms SSBOs | `SceneInstanceTransforms` | `m_currentFrameIndex` |

> [!CAUTION]
> **NOT double-buffered, contrary to what this table claimed until Aug 2026 — and this is an OPEN
> defect, not a design choice.** The row `| Light UBOs | LightSet | Dynamic offset |` was **false**.
> `SharedUniformBuffer::getByteOffsetForElement()` returns
> `(elementIndex % m_maxElementCountPerUBO) * m_blockAlignedSize`: the dynamic offset partitions the
> buffer **per light**, never per frame, and the class has no frame dimension anywhere. That line
> sat two lines above the rule it contradicted, and it is why the hole survived unnoticed.
>
> Still single-instance today, all written by the render thread **before** the frame's in-flight
> fence is waited on (`Core.cpp` `updateVideoMemory()` runs before `Renderer::renderFrame()`, whose
> first synchronisation is `inFlightFence()->wait()`):
>
> | Resource | Owner | Frame-varying content? | Status |
> |----------|-------|------------------------|--------|
> | Light UBOs | `Graphics::SharedUniformBuffer` (`LightSet`) | **YES for ANY light that moves** — a carried torch, a lamp on a vehicle, an animated sun, and every CSM light by construction | **FIXED** — one region per frame-in-flight, folded **into the existing dynamic offset** (separate descriptor sets would re-open the offset-blind dedup defect) |
> | Cascaded view UBO | `ViewMatricesCascadedUBO` | **YES, entirely** — its first 256 bytes are `mat4[4] cascadeViewProjectionMatrices`, refit to the camera frustum every tick | **FIXED** — one buffer + one descriptor set per frame-in-flight (not a dynamic-offset buffer) |
> | 2D / 3D view UBOs | `ViewMatrices2DUBO`, `ViewMatrices3DUBO` | **YES** — `WorldPosition` and `VelocityVector` change every frame while the camera moves, and they feed reflection, refraction and parallax occlusion mapping | **FIXED** — one buffer + one descriptor set per frame-in-flight, like the cascaded view |

> [!IMPORTANT]
> **Ask the TARGET for its region count, never the caller.** `RenderTarget::Abstract::createRenderTarget()`
> resolves it through the virtual `frameRegionCount()`, evaluated AFTER `onCreate()`. Both obvious
> sources are zero at the wrong moment: `Renderer::framesInFlight()` for every target created before
> `createRenderingSystem()`, and `SwapChain::imageCount()` before `onCreate()` builds the images —
> and `createRenderTarget()` is the FIRST thing `SwapChain::createOnHardware()` calls. Passing the
> count as a parameter looks like it fixes this and does not: the caller can only evaluate it too
> early. The failure is silent — *"Frame region overflow: asked for region #1 but only 1 exist"*, the
> update refused, the UBO left stale for the whole run, with zero VUID and a plausible image.
>
> **The region index must be bounded by the buffer's REAL region count**, never by a compile-time
> maximum that only sizes bookkeeping. Getting that wrong wrote past the end of the buffer and left
> every bind with a dynamic offset outside the allocation — while the frame still rendered plausibly.
> Only `VUID-vkCmdBindDescriptorSets-pDescriptorSets-01979` caught it.
>
> ⚠️ `ViewMatrices2DUBO.cpp` carries the rule in its own words — *"NEVER write a frame-varying value
> in here. This uniform buffer object is SINGLE-buffered … the raster of frame N can read what frame
> N±1 wrote."* — written after a TAA-jitter incident. The cascaded UBO is a byte-for-byte copy of
> that upload path **minus the warning**, and its whole payload is frame-varying.
>
> **Why the main image stays stable while shadows do not:** the main pass carries its matrices in
> **push constants**, baked into the command buffer at record time, so the GPU cannot see them
> change. CSM and cubemap shadows **cannot** — their matrices come from a UBO (indexed by
> `gl_ViewIndex` for a cubemap, by a pushed cascade index for a CSM), read at execution time. That is
> the entire asymmetry.

### Rules When Adding New GPU Data

1. **If you create a new SSBO/UBO that is written every frame**, create `framesInFlight()` copies.
2. **Index them by `m_currentFrameIndex`** (from `Renderer::currentFrameIndex()`).
3. **Update the descriptor set for the current frame only** — never write to all descriptor sets.
4. **Use `SceneMetaData::initializePerFrameBuffers()` as a reference** for the pattern.
5. **If in doubt, look at how `m_meshMetaDataSSBOs` works** — it was the fix for RT reflection flickering.

**Anti-pattern (causes flickering):**
```cpp
// WRONG: Single buffer overwritten every frame
m_ssbo->mapMemory();
memcpy(dst, data, size);
m_ssbo->unmapMemory();
```

**Correct pattern:**
```cpp
// RIGHT: Per-frame buffer, only the current frame's copy is written
m_ssbos[frameIndex]->mapMemory();
memcpy(dst, data, size);
m_ssbos[frameIndex]->unmapMemory();
```

### View Matrix State Index — Critical Trap

> [!CRITICAL]
> **Post-process effects that reconstruct world positions from the depth buffer MUST use
> the `readStateIndex` overloads of `viewMatrix()` and `projectionMatrix()`, NOT the
> default overloads.**

The `ViewMatricesInterface` provides two families of overloads:
- `viewMatrix(bool infinity, size_t viewIndex)` → reads `m_logicState` (current logic tick)
- `viewMatrix(uint32_t readStateIndex, bool infinity, size_t viewIndex)` → reads `m_renderState[readStateIndex]` (stable render snapshot)

The scene rendering pipeline uses `m_renderState[readStateIndex]` to compute the depth buffer.
If a post-process effect reconstructs world positions using `m_logicState` (the default overload),
the logic thread may have already advanced to the next tick. The matrices will disagree with the
depth buffer → **world position mismatch → flickering**.

**Fix pattern (used in RTR):**
```cpp
const auto readStateIndex = m_renderer->currentReadStateIndex();
const auto & viewMat = viewMatrices.viewMatrix(readStateIndex, false, 0);
const auto & projMat = viewMatrices.projectionMatrix(readStateIndex);
```

**Code references:**
- `Renderer.hpp:currentReadStateIndex()` — Getter for the stable read state index
- `Renderer.cpp:renderFrameWithPostProcessing()` — Captures `scene->preparedReadStateIndex()` before post-processing
- `Effects/Lighting/RTR.cpp:execute()` — Uses `readStateIndex` for NDC → world reconstruction
- `ViewMatrices3DUBO.cpp:viewMatrix()` — Two overloads: `m_logicState` vs `m_renderState[idx]`
