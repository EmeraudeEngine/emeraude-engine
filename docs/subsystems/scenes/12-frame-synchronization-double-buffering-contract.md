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

**The contract — the frame's view (`Graphics::FrameView`, 2026-11-15, owner decision).** An effect does not choose an
index any more. The truth is the scene's latch: `Scene::beginRenderFrame()` takes the published slot ONCE per frame
(`Scene::frameReadStateIndex()`). The renderer makes the view of the frame from the scene it renders —
`Renderer::frameView(scene)`: the main target's view matrices read at that slot — and the chain hands it to every effect
in `IndirectPostProcessEffect::FrameContext::frameView` (never null in the chain):

```cpp
const auto & frameView = *context.frameView;
const auto & viewMat = frameView.viewMatrix();           /* world to view */
const auto & rotation = frameView.infinityViewMatrix();  /* the view rotation alone (camera-relative math) */
const auto & projMat = frameView.projectionMatrix();     /* jittered: the one the depth was rasterized with */
```

It also gives `unjitteredProjectionMatrix()`, `position()`, `farPlane()`, the frame-history pair
`previousViewMatrix()` / `previousProjectionMatrix()`, and `readStateIndex()` — for what the logic publishes per slot
BESIDE the view (a light's published block: `LightGlints`; a cloud's render state: `VolumetricClouds::gatherClouds()`),
never to index the view matrices by hand.

**What it replaced.** `Renderer::m_currentReadStateIndex` and `Renderer::currentReadStateIndex()`: a COPY of the
scene's index, written in the middle of the frame (after `prepareRender()` and the clouds' shadow map, before the chain)
and read back by 16 files, each pairing it with `renderer().mainRenderTarget()->viewMatrices()`. Right only for what ran
after the line that wrote it:
- `Scene::recordCloudShadowMap()` ran BEFORE it: the clouds' shadow map was marched on the clouds published for the
  PREVIOUS frame's slot — a slot that frame had handed back, which the logic thread may be writing. It reads the
  scene's own `m_frameReadStateIndex` now.
- The direct path without post-processing never wrote it: `archiveStateAfterRendering()` archived an older slot. It
  archives `scene->frameReadStateIndex()`.
- Without a scene, the frame capture's camera metadata and `FrameDiagnostics::viewProjectionSparse` read the view at
  that stale index: they keep their defaults now (nothing was viewed).

**Rules.**
- An effect reads the camera through `context.frameView`: never `Renderer::mainRenderTarget()->viewMatrices()`, never a
  default (index-less) overload, never an index of its own.
- The renderer keeps no copy of a frame's state: a function that has the scene makes the frame's view from it
  (`Renderer::frameView()`), at any point of the frame, and gets the same one.
- A helper that needs the frame's view outside an effect takes a `const FrameView &` parameter
  (`GIDenoiser::updateFrameData()`, `PostProcessor::executeIndirectPostProcessEffects()`).

**Proof** (Linux, RTX 3070 Ti, 2026-11-15). Debug, the validation layer loaded, the binary before the change (two
launches) against the binary after, one capture per launch at a fixed delay, six scenes in which the thirteen effects
run (mean absolute difference / 255):
- `global-illumination` (RTGI, RTR, RTAO, the denoiser; static): 0.15 and 0.35 against the two "before" captures, which
  differ from each other by 0.28; no pixel further than 16.
- `light-and-shadow-debug` (lens flare, volumetric light and scattering, the traced contact shadows; static): ⚠️ a single
  capture depends on the PHASE of the temporal cycle — 16 captures of ONE run of the changed binary differ from each
  other by 0.015 to 0.11 (median 0.087). The captures at the phase of the "before" ones differ from them by 0.013 to
  0.019, the distance of two same-phase captures of one run (0.015). The two "before" captures had fallen on the same
  phase (0.001), which made three single captures "after" look different (0.06 to 0.09 from them, and from each other):
  a static scene's noise floor needs more than two launches, or several captures per launch.
- `city` (light glints, aerial perspective, the star field): 0.12 to 0.23, against 0.13 between the two "before" and
  0.17 between two "after".
- `reflexion-debug` on the ray-traced and on the screen-space lane (SSGI, SSR, the screen-space contact shadows) and
  `terrain` (the volumetric clouds and their shadow map): their content moves — the before / after distances (1.1 to
  2.4; 2.3 and 4.2) sit in the spread of two launches of one binary (0.75 and 2.1; 3.4). They say "nothing gross".
- In the 22 launches: the same `PostProcess.getStatus` text (the metering lines apart) and the same render statistics
  before and after, 0 VUID, 0 error line, exit 0.
- ThreadSanitizer (Debug, llvmpipe, window-less, the act-switch scenario on `default`, the traced chain running): 0
  report. ⚠️ No ThreadSanitizer run covers the clouds' shadow path: the software device crashes on `terrain` right after
  the scene starts (item `skinned-scene-crashes-the-software-vulkan-device`), with the binary before the change too. The
  defect of that path is established by the call order — the read came before the write in both frame functions — not
  by a measurement.
- GCC Release and Debug, clang, the Debug + TSan tree: 0 warning; `-Wdocumentation` on the 19 touched TUs.

**Code references:**
- `Graphics/FrameView.hpp` — the view of ONE frame
- `Graphics/Renderer.cpp:frameView()` — makes it from the scene's latch
- `Graphics/IndirectPostProcessEffect.hpp:FrameContext::frameView` — how the chain receives it
- `Scenes/Scene.rendering.cpp:beginRenderFrame()` — the ONE latch of the frame
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

What this does NOT cover: the pool worker that finishes a resource's load — the next section — and the members other
scenes still report (item `tsan-logic-render-instance-state-races`: an instance's frame index and flag word, the 2D
view matrices read in their logic state).

Proof (Linux, 2026-11-15). ThreadSanitizer — Debug, llvmpipe forced in a settings copy, the testbed driven through
the console (its active scene unloaded, reloaded, destroyed, rebuilt; the recipe: `docs/caution-points.md` § *TSan on a
machine whose display GPU runs the NVIDIA driver*):

| Scenario | Before | After |
|---|---|---|
| `default` | the lifetime, the effect chain, the ready flag and the light set reported in the 2026-11-13 run (RADV); the acceleration-structure read once in seven runs on llvmpipe | 0 engine report in 3 runs (and in the `default` part of the 3 runs below); one report per run inside OpenAL Soft, not ours — suppressed since (`resources/tsan_suppressions.txt`): 0 report and exit code 0 in 2 more runs |
| a switch to the skinned `animation-debug` | 4 reports on the skinning resources in each of 2 runs | 0 in each of 3 runs |
| `terrain` | — | 0 on the six in 1 run |

What those runs still reported was other work: a load completion handled on a pool worker and a geometry's bounding
volumes read while it loads (3 to 7 reports per run on `terrain` and `animation-debug`: the next section), a material
configured from the loaders' hook while its creation runs (1 per `animation-debug` run: scenes doc 19 § *onMeshLoaded
hands out resources still being created*). The software device dies on
the skinned scene and at the release of `terrain` (`skinned-scene-crashes-the-software-vulkan-device`): the reports are
written before, the exits of those runs are not clean.
Release, RTX 3070 Ti, the validation layer loaded, the binary before the change against the binary after: `default`
captured 12 s after the start — mean absolute difference 0.002 / 255, no pixel further than 16 (two launches of the SAME
binary 40 s apart differ by 0.33: the sky moves); `animation-debug` — the skinned characters animate (two captures 2 s
apart, looked at; 22 instances and 61 198 triangles in the view with both binaries); 0 VUID, 0 error line, exit 0 in the
five launches. A downstream consumer built against the change: 0 failing TU.
Peers' round 10 (2026-11-15, engine 9a7f4bba + testbed fa1aa4ea): macOS (Apple M6, MoltenVK 1.4.2, Apple clang with
`-Wdocumentation -Werror`) and Windows (RTX 3500 Ada, MSVC 19.44) build Release and Debug with 0 warning — no platform
file needed the five includes `Scenes/Scene.hpp` dropped — and run the same two checks as Linux: the active scene
switched eight times through the console (`default`, `animation-debug`, `light-and-shadow-debug`, unloaded kept and
destroyed), each scene giving the same render statistics at every return and the same as Linux (2 instances / 62
triangles; 22 / 61 198 with shadows 21 / 61 186; 7 / 673 958 with shadows 30 / 4 033 408), no "contract violation", 0
VUID, exit 0; and the skinned characters animated in two captures 2 s apart. ⚠️ One read on Windows gave shadows
32 / 4 033 412 for `light-and-shadow-debug`, 4 s after the switch from `animation-debug`, once in 17 reads (16 more,
three launch orders and the exact sequence twice: 30 / 4 033 408); not reproduced, not explained — two more instances
of two triangles each in the shadow lists of one frame.

### A resource's load reaches a scene object on the logic thread (2026-11-15, owner decisions)

`Resources::ResourceTrait` emits `LoadFinished` on the thread that completed the resource's last dependency — a pool
worker — and base `ObservableTrait::notify()` runs the observers there. A scene component must not act on it: its own
state and its entity's belong to the logic thread once the scene runs (and to the thread that builds the scene before).
The handlers of `Visual` and `MultipleVisuals` used to rewrite their bounding volumes there, then notify their entity,
whose `updateEntityProperties()` rewrote its flags, its bounding box and its collision shape while the logic thread
filed that entity in the octrees (ThreadSanitizer: 3 to 7 reports per run).

The contract:

1. **The handler records, nothing else.** `Component::Abstract::recordResourceLoaded()` stores an atomic flag; that is
   all an `onNotification(LoadFinished)` does on the loading thread.
2. **The logic thread consumes.** The component's `processLogics()` asks `consumeResourceLoaded()` (a plain load at
   every cycle, an exchange only when something was recorded), refreshes what it derives from the resource and notifies
   `ComponentContentModified`.
3. **The entity derives once, after its components.** A `ComponentContentModified` raised while
   `AbstractEntity::processLogics()` dispatches its components is deferred
   (`deferContentRefreshDuringDispatch()`, the rule `onContainerMove()` already followed): `updateEntityProperties()`
   runs once at the end of the dispatch, whatever the number of components that asked — from a component's
   `processLogics()` or from a setter another thread called meanwhile. Outside a dispatch nothing changes: the caller
   derives the properties itself.
4. **A constructor computes first and subscribes LAST**, then covers the load that finished in between: `publishedBefore`
   read first, the bounds computed, `observe()`, and `recordResourceLoaded()` when the renderable became published
   meanwhile. A notification must never reach an object whose constructor has not returned (`MultipleVisuals` did:
   both threads wrote its union).
5. **Nothing of a renderable is read before its publication.** A geometry caches its bounding volumes at the end of its
   own load, on the loading thread; a component reads its renderable's volumes, scale and flags through
   `publishedRenderable()` — the renderable once `isReadyForInstantiation()` (an atomic stored last), a null pointer
   before — and answers the null box until then. `MeshResource::boundingBox()` tests its own published level count,
   which is true as soon as the geometry is ATTACHED: not enough, the geometry may still be loading.

What it changes for an application: an entity gets its extents (visible, collidable, filed in the octrees) at the logic
cycle that FOLLOWS the end of the load — one cycle later at most — and not at all while the logic is paused; a
component built on a renderable not published yet contributes an empty extent until then (it did before too, or a box
half written).

Audit of the other `LoadFinished` handlers (owner: audit only): `CloudVolume::registerShape()` is written for the
loading thread (its published state is one atomic, the registration is idempotent and the bindless set is locked);
`SoundEmitter` and `AbstractLightEmitter` write plain members there — items
`sound-emitter-starts-its-sound-on-the-loader-thread` and
`light-emitter-registers-its-colour-projection-on-the-loader-thread` (by code reading: ThreadSanitizer reported neither
in one run each).

Proof (Linux, 2026-11-15). ThreadSanitizer, Debug, llvmpipe, the recipe of `docs/caution-points.md`:

| Scenario | Reports before | After |
|---|---|---|
| the active scene switched to the skinned `animation-debug` through the console (3 runs each) — a load completion handled on a pool worker | 3, 4, 4 | 0, 0, 0 |
| the same runs — a geometry's bounding volumes read while it loads | 3, 3, 1 | 0, 0, 0 |
| `terrain` (1 run each) — the union of a `MultipleVisuals` written by its constructor and by the pool worker | 3 | 0 (no report at all in the run) |

Release, RTX 3070 Ti, the validation layer loaded, the binary before the change against the binary after, the render
statistics 8 s and 14 s after the start: `forest` (instanced cells) view 261 batches / 1551 instances / 12 887 050
triangles and shadows 461 / 2741 / 20 729 956, `sprite` 4 / 100 003 / 200 016, `animation-debug` 22 instances /
61 198 triangles and shadows 21 / 61 186, `light-and-shadow-debug` 7 / 673 958 and 30 / 4 033 408 — IDENTICAL on both
sides (`particles` varies from one launch to the next by nature: 1926 against 1974 instances); 0 VUID, 0 error line, 0
component removed, exit 0 in the ten launches.
A downstream consumer built against the change (the include pass removed `Graphics/Renderer.hpp` from
`Component/MultipleVisuals.hpp` and `<mutex>` from `Scenes/AbstractEntity.hpp`): 0 failing TU.
