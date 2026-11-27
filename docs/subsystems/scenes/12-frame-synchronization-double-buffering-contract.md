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
  report. The clouds' shadow path: `terrain` under ThreadSanitizer on 2026-11-16 (engine 82c7efe0; the software device
  with the two traced effects that kill it disabled, `docs/caution-points.md` § *The software device gives a ray-query
  hit the identity of another instance*) — the cloud shadow map created and drawn, 40 s, a console shutdown: 0 report,
  exit 0. ⚠️ The binary BEFORE the change was never run there (the device died on `terrain` until that recipe): the
  defect of that path is established by the call order — the read came before the write in both frame functions — not
  by a before / after.
- GCC Release and Debug, clang, the Debug + TSan tree: 0 warning; `-Wdocumentation` on the 19 touched TUs.
- Peers' round 12 (2026-11-16, engine 244176b0 + testbed 29a37715 — this change and the chain executor's split,
  `docs/post-processing-pipeline.md` § 3d): macOS (Apple M6, MoltenVK 1.4.2, Apple clang with `-Wdocumentation -Werror`)
  and Windows (RTX 3500 Ada, MSVC 19.44) build Release and Debug with 0 warning — nothing on `FrameView` returned by
  value against a forward declaration, on the nested `ChainRun` defined out of its header, on the designated-initializer
  return. Window-less on both. `light-and-shadow-debug`, `city` and the scene switches give Linux's statistics on both;
  `terrain` gives them on Windows and differs by a few units on macOS (one triangle more in the view, four instances
  fewer in the shadows, stable inside the run — Linux gives the same counts windowed and window-less). The status names
  the expected effects (the screen-space lane on macOS); 0 VUID, exit 0, no "contract violation". By eye the glints
  sit on the lamp heads of `city` on both; the starting frame of `light-and-shadow-debug` holds no light source, and
  the clouds' shadows of `terrain` could not be told from the ground in two captures 10 s apart: those two checks
  conclude nothing.

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
| A renderable is ready for instantiation | the thread that finishes the load (a pool worker) | the logic and rendering threads | `Renderable::Abstract::m_readyForInstantiation`, an atomic of its own, stored LAST (release) and loaded with acquire. It was bit 0 of the renderable's plain flag word |
| What a renderable IS and what its load declares | the constructor (the flag word: `IsSprite`); the creating thread, BEFORE the publication (`enableVegetationWind()`, `setVegetationFoliageLayer()` inside the creation function; `setUprightBillboard()` inside the sprite's load) | a component built on a renderable still loading (`isSprite()`: the layout of its instance buffer); the rendering thread once it is ready (the wind, the upright billboard) | The flag word is the resource's own and a renderable's bits are CONSTRUCTOR-ONLY: never written afterwards, readable at any time. A declared property is a plain member, written before `setReadyForInstantiation(true)` and read only on a ready renderable (2026-11-17, owner decision). The loader used to write the upright bit into the word a `MultipleVisuals` was reading `isSprite()` from: ThreadSanitizer, `sprite`, 4 runs of 6 → 0 of 8. Peers' round 19 (engine 30f486f0, macOS and Windows): Release and Debug 0 warning, the same totals on `sprite` and `tree-generator`, the flames upright, the foliage moving (1.0 % and 1.1 % of the pixels between two captures; Linux 1.2 %) |
| The light set is enabled | the logic thread (the background's lighting), the main thread (the console) | the rendering thread (every frame, the shader generation) | `LightSet::m_enabled`, atomic, relaxed: a switch on its own |
| A skinned instance's buffer and descriptor sets exist | the rendering thread (`RenderableInstance::Abstract::createSkinningResources()`, on the instance's first pass) | the logic thread, every cycle (`hasSkinningResources()`, `updateSkinningMatrices()`) | `m_skinningResourcesPublished`, stored LAST (release) once the buffer, its pool and every descriptor set are built, never cleared before the destructor. The logic thread tests it — never the vector being filled nor the buffer pointer, through which it could write the joint matrices into a half-built object |
| An instance's animation time (its flipbook frame derives from it, per layer) | the logic thread (`Visual`, `MultipleVisuals`, `ParticlesEmitter::processLogics()` → `updateFrameIndex()`) | the rendering thread, at every draw of the instance (`frameIndexFor()`: the shadow maps, the colour passes) | a RENDER STATE SLOT (2026-11-16): the component's `publishStateForRendering()` calls `RenderableInstance::Abstract::publishAnimationTime(slot)`, the draws read `frameIndexFor(passContext, pushContext)` (the pass's slot, the draw's layer) — every pass of a frame draws the same flipbook frame. The logic value itself is the logic thread's alone |
| "The instanced positions are uploaded" | whoever changes the local data of a `RenderableInstance::Multiple` (the logic thread), and the upload | the upload | `Multiple::m_positionsSynchronized`, a member GUARDED by `m_localDataAccess` (the thread-safety analysis checks it). It was bit 0 of the instance's flag word, cleared at every logic update while the rendering thread read the word's configuration bits (2026-11-16) |
| An instance is broken | the thread that meets the failure (the rendering thread for a program, the logic thread for instance data) | both | `RenderableInstance::Abstract::m_broken`, atomic, relaxed: a switch on its own (`setBroken(message)` traces once, on the exchange). It was bit 1 of the flag word |
| An instance carries an extra transformation matrix | the logic thread (`publishTransformationMatrix()`, at every publication) or the scene's construction (`setTransformationMatrix()`) | the rendering thread (`applyLocalTransformation()`) | `m_transformationMatrixApplies`, atomic, stored LAST with release after the slot, loaded with acquire; never cleared. It was bit 11 of the flag word. ⚠️ The flag word of an instance now holds CONFIGURATION only: a bit that changes while the scene runs does not belong there |
| The view (its position, its matrices, its near and far planes) as the rendering side reads it outside the chain | the logic thread (`Camera` → `updateViewCoordinates()`; `DirectionalLight::updateCascades()` rewrites a cascaded shadow map's planes at every cycle) | the rendering thread: the sprite billboards of the acceleration-structure rebuild, the temporal-history mask, the editor's gizmos, the debug compass; the view distance of the two render list builders, the selection outline's planes, `RTGI`'s sky distance | the FRAME'S VIEW, like the chain's effects: `position(m_frameReadStateIndex)` for the rebuild, a `Graphics::FrameView` for the others (`Editor::Manager::render(commandBuffer, readStateIndex)`, `Gizmo::Abstract::render(commandBuffer, frameView)`, `Debug::Compass::render(commandBuffer, frameView)`, `SelectionOutline::recordComposite(…, frameView, …)`). The planes: `ViewMatricesInterface::nearPlane()` / `farPlane(readStateIndex)`, `FrameView::nearPlane()` / `farPlane()` — the frame's view itself answered the far plane from the logic state. ⚠️ `RenderTarget::Abstract::viewDistance()` and its six implementations are REMOVED (owner decision): the view distance of a frame is `viewMatrices().farPlane(readStateIndex)`, one source for every target. They read the view's LOGIC state (2026-11-16) |
| The scene's ambient (chromaticity, effective illuminance) as a traced effect adds it at its hits | the logic thread (`LightSet::setAmbientLight*()`, `Scene::applyBackgroundLightingNow()`) | the rendering thread (`RTR`'s frame block, the irradiance probe volume's inputs, `FrameContext::ambientIlluminance`) | the FRAME'S VIEW again: `Scene::refreshAmbientLightProperties()` already gives the ambient to the view matrices, and their published slot is what the raster of the frame reads — `FrameView::ambientChromaticity()` / `ambientIlluminance()` (`ViewMatricesInterface::ambient*(readStateIndex)`). The traced value is the raster's by construction; nothing on the rendering side reads the light set's ambient nor `Scene::m_IBLAmbientEnabled` any more (2026-11-16, owner decision over a slot of the light set's own) |
| What a frame CULLS an entity on (its world extent) | the logic thread: the entity's coordinates (physics, a vehicle, an animation), its render box (`updateEntityProperties()`; and, since 2026-11-22, `refreshRenderBoundaries()` each time a component's own box changes without its content — `ComponentBoundariesModified`: a beam or a path whose points moved, a skinned visual at every pose; the scene then files the entity again in its rendering octree, `takeRenderBoxChange()`, because a frame gathers its candidates sector by sector BEFORE it tests an extent), its collision model (`refreshCollisionBoundaries()` on the same notification) | the rendering thread: `Scene::populateRenderLists()` and `populateShadowCastingRenderList()` — they called `isVisibleTo()`, which read all three live | a RENDER STATE SLOT (2026-11-16, owner decision): `AbstractEntity::publishRenderExtent(slot)`, from `publishStateForRendering()` right after the coordinates of the slot, writes the world box of what the components draw (else the collision model's, else nothing: the published position decides); the two list builders test their volume against `publishedRenderExtent(readStateIndex)` (`isSeenBy()` in `Scene.rendering.cpp`). An entity that neither moved nor rebuilt its render box copies the extent of its previous publication. `LocatableInterface::isVisibleTo()` stays, for the LOGIC thread |
| Whether an entity draws anything, as a frame asks it before it visits the entity | the logic side: `AbstractEntity::updateEntityProperties()` rewrites the entity's renderable flag each time its components change — RESET first, set again when it finds a drawing component | the rendering thread: the two list builders (shadow casters, the ray-traced list, the raster lists, the particle emitters), the tangent-space pass | a RENDER STATE SLOT (2026-11-16, owner decision): the answer is the `None` kind of the extent the entity publishes (`RenderExtentKind::None`, `AbstractEntity::isPublishedRenderable(readStateIndex)`), written at the end of the cycle with the coordinates and the extent of the same state. The live flag (`isRenderable()`) is the logic side's: a frame that read it between the reset and the set left a DRAWN entity out, shadow included. A slot never written says `None`: an entity created during a cycle is not drawn by a frame that latched an earlier state (its coordinates of that state are the default ones, the world origin). `Scene::forEachRenderableInstance()` — no frame's state: the console, a new render target — visits every entity instead of testing the flag |
| An instance's CONFIGURATION: its flag word (lighting, shadow casting and receiving, ray tracing, depth test and write, the infinity view, the tangent-space display…), its draw range, its shadow distance and shadow level-of-detail bias | whoever builds the component; the console (main thread) | the rendering thread: the list builders at every frame, the program generation | a RULE, no mechanism (2026-11-16, owner decision): written BEFORE the component is linked to its entity — the `setup()` of the component builder, the frame cannot reach the instance yet — or under the scene's EXCLUSIVE access (`Scenes::Manager::withExclusiveActiveScene()`: the component console adapters since 2026-09-27, and the scene service's `enableTBN`, which took a mere snapshot of the scene). Never on a linked instance otherwise. `Toolkit::generateHLODProxy()` configured its instance after the link: moved into the setup (`docs/component-builder-pattern.md`) |
| The renderer's switches written by another thread: the tangent-space display, the grab pass forced on, the shadow maps and the render-to-textures | a scene being built while another one may be drawn (`enableTBNSpaceRendering()`, `enableGrabPass()`), the console, the main thread (`toggleOffscreenRendering()`, Core's key) | the rendering thread, at every frame | four atomics of `Graphics::Renderer`, relaxed: each a switch on its own. The renderer's, not a scene's — the exclusive access of the active scene does not order a scene under construction against the frame. The frame reads the shadow-map and render-to-texture switches ONCE (the video memory it updates for a target is that of a target it renders) |
| A background's celestial bodies (`AbstractBackground::stars()`) | the thread that LOADS the background, a pool worker (`parsePhotometry()` → `addStar()`, a vector which reallocates) | the logic thread, each cycle, and the rendering thread, each frame (`Scene::environmentStarMask()`); the logic thread once the load ended (`applyBackgroundLightingNow()`) | The resource's own publication: the list is complete and never written again once `isLoaded()` answers true (an atomic status, stored after the load). `environmentStarMask()` answers "no mask" for a background still loading (2026-11-27); the bake made without a mask in between is redone, the mask being part of the bake's identity. Before: a walk of the vector while it reallocated — ThreadSanitizer, heap-use-after-free, a `SEGV` (2026-11-26) |
| A background's luminance | the loading thread (`parsePhotometry()`), then the logic thread (`SkyFollowsSun`) or the console (`setLuminance()`) | the logic thread (`Scene::refreshAmbientLightProperties()`) and the rendering thread (the renderer's sky luminance) | `AbstractBackground::m_luminance`, atomic, relaxed: one word on its own (2026-11-27) |

Choosing the protocol of a new datum that both threads touch:

1. **Several values that must agree for one frame** → a render state slot (this document).
2. **One word that stands alone** → an atomic; relaxed when nothing else is read on the strength of it.
3. **"This object is built"** → a publication flag stored LAST with release, loaded with acquire; the other thread
   tests the flag, never the object's own members.
4. **A question whose answer lives in structures ONE thread owns** → that thread publishes the answer; the other reads
   the answer, one cycle late at most. Never walk the other thread's containers "because the worst case is one frame".
5. A comment that calls a race benign is a defect (Ave Robustus II): the indirect-diffuse question carried one.

Proof of the background's two rows (2026-11-27, ThreadSanitizer, Linux, Debug, the software device, window-less,
`default`): after the change, 0 report in three runs — two with the process held on two cores and ten
`setBackground(name, true)` of skies that declare stars sent in a burst, one at full core count with the switches
4 s apart.
⚠️ Not a before / after: the binary BEFORE the change gave 0 report on the stars in eight runs of that day (plain
startup, with and without a video recording, the two scenarios above), against two runs of two on 2026-11-26. The
difference is the music: until 2026-11-27 the playlist decoded its 32 tracks on the pool at startup, which delayed
the sky's load until the scene ran; with a playlist of names the pool is free and the load ends first. The fault
was in the code either way (a vector written by one thread, walked by two others, no order between them); the fix
is proven by the reading and by the absence of a report after. The two-core run of the binary before reported another
member of the family instead: item `scene-reads-a-background-texture-while-it-is-created`.

What this does NOT cover: the pool worker that finishes a resource's load — the next section. The members other
scenes reported after the first six (an instance's animation time and flag word, the view's logic state read on the
rendering side, the ambient, the extent a frame culls an entity on) are the seven rows that follow the first six.
The last three rows (2026-11-16) were never reported: found by READING — a configuration written on a LIVE object from
another thread (the console's tangent-space toggle, the renderer's switches), and an entity's renderable flag. Their
proof and the peers' round 15 close this section, and the item that carried all of them
(`tsan-logic-render-instance-state-races`, closed on 2026-11-17). What the main thread did to a running scene through
the scene service's other console commands is the last proof of this section (2026-11-17, closed after the peers'
round 16).

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
hands out resources still being created*). The software device died on
the skinned scene and at the release of `terrain` in those runs — a driver limit, avoided since by two settings keys
(`docs/caution-points.md` § *The software device gives a ray-query hit the identity of another instance*): the reports
are written before, the exits of those runs are not clean.
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

**The last six rows (2026-11-16, owner decisions) — proof.** Linux, engine 968dfc4a + the change.

ThreadSanitizer (Debug, window-less, the software device with the two traced effects that kill it disabled —
`docs/caution-points.md` § *The software device gives a ray-query hit the identity of another instance* —, a console
shutdown):

| Scenario | Before | After |
|---|---|---|
| `light-and-shadow-debug` with projections | 1 report: the instance's animation time | 0, exit 0 |
| `game-logic` | 4 reports: the instance's flag word (2), the view's logic position in the acceleration-structure rebuild (2) | 0, exit 0 |
| `global-illumination` with a projection | 0 | 0 |
| the act-switch scenario on `default`, 2 runs (the ambient was reported once in four runs on 2026-11-15) | — | 0 and 0 |

Hardware device (RTX 3070 Ti, Release, window-less, the validation layer loaded; the binary before against the binary
after: 16 launches in two series, 6 more alternated on `reflexion-debug`, every one exit 0, 0 VUID, 0 error line). The
render statistics, the GPU timing zones and the post-process status are the same in every launch of
`global-illumination` (19 / 266 | 30 / 15340), `light-and-shadow-debug` with projections (7 / 673958 | 30 / 4033408),
`reflexion-debug` (9 / 70529 | 15 / 153407) and `animation-debug` (22 / 61198 | 21 / 61186, metered 1822.2 nits by
both). `global-illumination` meters 109.9 and 108.6 nits before, 108.6 and 109.0 after; the closest captures of the two
binaries are 0.08 / 255 apart, against 0.60 between two launches of the binary before. `light-and-shadow-debug`: 0.004
against 0.005. `reflexion-debug` moves: five launches each, 1307 to 1349 nits before, 1318 to 1370 after, the same
1317.6 reached by both — no step. `game-logic` moves too (its counts change from launch to launch): the same three
emitters play among 157. The final code, after the analysers' form corrections: one launch of `global-illumination`
(109.0 nits) and one of `light-and-shadow-debug`, inside both series.

**The sweep (the same day, the final code): the testbed's 40 scenes under ThreadSanitizer**, one run each, 20 s, a
console shutdown. Every scene starts and reaches a clean engine exit; 33 report nothing. What the 7 others report is
NOT in the table above:
- `node-relation`, `collision-debug`, `beams`, `paths`, `city`, `citadel` — the CULLING reads an entity's logic state:
  fixed the same day (the extent row, its proof below).
- `sky-lab`, `city` — a sky material's atmosphere read while its creation function sets it on a pool worker: item
  `sky-material-atmosphere-read-while-its-creation-sets-it`.
- `city`, the testbed's own code — an image's average colour read while the image loads (the testbed's item
  `city-reads-an-image-average-colour-while-it-loads`).
- `sponza` — the software device dies, no engine report before it: its alpha-tested foliage needs a third settings
  key (caution-points, the same section).

**The extent a frame culls on (2026-11-16, owner decision) — proof.** Linux, engine 93486951 + the change.
ThreadSanitizer, the same recipe, one run of each scene the sweep had found: `node-relation` 6 reports → 0,
`collision-debug` 7 → 0, `beams` 1 → 0, `paths` 1 → 0, `citadel` 3 → 0, exit 0; `city` 10 → 2, neither on the culling
(the testbed's image read while it loads, and the view's far plane — the proof after this one). Hardware device (RTX 3070 Ti, Release, the
validation layer loaded, the binary before against the binary after, 0 VUID): the SAME render statistics — what the
culling keeps — on `terrain` (60611 instances / 31958641 triangles in the view, 8776 / 16147070 in the shadow lists),
`sponza` (290 / 10494160 | 454 / 12833689), `light-and-shadow-debug`, `global-illumination` and `animation-debug`.

The cost, measured with a temporary timer around the entities' publication (Release, i9-14900K, four launches of each
binary, the mean of 600 logic cycles): `terrain`, 13072 static entities — 4.5 to 5.1 ms per logic cycle before, 5.1 to
5.5 ms after; `city`, 721 static entities and its nodes — 62 to 79 µs before, 70 to 102 µs after. ⚠️ Computing the
extent of EVERY entity at EVERY publication cost 5.9 to 6.6 ms on `terrain` (about 100 ns per entity): hence the copy
of the previous publication's extent when nothing changed. The rendering thread no longer computes a box per entity
and per target; that gain was not measured. ⚠️ The publication itself — 4.7 ms per logic cycle on `terrain` before
this change, every static entity published again at every cycle — is the larger cost (item
`scene-publication-republishes-every-entity-at-every-cycle`).

**The view's planes (2026-11-16, owner decisions) — proof, and the second sweep.** Linux, engine f10bc200 + the
change. ThreadSanitizer, the same recipe: `city` no longer reports the cascaded shadow map's far plane (1 report left,
the testbed's image). Hardware device (RTX 3070 Ti, Release, the validation layer loaded, 0 VUID): the same render
statistics as before on `terrain`, `sponza`, `light-and-shadow-debug` and `global-illumination` — the view distance
the two list builders cull with is the same value, read from the frame's render state.

The sweep again, the final code, 39 scenes (`sponza` left out: the software device dies there): every scene reaches a
clean engine exit, 36 report nothing, and NO report is left between the logic and the rendering threads. The three
others are loading threads: `city` (the testbed's image), `sky-lab` (the sky material's atmosphere, item
`sky-material-atmosphere-read-while-its-creation-sets-it`) and, once, `sprite` — a renderable's flag word written by its
loader while a component built on it reads `isSprite()` (fixed 2026-11-17: the row "What a renderable IS" above).
⚠️ The sky report of `city` and the one of `sprite` each appeared in one sweep of two: a loading race shows when the
threads meet, a clean run proves nothing about it.

**Peers' round 14** (2026-11-16, engine 1d35ce56 + base e1249de + testbed 7b2e3d09: the seven rows above, the
skinning fix of the base, the interfaces that changed). macOS (Apple M6, MoltenVK 1.4.2, Apple clang with
`-Wdocumentation -Werror` and `-Wthread-safety`) and Windows (RTX 3500 Ada, MSVC 19.44) build Release and Debug with 0
warning — no platform file called a removed name nor relied on an include that left —, pass the base suite with its
new test, and run six scenes window-less with the validation layer loaded: exit 0, 0 VUID, 0 error line in the 21
launches. The same render statistics as Linux on `animation-debug` (22 / 61198 | 21 / 61186) and
`light-and-shadow-debug` with projections (7 / 673958 | 30 / 4033408); `terrain` culled the same twice in a row on
each (Windows 60611 / 31958641 | 8776 / 16147070 as Linux; macOS its own round 12 values). `global-illumination` with
a projection: macOS, screen-space lane, 14.4 to 14.6 nits over three launches (14.4 to 14.7 in round 13); Windows,
ray-traced, 97.7 to 102.4 nits over six (98.5 to 100.4 over the six of round 13). That wider spread is the machine's
launch-to-launch noise, not the change: twelve alternated launches there give 97.7 to 101.2 nits (median 100.4) with
an OLDER binary and 100.0 to 102.8 (median 101.4) with this one, the low extreme reached by each in turn — ⚠️ the
older binary was the only one the peer still held, engine 31d39b6a of five rounds earlier, a Release tree built with
the fault-injection option (no injection switch passed). Linux, the same configuration, six alternated launches of
each binary: 105.2 to 106.9 nits before, 104.8 to 106.5 after. ⚠️ Not verified by the peers: entities MOVING while they are culled — `node-relation`'s default framing shows
none of its moving objects (both peers); that part rests on Linux's ThreadSanitizer runs and statistics.

**A configuration written on a live object, and whether an entity draws (2026-11-16, owner decisions) — proof.**
Linux, engine ebe81919 + the change.

ThreadSanitizer (the same recipe, `default`, a console scenario: `enableTBN` on and off 16 times, `addTexturedShape`
three times in between): before, 1 report — `Renderer::enableTBNSpaceRendering()` on the main thread against the
frame's read of the switch — and exit code 66; after, 0 report and exit 0, the same replies (1 instance toggled and 1
skipped for its geometry without tangent space, 4 and 1 once the three shapes exist).
⚠️ The flag words of the instances were written 16 times while frames read them and were NOT reported before: when
both threads take a common mutex a little later (the scene's entity lists here), ThreadSanitizer orders the two
accesses after the fact. A clean run proves nothing about a window of a few microseconds — these three rows rest on
the reading, and on the widened window below.

The entity's flag, with the window WIDENED (a temporary pause of 400 ms between the reset and the set in
`updateEntityProperties()` for the node `Fox` of `animation-debug`; Release, RTX 3070 Ti, window-less, the validation
layer loaded, 0 VUID, exit 0):
- the logic thread recomputes the entity's properties through its deferred path — what the end of a resource's load
  does — 4 times in 24 s: the LIVE flag said "draws nothing" in 561 raster list builds (the test of the code before
  left the entity out each time), the frame's render state said so in NONE;
- ⚠️ the same recomputation started from the MAIN thread by a console command that takes no access
  (`attachMicrophone(Fox, …)`) still loses the entity: 153 and 161 list builds before, 150 and 161 after. The logic
  thread's publication reads the half-written flag and publishes it. The slot protects a frame from the logic thread's
  own work, not from a third thread that changes the entity in the middle of a cycle — fixed on 2026-11-17 by putting
  that thread's commands under the exclusive access (the last proof of this section: 0 list build).

Outside any pause the two tests decide the same: counted at every entity test of the three list builders, the live
flag and the frame's state agree on 3 000 000 tests of `game-logic` in 25 s (0 disagreement; none either on `particles`
and `animation-debug`).

Hardware device (RTX 3070 Ti, Release, window-less, the validation layer loaded; the binary before against the binary
after, one launch after the other, the render statistics read 10 s and 14 s after the start — 30 s and 36 s for the
last four; every launch exit 0, 0 VUID, 0 error line). The SAME statistics on the ten scenes that do not move:
`default` (2 / 62 | 0 / 0), `animation-debug` (22 / 61198 | 21 / 61186), `light-and-shadow-debug` (7 / 673958 |
30 / 4033408), `node-relation` (4 / 86), `sprite` (100003 / 200016), `beams` (8 / 1142), `terrain` (60611 / 31958641 |
8776 / 16147070), `sponza` (290 / 10494160 | 454 / 12833689), `global-illumination` (19 / 266 | 30 / 15340),
`reflexion-debug` (9 / 70529 | 15 / 153407). Two scenes move by themselves: `particles` (1939 to 1946 instances with
both binaries) and `game-logic`, whose counts change from launch to launch — five launches of each binary: 266 to 285
instances in the view and 93 to 111 shadow casters before, 265 to 291 and 87 to 116 after; hence the count above.
The tangent-space display, `geometry-loader`, the command sent to both binaries: the same replies, about 84 000 pixels
changed by the display with each (looked at: the vectors cover the model), back to the image of before once disabled.
The final code (the renderer's three other switches made atomic): the same statistics again on `default`,
`light-and-shadow-debug`, `animation-debug` and `liminal` (6 / 160 | 126 / 4457264, a scene that forces the grab pass
on), and the console scenario under ThreadSanitizer once more: 0 report, exit 0. `city` — the scene whose far proxies
(`Toolkit::generateHLODProxy()`) and detailed buildings are now configured in the builder's setup — gives the same
statistics with both binaries, two launches each: 1625 instances / 2760675 triangles in the view.
⚠️ `water-world` cannot serve such a comparison: its palms are planted at random (2520 to 2596 instances from launch
to launch with either binary), and in some launches 1050 MORE are in the lists (3596 to 3642, twice the shadow
batches) — with the binary of BEFORE too. NOT a defect (traced on 2026-11-17: the same view, every tile publishing a
valid render box in both cases): the culling is per entity, and the 270 m tile beside the observer, 190 m away, passes
the frustum test or not with that run's draw of palms; a tile whose box touches the view submits all its instances.
It looked like a regression of this change for two launches: a statistics A/B needs a scene that does not draw at
random (the testbed's water-world document carries the numbers).

**Peers' round 15** (2026-11-17, engine 59ddb472 + testbed f6a7f23e: the three rows above). macOS (Apple M6, MoltenVK
1.4.2, Apple clang with `-Wdocumentation -Werror`, `-Wswitch` and `-Wthread-safety` silent) and Windows (RTX 3500 Ada,
MSVC 19.44, no C4062 / C4715 on the switch without default) build Release and Debug with 0 warning — no platform file
touches the four renderer members — and run six scenes window-less with the validation layer loaded: exit 0, 0 VUID,
0 error line in the twelve launches. The same statistics as Linux on `animation-debug` (22 / 61198 | 21 / 61186),
`light-and-shadow-debug` with projections (7 / 673958 | 30 / 4033408), `liminal` (6 / 160 | 126 / 4457264) and `city`
(1625 / 2760675 in the view, twice on each; night on both: no shadow caster); `terrain` culled as in their round 14,
twice in a row. The tangent-space toggle on `geometry-loader`: Windows draws the vectors over the model and removes
them (1 instance toggled, 1 skipped for its geometry without tangent space); macOS refuses the display — its device
has no geometry stage — and disables on the two instances; twenty commands in a row answer in about 0.01 s each on
both: the exclusive access costs the console nothing visible.

**The scene service's console commands under the exclusive access (2026-11-17, owner decisions) — proof.** Linux,
engine 79dbf752 + the change (the rule and its three helpers: scenes doc 07 § console adapters). These commands ran on
the MAIN thread with a mere snapshot of the active scene and read or wrote it while the logic and rendering threads
ran — the third thread of the warning above.

ThreadSanitizer (the same recipe, `animation-debug`, one console scenario of 88 commands: a node created, moved
twelve times, turned and destroyed, the animated node `Fox` moved twelve times and read, a microphone attached to it,
two textured shapes, the compass on and off, the highlight style, a ground, a glTF file, the target family, a physics
recording, the statistics twelve times):

| | Before | After |
|---|---|---|
| reports | 9, exit code 66 | 0, exit 0 |
| `toggleCompass` | 3: `Debug::Compass::destroy()` against the frame's `Compass::render()` | — |
| `setHighlightStyle` | 3: `SelectionOutline::setColor()` / `setWidth()` against the frame's `recordComposite()` | — |
| `setGround` | 2: `Scene::registerSceneVisualComponents()` against the shadow list builder | — |
| `getNodePhysics` | 1: `Scene::cycle()` read while `Scene::processLogics()` writes it | — |

The window widened again (the 400 ms pause between the two writes of the renderable flag, `animation-debug`, Release,
RTX 3070 Ti): `attachMicrophone(Fox, …)` left the entity out of 150 to 161 raster list builds per command before; of
NONE after — the frames and the logic wait for the command, they no longer run inside it.

What the operator sees (Release, RTX 3070 Ti, window-less, the validation layer loaded, the binary before against the
binary after, one scenario of 76 commands on `animation-debug` and one of 20 on `sky-lab`; exit 0, 0 VUID): the SAME
replies once numbers and addresses are masked — errors included (an unknown node, a file that does not exist, a
node without a character controller) — but the two refusals below and the entity list that follows from them, and the
same latency: 0.021 s per command before, 0.022 s after (the client's own start), the slowest 0.05 s. Under the exclusive access a command waits for the frame and the logic cycle in
flight; at this frame rate that is not measurable (under ThreadSanitizer on the software device, where a frame takes
0.3 s, each command takes 0.3 to 0.4 s).

Left as they were, on purpose (the code says why at each): the four physics-recording commands, `writeImposterAtlases`
and the five commands of the physical sky — what they touch guards itself; `getActiveSceneName`, `targetActiveScene`.
Two refusals added at this trust boundary: `addMesh` and `setBackground` refuse a name their container does not know
(owner decision, the one of 2026-09-27 for `createScene`'s skybox: `getResource()` serves the default resource for an
unknown name, so `addMesh(NoSuchMesh, …)` answered "placed" and `setBackground(NoSuchSky)` "Background set").
`addTexturedShape` also guards its own input against a number that is not finite — ⚠️ a guard no operator reaches:
the console's argument parser refuses `nan`, `inf` and an overflowing `1e39` before any command runs (measured:
"Argument 'x' expects a float, got 'nan'."). It was first recorded here as a fixed defect; it never was one.
⚠️ Seen in these runs, in BOTH binaries, not explained: `sky-lab` ends the same scenario at one of two brightness
levels (mean 155 / 255 in four launches, 118 in two; the sky's reported state is identical).

**Peers' round 16** (2026-11-17, engine 8332013e: the console commands under the exclusive access). macOS (Apple M6,
MoltenVK 1.4.2, Apple clang with `-Wdocumentation -Werror`, `-Wunused-lambda-capture` silent) and Windows (RTX 3500
Ada, MSVC 19.44 under /W4 /WX) build Release and Debug with 0 warning — the generic lambdas captured by value, the
default argument of class type, the abbreviated templates, and nothing missing once `StaticVector.hpp` left the file —
and send the same 41 commands to `animation-debug` (window-less, the validation layer loaded): every one answers with
the expected text, the refusals included ("Unknown mesh …", "Unknown skybox …", "Argument 'x' expects a float, got
'nan'."), none waits for itself, exit 0, 0 VUID, 0 error line. `enableTBN`: 23 instances and 2 skipped then 25 on
Windows as on Linux; refused on macOS (no geometry stage), then 25. `getRenderStatistics()` thirty times: median 0.012 s
on both (macOS on a persistent connection, Windows with its client), the slowest 0.015 s.
⚠️ What the exclusive access costs shows where a frame is long: on Windows `getGroundLevel` answered in 0.76 s right
after `setGround(default)`, and the first `toggleCompass` in 0.37 s, every other command in 0.01 to 0.045 s (macOS:
0.18 s for `clearHighlight`, the rest under 0.045 s). Not investigated; it fits a command that waits for the frame in
flight — the one that prepares the new ground — and a compass whose resources are created under the access.

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
2. **The thread that owns the component acts.** `Component::Abstract::settleLoadedResource()` (a plain load at every
   cycle, an exchange only when something was recorded) calls the component's `onResourceLoaded()`, which refreshes
   what it derives from the resource and notifies `ComponentContentModified`. On a running scene that thread is the
   logic thread: every component calls it from its `processLogics()`.
   ⚠️ **A scene that is not enabled yet has no logic cycle**: nothing acts on the record, and its entities keep no
   extents however long one waits. Such a scene belongs to the thread that builds it, which plays the logic thread's
   role: `Scene::settleLoadedContent()` (every entity, `AbstractEntity::settleLoadedContent()`), called once the
   resources are loaded and before reading anything derived from them. The model viewer frames its subject that way.
   Measured 2026-11-17 (Linux, Release, window-less): reading the boxes without it, `Core.openFiles` held the main
   thread for the whole 5 s budget at EVERY opening since 2026-11-15 and framed on the fallback (camera at
   [6.94, 3.04, 6.95] whatever the model); with it:

   | Model | `Core.openFiles` before / after | Camera after |
   |---|---|---|
   | `Box` | 5.06 s / 0.04 s | [1.736, 0.760, 1.737] |
   | `MetalRoughSpheresNoTextures` (millimetric) | 5.15 s / 0.35 s | [0.0146, 0.0081, 0.0101] |
   | `Fox` (skinned, node mode) | 5.10 s / 0.06 s | [268.6, 156.9, 258.1] |
   | `Sponza` | 5.18 s / 0.20 s | [51.19, 27.83, 51.41] |

   0 fallback warning, 0 VUID, exit 0 on each; ThreadSanitizer on the viewer scenario (a file opened, two drags, the
   wheel): 0 report. ⚠️ `TextureTransformTest` still falls back, for another reason: its meshes are flat and a box
   without volume is not valid (item `modelviewer-extents-fallback-framing`).

   **Peers' round 18** (2026-11-17, engine 01b2d65d). Both: Release and Debug 0 warning; `Box`,
   `MetalRoughSpheresNoTextures` and `Fox` framed as on Linux (Windows digit for digit, macOS to seven digits), no
   fallback. ⚠️ **`Sponza` with a COLD texture cache is not solved**: the viewer waits for the meshes to be ready, a
   mesh is ready with its MATERIALS, and their textures are compressed on the CPU at the first opening - Linux
   (32 threads, an empty `--cache-directory`) 2.49 s, framed; macOS 3.5 s, framed; Windows 5.3 s and the FALLBACK
   ("1 of 1 mesh(es) not ready after 5000 ms"). Warm: 0.2 to 0.4 s, framed, everywhere. The Linux numbers of the
   table above are WARM. The framing needs the geometries' extents, not the materials (item
   `model-viewer-always-times-out-waiting-for-the-model-extents`).
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

Audit of the other `LoadFinished` handlers: `CloudVolume::registerShape()` is written for the loading thread (its
published state is one atomic, the registration is idempotent and the bindless set is locked). `SoundEmitter` and
`AbstractLightEmitter` wrote plain members there: both follow the contract since 2026-11-16 (next section).

#### The two emitters (2026-11-16, owner decisions)

**The lights — the family's logic cycle.** `AbstractLightEmitter::processLogics()` is the cycle of EVERY light,
`final`: it consumes the record of a colour projection texture that finished loading and registers it in the bindless
set (a light that is off registers it too, and only when nothing is registered yet), then — for an enabled light — runs
the animations and the type's own step, the virtual `onProcessLightLogics()` (empty by default; `PointLight` advances
the frame of an animated projection there). The four types used to repeat that cycle; a new light type implements the
hook, never `processLogics()`. `setColorProjectionTexture()` observes, THEN tests `isLoaded()` again and records the
load that ended in between.

What the handler did on the loading thread: `registerColorProjectionInBindless()` wrote the projection's bindless index,
its kind and its frame index, then `requestVideoMemoryUpdate()` incremented `m_logicGeneration` — while the logic thread
compared that generation and wrote the light's block for a render-state slot (`publishStateForRendering()`). A slot
published in that window is stamped with the NEW generation and holds the OLD block: it is never published again, and
the frames that latch it draw the light without its projection (the mechanism by reading; the measurement below).

**The sound emitter — a play that awaits its sound.** `SoundEmitter::onNotification()` records the END of the load,
success or failure alike (one flag); `processLogics()` reads the outcome from the resource (`ResourceTrait::status()`,
atomic) in `resolveAwaitedSound()`: loaded, it starts the awaited play; failed, it drops the attached sound.
`play()` and `replay()` on a sound still loading call `awaitAttachedSound()` — `m_playAwaitsLoad` (logic thread only),
`observe()`, then the status tested again for a load that ended in between. ⚠️ `stop()` and `clear()` CANCEL an awaited
play (owner decision): the sound stays attached, `replay()` awaits it again. It used to start when its load ended,
after the stop. The handler used to call `playAttachedSound()` on the loading thread: it took a source from the pool
and wrote `m_source`, which `processLogics()` and `move()` read, test and reset on the logic thread.

Proof (Linux, RTX 3070 Ti, 2026-11-16; Debug, window-less, the validation layer loaded, the binary before against
the binary after).
- `global-illumination` with a colour projection on its point light (`--demo-options 0,1`: the cubemap is requested
  and given to the light in the same call, so it is still loading) — **the picture of the binary BEFORE changes from one
  launch to the next**: metered scene average 102.4, 104.8, 106.9 and 107.7 nits over four launches, the closest
  captures of two launches 1.3 to 3.1 / 255 apart. The binary AFTER: 104.0, 104.4, 104.8 and 105.2 nits, the closest
  captures 0.15 to 0.53 apart; one launch of the binary before falls on the same picture (0.04). The projection is
  registered in the eight launches (`Color projection texture registered in bindless manager at index 16`).
- `light-and-shadow-debug` with projections on its three lights (`--demo-options 1`), whose textures are loaded when
  the lights ask: the closest captures of the two binaries 0.016 apart, the median 0.091 — against 0.002 and 0.091
  between two launches of the binary before; the same status, statistics and GPU timing zones (the three shadow maps
  come in a per-launch ORDER: compare them sorted).
- `game-logic` (an emitter plays the looping sound it requested in the same call): the same three emitters play before
  and after (`fall`, the fire's loop, the drone's engine), each with a source, the 154 others silent
  (`SoundEmitter.getState` on the 157 emitters, identical); the same GPU timing zones and status.
- 0 VUID, 0 error line, exit 0 in the 14 launches. The commit gate entirely green: clang-tidy 0, cppcheck 0,
  include-what-you-use 0 after one pass, GCC Release and Debug, clang, the Debug + TSan tree 0 warning.
- ThreadSanitizer (2026-11-16, engine 82c7efe0: Debug, window-less, the software device with the two traced effects
  that kill it disabled — `docs/caution-points.md` § *The software device gives a ray-query hit the identity of another
  instance* —, a console shutdown): `global-illumination` with the projection — registered in that run —, 0 report,
  exit 0; `animation-debug`, 0 report; `light-and-shadow-debug` with projections, 1 report, and `game-logic`, 4: none
  on a light or a sound emitter — they are three rows of § *Plain state shared by the logic and rendering threads* (an
  instance's animation time, its flag word, the view's logic state read by the acceleration-structure rebuild), fixed
  the same day.
  ⚠️ The binary BEFORE the change was not run under the sanitizer: this is the absence of a report after, not a
  before / after. The cancellation of an awaited play by `stop()` is verified by reading only: no console command
  played a sound on an emitter then, so the window between `play()` and the end of the load could not be driven from
  outside. `SoundEmitter.play` exists since 2026-11-26 (audio doc 15); that proof is still to run
  (item `console-command-to-set-a-light-projection`).
- Peers' round 13 (2026-11-16, engine 5dfe292a + testbed 3411e7c7), window-less, the validation layer loaded, every
  launch exit 0, 0 VUID, 0 error line: macOS (Apple M6, MoltenVK 1.4.2, Apple clang with `-Wdocumentation -Werror`,
  Release and Debug 0 warning) meters `global-illumination` with the projection at 14.4, 14.4 and 14.7 nits over
  three launches on its screen-space lane, the three captures 0.11 to 0.88 / 255 apart; Windows (RTX 3500 Ada, MSVC
  19.44, Release and Debug 0 warning) kept the binary before for three launches of each on the ray-traced lane: BEFORE
  99.3, 100.0 and 98.5 nits, AFTER 99.3, 100.4 and 99.6 nits. ⚠️ On that machine the binary before was already tight:
  the 102 to 108 nits of Linux did not show — the window is a matter of timing between the loading thread and the
  logic thread, it opens or not with the machine. Both: the projected cubemap on the walls, the same statistics as
  Linux (19 / 266 | 30 / 15340); `game-logic`, the three emitters play, `stop` → not playing with the sound still
  attached, `replay` → playing; `light-and-shadow-debug` with projections 7 / 673958 | 30 / 4033408.

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
