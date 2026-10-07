## Ray Tracing Architecture (SceneMetaData)

`SceneMetaData` manages all scene-level RT resources. It is inert when the device lacks RT support.

### Lifecycle
1. **Construction** (`Scene::Scene()`) — **Borrows** the single renderer-owned `AccelerationStructureBuilder` (`graphicsRenderer.accelerationStructureBuilder()`, created once at renderer init when RT is enabled; null otherwise) **and the renderer-owned `Vulkan::DeferredDestructor`** (`graphicsRenderer.deferredDestructor()`). `SceneMetaData` does **NOT** create or own either. `isRayTracingEnabled()` == (borrowed builder pointer != null).
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
- **Centralized** in `Geometry::Interface::onDependenciesLoaded()` — called after `createOnHardware()`. The geometry fetches the builder from `serviceProvider().graphicsRenderer().accelerationStructureBuilder()` (skips if null = RT off). The returned `AccelerationStructure` is owned by the geometry; the builder only builds it.
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
- `Graphics/Renderer.hpp/.cpp` — **owns** the single `AccelerationStructureBuilder` (`accelerationStructureBuilder()` getter); `SceneMetaData` and `Geometry::Interface` borrow it
