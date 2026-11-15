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

### Plain state shared by the logic and rendering threads under the SHARED scene access (2026-11-15, owner decisions)

The render state slots above carry what the logic thread PUBLISHES for a frame. Beside them, both threads hold the
active scene's access SHARED — the rendering thread for a whole frame, the logic thread for its cycle — so nothing
orders a plain write on one side against a read on the other. ThreadSanitizer found six such members (2026-11-13 and
2026-11-15); each one has ONE protocol now:

| State | Written by | Read by | Protocol |
|---|---|---|---|
| The scene's lifetime | the logic thread (`Scene::processLogics()`) | the rendering thread (the wind time in `Scene::updateVideoMemory()`, the animated textures of the bindless set) | ONE atomic count of logic cycles, `Scene::m_lifetimeCycles`; `lifetimeUS()` and `lifetimeMS()` are derived from it when read. They were two more plain words written one after the other: a reader could also see the microseconds of one cycle with the milliseconds of the previous one |
| "Does an enabled effect provide the indirect diffuse?" (the diffuse IBL leg is handed over to RTGI) | the rendering thread: it rebuilds the ordered effect chain, enables and disables its effects, stores the frame's acceleration structure | the logic thread (`Scene::updateIBLDiffuseOwnership()`) | the rendering thread PUBLISHES its answer once per frame — `Scene::publishIndirectDiffuseProvider()`, called by `Renderer::renderFrame()` once the frame's effects are final; the logic thread reads `m_indirectDiffuseProvided`, never the chain (`PostProcessStack::hasEnabledIndirectDiffuseProvider()` is a rendering-thread function). One frame late at most |
| A renderable is ready for instantiation | the thread that finishes the load (a pool worker) | the logic and rendering threads | `Renderable::Abstract::m_readyForInstantiation`, an atomic of its own, stored LAST (release) and loaded with acquire. It was bit 0 of the renderable's plain flag word, whose other bits are written before the renderable is published |
| The light set is enabled | the logic thread (the background's lighting), the main thread (the console) | the rendering thread (every frame, the shader generation) | `LightSet::m_enabled`, atomic, relaxed: a switch on its own |
| A skinned instance's buffer and descriptor sets exist | the rendering thread (`RenderableInstance::Abstract::createSkinningResources()`, on the instance's first pass) | the logic thread, every cycle (`hasSkinningResources()`, `updateSkinningMatrices()`) | `m_skinningResourcesPublished`, stored LAST (release) once the buffer, its pool and every descriptor set are built, never cleared before the destructor. The logic thread tests it — never the vector being filled nor the buffer pointer, through which it could write the joint matrices into a half-built object |

Choosing the protocol of a new datum that both threads touch:

1. **Several values that must agree for one frame** → a render state slot (this document).
2. **One word that stands alone** → an atomic; relaxed when nothing else is read on the strength of it.
3. **"This object is built"** → a publication flag stored LAST with release, loaded with acquire; the other thread
   tests the flag, never the object's own members.
4. **A question whose answer lives in structures ONE thread owns** → that thread publishes the answer; the other reads
   the answer, one cycle late at most. Never walk the other thread's containers "because the worst case is one frame".
5. A comment that calls a race benign is a defect (Ave Robustus II): the indirect-diffuse question carried one.

What this does NOT cover: a load completion handled on a pool worker (`ResourceTrait::LoadFinished` → the scene
components' handlers → `AbstractEntity::updateEntityProperties()`), item
`load-finished-runs-scene-handlers-on-a-pool-thread`.

Proof (Linux, 2026-11-15). ThreadSanitizer — Debug, llvmpipe forced in a settings copy, the testbed driven through
the console (its active scene unloaded, reloaded, destroyed, rebuilt; the recipe: `docs/caution-points.md` § *TSan on a
machine whose display GPU runs the NVIDIA driver*):

| Scenario | Before | After |
|---|---|---|
| `default` | the lifetime, the effect chain, the ready flag and the light set reported in the 2026-11-13 run (RADV); the acceleration-structure read once in seven runs on llvmpipe | 0 engine report in 3 runs (and in the `default` part of the 3 runs below); one report per run inside OpenAL Soft, not ours — suppressed since (`resources/tsan_suppressions.txt`): 0 report and exit code 0 in 2 more runs |
| a switch to the skinned `animation-debug` | 4 reports on the skinning resources in each of 2 runs | 0 in each of 3 runs |
| `terrain` | — | 0 on the six in 1 run |

What those runs still report is other work, each in its item: a load completion handled on a pool worker and a
geometry's bounding volumes read while it loads (`load-finished-runs-scene-handlers-on-a-pool-thread`: 3 to 7 reports
per run on `terrain` and `animation-debug`), a material configured from the loaders' hook while its creation runs
(`mesh-loaded-hook-configures-materials-still-being-created`: 1 per `animation-debug` run). The software device dies on
the skinned scene and at the release of `terrain` (`skinned-scene-crashes-the-software-vulkan-device`): the reports are
written before, the exits of those runs are not clean.
Release, RTX 3070 Ti, the validation layer loaded, the binary before the change against the binary after: `default`
captured 12 s after the start — mean absolute difference 0.002 / 255, no pixel further than 16 (two launches of the SAME
binary 40 s apart differ by 0.33: the sky moves); `animation-debug` — the skinned characters animate (two captures 2 s
apart, looked at; 22 instances and 61 198 triangles in the view with both binaries); 0 VUID, 0 error line, exit 0 in the
five launches. A downstream consumer built against the change: 0 failing TU.
