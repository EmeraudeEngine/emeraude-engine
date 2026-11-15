## Ray Tracing Architecture (SceneMetaData)

`SceneMetaData` manages all scene-level RT resources. It is inert when the device lacks RT support.

### The session, the frame, the effect family (2026-11-15, owner decisions)

The ray tracing was built iteratively; it is now ONE system on the base's own moulds — a simple interface implemented
by a `final` class at each level, what is common grouped at the level where it is common (the renderable hierarchy:
`Renderable::Abstract` → `AbstractBackground` → the concrete resources). Three levels:

| Level | Class | What it groups | Its contract |
|---|---|---|---|
| **Session** | `Graphics::RayTracing` (`final : ServiceInterface`), the renderer's twelfth sub-service — `Renderer::rayTracing()` | the `AccelerationStructureBuilder`, the ray-tracing set layout and one descriptor set per frame in flight, the `SkinnedGeometryProcessor` (optional), the `IrradianceProbeVolume` (optional) | `usable()` IS "the session can trace": the renderer starts it when the device traces AND the lighting lane may need it (`Auto` / `RayTracing`), after the descriptor pool and the frame scopes, and the sub-services loop terminates it first. Its accessors answer null otherwise. Nothing else holds one of those objects |
| **Frame** | `Graphics::RayTracingFrame` (a value: the descriptor set as a REFERENCE, the frame's light count) | what ONE frame traces | made by `RayTracing::bindFrame()` once the frame's TLAS build is recorded and the five bindings are written; bounded by `RayTracing::FrameGuard` (opened at the top of `Renderer::renderFrame()`: nothing bound when a frame starts, nothing left when it ends); handed to the chain in `IndirectPostProcessEffect::FrameContext::rayTracing` — null = this frame does not trace |
| **Effect family** | `Graphics::AbstractRayTracedEffect`, between `IndirectPostProcessEffect` and the `final` effects (RTGI, RTR, RTAO, RTContactShadows) | `requiresRayTracing()` (`final`, true); the trace pipeline layout on the family's set convention; the binding of those sets for a frame | `createTraceLayout(classId, inputs, RadianceCache, pushConstants)` and `bindTraceSets(commandBuffer, inputs, context, frameIndex)`. Set 0 = what the frame traces, set 1 = the effect's own inputs, set 2 = the bindless textures, set 3 = the irradiance probes (`RadianceCache::Read`: RTGI, RTR). A concrete effect brings its shaders, its targets and its set 1 |

**One gate.** An effect DECLARES what it requires (`requiresRayTracing()`, `requiresLightSet()`,
`requiresCloudVolumes()`); `PostProcessStack::canOccupantRun(effect, FrameCapabilities)` tests it against what a frame
offers. The same function is applied twice in a frame, each time with a value made where the facts are known:

1. at the START of the frame, by the renderer — the selection (`syncSlotSelection()`) and the indirect-diffuse answer
   (`hasEnabledIndirectDiffuseProvider()`): `rayTracing` = the service is usable AND the scene being rendered holds a
   created structure (`Scene::hasAccelerationStructure()`: its previous build; the first frame of a scene has none,
   which `FallbackGraceFrames` waits for);
2. by the executor, from the frame it records: `rayTracing` = the frame bound what it traces
   (`context.rayTracing != nullptr`).

A traced effect is therefore recorded only with the frame's set, and binds it WITHOUT a test (a Debug assert states the
contract). `RTGI::providesIndirectDiffuse()` answers `true` — what it provides — and no longer repeats the gate.

**What it replaced.** The renderer kept the frame's state in members read by everybody, at different moments of the
frame: `m_currentTLAS` (a raw pointer set in the MIDDLE of a frame and read at the START of the next one — after a scene
switch it named a destroyed structure: `heap-use-after-free` on the rendering thread, ThreadSanitizer), `rtDescriptorSet()`,
`isRayTracingReady()`, `rtLightCount()`, `updateRTDescriptorSet()`; four pointers created and released one by one
(`accelerationStructureBuilder()`, `skinnedGeometryProcessor()`, `irradianceProbeVolume()`, the descriptor sets); the same
condition evaluated in three places under a comment "the SAME gate as…"; and four effects each assembling the same
pipeline layout and binding the same sets behind their own null tests. All of those are gone; `Scene::TLAS()` too — a
scene answers `hasAccelerationStructure()` and nothing stores its structure.

**Rules.**
- A new traced effect derives from `AbstractRayTracedEffect` and uses its two functions: never a private copy of the
  layout assembly or of the binds, never a test of `context.rayTracing`.
- Nothing stores a pointer to an object a scene owns across frames. What a frame needs from its scene is bound by that
  frame and handed down in the frame context.
- "Can the session trace" is `Renderer::rayTracing().usable()`, nothing else.
- Still open, the same shape for the read-state index: item `renderer-read-state-index-stored-across-frames`.

**Proof** (Linux, 2026-11-15).
- ThreadSanitizer (Debug, llvmpipe, the active scene switched through the console to a skinned scene): the
  `heap-use-after-free` of `isRayTracingReady()` once in 3 runs before; 0 in 4 runs after — three of them without any
  report, the fourth with a race of another family (the light set's ambient, item
  `tsan-logic-render-instance-state-races`).
- Release, RTX 3070 Ti, the validation layer loaded, the binary before against the binary after:
  `global-illumination` — the lane lines of `PostProcess.getStatus` identical (RTGI, RTR, RTAO running; the contact
  shadows on their fallback in both), statistics identical (19 instances / 266 triangles, shadows 30 / 15 340), mean
  absolute difference 0.66 / 255 with no pixel further than 16 (two launches of the SAME binary differ by 0.83);
  `reflexion-debug` — status and statistics identical (9 / 70 529, shadows 15 / 153 407); its content moves, so its
  pixel difference (4.3) only says it is under the 5.4 of two launches of the same binary.
- Nine switches between traced scenes through the console (`default`, `global-illumination`, `reflexion-debug`, an act
  destroyed, an act kept): the traced occupants are back after each switch and each scene gives the same statistics at
  every return; 0 VUID, exit 0. The log holds `RayTracingService service up!` and
  `RayTracingService sub-service terminated gracefully!`.
- The lane set to `ScreenSpace` in a settings copy: the service is not started, the stack reports "ScreenSpace ONLY …
  FOR THIS WHOLE SESSION", the screen-space occupants run; 0 VUID, exit 0.
- The commit gate: GCC Release, clang, cppcheck and include-what-you-use clean; clang-tidy 8 findings on the changed
  lines, all kept ON PURPOSE (ledger § 7f); GCC Debug 0 warning.

### Lifecycle
1. **Construction** (`Scene::Scene()`) — **Borrows** the single renderer-owned `AccelerationStructureBuilder` (`graphicsRenderer.rayTracing().accelerationStructureBuilder()`, owned by the renderer's ray-tracing sub-service; null when the session cannot trace) **and the renderer-owned `Vulkan::DeferredDestructor`** (`graphicsRenderer.deferredDestructor()`). `SceneMetaData` does **NOT** create or own either. `isRayTracingEnabled()` == (borrowed builder pointer != null).
2. **Per-frame buffer init** (`Scene::Scene()`) — `initializePerFrameBuffers(framesInFlight())` creates per-frame SSBOs
3. **Per-frame rebuild** (`Scene::prepareRender()`) — `rebuild(renderLists, ..., frameIndex)` collects TLAS instances, uploads SSBOs
4. **Destruction** — **Retires** this scene's RT resources (per-frame SSBOs, TLAS, pending build request) through the `DeferredDestructor`: a scene can be deleted at runtime while frames are still in flight. The builder is renderer-owned — nothing to unregister.

> [!CRITICAL]
> **All runtime destruction of TLAS/build requests goes through `Vulkan::DeferredDestructor`**
> (2026-07-05): the transiently-empty instance list retires (never destroys in place) the live
> TLAS, and each recorded rebuild retires its previous request frame-stamped — the old
> count-capped deque ("keep at most 3") under-covered rebuild bursts and caused GPU
> use-after-free (Xid 109 DEVICE_LOST). See `src/Vulkan/AGENTS.md`, "Deferred destruction
> contract".

> [!CRITICAL]
> **The `AccelerationStructureBuilder` is owned ONCE by the Renderer, never per-scene.** BLAS are
> built for SHARED geometries that outlive any scene; a per-scene builder (the old design, via a
> `Geometry::Interface` global static) got destroyed/nulled when a scene was deleted and broke the
> active scene's BLAS. See [`docs/multi-scene-resource-ownership.md`](../../multi-scene-resource-ownership.md).

### BLAS Building
- **Centralized** in `Geometry::Interface::onDependenciesLoaded()` — called after `createOnHardware()`. The geometry fetches the builder from `serviceProvider().graphicsRenderer().rayTracing().accelerationStructureBuilder()` (skips if null = the session cannot trace). The returned `AccelerationStructure` is owned by the geometry; the builder only builds it.
- **TriangleStrip support** — `generateTriangleListIndicesForRT()` virtual method converts strip+primitive restart to triangle list. Persistent `m_rtIndexBufferObject` stored in `Geometry::Interface` for shader access to converted indices.
- **Subclasses**: `VertexGridResource` overrides `generateTriangleListIndicesForRT()` for strip conversion; `CDLODTerrainResource` (every terrain floor, since 2026-09-22) draws a flat patch its vertex stage displaces, so it traces a DEDICATED proxy instead (`dedicatedRTVertexBufferObject()` / `dedicatedRTIndexBufferObject()` / `dedicatedRTVertexFlags()`), which the BLAS build and the hit-shading metadata read through `rtVertexBufferObject()` / `rtIndexBufferObject()` / `rtVertexFlags()` — ⚠️ its predecessor, the adaptive grid, inherited the base returning `{}` until 2026-09-22 and every terrain was silently absent from the TLAS while the log repeated `uses TriangleStrip but generateTriangleListIndicesForRT() returned empty indices` (2237 times in a 2-minute `forest` run: `SceneMetaData` retries the build on every TLAS refresh).
- ⚠️⚠️ **A NON-INDEXED triangle list gets an identity RT index list (0..n-1) the same way** (2026-10-07, owner's choice):
  the hit shaders (`RTAlphaTestGLSL.hpp` `getMeshAccessor()`, and its copies in `RTR.cpp` / `RTGI.cpp`) ALWAYS read three
  indices through `GPUMeshMetaData::indexBufferAddress`, which stayed 0 for a geometry without an index buffer: a NULL
  read on the GPU, `VK_ERROR_DEVICE_LOST` with `VK_EXT_device_fault` `READ_INVALID addr=0x0` and 0 VUID
  (`raw-geometry-loader`'s `RawVertexResource` triangle and pyramid; RT lane only — ScreenSpace and the M2 never trace).
  Proof (Linux RTX 3070 Ti): lost every launch before, 0 / 3 after with the RayTracing lane selected; the A/B without the
  two non-indexed entities was clean too. A trailing partial triangle (vertex count not a multiple of 3) is left out.
  Any new hit-shading consumer can rely on a valid index address for every traced instance.
  ACCEPTED on the three OS (engine a0a9b373): Windows NVIDIA RTX 3060 + AMD iGPU 3 / 3 each on the RayTracing lane
  (both lost the device on 2026-10-05), 0 validation error; macOS M2 no-regression (no RT there).
  - ⚠️ **Cost measured on `terrain` (4096 m at 1 m, 2026-09-22)**: the exact surface = 33 554 432 triangles and **7.5 GiB** of VRAM against **5.05 GiB** at the default `Core/Graphics/RayTracing/TerrainBLASMaxTriangles` (2 000 000: step 8, 524 288 triangles) — an 8 GiB card. The surface is quadratic in the division count and unbounded; a small floor keeps its exact surface.
  - **A geometry that REPLACES what it traces rebuilds its BLAS through the frame path** (2026-09-22). The worker that built the new proxy only STAGES it; the RENDER thread publishes it in `updateVideoMemory()` — reached through `Renderer::requestGeometryVideoMemoryUpdate()` / `flushGeometryVideoMemoryUpdates()`, the geometries' twin of the materials' flush — and calls `Interface::markAccelerationStructureStale()` as its LAST statement, so `SceneMetaData::rebuild()` consumes it next to the on-demand build of a missing BLAS: `if ( blas == nullptr || geometry->isAccelerationStructureStale() )`. `buildAccelerationStructure()` clears the flag as soon as it commits, and retires the old BLAS through the `DeferredDestructor`.
    - A REBUILD, not a refit: a re-centre is rare (1500 m of margin) while a refit freezes a BVH partition built for another surface. Refit is the tool of per-frame motion — skinning — and stays there (owner decision, 2026-09-22).
    - ⚠️ Proven at LOG level (one rebuild per re-centre), never by comparing the images of two runs: they do not move the same way.

### TLAS Async Build (Inline Recording)

> [!CRITICAL]
> **TLAS builds are recorded inline into the render command buffer via `recordTLASBuild()`.**
> The old synchronous `buildTLAS()` (fence wait per frame) has been removed.

**Two-phase API:**
1. `SceneMetaData::rebuild(renderLists, ..., frameIndex)` — Collects TLAS instances, calls `AccelerationStructureBuilder::prepareTLAS()` (CPU-side buffer preparation)
2. `Scene::recordTLASBuild(commandBuffer)` → `SceneMetaData::recordTLASBuild(commandBuffer)` → `AccelerationStructureBuilder::recordTLASBuild(commandBuffer, request)` — Records build commands into the render command buffer

**Call site in Renderer:**
```
prepareRender() → scene->recordTLASBuild(commandBuffer) → beginRenderPass()
```

### TLAS Buffer Lifetime & Retirement

TLAS buffers (TLAS + instance buffer + scratch buffer) are **per-request**, not persistent.
Each `TLASBuildRequest` owns its buffers. After recording, the request is retired into a
`std::deque`. Requests are popped from the front when the deque exceeds `framesInFlight()`
entries. This prevents use-after-free where a persistent buffer was written by the CPU
while the GPU was still reading it from a previous frame's command buffer.

### Pre-Allocated Rebuild Vectors

`SceneMetaData::rebuild()` reuses persistent vectors as class members (`m_instances`,
`m_meshMetaDataEntries`, `m_materialDataEntries`) instead of per-frame heap allocations.
These are cleared and refilled each frame without deallocating.

### Key Files
- `Scenes/SceneMetaData.hpp/.cpp` — TLAS, per-frame SSBOs, texture registration cache, `recordTLASBuild()`
- `Scenes/GPUMeshMetaData.hpp` — GPU-side struct (VB/IB addresses, stride, offsets, material index)
- `Graphics/Geometry/Interface.hpp/.cpp` — `buildAccelerationStructure()`, `generateTriangleListIndicesForRT()`, `m_rtIndexBufferObject`
- `Graphics/Geometry/VertexGridResource.cpp` — Strip→TriangleList conversion
- `Vulkan/AccelerationStructureBuilder.hpp/.cpp` — BLAS/TLAS building, `TLASBuildRequest`, `prepareTLAS()`, `recordTLASBuild()`, retired request deque
- `Graphics/RayTracing.hpp/.cpp` — the renderer's ray-tracing sub-service: **owns** the single `AccelerationStructureBuilder` (`Renderer::rayTracing().accelerationStructureBuilder()`); `SceneMetaData` and `Geometry::Interface` borrow it. Also the set layout, the per-frame sets, the skinned geometry processor and the irradiance probe volume (§ below)
