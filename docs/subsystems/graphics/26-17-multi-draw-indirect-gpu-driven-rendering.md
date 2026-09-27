## 17. Multi-Draw Indirect (MDI) — GPU-Driven Rendering

### Overview

MDI reduces CPU overhead for scenes with many objects by:
1. **Phase 1A (Active)**: State-sorted opaque rendering + redundant bind elimination via `RenderStateTracker`
2. **Phase 1B (Infrastructure ready, dispatch inactive)**: Per-draw SSBO + `vkCmdDrawIndexedIndirect` for batched objects

### Phase 1A — State-Sorted Rendering

Opaque objects are sorted by a composite 64-bit key `(pipeline|material|geometry|distance)` instead of distance-only. A `RenderStateTracker` skips redundant Vulkan bind commands between consecutive draws.

**Sort key structure** (`RenderBatch::createStateSorted()`):
- Bits 63-48: Pipeline identity (instance flags hash)
- Bits 47-32: Material identity (low bits of pointer address)
- Bits 31-16: Geometry identity (low bits of pointer address)
- Bits 15-0: Quantized distance (front-to-back for early-Z)

**Tracked state** (`RenderStateTracker`):
- Pipeline bind (skipped if same `VkPipeline` handle)
- Viewport/scissor (set once per pass)
- Geometry bind (skipped if same geometry + layer)
- Descriptor sets: view, material, light, bindless (skipped if same `VkDescriptorSet` handle)
- Push constants: **NEVER skipped** (unique model matrix per object)
- Pipeline change **invalidates** all descriptor set tracking (Vulkan layout compatibility)

**Special objects exclusion**: Sprites (`isSprite()`), InfinityView, depth-test-disabled, depth-write-disabled objects use distance-only sorting to preserve order-dependent rendering behavior.

**Frustum culling fix**: Sprites skip frustum culling (`Scene.rendering.cpp`). Billboard rotation is a vertex shader operation, but culling uses CPU-side AABB from the flat quad geometry (Z=0 extent). At grazing angles, the AABB is paper-thin and falsely rejected by the frustum.

**Code references:**
- `Scenes/RenderBatch.hpp:createStateSorted()` — Composite sort key
- `RenderableInstance/RenderStateTracker.hpp` — POD state tracker
- `RenderableInstance/Abstract.cpp:render(..., RenderStateTracker &)` — Tracked render overload
- `Scenes/Scene.rendering.cpp:renderOpaque()` — Phase 1A render loop
- `Scenes/Scene.rendering.cpp:renderLightedSelection()` — Phase 1A for lighted objects

### Phase 1B — MDI Dispatch (Active)

Per-draw model matrices stored in an SSBO accessed via Buffer Device Address (BDA) + `gl_DrawID`. Indirect draw commands in a `VkBuffer`. Both double-buffered per frame-in-flight.

**Vulkan features required** (enabled in `Instance.cpp`):
- `multiDrawIndirect`, `drawIndirectFirstInstance` (VK 1.0)
- `shaderInt64` (VK 1.0) — for `uint64_t` BDA reconstruction in GLSL
- `shaderDrawParameters` (VK 1.1) — for `gl_DrawID`
- `bufferDeviceAddress` (VK 1.2) — already enabled for RT

**GLSL extensions** (registered in `VertexShader::enableMDI()`):
- `GL_EXT_buffer_reference` + `GL_EXT_buffer_reference2` (BDA struct + array indexing)
- `GL_ARB_gpu_shader_int64` (`uint64_t` + `packUint2x32`)
- `GL_ARB_shader_draw_parameters` (`gl_DrawID`)

**MDI push constant layout** (76 bytes):
```
[0-3]   perDrawAddrLo  (uint32)
[4-7]   perDrawAddrHi  (uint32)
[8-71]  viewProjectionMatrix (mat4)
[72-75] frameIndex      (float)
```

**MDI shader model matrix read:**
```glsl
const uint64_t addr = packUint2x32(uvec2(pcMatrices.perDrawAddrLo, pcMatrices.perDrawAddrHi));
const mat4 M = mat4(PerDrawDataRef(addr)[gl_DrawID].modelMatrix);
```

**Objects excluded from MDI**: Sprites, InfinityView, depth-test/write-disabled, adaptive LOD geometry. These are rendered via Phase 1A fallback.

**Dispatch status**: Active. `BatchBuilder::dispatch()` is wired into `Scene::renderOpaque()`. For multi-draw batches with a valid MDI program, `vkCmdDrawIndexedIndirect` is issued. Otherwise, ALL objects in the batch are rendered individually via the Phase 1A tracked render fallback.

**Multi-layer geometry**: Indirect draw commands use `geometry->subGeometryRange(layerIndex)` for correct `firstIndex`/`indexCount` per layer. Using the full `indexBufferObject()->indexCount()` causes one layer to draw another layer's indices with the wrong material.

**Batch storage**: `MDIBatch` stores a `std::vector<const RenderBatch*>` of ALL render batches in the group (not just the representative). This ensures the fallback path renders every object, not just the first.

**Setting**: `Core/Graphics/MDI/Enabled` (default: false)

**Known limitation (UNVERIFIED since the material merge)**: materials without lighting enabled may not render correctly in the Opaque (non-lighted) list. This is a material/demo configuration issue, not an MDI bug. ⚠️ This note predates the merge, where it meant the DELETED legacy Blinn-Phong material; whether the limitation survived into the single lit material was never checked. Re-verify before trusting it either way.

**Code references:**
- `MDI/PerDrawData.hpp` — GPU-side struct (mat4 + uint frameIndex + padding = 80 bytes)
- `MDI/BatchBuilder.hpp/.cpp` — Batch building + dispatch (currently inactive)
- `Vulkan/IndirectBuffer.hpp` — Buffer with `VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT`
- `Vulkan/CommandBuffer.cpp:drawIndexedIndirect()` — Wraps `vkCmdDrawIndexedIndirect`
- `Renderable/ProgramCacheKey.hpp` — `isMDIEnabled` field
- `Saphir/Generator/Abstract.hpp` — `IsMultiDrawIndirectEnabled` flag
- `Saphir/AbstractVertexStage.cpp` — MDI paths in all synthesize/prepare methods
- `Renderer.hpp/.cpp` — `m_MDIBatchBuilder`, `m_MDIEnabled`, init in `onSetup()`

### Extension Registration Ordering (Critical)

> [!WARNING]
> **GLSL extensions used by BDA (`GL_EXT_buffer_reference`) must be registered in `enableMDI()`,
> NOT in `onSourceCodeGeneration()`.** The `generateHeaders()` method emits `#extension` directives
> BEFORE `onSourceCodeGeneration()` runs. Registering extensions in the latter causes the
> `PerDrawDataRef` struct to appear before the extension directive → compilation error.
