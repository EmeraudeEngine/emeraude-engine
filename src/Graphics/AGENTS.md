# Graphics System - AI Context

> [!CRITICAL]
> **Before modifying ANY cache-related code, READ [`docs/pipeline-caching-system.md`](../../docs/pipeline-caching-system.md) FIRST!**
>
> This includes: `ProgramCacheKey`, `computeProgramCacheKey()`, `getHash()`, `m_programs`, `m_graphicsPipelines`, or any code involving render pass compatibility.
>
> **Common AI mistake**: Forgetting `renderPassHandle` in cache keys causes Vulkan validation errors that are extremely difficult to debug.

## 1. Context

**High-Level Abstraction**: OpenGL-style declarative interface over Vulkan.
**Key Concept**: `Geometry` + `Material` = `Renderable` -> `Visual` (Scene Instance).
**Dependencies**: Uses `Saphir` for shader generation and `Resources` for fail-safe loading.

## 2. Architecture Map

| Concept | Description |
|---|---|
| **Overview** | [`docs/graphics-system.md`](../../docs/graphics-system.md) |
| **Subsystems** | [`docs/graphics-subsystems.md`](../../docs/graphics-subsystems.md) (Managers, Transfer) |
| **Off-Screen** | [`docs/render-targets.md`](../../docs/render-targets.md) (Shadow, Cubemaps) |
| **Instancing** | [`docs/renderable-instance-system.md`](../../docs/renderable-instance-system.md) |
| **Shaders** | [`docs/saphir-shader-system.md`](../../docs/saphir-shader-system.md) |

## 3. Core Axioms

### Architecture
1.  **Declarative**: You define WHAT (Material/Geometry), engine handles HOW (Vulkan pipeline).
2.  **Instancing**: `Renderable` is shared. `RenderableInstance` is the unique usage.
3.  **Visuals**: Scene nodes use `Visual` components to attach to Graphics.

### Constraints
1.  **Thread Safety**: `TransferManager` handles CPU->GPU. Main thread for Logic.
2.  **Y-UP**: Strictly Y-up coordinate system (`+X` right, `+Y` up, `-Z` forward).
3.  **Fail-Safe**: Resources must never be null. Use neutral fallbacks.
4.  **G-Buffer MRT (fixed order)**: scene target color attachments are
    `[0]=color, [1]=normals, [2]=materialProperties, [3]=albedo, [4]=velocity` (+ depth
    last), allocated ON DEMAND from the enabled post-process effects' `requires*()` flags —
    each attachment forces every one before it, and the shader generator detects the layout
    **by color attachment count**. Velocity is RG16F (NDC-delta motion vectors), written by
    the ambient/simple passes only (light passes have a zeroed write mask), consumed with a
    3x3 depth-nearest dilation (RTGI temporal; TAA/motion blur later). Full contract + pitfalls (clear-value indices, blend-state counts,
    GrabPass copies): `docs/caution-points.md` § "SSGI Indirect Light Ignored Receiver
    Albedo — New Albedo G-Buffer Attachment".
5.  **Instance transforms (motion vectors B1)**: every visible NON-instanced
    `RenderableInstance` stages its `{model, previousModel}` matrices into the scene's
    `InstanceTransforms` SSBO during `Scene::prepareRender()`
    (`Abstract::stageInstanceTransforms()`, slot retained via `instanceTransformsSlot()`).
    The classic scene path CONSUMES it: push = VP + frameIndex, model matrix from the SSBO,
    slot in the `firstInstance` draw parameter (`CommandBuffer::drawWithFirstInstance()`).
    Advanced/cubemap/CSM/shadow paths still push their matrices (B1 milestone 4 pending).
    Contract details: `src/Scenes/AGENTS.md` § "Instance Transforms (SceneInstanceTransforms)"
    and `src/Saphir/AGENTS.md` § "InstanceTransforms SSBO Path".
6.  **`Renderer.hpp` include diet (no regrowth)**: `Graphics/Renderer.hpp` is included by ~77 TUs
    directly and propagates via `Core.hpp` and `Overlay/UIScreen.hpp`, so one `#include` added
    there is paid by most of the engine. `SwapChain`, `Vulkan::Instance`, `Window`,
    `Resources::Manager`, `GrabPass`, `MDI::BatchBuilder`, `SceneRenderTarget` and
    `TextureResource::TextureCubemap` are **forward-declared on purpose** (legal because the
    destructor is out-of-line — the "exported pimpl" pattern). `setSwapChainDegraded()` /
    `isSwapChainDegraded()` are out-of-line for the same reason: do not re-inline them.
    When a consumer TU stops compiling, **add the include to the consumer**, never back into
    `Renderer.hpp`. Full contract: `docs/windows-export-api.md` § "Exported pimpl".

## 4. Caching Architecture

### Pipeline Selection Flow

When a `RenderableInstance` needs to be drawn, the system follows this exact sequence:

```
┌─────────────────────────────────────────────────────────────────────┐
│ 1. RENDER REQUEST                                                   │
│    RenderableInstance wants to draw on a RenderTarget               │
│    File: Graphics/RenderableInstance/Abstract.cpp                   │
└─────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────┐
│ 2. RENDERABLE-LEVEL CACHE                                           │
│    File: Graphics/RenderableInstance/Abstract.cpp                   │
│                                                                     │
│    → Builds ProgramCacheKey with:                                   │
│      - programType, renderPassType, renderPassHandle (!)            │
│      - layerIndex, isInstancing, isLightingEnabled...               │
│                                                                     │
│    → Looks up Renderable::m_programCache[cacheKey]                  │
│                                                                     │
│    ✓ HIT  → Use this program, skip to step 5                        │
│    ✗ MISS → Continue to step 3                                      │
└─────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────┐
│ 3. RENDERER-LEVEL PROGRAM CACHE                                     │
│    File: Graphics/Renderer.cpp                                      │
│                                                                     │
│    → Generator (SceneRendering, etc.) computes key via              │
│      computeProgramCacheKey() which includes:                       │
│      - renderPassHandle (!), isCubemap, renderableName              │
│      - layerIndex, renderPassType, flags...                         │
│                                                                     │
│    → Looks up Renderer::m_programs[generatorCacheKey]               │
│                                                                     │
│    ✓ HIT  → Use this program, skip to step 5                        │
│    ✗ MISS → Continue to step 4                                      │
└─────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────┐
│ 4. SHADER PROGRAM GENERATION                                        │
│    Files: Saphir/Generator/*.cpp                                    │
│                                                                     │
│    → Generator::onGenerateShadersCode() creates shaders             │
│      (vertex, fragment, geometry...)                                │
│    → Generator::onCreateDataLayouts() creates descriptor layouts    │
│    → Compiles SPIR-V shaders                                        │
│    → Stores in Renderer::m_programs                                 │
└─────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────┐
│ 5. RENDERER-LEVEL PIPELINE CACHE                                    │
│    File: Graphics/Renderer.cpp → finalizeGraphicsPipeline()         │
│                                                                     │
│    → GraphicsPipeline::getHash(renderPass) computes hash with:      │
│      - renderPassHandle (!)                                         │
│      - shader stages, vertex input, topology                        │
│      - rasterization, depth/stencil, color blend states...          │
│                                                                     │
│    → Looks up Renderer::m_graphicsPipelines[pipelineHash]           │
│                                                                     │
│    ✓ HIT  → Use this pipeline                                       │
│    ✗ MISS → Continue to step 6                                      │
└─────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────┐
│ 6. VULKAN PIPELINE CREATION                                         │
│    File: Vulkan/GraphicsPipeline.cpp                                │
│                                                                     │
│    → vkCreateGraphicsPipelines() with the specific RenderPass       │
│    → Stores in Renderer::m_graphicsPipelines                        │
└─────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────┐
│ 7. DRAW CALL                                                        │
│    → vkCmdBindPipeline(pipeline)                                    │
│    → vkCmdDraw(...)                                                 │
└─────────────────────────────────────────────────────────────────────┘
```

> [!CRITICAL]
> **renderPassHandle is MANDATORY in ALL cache keys!**
>
> Vulkan pipelines are tied to specific render passes. A pipeline created for render pass A
> (e.g., offscreen 1 sample) CANNOT be used with render pass B (e.g., main view 4 samples).
>
> ALL THREE cache levels must include renderPassHandle:
> 1. `ProgramCacheKey::renderPassHandle` (Renderable level)
> 2. `Generator::computeProgramCacheKey()` (Renderer program cache)
> 3. `GraphicsPipeline::getHash(renderPass)` (Renderer pipeline cache)
>
> Missing renderPassHandle in ANY cache causes Vulkan validation errors:
> - "sample count mismatch"
> - "format mismatch"
> - "VkRenderPass incompatible"

### Renderer-Level Caches

The `Renderer` maintains global caches for performance optimization:

| Cache | Member | Key Source | Purpose |
|-------|--------|------------|---------|
| Programs | `m_programs` | `Generator::computeProgramCacheKey()` | Saphir Program cache (biggest gain) |
| Pipelines | `m_graphicsPipelines` | `GraphicsPipeline::getHash(renderPass)` | Vulkan GraphicsPipeline cache |
| Samplers | `m_samplers` | `hashSamplerCreateInfo()` — the **content** of the `VkSamplerCreateInfo` | Texture sampler cache |

> [!CAUTION]
> **The sampler cache key is the create-info CONTENT, not the identifier.** It used to be the
> identifier string, which meant the FIRST caller of a given name imposed its sampler on every
> later one — silently, because `getSampler()` skipped the setup lambda on a cache hit. Two call
> sites shared the name `"ShadowMap"` while requesting **opposite** addressing, so every real shadow
> map sampled with `DummyShadowTexture`'s `CLAMP_TO_EDGE` instead of the `CLAMP_TO_BORDER` +
> opaque-white border it asked for. Visible symptom: a broad black band past the edge of a
> directional shadow map's coverage (the border texel ring smeared over the whole exterior), and a
> point-light `samplerCube` fed a compare-enabled sampler — undefined per spec, working only by
> driver leniency. Consequence of the fix: **the setup lambda now runs on every call**, so it must
> stay cheap and side-effect-free, and the identifier is a debug label only — differentiating two
> samplers no longer requires encoding anything in the name.

**Statistics** available at shutdown via `programBuiltCount()`, `programsReusedCount()`, `pipelineBuiltCount()`, `pipelineReusedCount()`.

> [!CRITICAL]
> **These caches OWN their objects; consumers only borrow.** A `shared_ptr` handed out by
> `getSampler()` (and likewise programs/pipelines/layouts) is shared by many users. A consumer's
> teardown must **release its reference** (`m_x.reset()`) — **never** call `destroyFromHardware()`
> on it. The Renderer destroys each cached object **once**, at shutdown (`onTerminate`). Destroying
> a cached sampler from a texture/overlay teardown invalidated it for every other user
> (`VUID-vkDestroySampler-sampler-01082` + invalid descriptors). Fixed Jun 2026 across all
> `TextureResource` types and `Overlay::Surface`; see [`docs/caution-points.md`](../../docs/caution-points.md)
> and [`docs/multi-scene-resource-ownership.md`](../../docs/multi-scene-resource-ownership.md).

> [!NOTE]
> **`Texture2D`'s `-<U><V>` name suffix is now REDUNDANT, and kept as a debug label.** This block
> used to say the opposite of the one above — "the identifier IS the sampler cache key, anything
> that must distinguish two samplers has to appear in the NAME" — and that was true when it was
> written (Aug 2026) and false a day later, once the key became the create-info content. Two
> contradicting CAUTION blocks stood in this file until the second one was reread.
>
> The history is still worth keeping, because it is what the suffix is for: `Texture2D` used the
> bare identifier `"Texture2D"` for every 2D texture in the engine, so when glTF sampler addressing
> was wired in, whichever texture happened to be created first would have imposed its wrap modes on
> all the others. The suffix (`R`/`M`/`C`, appended only when the modes are not the default
> repeat/repeat) fixed that under the old keying. Under content keying the two samplers separate on
> their own, so the suffix no longer carries correctness — it only makes the two entries legible in
> a capture. ⚠️ Do not restore the old rule: putting distinguishing state in the NAME is now
> pointless, and a name that varies per material fragments nothing but the debug labels.

### Texture addressing comes from the ASSET, not from a global default

`TextureResource::Abstract` carries a `WrapMode` per axis (`setWrapModes()`, defaults repeat/repeat
— the Vulkan **and** glTF default), consumed once when the sampler is built and never revisited, so
it must be set BEFORE creation on hardware, exactly like `enableSRGB()`.

⚠️ Ignoring an asset's addressing does not fail loudly — the texture simply TILES where the asset
asked for a clamp, which silently corrupts anything authored with a border. Measured on the Khronos
`TextureTransformTest`, whose scaled quads repeated instead of showing their grey border once; the
fix moved **51 %** of those two quads' pixels while leaving the quads whose UVs stay inside [0,1]
**bit-identical**. That last point is the useful one for judging regression risk: addressing is
observable ONLY where UVs actually leave [0, 1], so content that stays in range renders identically
whatever it declares (`MetalRoughSpheres` declares clamp on both axes and does not move).

### Persistent (on-disk) Caches — the Renderer owns the pipeline-cache and texture-cache I/O

The three caches above live for **one run**. Three caches survive across runs, and the disk I/O of
**two** of them is implemented in this directory:

| On-disk cache | Setting | Default | Owns the object | Does the disk I/O |
|---|---|---|---|---|
| `VkPipelineCache` (driver blob) | `Core/Graphics/Shader/EnablePipelineCache` | **true** | `Vulkan::Device` | **`Graphics::Renderer`** |
| SPIR-V binary cache | `Core/Graphics/Shader/EnableBinaryCache` | **true** (flipped Aug 2026) | `Saphir::ShaderManager` | `Saphir::ShaderManager` |
| BC7 texture cache | `Core/Graphics/Texture/EnableTextureCache` | **true** (added Aug 2026; it had NO setting before, it was on whenever the GPU reported `textureCompressionBC`) | **`Graphics::TextureCache`** (Renderer sub-service) | **`Graphics::TextureCache`** |
| Generated-GLSL dump (NOT a cache) | `Core/Graphics/Shader/EnableSourceCodeDump` | `false` | `Saphir::ShaderManager` | a DUMP — nothing ever reads it back; renamed Aug 2026, the old `EnableSourceCodeCache` key is silently ignored (no migration) |

**The part that lives here.** `Renderer::loadPipelineCache()` runs right after the device is
acquired — the cache **must exist BEFORE any pipeline is created**, so do not move that call —
and `Renderer::savePipelineCache()` runs in `onTerminate()` after `waitIdle()`, when every
pipeline the run compiled is in it. Measured on `material-debug` (294 graphics pipelines,
RTX 3070 Ti): driver disk cache OFF = **5 702 ms**, driver disk cache OFF but the engine cache
restored from disk = **31 ms** (182×, 7.4 MB blob); with the driver cache active, 33 ms. The blob
is never handed to the driver raw — the application header, the load marker and the
write-then-rename are each answering a real crash mode: full rationale in
[`src/Vulkan/AGENTS.md`](../Vulkan/AGENTS.md) § "VkPipelineCache".

**The binary cache, for context only** — no Graphics code touches it. Same demo, 232 shader
modules: **393 ms** with the cache OFF → **10.3 ms** warm (38×, 383 ms saved), and the cold run
that WRITES the 232 blobs costs 391 ms, i.e. nothing — there is no first-launch penalty, which is
why it is now on by default. It is safe to leave on because an application header, including a
**toolchain identity hash** (glslang version + SPIR-V generator version + client/target
environment pair + engine version), is validated in full before any byte reaches Vulkan. See
[`src/Saphir/AGENTS.md`](../Saphir/AGENTS.md).

**The BC7 texture cache, also implemented here** — `Graphics::TextureCache`, a sub-service of the
`Renderer`, stores the BC7 mip chains produced by the pixel path in
`~/.cache/<app>/texture-cache/` with a `.bc7cache` extension (⚠️ **not** `TextureCache/` — several
documents have claimed that directory name and it has always been wrong). Same demo
(`material-debug`, all 10 options, RTX 3070 Ti, Release): cold cache = **231 mip-level
compressions, 7 705 ms** of BC7 compression → warm cache = **0 compressions, 0 ms**. That makes it
the single biggest on-disk saving of the three — more than the `VkPipelineCache` (5 702 → 31 ms)
and about twenty times the SPIR-V binary cache (393 → 10.3 ms). Full contract: § "The two BC7
sub-services" below.

`--clear-renderer-cache` (renamed from `--clear-shader-cache`) clears **all three** on-disk
renderer caches: the SPIR-V binary cache, the `VkPipelineCache` blob, and — since Aug 2026, in
`TextureCache::onInitialize()` — the BC7 texture cache.

⚠️ **An absent cache file is the nominal first-launch state, not an error.** `loadPipelineCache()`
checks `std::filesystem::exists()` before reading, and the `--clear-renderer-cache` branch guards
each `IO::eraseFile()`; without that, the one launch where the blob and the `.loading` marker are
*supposed* to be missing printed an IO error per file — on every fresh install, since the cache is
enabled by default. See [`docs/caution-points.md`](../../docs/caution-points.md) § "Flipping a
default to ON runs a path nobody had ever run".

### Renderable-Level Cache

Each `Renderable::Abstract` maintains a program cache per render target:

| Member | Type | Purpose |
|--------|------|---------|
| `m_programCache` | `Map<RenderTarget → Map<ProgramCacheKey → Program>>` | Programs for this Renderable |
| `m_programCacheMutex` | `std::mutex` | Thread-safe access |

**Key principle**: Programs are cached at Renderable level, not per-instance. All `RenderableInstance` objects sharing a `Renderable` share its cached programs.

**ProgramCacheKey** identifies a unique program configuration:
- `programType`: Rendering, ShadowCasting, TBNSpace
- `renderPassType`: Ambient, directional, point, spot lights
- **`renderPassHandle`**: VkRenderPass handle (CRITICAL for pipeline compatibility)
- `layerIndex`: Material layer
- `isInstancing`: Unique vs Multiple rendering
- `isLightingEnabled`, `isDepthTestDisabled`, `isDepthWriteDisabled`: Instance flags

See: `Renderable::Abstract::findCachedProgram()`, `cacheProgram()`, `ProgramCacheKey.hpp`

> [!CRITICAL]
> **A shared cached program does NOT make an instance ready.** Some descriptor sets the
> sealed pipeline layout demands are **per instance** — today the skeletal skinning SSBO
> (`SetType::PerModel`). `isReadyToRender()` / `isReadyToCastShadows()` therefore also test
> `isMissingSkinningResources()`, and `getReadyForRender()` /
> `getReadyForShadowCasting()` both call `prepareSkinningResources()` before generating
> anything. Skipping that test let a second instance of a skeletal mesh be drawn without its
> `PerModel` set — which shifted every following set one slot down (Aug 2026, see
> [`docs/caution-points.md`](../../docs/caution-points.md) § Vulkan Validation and
> [`src/Saphir/AGENTS.md`](../Saphir/AGENTS.md) § "Descriptor set binding contract").

### Window Resize and Render Pass Handle Invalidation

> [!CRITICAL]
> **When the window is resized, the swapchain is recreated with a NEW render pass handle!**
>
> This means ALL cached programs for the main view become stale because their `ProgramCacheKey::renderPassHandle` no longer matches the current render pass.

**Problem scenario (before fix):**
1. Window resize → swapchain recreated → new render pass handle
2. `isReadyToRender()` checked `hasAnyCachedPrograms()` → returned `true` (old programs exist)
3. `render()` tried to find program with NEW handle → failed
4. Error: "There is no suitable render program for the renderable instance"

**Solution:**
The `isReadyToRender()` function now validates that cached programs have a matching render pass handle:

```cpp
bool Abstract::isReadyToRender (const std::shared_ptr< RenderTarget::Abstract > & renderTarget) const noexcept
{
    // ...
    const auto renderPassHandle = reinterpret_cast< uint64_t >(renderTarget->framebuffer()->renderPass()->handle());
    return m_renderable->hasAnyCachedProgramsForRenderPass(renderTarget, renderPassHandle);
}
```

**Code references:**
- `Renderable/Abstract.cpp:hasAnyCachedProgramsForRenderPass()` - Validates render pass handle against cached keys
- `RenderableInstance/Abstract.cpp:isReadyToRender()` - Uses handle validation
- `Vulkan/SwapChain.cpp:recreate()` - Creates new render pass on resize

## 5. Material UBO System (SharedUniformBuffer)

### Architecture

Materials use a **SharedUniformBuffer** for GPU-side property storage:

```
┌─────────────────────────────────────────────────────────────────────┐
│ SharedUniformBuffer (single Vulkan UBO)                             │
├─────────────────────────────────────────────────────────────────────┤
│ Material 0 (index=0, offset=0)      │ 128 bytes (blockAlignedSize)  │
├─────────────────────────────────────────────────────────────────────┤
│ Material 1 (index=1, offset=128)    │ 128 bytes                     │
├─────────────────────────────────────────────────────────────────────┤
│ Material 2 (index=2, offset=256)    │ 128 bytes                     │
└─────────────────────────────────────────────────────────────────────┘
```

Each `StandardResource` has:
- `m_sharedUBOIndex`: Slot in the shared buffer (0, 1, 2...)
- `m_materialProperties[]`: Float array with material data (albedoColor, roughness, metalness, etc.)

### Concurrency Contract (MANDATORY)

> [!CRITICAL]
> **Materials are loaded concurrently from the resource thread pool, and they SHARE their buffer
> identifier by design.** `getSharedUniformBufferIdentifier()` encodes only the material kind and its
> texture count (`MaterialStandardResource2Textures`, `MaterialStandardResource4Textures`, …), so two
> unrelated materials routinely request the very same buffer from different threads.
>
> **Two invariants follow, both enforced in the engine — do not undo them:**
>
> 1. **Reaching a shared buffer is a single atomic call.** Use
>    `SharedUBOManager::getOrCreateSharedUniformBuffer(name, blockSize)`. Never write
>    `getSharedUniformBuffer()` followed by `createSharedUniformBuffer()`: that check-then-act
>    sequence hands a `nullptr` to every loser of the race, and a material that cannot reach its
>    buffer **fails to load entirely**, silently removing every sub-mesh using it from the scene.
>    `createSharedUniformBuffer()` stays strict (it reports a genuine duplicate name) and is for
>    owners of a unique name, such as `LightSet`'s per-scene light buffers.
> 2. **Claiming a seat is guarded.** `SharedUniformBuffer::addElement()` scans the seat table for a
>    free slot then writes it; the `m_elementsAccess` mutex makes the pair atomic. Without it two
>    materials receive the **same** `m_sharedUBOIndex`, hence the same UBO byte offset, and overwrite
>    each other's uniform block with no error reported at all.
>
> `writeElementData()` is deliberately **not** guarded: once a seat is granted its owner holds it
> exclusively and writes to a byte range that belongs to no one else. Do not add a lock there — it
> would serialize every per-frame material update.
>
> **Lived symptom (Aug 2026):** the `reflexion-debug` dragon loaded incomplete, missing its wings or
> its body depending on the run. Log signature: the same `There is no shared uniform buffer named 'X'`
> info line **twice in a row** (two threads missing the lookup together), then
> `A shared uniform buffer named 'X' already exists !` → `Unable to get the shared uniform buffer` →
> `Unable to load the material resource ...`. The doubled info line is the tell: single-threaded, the
> first miss would have created the buffer.
>
> **Files:** `Graphics/SharedUBOManager.{hpp,cpp}`, `Graphics/SharedUniformBuffer.{hpp,cpp}`,
> `Graphics/Material/Interface.cpp` (`getSharedUniformBuffer()`). Same pattern already applied in
> `Vulkan/LayoutManager.cpp`.

### Descriptor Binding (Preferred API)

When creating descriptor sets, use the **explicit helper methods** that ensure correct byte offsets:

```cpp
// PREFERRED - Uses SharedUniformBuffer's getDescriptorInfoForElement()
// This method handles the element index → byte offset conversion internally
const auto descriptorInfo = m_sharedUniformBuffer->getDescriptorInfoForElement(m_sharedUBOIndex);
m_descriptorSet->writeUniformBuffer(bindingPoint, descriptorInfo);
```

**Available helper methods:**

| Method | Returns | Purpose |
|--------|---------|---------|
| `getByteOffsetForElement(index)` | `VkDeviceSize` | Byte offset for element within its UBO |
| `getDescriptorInfoForElement(index)` | `VkDescriptorBufferInfo` | Complete descriptor info ready to use |

**Why this API exists:** The old API passed an "offset" parameter that was ambiguous (element index vs byte offset). This caused bugs where all materials read from offset 0. The new explicit methods eliminate this ambiguity.

> [!NOTE]
> **AI-Friendly Design:** These methods follow the "Clarity Over Cleverness" principle from [`docs/cpp-conventions.md`](../../docs/cpp-conventions.md#ai-friendly-code-guidelines)

### Material Property Layout (std140)

**StandardResource** — the ONE lit material (Cook-Torrance metallic-roughness) — stores properties
in an 80-float array (320 bytes, std140):

| Offset | Property | Type | Range/Default |
|--------|----------|------|---------------|
| 0-3 | albedoColor | vec4 | Base color |
| 4 | roughness | float | 0.0-1.0 (0.5) |
| 5 | metalness | float | 0.0-1.0 (0.0) |
| 6 | normalScale | float | 0.0-1.0 (1.0) |
| 7 | specularFactor | float | 0.0+ (1.0) — KHR_materials_specular |
| 8 | ior | float | 1.0-3.0 (1.5) |
| 9 | iblIntensity | float | 0.0-1.0 (1.0) |
| 10 | autoIlluminationAmount | float | 0.0+ (0.0) |
| 11 | aoIntensity | float | 0.0-1.0 (1.0) |
| 12-15 | autoIlluminationColor | vec4 | Emissive color |
| 16 | clearCoatFactor | float | 0.0-1.0 (0.0) |
| 17 | clearCoatRoughness | float | 0.0-1.0 (0.0) |
| 18 | subsurfaceIntensity | float | 0.0-1.0 (0.0) |
| 19 | subsurfaceRadius | float | 0.0+ (1.0) |
| 20-23 | subsurfaceColor | vec4 | SSS tint (1.0, 0.2, 0.1) |
| 24-27 | sheenColor | vec4 | Sheen tint (black = off) |
| 28 | sheenRoughness | float | 0.0-1.0 (0.5) |
| 29 | anisotropy | float | -1.0-1.0 (0.0) |
| 30 | anisotropyRotation | float | 0.0-1.0 (0.0) |
| 31 | transmissionFactor | float | 0.0-1.0 (0.0) |
| 32-35 | attenuationColor | vec4 | Volume attenuation |
| 36 | attenuationDistance | float | 0.0+ — engine default **1.0 m**, glTF's is **+INFINITY** (see below) |
| 37 | thicknessFactor | float | 0.0+ — engine default **1.0**, glTF's is **0** (thin-walled) |
| 38 | heightScale | float | 0.0+ (0.02) — POM depth |
| 39 | iridescenceFactor | float | 0.0-1.0 (0.0) |
| 40 | iridescenceIOR | float | 1.0+ (1.3) |
| 41 | iridescenceThicknessMin | float | nm (100.0) |
| 42 | iridescenceThicknessMax | float | nm (400.0) |
| 43 | dispersion | float | 0.0+ (0.0) |
| 44-47 | specularColorFactor | vec4 | KHR specular color (white) |
| 48 | emissiveStrength | float | 0.0+ (1.0) — HDR multiplier |
| 49 | clearCoatNormalScale | float | 0.0+ (1.0) — CC normal map intensity |
| 50 | opacity | float | 0.0-1.0 (1.0) — global transparency |
| 51 | alphaThreshold | float | 0.0-1.0 (0.5) — cutout cutoff |
| 52 | reflectionAmount | float | 0.0-1.0 (**1.0**) — artistic override; the neutral 1.0 leaves the mix BRDF-controlled |
| 53 | refractionAmount | float | 0.0-1.0 (**1.0**) — artistic override; the neutral 1.0 leaves the blend Fresnel-controlled |
| 54-55 | padding | float | std140 alignment |
| 56-79 | UVW transforms | 6 × vec4 | Albedo/Roughness/Metalness/Normal/AmbientOcclusion/AutoIllumination, `(scale.xy, offset.zw)`, neutral (1,1,0,0) |
| 80-103 | UVW rotations | 6 × vec4 | same component order, `(cos, sin, 0, 0)`, neutral **(1,0,0,0)** — KHR_texture_transform's `rotation`, trig resolved once on the CPU |

The GLSL struct is generated to match this layout exactly.

> [!NOTE]
> **Slots 7, 8 and 44-47 were dead until 2026-08-28.** The layout, the codegen and the BRDF term
> (`LightGenerator.PBR.cpp`: `dielectricF0 = ((ior-1)/(ior+1))²`, then
> `F0 = mix(min(dielectricF0 · specularColor · specularFactor, 1), albedo, metalness)`) were all in
> place and spec-exact, but **no loader ever wrote them**, so every asset got the identity. Because
> the identity IS the default, there was no symptom. `GLTFLoader` now fills all three (factors and
> both textures — see [`Scenes/Loaders/AGENTS.md`](../Scenes/Loaders/AGENTS.md) § *Known gaps*);
> `FBXLoader` deliberately does not, the ufbx semantics being ambiguous between OpenPBR and legacy
> Phong. ⚠️ A UBO slot that a shader reads is NOT evidence anything writes it — check both ends.
> Recorded as a trap in [`../../docs/caution-points.md`](../../docs/caution-points.md) § *An
> IDENTITY default makes an unwired feature indistinguishable from a disabled one*.
>
> ⚠️ There are **six** UV transform slots and they do not cover the specular maps: a
> `KHR_texture_transform` on `specularTexture` / `specularColorTexture` is logged and dropped.

> [!CAUTION]
> **Slots 32-37 (the KHR_materials_volume group) carry ENGINE defaults that are NOT glTF's.**
> `attenuationDistance` defaults to 1.0 m here and to **+infinity** in the extension;
> `thicknessFactor` to 1.0 here and to **0** — thin-walled, no volume — in the extension. The
> divergence is invisible only because `attenuationColor` defaults to WHITE and the absorption is
> `exp(log(colour) / distance * thickness)`: `log(1)` is 0, so the whole product is 0 and the
> transmittance is 1 whatever the other two say. **Set a colour without setting a distance and the
> engine invents an absorption over one metre.** `GLTFLoader` states the spec's defaults explicitly
> for that reason; the JSON material format still uses these, so any new consumer must decide which
> contract it is honouring rather than assume they agree.

### Material Opacity and GrabPass

`Material::Interface` provides two key query methods used by the rendering pipeline for render list dispatch:

- **`isOpaque()`**: Returns `!BlendingEnabled`, but also returns `false` when `requiresGrabPass()` is `true` (a material requiring grab pass is inherently non-opaque). It deliberately ignores `AlphaTestEnabled` — see [Alpha Test](#alpha-test--the-binary-cutout-contract-aug-2026).
- **`requiresGrabPass()`**: Virtual method (default `false`). Overridden by `StandardResource` based on material properties (e.g., transmission with screen-space refraction).

These are propagated through `Renderable::Abstract::isOpaque(layerIndex)` and `Renderable::Abstract::requiresGrabPass(layerIndex)` to all concrete renderables, enabling the Scene to dispatch into 3 render categories: Opaque, Translucent, and TranslucentGB.

**Code references:**
- `Material/Interface.hpp:isOpaque()` — non-virtual, checks blending and grab pass
- `Material/Interface.hpp:requiresGrabPass()` — virtual, default false
- `Material/StandardResource.hpp:requiresGrabPass()` — override
- `Renderable/Abstract.hpp:requiresGrabPass()` — pure virtual

### Alpha Test — the Binary Cutout Contract (Aug 2026)

`MaterialFlagBits::AlphaTestEnabled = 1U << 16` declares a material a **binary CUTOUT**: the fragment
shader discards the texels whose alpha falls below a cutoff, and the material **STAYS OPAQUE** — opaque
render list, depth write kept, no back-to-front sorting, state-sorted batching preserved.

Two setters raise the flag:

- **`BasicResource::enableAlphaTest()`** — fixed 0.5 cutoff. It requires a texture whose alpha channel
  is enabled (`setTextureResource(texture, true)`); without one the flag emits no code. Like every other
  material setter it refuses to act once the resource is created (it warns and returns).
- **`StandardResource::enableAlphaTest(threshold = 0.5)`** (Aug 2026) — **configurable, UBO-backed** cutoff
  (`AlphaThreshold`, UBO offset 51): the generated GLSL compares against the uniform, never a literal,
  so the threshold is per-material and runtime-adjustable (`setAlphaThresholdToDiscard()`). The alpha
  source is the **opacity texture component** when present (red channel), the **albedo texture alpha**
  otherwise (glTF `alphaMode: MASK`). It also disables the blending flag — cutout and blending are
  mutually exclusive by construction.

**Opacity — the owner's 3-rule contract (Aug 2026).** `StandardResource` expresses opacity exactly three
ways, parsed from the JSON `Opacity` component and mirrored by `setOpacityComponent()`:

1. **Scalar value [0,1]** → GLOBAL transparency: uniform alpha from the UBO (`Opacity`, offset 50),
   blending (glTF BLEND).
2. **Map + `AlphaThreshold` key** → binary CUTOUT: per-pixel discard below the UBO threshold, NO
   blending, stays opaque, casts cutout shadows, RT alpha-tests at the same cutoff (glTF MASK +
   `alphaCutoff`).
3. **Map without `AlphaThreshold`** → grayscale per-pixel alpha SCALE: `texel.r × amount`, blending.

Loader wiring: glTF `alphaMode MASK` → `enableAlphaTest(alphaCutoff)`; USD
`opacityThreshold > 0` → cutout, translucent USD/FBX materials get
`setOpacityComponent()` so the alpha VALUE finally reaches the blend (both used to raise the blending
flag with no alpha wired).

The discard fires on that flag **INDEPENDENTLY of the blending mode**. Gating it on blending was exactly
what used to force a cutout out of the opaque list: the only way to obtain a discard was to call
`enableBlending()`, which bought a distance sort that a coverage mask does not need.

| | `enableAlphaTest()` | `enableBlending(mode)` |
|---|---|---|
| Render list | **Opaque** (front-to-back, early-Z) | Translucent (back-to-front) |
| Colour blending | disabled (`blendEnable = VK_FALSE`) | enabled, per `blendingMode()` |
| Per-frame distance sort | none | mandatory |
| State-sorted batching | preserved | given up to the distance order |
| Transparency expressed | strictly binary — in or out | a genuine gradient |

Depth write is untouched by the flag: a cutout writes depth like any other opaque surface (depth write
is decided by the `RenderableInstance`, never by the material's transparency mode).

> [!WARNING]
> **`isOpaque()` must NOT be taught about `AlphaTestEnabled`, and must stay that way — an alpha-tested
> material IS opaque.** Returning `false` there does two damaging things at once:
>
> 1. The Scene dispatches the layer into the **distance-sorted translucent list**, paying for a sort
>    and losing the state-sorted batching, for a mask that has nothing to sort.
> 2. `Vulkan::GraphicsPipeline::configureColorBlendState()` keys its default branch on
>    `material.isOpaque()`: a `false` flips `blendEnable` to `VK_TRUE` and installs the blend factors of
>    `blendingMode()`, so the cutout's already-binary alpha gets **colour-blended** on top.
>
> Either one defeats the flag entirely. This is the single invariant that makes the cutout mode worth
> having: the flag adds a discard and changes **nothing else** about how the material is classified.

**The two other paths honour the flag as well:**

- **`isAlphaTest()`** returns `true` for `AlphaTestEnabled` (in addition to `OpacityEnabled` and
  `BlendingEnabled`), so the **RT pipeline alpha-tests at hit time** — candidate hits are confirmed
  against the material's cutoff instead of being taken as solid.
  `StandardResource::exportRTMaterialData()` exports its UBO threshold as `alphaCutoff` (Basic keeps 0.5).
  See [`docs/reflection-pipeline.md`](../../docs/reflection-pipeline.md).
- **`requiresAlphaTestedShadows()`**: a cutout must cast a **CUTOUT shadow**, not a solid rectangle.
  `StandardResource` returns `true` when an alpha source exists AND (the flag is set **OR** the blending
  mode is `Normal`), and its shadow discard **reads the UBO threshold** (the shadow fragment shader
  declares the material uniform block — the colour pass and the shadow agree by construction).
  ⚠️ The `BlendingMode::Normal` branch is load-bearing, not belt-and-braces: a JSON `"Opacity"` texture
  arms blending and NOT the flag, and no JSON key can request a cutout — gating on the flag alone made
  every JSON-authored foliage shadow its quad. See [`docs/shadow-mapping.md`](../../docs/shadow-mapping.md).

> [!CAUTION]
> **BasicResource's cutoff is FIXED at 0.5** (StandardResource's is configurable — see above). The program
> caches now key on the material FLAG BITS as well as the descriptor layout hash (Aug 2026: both
> `Renderable::ProgramCacheKey` and the generators' `computeProgramCacheKey()` fold in
> `material->flags()`), so the *structural* presence of the discard is discriminated. But plain VALUES
> are still not part of any key: a per-material cutoff **literal** baked into the generated GLSL could
> still serve one material's program to another sharing layout and flags. The rule is therefore:
> **a configurable threshold lives in the material UBO** (StandardResource's `AlphaThreshold` slot) — never
> in the GLSL. Basic cannot follow: its 12-float material block is FULL (diffuseColor 0-3,
> specularColor 4-7, shininess 8, opacity 9, autoIllumination 10, emissiveStrength 11); growing it is
> the price of ever making Basic's cutoff configurable. See
> [`docs/pipeline-caching-system.md`](../../docs/pipeline-caching-system.md).
>
> 0.5 is the right value for a mask authored as coverage, and Basic's **three paths agree at 0.5**: the
> colour discard, the shadow discard, and `GPURTMaterialData::alphaCutoff`. Standard's three paths agree
> on its UBO threshold the same way.

**Which mode for which authoring intent:**

| The asset expresses… | Use |
|---|---|
| A **coverage mask** — cutout foliage, a fence, a grate, a Doom two-sided middle texture (vanilla writes the texel straight to the framebuffer and never reads the destination, so its transparency is strictly binary) | **alpha test** |
| A **genuine gradient** — smoke, a soft particle, glass that tints what is behind it | **blending** |
| **Refraction** — bending what is behind the surface | **grab pass** (`requiresGrabPass()` → TranslucentGB) |

**Code references:**
- `Material/Interface.hpp:MaterialFlagBits::AlphaTestEnabled` — the flag and its contract
- `Material/Interface.hpp:isOpaque()` — blind to the flag ON PURPOSE
- `Material/Interface.hpp:isAlphaTest()` — RT hit-time alpha test
- `Material/StandardResource.cpp:requiresAlphaTestedShadows()` — alpha source AND (flag OR `BlendingMode::Normal`)
- `Material/StandardResource.hpp:enableAlphaTest(threshold)` — the configurable, UBO-backed setter
- `Material/StandardResource.cpp:parseOpacityComponent()` — the 3-rule JSON contract
- `Material/StandardResource.cpp:alphaSourceTextureComponent()` — opacity component, else albedo alpha
- `Material/StandardResource.cpp:generateShadowAlphaTestCode()` — shadow discard against the UBO threshold
- `Material/GPURTMaterialData.hpp:alphaCutoff` — the RT side (Basic 0.5, Standard = UBO threshold)
- `Graphics/Renderable/ProgramCacheKey.hpp:materialFlags` — codegen flags in the program cache key
- `Vulkan/GraphicsPipeline.cpp:configureColorBlendState()` — the `isOpaque()` branch

### Normal Map Scale

The `normalScale` parameter (offset 6) controls normal map intensity by scaling the tangent-space XY components before re-normalizing:

```glsl
vec3 raw = texture(normalSampler, uv).rgb * 2.0 - 1.0;
vec3 normal = normalize(vec3(raw.xy * ubMaterial.normalScale, raw.z));
```

- `1.0` = full normal map effect (default)
- `0.5` = half intensity (smoother bumps)
- `0.0` = flat surface (normal map ignored)

**Code references:** `StandardResource.cpp:generateFragmentShaderCode()`

### Parallax Occlusion Mapping (POM)

POM ray-marches through a height map in the fragment shader to create depth/relief illusion on flat surfaces without extra geometry. Uses `ComponentType::Displacement` with height map textures.

**Activation conditions** (all must be true):
1. Material has a Height component (`m_useParallaxOcclusionMapping`)
2. The renderer asks for the high quality tier (`Generator::Abstract::HighQualityEnabled`)
3. POM iterations > 0 (`POMIterationsKey > 0`)

When active, a displaced UV (`pomTexCoords`) is computed at the start of the fragment shader and ALL subsequent texture samples use it automatically via `textCoords()`.

**Key implementation details:**
- Height map convention: white = high, black = low. POM inverts: `depth = 1.0 - texture().r`
- UV displacement uses `pomViewDir.xy * heightScale` directly (no `/z` division — prevents angle-dependent depth)
- Loop uses compile-time constant upper bound with early `break` for GPU safety
- Occlusion interpolation (relief mapping refinement) for smooth results
- `mutable bool m_pomGenerationActive` flag set at generation time, checked by `textCoords()` to return correct UV variable

**Distance-based POM fade:**
- POM effect fades out based on camera distance to prevent GPU stress on large surfaces
- Full effect within 8 world units, fully disabled beyond 18 units
- Both `heightScale` and `numLayers` are scaled by the fade factor
- Complete early-out when `pomFade < 0.001` (returns original UVs, no ray-marching)
- Uses `smoothstep(8.0, 18.0, distance)` for smooth transition

**Vertex shader requirements** (when POM active):
- `TangentToWorldMatrix` — transform view direction to tangent space
- `PositionWorldSpace` — fragment world position
- `CameraWorldPosition` — camera position (reuses Reflection/Refraction output if present)

**Code references:**
- `StandardResource.cpp:generateFragmentShaderCode()` — POM GLSL generation (+ distance fade)
- `StandardResource.cpp:textCoords()` — UV variable selection
- `Saphir/Keys.hpp:ParallaxTextureCoordinates` — `"pomTexCoords"`
- `Saphir/Keys.hpp:HeightSampler` — `"uHeightSampler"`

## 6. Bindless Textures Manager

### Overview

The `BindlessTexturesManager` provides a global descriptor set with arrays of textures that can be indexed dynamically in shaders using non-uniform indexing. This eliminates the need to rebind descriptor sets for each material.

> [!CRITICAL]
> **Per-scene ownership — the manager only ever reflects the ACTIVE scene.**
> The bindless table has no scene concept of its own. Each scene owns a
> `Scenes::BindlessTextureSet` (the bindless analogue of `LightSet`) that **describes** the
> textures it uses (RT material textures, light color projections, environment cubemap) and
> **allocates its own dynamic slots** from `FirstDynamicSlot`. Scene code (`SceneMetaData`,
> light emitters, `Scene::enable`) registers into that per-scene set, **never into the manager
> directly**. The manager READS the active scene's set each frame via
> `BindlessTextureManager::syncTextureSet(set, sceneTimeMS)` (driven by the `Renderer` right
> after `Scene::prepareRender`) and writes the descriptor table from it — this also performs
> the per-frame animated-texture frame-view swap and the environment-cubemap write (falling
> back to the engine default cubemap when the scene has none).
>
> Because only one scene is active at a time and the table is mirrored from the active set,
> two scenes may legitimately reuse the same slot indices — **table capacity is the largest
> scene, not the sum of all scenes**.
>
> **On scene disable**, `Scenes::Manager::disableActiveScene()` calls
> `BindlessTextureManager::clearTextureSet(scene.bindlessTextureSet())` (under the exclusive
> lock): a `device->waitIdle()` then each of that scene's freed dynamic slots is overwritten
> with an engine dummy (2D → dummy color-projection 2D; cube + reserved env slot → default
> cubemap). This is REQUIRED — the global table outlives a single scene, so a scene that stops
> being active must leave no descriptor behind, otherwise a later `deleteScene` destroys
> samplers/images still referenced by the descriptor set (`VUID-vkDestroySampler-sampler-01082`).
> The set's CPU data persists with the scene (re-enable re-syncs it). cube-array slots have no
> dummy yet (rare; see `docs/caution-points.md`).
>
> This replaced an earlier design where scenes registered/unregistered directly in the manager,
> which leaked slots and left dangling descriptors on scene switch.

### Architecture

```
┌─────────────────────────────────────────────────────────────────────┐
│ BindlessTexturesManager (single global descriptor set)              │
├─────────────────────────────────────────────────────────────────────┤
│ Binding 0: sampler1D[256]   │ 1D texture array                      │
│ Binding 1: sampler2D[4096]  │ 2D texture array                      │
│ Binding 2: sampler3D[256]   │ 3D texture array                      │
│ Binding 3: samplerCube[256] │ Cubemap texture array                 │
└─────────────────────────────────────────────────────────────────────┘
```

### Table Capacities Are Device-Dependent — Never Hardcode Them

> [!CRITICAL]
> The array sizes are **resolved at runtime** in `BindlessTextureManager::computeCapacities()` from
> the device's update-after-bind budget. The `DesiredMaxTextures*` constants (256/4096/256/256/64)
> are a **target, not the effective capacity**.
>
> **Why:** a `COMBINED_IMAGE_SAMPLER` descriptor is charged to BOTH the sampler and the sampled-image
> update-after-bind limits, per set AND per stage. Desktop drivers advertise millions and get the
> desired capacities. MoltenVK advertises **1024 samplers** (Metal's argument-buffer limit) while
> sampled images stay at 1M, so the desired total (4928) blew the budget by ~5x and every pipeline
> layout including the bindless set was rejected. Without the validation layers it did not error — it
> was silently undefined behaviour.
>
> **Reduced profile** on Apple GPUs: **1D[32] 2D[768] 3D[32] Cube[128] CubeArray[32]**, announced by
> a `TraceWarning` at startup. A headroom is withheld from the budget because the update-after-bind
> VUIDs sum **every** set of a pipeline layout, including non-UAB ones such as the SSR/RTGI inputs.
>
> **Rules:**
> - Bound a slot index with `manager.maxTextures2D()` and friends, **never** with
>   `DesiredMaxTextures2D`.
> - The per-scene `Scenes::BindlessTextureSet` receives these capacities from `Scene`'s constructor
>   via `setCapacities()` — a set handing out a slot beyond the table size would have its descriptor
>   write rejected by the manager and the texture would never appear.
> - The generated GLSL declares **unbounded** arrays (`Declaration::Sampler::UnboundedArray`), so
>   capacities never leak into shader code. Keep it that way.
> - Read the startup trace before blaming a missing texture on anything else.

### Reserved Slots

> [!IMPORTANT]
> Each slot is an index into **ONE typed array** — cube slots index `texturesCube[]`
> (binding 3), 2D slots index `textures2D[]` (binding 1). The numbering restarts per array.

| Slot | Array | Constant | Purpose | Written by |
|------|-------|----------|---------|------------|
| 0 | cube | `EnvironmentCubemapSlot` | Scene environment cubemap | `syncTextureSet()` per frame (default cubemap fallback) |
| 1 | cube | `IrradianceCubemapSlot` | IBL diffuse irradiance (32² RGBA16F, stores E/π) | `Scene::updateEnvironmentIBL()` → `BindlessTextureSet` → `syncTextureSet()` (default fallback) |
| 2 | cube | `PrefilteredCubemapSlot` | IBL GGX-prefiltered environment (128² RGBA16F, 6 mips) | same path as slot 1 |
| 3 | 2D | `BRDFLutSlot` | Split-sum BRDF LUT (128² RGBA16F, scale/bias on F0 in RG) | `Renderer::createDefaultResources()` at boot, baked by `Compute::IBLBaker` |
| 4 | 2D | `GrabPassSlot` | Scene color grab pass | Renderer per frame |
| 5 | 2D | `GrabPassDepthSlot` | Scene depth grab pass | Renderer per frame |
| 16+ | all | `FirstDynamicSlot` | Dynamic texture allocation | per-scene `BindlessTextureSet` |

### Usage

**Registering a texture (scene side — into the per-scene set, NOT the manager):**
```cpp
// e.g. from SceneMetaData::rebuild or a light emitter
uint32_t index = scene.bindlessTextureSet().registerTexture2D(texture); // global table index
// Store 'index' in the material SSBO / light UBO for shader access.
```

**Reflecting the active scene into the GPU table (Renderer side, per frame):**
```cpp
bindlessManager.syncTextureSet(scene.bindlessTextureSet(), scene.lifetimeMS());
```

**Updating reserved slots directly (Renderer-owned: grab pass, default env, IBL):**
```cpp
bindlessManager.updateTexture2D(BindlessTextureManager::GrabPassSlot, grabPass);
```

**In GLSL shaders:**
```glsl
layout(set = BINDLESS_SET, binding = 1) uniform sampler2D textures2D[];

// Access with non-uniform index
vec4 color = texture(textures2D[nonuniformEXT(textureIndex)], uv);
```

### Color Projection via Bindless

Light color projection textures are registered into the **scene's `BindlessTextureSet`** during `createOnHardware()` or asynchronously via `ObserverTrait` notification when resource loading completes. Each light UBO carries a `ColorProjectionIndex` field (`uint` encoded as `bit_cast<float>`) that indexes into the bindless 2D or Cube array. The light stores a `Scenes::BindlessTextureSet *` (set in each light's setup from `scene.bindlessTextureSet()`), not a manager pointer.

- **2D lights** (directional, spot): `set.registerTexture2D()` → `sampler2D` array at binding 1
- **Point lights**: `set.registerTextureCube()` → `samplerCube` array at binding 3
- **Sentinel value**: `UINT32_MAX` means no color projection texture assigned
- **Unregistration**: by texture instance (`set.unregisterTexture2D(texture.get())`), done in the light's `destroyFromHardware()`

The bindless set is bound during lighting passes when `renderPassUsesColorProjection(renderPassType)` returns true, alongside the standard environment cubemap usage.

**Code references:**
- `Scenes/Component/AbstractLightEmitter.cpp:registerColorProjectionInBindless()` - Registration
- `Scenes/Component/AbstractLightEmitter.cpp:unregisterColorProjectionFromBindless()` - Cleanup
- `RenderableInstance/Abstract.cpp:render()` - Bindless set binding condition
- `Saphir/Generator/SceneRendering.hpp` - Pipeline layout enablement

### Lifecycle Constraints

> [!CRITICAL]
> **VMA Allocation Order**
>
> The BindlessTexturesManager holds references to Vulkan resources. During shutdown:
> 1. `Renderer::clearDefaultResources()` releases texture references
> 2. `ResourceManager::unloadUnusedResources()` frees VMA allocations
> 3. Only then can `Device::destroy()` safely destroy VMA allocator
>
> The `Core::terminate()` loop calls `unloadUnusedResources()` after each service
> to ensure proper cleanup order.

**Code references:**
- `BindlessTexturesManager.hpp/cpp` - Manager implementation
- `Renderer::createDefaultResources()` - Default cubemap initialization
- `Renderer::clearDefaultResources()` - Cleanup before shutdown

### GrabPass — NEED-DRIVEN arming (Aug 2026)

The grab blit (scene capture for TranslucentGB refraction/transmission) records whenever the
frame contains TranslucentGB objects (`Scene::hasTranslucentGBObjects()`); `Renderer::enableGrabPass(true)`
remains a manual force-on.
> [!WARNING]
> The flag alone used to gate the blit and NOTHING in the engine ever set it: the machinery
> was pre-allocated but dead, and every grab-pass material sampled an unfilled bindless slot
> (measured on CarConcept: uniform sky-blue glass, no interior, and NO "GrabPass" zone in
> `getGPUTimings()`). **The GPU timings are the one-command diagnostic**: a grab-pass material
> on screen without a GrabPass zone in the frame = the blit is not armed.

## 7. Frame Rate Limiter

Optional software frame rate limiter for precise FPS control.

**Settings key:** `Core/Video/FrameRateLimit` (default: 0)

| Value | Behavior |
|-------|----------|
| `0` | Disabled (unlimited FPS) |
| `60` | Limit to 60 FPS |
| `144` | Limit to 144 FPS |
| etc. | Target FPS value |

**Implementation:**
- Hybrid sleep + busy-wait for precision
- Sleep for bulk of remaining time (saves CPU)
- Busy-wait for final ~1ms (timing accuracy)

**When to use:**
- Linux with compositor (GNOME/KDE): VSync OFF + FrameRateLimit ON
- Reduce GPU power consumption without VSync
- Consistent frame pacing for recording/streaming

**Code references:**
- `Renderer.cpp:renderFrame()` - Frame limiting logic at end of function
- `Renderer.cpp:onInitialize()` - Setting initialization
- `Renderer.hpp:m_frameRateLimit`, `m_frameDuration`, `m_frameStartTime`
- `SettingKeys.hpp:VideoFrameRateLimitKey`

## 8. Material Component System

### Base colour — the texture is TINTED by the colour (Aug 2026)

When the albedo component is a **Texture**, the generated fragment code multiplies the
sampled texel by the material's base colour:

```glsl
const vec4 SurfaceAlbedoColor = texture(AlbedoSampler, uv) * MaterialUB(AlbedoColor);   /* StandardResource */
```

This is what every source format means: glTF `baseColorFactor` and FBX `base_color` both specify
the **PRODUCT** of factor and texture. Both loaders used to call `setAlbedoComponent(texture)` and
**drop the factor entirely** — a material tinted by factor over a neutral texture imported
untinted, and the factor's **alpha went with it**. They now set the tint through
`setAlbedoColor()` alongside the texture component.

> [!IMPORTANT]
> **`DefaultAlbedoColor` is `White`, and that is load-bearing — it was `Grey`.** The colour is now
> a multiplicative factor on the textured path, so its neutral value MUST be the multiplicative
> identity. Leaving it grey would have darkened **every** textured material in the engine by half.
> Only a material that configures no colour at all sees any change, and white is the correct
> neutral there too. ⚠️ `BasicResource` tints the same way but kept `DefaultDiffuseColor{Grey}` —
> the cheap tier is not covered by this reasoning.

> [!WARNING]
> **The multiplication is UNCONDITIONAL, and must stay that way.** The shader program cache keys
> on the material's **descriptor layout**, never on its flags or values, so a variant emitted only
> when the tint differs from white could serve one material's program to another sharing the same
> layout. Same reasoning as the fixed 0.5 alpha-test cutoff — see
> [Alpha Test](#alpha-test--the-binary-cutout-contract-aug-2026).

> [!NOTE]
> `AlbedoColor` is declared **unconditionally** in `getUniformBlock()`, whatever
> the component's filling type — the uniform block is a fixed layout mirroring the whole
> `m_materialProperties` array. That is precisely why the textured path may reference them.

### Scalar components — source CHANNEL + multiplying FACTOR (Aug 2026)

A texture-driven scalar component (roughness, metalness) reads **one color channel** of the
sampled texel, selected per component via `Component::Texture::setSourceChannel()`
(`EmEn::Base::PixelFactory::Channel`, **default Red** — the grayscale/single-channel convention).
Packed textures select theirs: glTF metallic-roughness packs **roughness in GREEN** and
**metalness in BLUE** (glTF 2.0 § material.pbrMetallicRoughness; reference implementation:
Khronos glTF-Sample-Renderer `material_info.glsl`, `getMetallicRoughnessInfo()`). JSON materials
may set the optional `"SourceChannel"` key (numeric, 0:R 1:G 2:B 3:A) on a texture component.

> [!WARNING]
> **Reading the wrong channel does not fail — it flattens.** The RED channel of a packed glTF
> metallic-roughness texture is typically EMPTY (measured mean 0.9/255 on DamagedHelmet, while
> G and B carried stdev 73 and 108): both properties silently collapse to ~0 over the whole
> surface — mirror-perfect dielectric everywhere, zero surface disparity, no error anywhere.
> That is a *material-identity* bug that reads like a lighting bug.

The generated definition folds **channel, inversion and factor** — every consumer (direct-light
BRDF, IBL prefiltered LOD, transmission LOD, material-properties G-buffer) reads this single
final variable and must NEVER re-apply any of them:

```glsl
const float SurfaceRoughness = texture(RoughnessSampler, uv).g * MaterialUB(Roughness);  /* factor contract */
const float SurfaceMetalness = texture(MetalnessSampler, uv).b * MaterialUB(Metalness);
/* smoothness/gloss source (m_invertRoughness): (1.0 - texel) BEFORE the factor applies */
```

- The UBO scalar is the **VALUE** when no texture drives the component, and the **MULTIPLYING
  FACTOR** when one does — the glTF `roughnessFactor * texel.g` / `metallicFactor * texel.b`
  contract. `DefaultTextureFactor{1.0F}` is the neutral default of the texture overloads —
  **same precedent as the White `DefaultAlbedoColor`** (a 0.5 default would halve every map;
  the old `DefaultMetalness` 0.0 would ZERO metalness maps out).
- **Format translation is the loader's job**: glTF factors multiply (pass them through); in
  **FBX a connected texture REPLACES the scalar** — the loader passes the neutral factor,
  never the authored scalar (a metalness scalar of 0, the FBX default, would erase the map).
- **RT parity**: `RTTextureSlot` carries the channel; `SceneMetaData` packs it into the RT
  material `flags` as 2-bit indices (`RoughnessChannelShift`/`MetalnessChannelShift`), plus
  `RoughnessTexInverted` for gloss sources. The RTR hit shading applies channel, inversion and
  factor exactly like the raster (see `Effects/Framebuffer/RTR.cpp`) — keep both sides in sync.

### Per-component UV transform — UBO values, never literals (Aug 2026)

Texture components carry a UV transform (`uv * scale + offset`) stored ON the component
(`Component::Texture::setUVWScale/setUVWOffset`, JSON keys `"UVW"` / `"UVWOffset"`) and synced
at creation into per-component material UBO vec4 slots (offsets 56-79) — Albedo/Roughness/
Metalness/Normal/AmbientOcclusion/AutoIllumination.
Applied UNCONDITIONALLY with the identity neutral (1,1,0,0) — same precedent as the White
albedo. Public API: `StandardResource::setComponentUVWTransform()` (components without a slot
return false). Source: glTF `KHR_texture_transform` via the loader.
⚠️ The `m_UVWScale` member existed for years but NO codegen consumed it — a transform that
is stored but never applied fails SILENTLY (stretched textures, zero log).
⚠️ RT hit shading does not apply these transforms yet — known parity gap.

### FillingType Enum

Material components use `FillingType` to determine how data is sourced:

| Value | Description | Data Format |
|-------|-------------|-------------|
| `Value` | Single float | Numeric JSON |
| `Color` | RGB/RGBA color | Array `[r, g, b]` or `[r, g, b, a]` |
| `Texture` | 2D texture | Object `{ "Name": "path" }` |
| `VolumeTexture` | 3D texture | Object `{ "Name": "path" }` |
| `Cubemap` | Cubemap texture | Object `{ "Name": "path" }` |
| `AnimatedTexture` | Animated texture | Object `{ "Name": "path" }` |
| `AlphaChannelAsValue` | Use alpha as value | Object |
| `Automatic` | Auto-configure | Optional params (Amount, IOR, etc.) |
| `None` | Disabled | No data required |

**Code reference:** `Graphics/Types.hpp:FillingType`

### Component JSON Parsing

All material components follow the same parsing pattern via `parseComponentBase()`:

```json
{
    "ComponentName": {
        "Type": "Texture",
        "Data": { "Name": "Category/TextureName" },
        "OptionalParam": 1.0
    }
}
```

**Special case - Automatic type:**
- No `Data` key required
- Parameters read directly from component object
- Used for Reflection/Refraction to use scene environment cubemap

```json
{
    "Reflection": { "Type": "Automatic", "Amount": 0.1 },
    "Refraction": { "Type": "Automatic", "IOR": 1.5 }
}
```

> [!CRITICAL]
> **`"Shininess"` in a manifest is a GLOSSINESS in [0,1], and the lit material stores a ROUGHNESS.**
> The whole data store was authored as a perceptual glossiness (3834 material files of 3917 hold
> `0.1`). Since the material merge there is no Blinn-Phong exponent left on the lit path: the
> conversion happens at the parse boundary ONLY, in `StandardResource::parseSpecularComponent()`,
> and it is the canonical complement
>
> ```
> roughness = 1 - clampToUnit(glossiness)   // 0.1 -> 0.9 | 0.4 -> 0.6 | 1.0 -> 0.0
> ```
>
> (Khronos archived spec-gloss extension). The absent-key fallback is `DefaultRoughness{0.5F}`.
> ⚠️ The Khronos "F0 = specular colour" half is DELIBERATELY NOT applied: legacy Phong specular
> colours are highlight intensities (bright greys), not a dielectric F0 (~0.04) — mapped raw they
> read near-mirror. F0 stays the 0.04 dielectric default; the low roughness carries the highlight.
> ⚠️ `BasicResource` reads the SAME key as a raw Blinn-Phong exponent (`DefaultShininess{200}`,
> no conversion) — the cheap tier still shades Blinn-Phong. Do not port either rule to the other.
>
> See `docs/caution-points.md`, "The legacy specular was not energy-normalised, and `Shininess` was
> authored as a glossiness".

**Code references:**
- `Graphics/Material/Helpers.cpp:parseComponentBase()` - Base parsing
- `Graphics/Material/StandardResource.cpp:parseReflectionComponent()` - Automatic handling
- `Graphics/Material/StandardResource.cpp:parseSpecularComponent()` - Glossiness → roughness

### Material Types Array

> [!CRITICAL]
> **All material resource types must be registered in `Material::Types`!**
>
> `Materials.hpp` defines the valid material types for JSON validation:
> ```cpp
> constexpr auto Types = std::array< std::string_view, 2 >{
>     BasicResource::ClassId,      // "MaterialBasicResource"
>     StandardResource::ClassId    // "MaterialStandardResource"
> };
> ```
>
> Missing types cause silent fallback to `BasicResource` during mesh loading.
>
> ⚠️ There is ONE lit material since the merge: `StandardResource` IS the Cook-Torrance
> metallic-roughness material (it kept the `"MaterialStandardResource"` ClassId). The name
> `PBRResource` and the ClassId `"MaterialPBRResource"` no longer exist. In a JSON **scene**
> definition the `"Type"` strings `"Standard"` and `"PBR"` are both accepted, as synonyms
> (`Scenes/DefinitionResource.cpp`).

## 9. Shadow Mapping & Color Projection Global Control

The `Renderer` provides a global shadow mapping enable/disable via `isShadowMapsEnabled()`.

**Setting key:** `GraphicsShadowMappingEnabledKey` (`Core/Graphics/Renderer/ShadowMappingEnabled`)

**Integration with Scene:**
The Scene checks this setting when selecting `RenderPassType` for each light. The pass type is selected from a 4-branch matrix:

| Shadow | Color Projection | Pass Type (example: Spot) |
|--------|-------------------|---------------------------|
| No | No | `SpotLightPass` (0 samplers) |
| Yes | No | `SpotLightPassShadowMap` (1 sampler) |
| No | Yes | `SpotLightPassColorMap` (1 sampler) |
| Yes | Yes | `SpotLightPassFull` (2 samplers) |

Each pass type generates a **distinct shader program**. When a feature is inactive, its sampling code is not generated — no dummy texture samples, no wasted GPU cycles.

**Descriptor set architecture:** Each light creates a 2-binding descriptor set (UBO + shadow sampler) when shadow mapping is active, or uses the shared UBO-only descriptor set otherwise. Color projection is handled via the global `BindlessTextureManager` — the light UBO carries a bindless index, and the shader samples from the bindless texture array. See: Section 6 → Color Projection via Bindless.

**Why global control matters:**
Without the global check, disabling shadow mapping via settings caused Vulkan validation errors because shadow map images remained in `VK_IMAGE_LAYOUT_UNDEFINED` but descriptor sets still tried to bind them.

**Code references:**
- `Renderer.hpp:isShadowMapsEnabled()` - Global accessor
- `Scenes/Scene.rendering.cpp:renderLightedSelection()` - 4-branch pass type selection
- `Graphics/Types.hpp:RenderPassType` - 16-value combinatorial enum
- `Graphics/Types.hpp:renderPassUsesColorProjection()` - Color projection helper
- `SettingKeys.hpp:GraphicsShadowMappingEnabledKey` - Setting key

See [`docs/shadow-mapping.md`](../../docs/shadow-mapping.md) for complete shadow mapping and color projection architecture.

## 10. Video Recording (Graphics::Recorder — "RushMaker" video track)

Studio-quality video recording of the Vulkan swap-chain framebuffer, encoded VP9/IVF.
Part of the RushMaker studio workflow: separate tracks (video IVF + game audio WAV + voice-over WAV)
assembled by an auto-generated ffmpeg script (`Core::startAudioVideoRecording()`).

### Encoder selection (hardware H.265 vs software VP9)
ONE decision point: `Recorder::hardwarePath()`. Hardware when the device exposes Vulkan
Video H.265 encode, software VP9 otherwise (the fallback is the cross-hardware guarantee:
AMD/Intel/older GPUs record without any configuration). Everything downstream follows the
choice automatically — file extension (`.h265` / `.ivf`), container and audio codec in the
assemble script (MP4/AAC / WebM/Opus).
**`Core/RushMaker/ForceCPUEncoding`** (default `false`) forces the software path on a
hardware-capable device: A/B comparison of the two encoders, and royalty-free WebM/VP9 on
demand. The startup log states which one is active — `Encoder: hardware H.265` /
`software VP9 (forced by settings)` / `software VP9 (no hardware support)`.

### ONE mode: studio CFR (owner decision, Aug 2026)
The RushMaker produces promotion rushes — **image quality is the only metric**. There is no
realtime/quality mode duality: one quality-first VBR configuration (16-frame lookahead,
complexity AQ, `VPX_DL_GOOD_QUALITY`, bitrate ladder via `QualityPreset`), and the output is
**constant frame rate on the wall clock** — the video timeline is real time, so the separately
recorded audio tracks stay in sync and the engine remains interactive during capture.
Should a low-latency capture need ever arise, it will be a **separate concept ("Streamer")**,
not a mode of this recorder.

### Pipeline
1. **GPU async readback** (4-slot round-robin) — copies swap-chain image to host-visible staging
   buffer, paced at the target FPS on the wall clock (`shouldCaptureFrame()`)
2. **Bounded grab buffer** (`Core/RushMaker/MaxQueuedFrames`, default 32) — gives the encoder time
   to write the file; above the bound, captures are **skipped and counted** (backpressure) so a
   slow encode cannot balloon RAM
3. **Dedicated encoding thread** — BGRA→I420 conversion (SIMD dispatched: scalar/SSE4.1/AVX2),
   VP9 encoding, IVF writing at **constant frame rate**: every missing capture slot (renderer
   slower than the target FPS, backpressure skip) is filled by re-encoding the PREVIOUS image
   (`EncodingSession::encodeImageAt()` called before the next conversion overwrites the planes —
   a static VP9 frame costs almost nothing). The timeline never judders and never drifts.
   Encoder threads are capped at **hardware_concurrency/4**: the runtime stays interactive —
   half the cores at cpuUsed=1 measurably starved the logic and rendering threads (Aug 2026).

### Time model (owner rule, Aug 2026): the ONLY realtime element is the capture
The capture runs at the target FPS on the wall clock; the **encoder is free to take its
time** — it works in libvpx's quality path (`VPX_DL_GOOD_QUALITY` on every encode, including
the flush) and keeps draining in background after the recording stops. Never reintroduce
`VPX_DL_REALTIME` (a libvpx deadline constant, not a recorder mode — the realtime notion was
removed from this recorder entirely).

### Adaptive encoder speed (real frames beat encoding effort)
Software VP9 at maximum effort cannot hold 30 FPS at high resolution (measured Aug 2026:
cpuUsed=1 at 2880×1620 ≈ 6 real FPS → 4 frames out of 5 were CFR duplicates — unusable rush).
The encoding thread therefore **adapts `VP8E_SET_CPUUSED` live** (allowed mid-stream by libvpx),
**inside the good-quality mode only** (speeds 1..5 — 6+ belongs to the realtime path, unused):
once per second, if the grab buffer sits at its bound (captures being skipped), speed goes up
one notch (real frames beat per-frame effort); when the buffer drains below a quarter, speed
eases back toward the preset value. The next session **warm-starts** from the converged speed
(`m_adaptedCpuUsed`) instead of replaying the ramp. The periodic stats print `Speed:` — watch
it to know what the CPU actually sustains; the definitive fix for encoding at full effort and
full rate is the Vulkan Video hardware chantier.
Buffer knob: `Core/RushMaker/MaxQueuedFrames` (default 90 ≈ 3 s of elasticity at 30 FPS;
one buffered frame = width×height×4 bytes ≈ 1.6 GB at 2880×1620 — raise it for short takes
if RAM allows: zero skip, the encoder finishes in background). ⚠️ `getOrSetDefault` persists
the first-seen value: an older settings.json may still carry 32.

### Colorimetry (BT.709 — do not regress to BT.601)
The BGRA→I420 conversion uses **BT.709 limited-range** coefficients — single source of truth:
**`VideoColorConversion.hpp`** (`VideoColor::YCoef*/UCoef*/VCoef*`), consumed by the CPU
scalar/SIMD paths (`Recorder.cpp`) AND by the GPU compute converter (the GLSL receives them
through `VideoColor::glslDefines()`). The matrix is signalled in the VP9 bitstream
(`VP9E_SET_COLOR_SPACE` = BT.709, studio range) and tagged at container level by the generated
ffmpeg script (`-colorspace bt709 ...`). HD players assume BT.709; BT.601 coefficients on HD
content shift hues on playback.

### Hardware encode chantier (Vulkan Video H.265 — in progress, Aug 2026)
- **M1 done** — device plumbing: `Vulkan/Instance.cpp` enables `VK_KHR_video_queue` +
  `VK_KHR_video_encode_queue` + `VK_KHR_video_encode_h265` when present and logs the H.265
  encode capabilities; `Vulkan/Device` configures the dedicated VIDEO_ENCODE queue family
  (`videoEncodeH265Enabled()`, `getVideoEncodeQueue()`). Validated on the RTX 3070 Ti:
  8192×8192 max, 16 DPB slots, 120 Mbps, 7 quality levels, queue family #4.
- **M2 done** — `Graphics/VideoFrameConverter`: compute BGRA→NV12 planes (R8 luma full-res +
  R8G8 chroma half-res), **integer math identical to the CPU converters** — validated
  byte-for-byte via the console command `Core.RendererService.testVideoFrameConverter()`
  (procedural hash pattern generated identically in GLSL and C++, no upload involved).
- **M3 done** — `Vulkan/VideoEncoderH265`: session + memory binding, std VPS/SPS/PPS (driver
  returns the encoded Annex-B header via `vkGetEncodedVideoSessionParametersKHR`), two-slot
  DPB (separate images), VBR rate control from the presets, IDR+P GOP, encode-feedback query,
  Annex-B packets. Validated end-to-end via `Core.RendererService.testVideoEncoderH265()`:
  90 frames / 3 GOPs, ffmpeg decode with ZERO errors, decoded content matches the pattern.
  **Three NVIDIA lessons paid for in blood (do not regress):**
  1. `VkVideoEncodeH265RateControlLayerInfoKHR` MUST be chained to the rate-control layer —
     without it `vkCmdControlVideoCoding` invalidates the command buffer
     (`vkEndCommandBuffer` → `VK_ERROR_INITIALIZATION_FAILED`, validation layer silent).
  2. Picture images (src + DPB) MUST be allocated aligned on
     `pictureAccessGranularity` (32×32 on NVIDIA; we align on 64) — an under-aligned image
     HANGS the encode engine (infinite `waitIdle`). The logical `codedExtent` stays exact.
  3. Do NOT force `cu_qp_delta_enabled_flag` in the std PPS — the driver manages it; forcing
     it desynchronises the P-slice entropy (decoder reads out-of-range `cu_qp_delta`).
  Also required: the `synchronization2` device feature (video barriers are sync2-only) —
  enabled with the video extensions in `Instance.cpp`.
- **M4 done** — Recorder integration, validated pixel-exact against a live framebuffer
  screenshot (animation-debug @ 2880×1620 Ultra). Hardware path when
  `videoEncodeH265Enabled()`: swap-chain → GPU snapshot slots (image copy, NO CPU readback)
  → `VideoFrameConverter::convertFrom()` (sampler variant) → `VideoEncoderH265` on a
  dedicated encoding thread (CFR fillers by re-encoding the still-loaded planes). Software
  VP9 path untouched as the cross-hardware fallback. `Core.toggleRecording()` console
  command (= Shift+Ctrl+F12). The assemble script muxes `.h265`+WAV → MP4/AAC with BT.709
  tags (`-framerate` input option is mandatory for a raw elementary stream).
  **v1 is ALL-INTRA (idrPeriod 1)** — the professional mezzanine layout (frame-exact seek,
  no error propagation, ideal for editing); hardware preset bitrates are raised accordingly
  (8/16/30/60 Mbps).
  ⚠️ **Open issue — P frames**: with references enabled, P frames predict from an empty
  reconstruction (green frames) whatever the DPB organisation (separate images, layered
  array, per-frame availability barriers, slot-index conventions, SPS minCB 8/16,
  codedExtent variants — all bisected on a deterministic 2880×1620 structured-pattern
  bench, `Core.RendererService.testVideoEncoderH265()`). IDR frames are pixel-perfect.
  Next lead: trace the nvpro reference encoder with gfxreconstruct and diff the API
  streams, or try explicit quality-level session parameters. All-intra sidesteps it.
  More NVIDIA lessons (in addition to the M3 three): the bitstream buffer SIZE must be
  aligned on minBitstreamBufferSizeAlignment (a misaligned dstBufferRange corrupts the
  stream — 1280×720 was aligned by luck, 2880×1620 was not) and a bench with a STRUCTURED
  test pattern is mandatory — corruption is invisible in noise (the M3 "clean" validation
  was noise-blind).
- **HDR10 lookahead (owner request)**: keep the profile/bit-depth parametric — Main 10 +
  P010 planes (16-bit containers) + BT.2020/PQ shader variant sourcing the PRE-tonemap HDR
  buffer + mastering-display SEI. Nothing in M2/M3 may hardcode 8-bit assumptions in the API.

### Symmetric API

| Method | Purpose |
|--------|---------|
| `startRecording(path)` | Begin recording to IVF file at given path |
| `stopRecording()` | Stop recording, flush encoder, patch IVF frame count |
| `isRecording()` | Check if recording is active |
| `shouldCaptureFrame()` | Frame pacing check (target FPS) |
| `captureAndSubmitFrame()` | Capture current frame via async GPU readback |

Path generation is owned by `Core::startAudioVideoRecording()` — see `src/AGENTS.md` Core section.

### Transfer Queue Optimization
When a dedicated transfer queue family is available, uses a two-step copy path:
1. Graphics queue: swap-chain image → device-local buffer (with layout transitions)
2. Transfer queue: device-local → host-visible staging (DMA, signaled by semaphore)

### Code References
- `Recorder.hpp` — Full class with Doxygen documentation
- `Recorder.cpp` (top) — Shared BT.709 conversion coefficients (single source of truth)
- `Recorder.cpp:startRecording()` — VP9 init (incl. colour-space signalling), async resource creation, thread start
- `Recorder.cpp:captureAndSubmitFrame()` — Backpressure gate + harvest + GPU copy submit
- `Recorder.cpp:encodingThreadFunc()` — CFR filler loop + BGRA→I420 + VP9 encode
- `Recorder.cpp:EncodingSession::encodeImageAt()` — Encode current image at a given PTS (also duplicates into empty CFR slots)
- `Recorder.cpp:submitGPUCopy()` / `submitTransferQueueCopy()` — Async readback paths
- `cmake/SetupLibVPX.cmake` — Build configuration for libvpx

## 11. Animated Texture Cubemap System

### Overview

The Animated Texture Cubemap system provides animated cubemap textures stored as **Vulkan cube arrays**. It consists of two resource layers:

| Resource | File | Purpose |
|----------|------|---------|
| `CubemapMovieResource` | `Graphics/CubemapMovieResource.hpp/cpp` | CPU-side frame data (6 face pixmaps per frame + duration) |
| `AnimatedTextureCubemap` | `Graphics/TextureResource/AnimatedTextureCubemap.hpp/cpp` | GPU-side Vulkan TextureCubeArray wrapping a CubemapMovieResource |

**Primary use case:** Color projection textures for **point lights** (animated light patterns, fire flicker, etc.).

### Architecture

```
┌─────────────────────────────────────────────────────────────┐
│ CubemapMovieResource (CPU)                                   │
│  std::vector< pair< CubemapPixmaps, uint32_t > >            │
│  Frame 0: [6 face pixmaps] + duration (ms)                   │
│  Frame 1: [6 face pixmaps] + duration (ms)                   │
│  ...                                                         │
│  Frame N: [6 face pixmaps] + duration (ms)                   │
└──────────────────────┬──────────────────────────────────────┘
                       │ load()
                       ▼
┌─────────────────────────────────────────────────────────────┐
│ AnimatedTextureCubemap (GPU)                                  │
│  VkImage (CUBE_COMPATIBLE, arrayLayers = 6 × frameCount)     │
│  VkImageView (CUBE_ARRAY)                                    │
│  VkSampler ("AnimatedCubemap", no mipmap, no anisotropy)     │
└─────────────────────────────────────────────────────────────┘
```

**Memory layout:** All frames are packed into a single cube array image. Each frame occupies 6 consecutive array layers. `totalLayers = CubemapFaceCount × frameCount`.

**Frame indexing:** The W coordinate selects the frame index at runtime. `request3DTextureCoordinates()` returns `true`.

### CubemapMovieResource JSON Formats

**Data store directory:** `./data-stores/CubemapMovies/`

#### Parametric Loading (numbered sequence)

Generates cubemap names from a pattern with zero-padded indices:

```json
{
    "BaseCubemapName": "FireProjection/frame_{3}",
    "FrameCount": 30,
    "FrameRate": 24,
    "IsLooping": true
}
```

- `BaseCubemapName`: Pattern with `{N}` where N = zero-padding width. E.g. `{3}` → `001`, `002`, ...`030`
- `FrameCount`: Total frames (required)
- Frame timing (pick one, priority order):
  - `FrameRate`: FPS → `duration = 1000 / fps`
  - `AnimationDuration`: Total ms → `duration = total / frameCount`
  - `FrameDuration`: Per-frame ms (default: `1000/30 ≈ 33ms`)
- `IsLooping`: Loop animation (default: `true`)

Each generated name (e.g. `FireProjection/frame_001`) is resolved as a `CubemapResource` from the cubemap container.

#### Manual Loading (explicit frame list)

```json
{
    "Frames": [
        { "Cubemap": "Effects/fire_burst_01", "Duration": 50 },
        { "Cubemap": "Effects/fire_burst_02", "Duration": 50 },
        { "Cubemap": "Effects/fire_burst_03", "Duration": 100 }
    ]
}
```

- `Frames`: Array of frame objects
  - `Cubemap`: CubemapResource name (required)
  - `Duration`: Frame duration in ms (default: `33ms`)

### Sampler Configuration

The `AnimatedTextureCubemap` sampler differs from regular texture samplers:

| Property | AnimatedTextureCubemap | Regular Textures |
|----------|------------------------|------------------|
| Mag/Min Filter | Settings-driven (linear/nearest) | Settings-driven |
| Mipmap Mode | `NEAREST` | `LINEAR` |
| Anisotropy | `VK_FALSE` | Settings-driven |
| Max LOD | `0.0` | Computed |

**Rationale:** No mipmaps are generated for animated textures (single mip level), so mipmap filtering and anisotropy are disabled.

### Resource Container Registration

Both resources are registered in `Resources::Manager`:

```cpp
// Container aliases
using CubemapMovies = Resources::Container< Graphics::CubemapMovieResource >;
using AnimatedTextureCubemaps = Resources::Container< Graphics::TextureResource::AnimatedTextureCubemap >;
```

Both containers share the `"CubemapMovies"` local store directory.

### Usage Pattern

```cpp
// Get the default animated cubemap resource (for color projection)
auto animCubemap = resources.container< TextureResource::AnimatedTextureCubemap >()->getDefaultResource();

// Set as color projection texture on a point light
lightComponent.setColorProjectionTexture(animCubemap);
```

**Default resource:** In debug mode, generates 3 frames (Red, Green, Blue) at 32×32. In release, generates 5 noise frames.

### Animation Timing

- `frameCount()`: Number of frames in the animation
- `duration()`: Total animation duration in ms (sum of all frame durations)
- `frameIndexAt(sceneTime)`: Returns frame index for a given scene time point
  - Loops via `timePoint % duration` when looping is enabled
  - Clamps to last frame when not looping and past duration

### Procedural Caustics Generation

`CubemapMovieResource::loadCaustics()` generates animated Voronoi water caustics programmatically:

```cpp
bool loadCaustics(
    uint32_t faceSize,        // Cubemap face resolution (e.g. 128)
    uint32_t frameCount,      // Number of animation frames (e.g. 60)
    uint32_t frameDuration,   // Duration per frame in ms (e.g. 33)
    float scale = 4.0F,       // Voronoi cell density (higher = finer)
    uint32_t seed = 0,        // Random seed for pattern
    float baseIntensity = 0.7F,    // Background brightness [0,1]
    float causticIntensity = 1.0F  // Caustic line brightness [0,1]
) noexcept;
```

**Algorithm:** Inverted Voronoi F2-F1 distance (bright at cell edges = caustic lines, dark at cell centers). Temporal animation uses a circular sin/cos path through noise space for seamless looping. See: `Base/Algorithms/VoronoiNoise.hpp` for the underlying noise.

**Usage pattern:** See `projet-alpha/src/Builtin/PoolRooms.cpp:onSetupLighting()`.

### Code References

- `Graphics/CubemapMovieResource.hpp/cpp` — CPU frame storage, JSON loading, procedural caustics
- `Graphics/TextureResource/AnimatedTextureCubemap.hpp/cpp` — Vulkan cube array texture resource
- `Base/Algorithms/VoronoiNoise.hpp` — Voronoi noise: `evaluate()`, `caustic()` (F2-F1 clamped)
- `Resources/Manager.cpp` — Container registration (lines 531-533)
- `projet-alpha/src/Actor/Fire.cpp:77` — Usage: fire point light color projection
- `projet-alpha/src/Builtin/LightAndShadowDebug.cpp:113` — Usage: debug scene color projection
- `projet-alpha/src/Builtin/PoolRooms.cpp` — Usage: procedural caustics in closed room

## 12. Post-Processing Effects

### Overview

The engine provides a multi-pass post-processing pipeline via `PostProcessor`. Effects chain together: each effect's output becomes the next effect's input. All effects inherit from `PostProcessEffect`.

**Requirements contract** — The `PostProcessor` aggregates its chain's needs and exposes them as a formal contract:
- `requiresHDR()` / `requiresDepth()` / `requiresNormals()` / `requiresVelocity()`
- The Renderer queries these methods to decide scene target format, MRT attachments, etc.
- No manual toggle (the old `enableHDR()` has been removed). Requirements are inferred from the active effect chain.
- `requiresRayTracing()` effects are additionally gated per frame on
  `Renderer::isRayTracingReady()` (TLAS built AND RT descriptor sets live): during the async
  TLAS build of a scene's first frames — or in a scene with no RT geometry — the chain skips
  them and forwards the previous output. An RT effect's `execute()` can therefore assume a
  consumable TLAS; never record a trace pass behind a `if (rtDescSet != nullptr)` bind alone
  (drawing with set 0 unbound is what that guard silently allowed before Aug 2026 — see
  `docs/caution-points.md` § Vulkan Validation).
- `requiresJitter()` (temporal anti-aliasing) — when any effect in the active stack declares
  it, `Renderer::prepareFrameJitter()` advances a Halton (2,3) sub-pixel sequence and applies
  it to the MAIN view only, once per rendered frame, before the video-memory update. Implies
  `requiresVelocity()` in practice: a temporal effect without motion vectors smears.
  ⚠️ The jitter reaches the shaders through **per-draw push constants**, never through the
  view UBO — see § 16 Rule 4 for the data race that design cost, and
  `src/Saphir/AGENTS.md` § "TAA Sub-Pixel Jitter" for the three-site lockstep contract.
  `ViewMatricesInterface` exposes both forms and they are NOT interchangeable:
  `projectionMatrix(readStateIndex)` serves the **jittered** matrix (what the frame was
  rasterized with — post-process depth unprojection, CPU-computed MVP paths), while
  `unjitteredProjectionMatrix(readStateIndex)` serves the clean one and MUST be used by
  anything feeding a velocity clip position (the InstanceTransforms SSBO header, the pushed
  view-projection of the paths that jitter in the shader).

- `PushConstants::deltaTime` — duration of the previous RENDERED frame, in seconds, clamped to
  [1/1000, 1/15]. The chain's SINGLE source of truth for anything converting the per-frame
  velocity G-buffer into a physical duration: `MotionBlur` divides the camera's shutter speed by
  it to get the shutter angle (how many frames of motion the exposure covers). Effects must NOT
  measure time themselves — a chain where two effects disagree on the frame duration cannot be
  reasoned about. Zero on the direct (lens) chain, which has no temporal consumer.
- ⚠️ **Amplifying the velocity buffer requires a raw dead zone.** Its two clip positions come
  from differently-computed matrix products, so a static camera leaves ~1e-4 px of rounding
  noise. Any effect scaling velocity by more than 1 must reject that on the RAW per-frame
  magnitude, before its own factor — see `docs/caution-points.md` § "A velocity buffer has a
  floating-point noise floor".

### Inter-Pass Synchronization — the MoltenVK contract (Aug 2026)

Chained post-process passes render into `IntermediateRenderTarget`s whose render pass declares
full (non-by-region) `VK_SUBPASS_EXTERNAL` dependencies in both directions — that is the
**Vulkan-level** guarantee, and Synchronization Validation is clean with it alone.

It is NOT sufficient on MoltenVK. Back-to-back render passes become separate Metal command
encoders, and the external-dependency translation was **measured insufficient on Apple M2**:
tile-granular stale reads in the motion-blur chain (displaced 64 px blocks around moving
objects), uninitialized reads in the bloom chain (green blocks), and implausible luminance
samples in the auto-exposure chain (`ToneMapping::meteredRejectedCount()` at ~2/s). The
corruption is timing-dependent — `MTL_DEBUG_LAYER=1` serialization suppresses it completely,
which is how it was cornered.

**The contract:** `IndirectPostProcessEffect::recordFullscreenPass()` emits an explicit
`vkCmdPipelineBarrier` (write→read, `SHADER_READ_ONLY → SHADER_READ_ONLY`, no layout change)
after `endRenderPass()`, forcing a real inter-encoder fence. On conforming desktop drivers it
is redundant with the subpass dependencies and free. **Any pass recorded OUTSIDE
`recordFullscreenPass()` that writes an image another pass samples must emit the same barrier
itself.** Full story: `docs/troubleshooting.md` § "Blocky corruption on macOS",
`docs/caution-points.md` § Vulkan Validation.

### Light Attenuation — Physical, Not Artistic (since 2026-07-26)

Point and spot lights fall off as a **windowed inverse square** (Karis, "Real Shading in Unreal
Engine 4", SIGGRAPH 2013), generated by `Saphir/LightGenerator.PerFragment.cpp`:

```glsl
saturate(1 - (d/r)^4)^2 / (d^2 + 1)
```

The `+1` removes the singularity at the source (a bare `1/d^2` blows up any surface touching the
light, at the cost of one lux of accuracy at one meter). The window forces the contribution to
reach exactly zero at the radius, which an inverse square never does, so the renderer keeps
culling lights by radius; squaring it removes the visible edge.

This is what makes a light intensity in **candela** mean anything: the illuminance it produces is
`I/d^2`. The previous falloff, `max(1 - (d/r)^2, 0)`, was radius-bounded and artistic — nothing
was proportional to `1/d^2` anywhere, so a value in lumens was arbitrary.

`legacyUnitCompensation` — the TEMPORARY per-light factor that restored the pre-change
brightness while content was authored in the old units — is GONE: photometric phase 2
removed it (see [`docs/todo/photometry-phase-2-relight-demos.md`](../../docs/todo/photometry-phase-2-relight-demos.md)), the generated falloff is the clean
`saturate(1-(d/r)^4)^2 / (d^2+1)`.

**Interface** (`PostProcessEffect.hpp`):
- `create(renderer, width, height)` — Allocate GPU resources (IRTs, pipelines, descriptors)
- `destroy()` — Release resources
- `resize(renderer, width, height)` — Recreate on window resize
- `execute(commandBuffer, inputColor, inputDepth, inputNormals, constants)` — Run effect
- `requiresDepth()` / `requiresNormals()` / `requiresHDR()` — Declare input dependencies

### Available Effects

| Effect | File | Passes | Dependencies |
|--------|------|--------|-------------|
| **SSAO** | `Effects/Framebuffer/SSAO.hpp/cpp` | Multi-pass | Depth, Normals |
| **SSR** | `Effects/Framebuffer/SSR.hpp/cpp` | 5-pass (Trace→Resolve→BlurH→BlurV→Composite) | Depth, Normals, HDR |
| **Bloom** | `Effects/Framebuffer/Bloom.hpp/cpp` | Multi-pass | HDR |
| **DepthOfField** | `Effects/Framebuffer/DepthOfField.hpp/cpp` | 7-pass (Focus→Setup→DilateH/V→FarGather→NearGather→Composite) | Depth, MaterialProps, **camera-materialized** |
| **ToneMapping** | `Effects/Framebuffer/ToneMapping.hpp/cpp` | Multi-pass (auto-exposure chain) | HDR, **camera-materialized** |
| **VolumetricLight** | `Effects/Framebuffer/VolumetricLight.hpp/cpp` | 2-pass (Occlusion+EMA ping-pong → RadialBlur); IGN-dithered march, jitter-compensated mask, `temporalAlpha` 0.2 (sub-pixel sources rasterize jitter-unstable — caution-points § dash train) | Depth, HDR |

> [!CAUTION]
> **`VolumetricLight` is NOT a volumetric effect, and its settings keys are an OVERRIDE, not a
> default.** It is the screen-space radial-blur god ray (Mitchell, GPU Gems 3): the occlusion mask
> is a depth threshold at 0.9999 — "this pixel is sky" — and the second pass marches in SCREEN space
> toward the sun's projected position. `density` is a screen-space step multiplier, `decay` an ad-hoc
> geometric falloff, and `exposure` an arbitrary gain converting the light's **LUX** into the **nits**
> buffer. **There is no participating medium anywhere in it**: no scattering coefficient, no phase
> function, no height profile, and nothing shared with `AtmosphericFog`'s.
>
> ⚠️ Its keys (`Core/Graphics/VolumetricLight/{Density,Decay,Exposure,SampleCount,TemporalAlpha}`)
> are read with `settings.get(key, m_parameters.x)` — **`get()`, not `getOrSetDefault()`, and the
> CURRENT parameter as the fallback**. This deliberately breaks the TAA/MotionBlur contract, where a
> setting overrides the constructor and registers itself in the file. Five demos pass deliberately
> tuned values (Citadel 1.2/0.97/0.12/96, Liminal 0.6/0.98/0.12/96, LightAndShadowDebug and
> BasicScenery 0.8/0.98/0.12/64) and an engine-wide default would **silently double their god rays**
> (exposure 0.12 against a 0.25 default); worse, `getOrSetDefault` would let whichever demo runs
> FIRST write its own values into a key the other seven then inherit. An absent key must change
> nothing.
>
> Verified: with no key the frame sits at 580 k differing pixels against a 634 k run-to-run noise
> floor — below it, so nothing moved; with `Exposure = 1.0` the sun-facing mean goes 188.4 → 240.5.
>
> **The keys exist to make this effect comparable at runtime**, because it had none at all while
> eight demos used it, and the world-space single-scattering pass meant to replace it needs an A/B
> that does not require a rebuild. Every one of these knobs becomes meaningless the day the medium
> is real.
| **AtmosphericFog** | `Effects/Framebuffer/AtmosphericFog.hpp/cpp` | 1-pass | Depth, HDR |
| **RTR** | `Effects/Framebuffer/RTR.hpp/cpp` | 4-pass (Trace→BlurH→BlurV→Composite) | Depth, Normals, RT (TLAS+SSBOs) |
| **RTGI** | `Effects/Framebuffer/RTGI.hpp/cpp` | SVGF chain (Trace→Temporal→Moments→NormalHistory→À-trous×N→Apply); all post-trace passes live in the owned `GIDenoiser` | Depth, Normals, MaterialProps, Albedo, Velocity, RT (TLAS+SSBOs) |
| **RTAO** | `Effects/Framebuffer/RTAO.hpp/cpp` | Multi-pass | Depth, Normals, RT (TLAS+SSBOs) |
| **SSGI** | `Effects/Framebuffer/SSGI.hpp/cpp` | SVGF chain (Trace→GIDenoiser, same shape as RTGI) | Depth, Normals, MaterialProps, Albedo, Velocity, HDR |
| **ContactShadows** | `Effects/Framebuffer/ContactShadows.hpp/cpp` | Multi-pass | Depth, Normals |
| **LensFlare** | `Effects/Framebuffer/LensFlare.hpp/cpp` | Multi-pass | Depth, HDR |
| **FogEnvironment** | `Effects/Framebuffer/FogEnvironment.hpp/cpp` | 1-pass | Depth |

### SSR (Screen-Space Reflections)

> [!CAUTION]
> **The Hi-Z pyramid's mip 0 is a DOWNSAMPLE, not a copy — and it must be a MIN.** With pixel
> doubling on (`Core/Graphics/ScreenSpace/Reflection/PixelDoubling`), the trace target is half-res
> while the scene depth stays full-res. `SSRHiZCopyComputeShader` used to do
> `texelFetch(srcDepth, p)` with `p` the DESTINATION texel, which copied the source's **top-left
> corner** 1:1 into the whole pyramid — `sourceMaxX/Y` only ever clamped, they never scaled. The
> march then compared its rays against depths belonging to entirely different pixels.
>
> Measured before the fix (RenderDoc, `reflexion-debug --demo-options 0,5,0`): mip 0 held the
> top-left quarter magnified 2×, **88 %** of its texels at the far plane, and the trace kept hits
> on **3.19 %** of the screen (confidence is channel **B** of the trace target, not alpha —
> `outHit = vec4(hitUV, confidence, 0.0)`). After: **41.7 %** at the far plane, matching the real
> frame, and **13.59 %** hit rate — 4.3× more.
>
> ⚠️ The reduction is a MIN because a MIN pyramid is conservative; averaging depths invents a
> surface halfway between two of them. Mip 0 is no exception. ⚠️ The defect was LATENT at full
> resolution (the engine default), where destination and source sizes coincide — it only appears
> once pixel doubling is enabled, which is why nothing caught it.

> [!CAUTION]
> **A camera-ward ray is CLIPPED to the near plane, never rejected.** The trace used to bail out
> on `reflDir.z < 0.0` ("rays toward the camera cannot be resolved against a single depth layer").
> That threw away every reflection of the geometry sitting **between the surface and the eye** —
> on a mirror sphere, the entire near floor. Only the *projection* of an endpoint that crossed
> behind the eye was ill-defined; the screen-space segment itself is perfectly marchable, so the
> ray length is now solved against the near plane instead (McGuire & Mara, *Efficient GPU
> Screen-Space Ray Tracing*, JCGT 3(4), 2014, § 3).
>
> Measured (RenderDoc, `reflexion-debug --demo-options 0,5,0`): the sphere's hits occupied
> `hitUV.y ∈ [0.111, 0.578]` with **67 %** piled into the single 0.5–0.6 bin and the lower **42 %**
> of the screen never reached once — a hard wall, not a fade. After the clip, stone in the
> sphere's lower half went **7.6 % → 13.6 %** (RTR ground truth 28.7 %), confirmed on screen.
>
> ⚠️ This makes `D.z < 0` REACHABLE in the traversal, where `tPlane`'s `1e18` branch used to be
> dead code. `1e18` is the CORRECT answer there: depth decreases along such a ray, so one already
> in front of a cell's nearest surface can never meet it, and the free-flight branch is right —
> it is not a missed refinement. ⚠️ This was **not** a Y-up residual: the rejection predates the
> flip. The owner's inversion hypothesis was reasonable and wrong, and only the hit-destination
> histogram separated the two.
>
> ⚠️ **The 0.0746 confidence ceiling that looked like a bug was the FLOOR, not the sphere.** A
> disc validated at one camera pose was reused on a capture taken at another and measured
> pavement while reporting "sphere". Select a surface by what the shader itself publishes — the
> normals attachment's packed `roughness + metalness * 2` — never by a remembered pixel disc. On
> the real sphere pixels the trace is healthy: 62.3 % hit rate, mean confidence 0.494, max 1.0.
>
> ⚠️ `reflexion-debug` is **not run-to-run deterministic**: two captures from the SAME binary
> differ on ~326 k pixels (max 194 LSB). A bit-identical control is inapplicable on this scene —
> a change can only be shown to sit BELOW that floor.

> [!NOTE]
> **The residual gap against RTR is STRUCTURAL — do not spend another session chasing it.** After
> the near-plane clip, every miss on `reflexion-debug`'s mirror sphere was attributed by a
> temporary miss-reason code in the trace target's alpha (the resolve reads only `.xy` and `.z`, so
> the instrumentation could not change a pixel — and the hit count confirmed it, 3,225 vs 3,226).
> Pose (0, 2, 9), sphere selected by the normals attachment's packed value 2.1:
>
> | share of the sphere | reason |
> |---|---|
> | 41.6 % | hit |
> | 43.3 % | the reflected ray LEAVES THE SCREEN |
> | 9.1 % | end of the ray reached with no hit — rays aimed at the sky, and since the skybox sits at the far plane a ray never gets behind it, so the cubemap fallback is the CORRECT answer |
> | 6.0 % | step budget exhausted |
>
> The dominant term is the structural limit of screen space, which is exactly what RTR does not
> suffer and what the whole 13.6 % vs 28.7 % gap measures. **The step budget is not recoverable
> either**: raising `maxSteps` 128 → 512 moved that term 6.0 % → 2.0 % yet left the hit rate at
> 41.6 % — the freed rays go on to leave the screen or reach the ray's end, never to hit. Four
> times the iterations, zero extra hits. `maxSteps{128}` stays.
>
> ⚠️ The first attempt at that comparison was INVALID and said the opposite (hits 41.6 % → 11.5 %,
> step-exhaustion 6 % → 28 %, which is arithmetically impossible when the budget grows). The
> sphere mask held 8,059 texels instead of 7,749: under `renderdoccmd` everything is slowed by
> orders of magnitude and a fixed sleep does NOT guarantee the camera has been placed. **Read the
> pose back (`Act.getPosition()`) into the capture log, and treat the mask's texel count as the
> control** — a differing count means the two captures are not comparable, whatever the numbers
> say. ⚠️ Never let a capture runner delete previous `.rdc` files either: it destroyed the
> baseline the probe had to be compared against.

> [!CAUTION]
> **`needsMaterialProperties` is a COMBINE-PASS codegen request, not "give me the texture", and
> two disjoint delivery paths exist.** A previous revision of this file claimed the `reflection`
> nibble was "written and read by NOBODY" — that was WRONG, and the mistake came from grepping the
> effects for `materialPropsTex`/`matProps` while the generated combine sampler is named
> **`emMaterialProps`**. Seven effects read it there: `SSR.cpp:1655` and `RTR.cpp:1537` decode it
> as `float(uint(texture(emMaterialProps, vUV).r * 255.0) >> 4u) / 15.0` into
> `ssrReflectivity`/`rtrReflectivity`, gate on `> 0.0`, and weight their mix by it —
> `mix(em_Color.rgb, data.rgb / confidence, confidence * intensity * reflectivity)`. SSAO, SSGI,
> RTAO, RTGI and ContactShadows read their own nibbles the same way.
>
> The two paths:
> - **Overlay effects** (`producesOverlay()`) set `CombineContribution::needsMaterialProperties`
>   and read `emMaterialProps` in their combine snippet. `CombinePass` emits the sampler, hashes
>   it into the pipeline variant key, and binds `context.materialProperties` — aborting the whole
>   combine group with a `TraceError` if it is null. This allocates nothing.
> - **Direct effects** (`AtmosphericFog`, `Bloom`, `DepthOfField`) declare their own
>   `materialPropsTex` in `set = 0` at the last binding of `getInputLayout(N)`, and hand-write
>   `context.materialProperties` into their per-frame set inside `execute()`.
>
> Allocation is a THIRD, separate thing: the virtual `requiresMaterialProperties()`, OR-ed by
> `PostProcessStack` and turned into the `VK_FORMAT_R8G8B8A8_UNORM` MRT attachment by `Renderer`.
> `PostProcessor` skips any effect whose flag is set while the texture is null, so the pointer is
> guaranteed non-null inside `execute()`.

> [!CAUTION]
> **The post-process reflectivity is a UBO value behind a FLAG, never a GLSL literal.**
> `"Reflection": { "Type": "Value", "Data": x }` publishes a reflectivity for SSR/RTR without any
> cubemap or sampler. The scalar used to reach the shader as a baked literal
> (`declareSurfaceReflectivityMap("(" + std::to_string(amount) + ")")`), and **the program caches
> key on the descriptor layout and on material FLAG BITS — never on plain values**. Two materials
> differing only in that amount were one edit away from sharing a program built with the other
> one's literal. The scalar now lives in the UBO (sharing the `ReflectionAmount` slot, which this
> path never sets — the two branches of `parseReflectionComponent` are alternatives) and the
> routing in `MaterialFlagBits::PostProcessReflectivityEnabled`, which the caches DO key on.
>
> ⚠️ Same pass fixed the parse: it read `getValue(componentData, "Amount")` while
> `parseComponentBase` sets `componentData` to the **NUMBER itself** for a `Value` type, so the
> authored figure was silently replaced by the 0.5 fallback — the path had never honoured its own
> parameter. It now uses `parseValueComponent()`, the helper the Roughness component uses for the
> identical shape. **The correct JSON is `"Data": x`, not `"Amount": x`** — an earlier revision of
> this file documented the wrong key.
>
> ⚠️ **DIAGNOSED AND FIXED (2026-08-26) — it was a FLAG BIT COLLISION, and it was introduced by the
> commit above, not inherited.** `PostProcessReflectivityEnabled` was first written as `1U << 17`,
> the value `UnlitEnabled` already held. Nothing checks these values: a duplicate compiles silently,
> and `enableFlag(PostProcessReflectivityEnabled)` therefore also set `UnlitEnabled` — so declaring a
> post-process reflectivity on a material silently made it **UNLIT**.
>
> The symptom surfaced far from the cause. Authoring `"Reflection": { "Type": "Value", "Data": 0.0 }`
> on `Grounds/Pavement005` made that material's authored roughness of 0.8 read as the 0.5 default in
> the normals attachment — the 1,050,417-pixel floor group at packed 0.800 vanishing into the 0.500
> group — because the unlit codegen path does not declare roughness the same way. The material loaded
> without error and a JSON round-trip of the file was byte-identical, which is what made it look like
> a parsing mystery.
>
> ⚠️ **The mistake that produced it: "the enum" was read, but not to its closing brace.** The free-bit
> survey stopped at `AlphaTestEnabled = 1U << 16` and never reached `UnlitEnabled` further down. A
> `NEXT FREE BIT` marker now sits at the end of `MaterialFlagBits` — keep it current, and add new bits
> there.
>
> ⚠️ **And the earlier claim that the defect "predates this change" was FALSE.** It was asserted in a
> commit message without being tested: the `Value` path had never once been exercised against the
> ORIGINAL code, because the first attempt used the wrong JSON key and failed to load.
>
> Verified after moving the flag to `1U << 18`, same scene and pose: the floor's packed roughness is
> back to **0.800** (1,049,893 px) and its published reflectivity is exactly **0.0000** — which is
> also the first end-to-end validation that the `Value` path honours its authored parameter at all.

> [!CAUTION]
> **The material-properties A channel had TWO consumers and NO producer until Aug 2026.**
> `fogResponse` (high nibble) and `dofMask` (low nibble) were decoded faithfully by
> `AtmosphericFog` and `DepthOfField` — and the pack wrote a hardcoded literal `1.0` for the whole
> channel, so both nibbles were pinned at 15 forever and neither modulation did anything. A
> contract with two careful readers and no writer reads as working code in every review; the only
> way to catch it is to trace the value back to its producer.
>
> Both are now real material properties, following the pattern of the two live nibbles beside them
> (`aoResponse` from `aoIntensity`, `emissiveMask` from `autoIlluminationAmount`): continuous,
> UBO-backed, `clamp(x, 0, 1)`, authored by the root-level JSON keys `"FogResponse"` and
> `"DoFMask"` or by `setFogResponse()` / `setDoFMask()`. **Both default to 1.0**, so a manifest
> that says nothing keeps the previous behaviour exactly.
>
> ⚠️ They cost NO UBO growth: offsets 54-55 were the two STD140 padding floats before the first
> `vec4` UV transform, implicit on the GLSL side. Declaring them explicitly fills the hole without
> shifting a single later offset. There is no room left there — the next scalar needs a real
> layout change.
>
> Verified at runtime on `light-and-shadow-debug`: with the defaults the frame sits inside the
> scene's own run-to-run noise (646 k pixels / 33 LSB against a 634 k / 33 LSB floor from the same
> binary twice), and forcing the ground to `FogResponse = 0` drops it to a mean of **93.60** —
> **bit-identical to the same ground with the fog switched off entirely**.

> [!CAUTION]
> **`"Reflection": { "Type": "None" }` does NOT publish a zero reflectivity — the name is
> misleading, and the explicit opt-out is a DIFFERENT declaration.** With `None` the parse returns
> early, `m_useReflection` stays false, and `LightGenerator::materialPropertiesExpression()` falls
> through its priority ladder to `clamp(max(metalness, 1.0 - roughness), 0.0, 1.0)` — a
> **participation mask** for the traced reflections, deliberately not an energy weight (the code
> says why: a smooth DIELECTRIC — glass, metalness 0, roughness 0 — must participate, and the old
> `metalness * smoothness` product zeroed it so glass lost every traced reflection; the effects
> apply the real Fresnel and roughness fade themselves). Measured on `reflexion-debug`: the
> polished metal sphere publishes **1.0**, and `Grounds/Pavement005` — roughness 0.8,
> `Reflection: None` — publishes **0.2**, not 0.
>
> **To publish exactly zero, author `"Reflection": { "Type": "Value", "Amount": 0.0 }`.** No new
> format value is needed and none was added: `FillingType::Value` routes through
> `StandardResource::parseReflectionComponent()` to `m_postProcessReflectivityAmount`, which
> `setupLightGenerator()` hands to `declareSurfaceReflectivityMap()` — **priority 1**, the top of
> the ladder — as a GLSL literal. `ssrReflectivity`/`rtrReflectivity` then fail their `> 0.0` gate
> and the surface receives no traced reflection at all. The sentinel is `-1.0F` ("not declared"),
> so `0.0` is honoured rather than treated as absent. The other two ways to reach zero are the
> artistic-cubemap flag (`m_reflectionArtistic`) and a reflectivity map authored to zero.
>
> ⚠️ That literal is NOT keyed by the program caches, which hash descriptor layout + material FLAG
> BITS and never plain values. It is safe here only because the global key also hashes the
> **renderable's name** (`SceneRendering::computeProgramCacheKey()` point 3), so two materials with
> different `Amount`s on different renderables cannot collide. The residual hazard is narrow but
> real: swapping a material for a differently-valued one on the SAME renderable keeps the same key
> and reuses the program built with the OLD literal.

> [!NOTE]
> **The nibble quantization used to TRUNCATE — fixed Aug 2026, and it was worth a full step.** The
> pack was `uint(x * 15.0)`; it is now `uint(x * 15.0 + 0.5)` for all three live fields
> (reflectivity, aoResponse, emissiveMask; R's low nibble is hardcoded 0, G's low and B's high are
> hardcoded 15, A is a literal 1.0). Since 0.8 is not representable in binary,
> `1.0 - 0.8 = 0.19999998807907104` in float32, times 15 is `2.999999761581421`, and the bare
> `uint()` yielded **2** where the intent was 3 — the pavement published 0.1333 instead of 0.2, a
> 33 % under-report. Measured in the attachment with RenderDoc, at a verified pose (sphere mask
> bit-identical at 31,004 px): floor **0.1333 → 0.2000**, sphere **1.0 → 1.0** unchanged.
>
> ⚠️ Only EXACT values escaped the defect (metalness 1.0 gives exactly 15), which is why a mirror
> read a clean 1.0 and hid it for every other surface — a reference object that happens to sit on
> an exactly-representable value is the worst possible witness for a quantization bug.
> ⚠️ `+ 0.5` with truncation, not `round()`: GLSL leaves `round()`'s behaviour on a .5 tie
> implementation-defined. x is clamped to [0,1] upstream, so `x * 15.0 + 0.5` stays in [0.5, 15.5]
> and can never overflow the nibble.

> [!NOTE]
> **The resolve reads the trace target with `texelFetch`, not `texture`.** Channels R and G carry
> hit **coordinates**, which are not a filterable quantity. Trace and resolve are both half-res,
> so `vUV` lands on a texel centre and bilinear happened to return that texel untouched — the
> trap is latent, not active. Move the resolve to full-res with bilinear restored and every
> hit/miss boundary fabricates a UV halfway toward (0,0) carried by a non-zero confidence: the
> resolve samples the screen corner and believes it.

5-pass pipeline at half resolution (except composite at full-res):

1. **Trace**: Ray-marches in screen space using depth+normals, outputs hitUV + confidence
2. **Resolve**: Samples reflected color at hitUV; on SSR miss, falls back to environment cubemap
3. **Blur H**: Horizontal Gaussian blur on resolved colors
4. **Blur V**: Vertical Gaussian blur
5. **Composite**: Blends blurred SSR with scene color

**Cubemap Fallback** (UE4/UE5 standard approach):
When SSR ray finds no screen-space hit, the resolve pass reconstructs the reflection direction in view space, transforms to world space via inverse view matrix, and samples the environment cubemap. This eliminates black patches at screen edges.

Key design:
- Inverse view matrix (3×3 rotation) passed via push constants (3 × vec4 = 48 bytes)
- `envFallbackIntensity` parameter controls fallback strength (0.0 = disabled, 0.3 = default)
- Cubemap set via `setEnvironmentCubemap()` before `create()`; falls back to `Renderer::getDefaultTextureCubemap()` if none set
- Resolve descriptor set: 6 bindings (color, trace, depth, normals, pyramid, albedo — the effect
  declares `requiresAlbedo()`)

**Primary-surface Fresnel is a COLOR (same model as RTR, Aug 2026):** the resolve pass tints the
reflection (hit AND cubemap-fallback paths) by `F = F0 + (1-F0)·(1-NdotV)⁵` with
`F0 = mix(vec3(0.04), albedo, metalness)` — a metal reflects in its own color, a dielectric at
the physical 4 % head-on.
> [!WARNING]
> **The Fresnel rides on the COLOR, never on the confidence.** The combine computes
> `mix(scene, ssrData.rgb / confidence, confidence × intensity × reflectivity)` — any per-pixel
> weight folded into the confidence is CANCELLED by that division. Only the color survives.

**Reflectivity mask (material-properties G-buffer, R high nibble) is PARTICIPATION, not energy
(Aug 2026):** materials without an explicit reflection component publish
`max(metalness, 1 - roughness)` (`LightGenerator::materialPropertiesExpression()`, priority 3) —
`max`, not the former product `metalness × (1-roughness)`, which zeroed out every smooth
DIELECTRIC (glass: metalness 0 → mask 0 → no traced reflection at all). The physical attenuation
belongs to the effects' per-pixel Fresnel + roughness fade, never to the mask.

**Code references:**
- `Effects/Framebuffer/SSR.hpp` — Parameters, ResolvePushConstants, setEnvironmentCubemap()
- `Effects/Framebuffer/SSR.cpp` — Shader source, descriptor layouts ("SSRResolveInput"), pipeline creation
- `PostProcessEffect.hpp` — Base interface
- `PostProcessor.hpp/cpp` — Chain management, push constants. `configure()` retires its previous grab pass + per-frame descriptor sets through `Renderer::deferredDestructor()` (frames-in-flight safety, no mid-frame `waitIdle`) — see `src/Vulkan/AGENTS.md`, "Deferred destruction contract". `recordBlit()` (and `GrabPass::recordBlit()`) follow the **batched barrier contract**: exactly two batched `pipelineBarrier()` calls around the back-to-back copies, never one barrier per transition — see [`docs/post-processing-pipeline.md`](../../docs/post-processing-pipeline.md) § 3 before touching either.

### AtmosphericFog (Exponential Height Fog)

Single-pass analytical fog using closed-form integral (no iterative sampling). Reads depth buffer to reconstruct world-space positions, applies exponential height fog with directional inscattering.

**Algorithm:**
1. Reconstruct world position from depth + camera basis vectors (push constants)
2. Exponential height fog integral: `ρ(y) = density * exp(k * (y - baseHeight))` along the view ray
3. Directional inscattering (simplified Henyey-Greenstein): bright halo when looking toward the sun
4. Sky fog option: when `skyFogEnabled = true`, fog covers skybox pixels using `maxDistance` as fictive distance

**Push constants** (116 bytes): Camera basis (pos, right, forward), depth reconstruction (near, far, tanHalfFovY, aspectRatio), fog params (density, heightFalloff, baseHeight, maxDistance, color), inscatter params (lightDir, exponent, color, intensity), skyFogEnabled.

**Height fog sign convention:** `Parameters::heightFalloff` is a **POSITIVE decay rate** — how fast density falls off going **UP** (`+Y`). The shader NEGATES it (`float k = -fogHeightFalloff;`) before feeding `exp(k · (y − baseHeight))`. Never pass a negative value.

⚠️ The negation was missing until Aug 2026, so under Y-up the fog grew **DENSER WITH ALTITUDE**. It was silent because the analytic integral below it stays valid for either sign of `k`: nothing breaks, nothing warns, the fog is simply upside down.

> [!CAUTION]
> **This section used to claim the ray reconstruction "was always sound […] it rides the signed
> `tanHalfFovY` contract and followed the flip on its own. Do not fix it in passing." That was
> WRONG, and it actively told the next reader to leave a live defect alone.** The shader rides the
> contract; the **C++ did not hand it the contract**. `execute()` recomputed
> `std::tan(fovDeg · π / 360)` locally, without `projectionYSign` — the only site in the engine that
> did — while `PostProcessor` publishes the signed value in `context.constants` and SSAO, SSGI and
> SSR all forward it. The shader's own header even states "the DOWNWARD screen direction is carried
> by the SIGN of tanHalfFovY (negative since the Y-up flip)", i.e. it documented a contract its
> caller was breaking.
>
> ⚠️ **A genuine Y-up residual, unlike two others found the same week.** Before the flip
> `projectionYSign` was `+1`, so recomputing the magnitude was *harmless*; the local recomputation
> became wrong at the exact moment of the flip. (Contrast the point-light gobo and SSR's
> camera-ward ray rejection, both of which `git log -S` places six months BEFORE the flip — check
> the history before filing a sign error under a nearby migration.)
>
> ⚠️ **Commit `2571a4b6` claimed this file** in its signed-`tanHalfFovY` pass and delivered nothing:
> it added `abs(t)` to the X terms, and `t` was already a positive magnitude, so the edit was a
> **no-op**. Its own message admitted the fog was never verified by observation. A fix applied to a
> site nobody has run is a fix you have not made.
>
> Measured on `light-and-shadow-debug` at a pinned sunny-16 (auto-exposure OFF), same pose
> throughout: with the sign wrong, `exp(k · heightDiff)` overflows to `+inf` for the whole sky above
> the horizon, `fogAmount` is exactly 1.0, and the sky is REPLACED — sky mean **207.5**, ground
> 114.9, i.e. fog thicker with altitude. After forwarding `constants.tanHalfFovY`: sky **70.1**,
> ground **134.7** — thinner up, thicker down, the palm's fronds legible again and the skybox back.
> Without the sign bug the saturation would have been confined to ~1.3° above the horizon.

> [!CAUTION]
> **The MEDIUM belongs to the scene, not to the effect.** `AtmosphericFog::Parameters` used to hold
> both: a participating medium (density, height falloff, base height, max distance, chromaticity,
> luminance) and the technique's own knobs (inscatter exponent and intensity, sky fog). The medium
> half now lives in `Scenes::ParticipatingMedium`, owned by the `Scene` beside its
> `EnvironmentPhysicalProperties`, and reaches effects through `FrameContext::medium` the way
> `skyLuminance` already does.
>
> ⚠️ It had to move before anything else could share it: an effect's `Parameters` are private to one
> instance and effects in a stack cannot see each other, while `AtmosphericFog` is used by ONE demo
> and `VolumetricLight` by EIGHT. "Share the fog's medium" had no instance to share with in seven
> cases out of eight.
>
> ⚠️ **No medium means NO FOG** — the effect returns the chain colour untouched and warns once. An
> atmospheric fog without an atmosphere is a pass-through, not a fog with invented parameters; that
> is the whole point of having exactly one place that describes the air. Every `Scene` defaults to
> `ParticipatingMedium::Vacuum()`, so a scene that adds the effect must declare a medium.
>
> ⚠️ `ParticipatingMedium::density` is an extinction coefficient in **1/m**.
> `VolumetricLight::Parameters::density` is a **screen-space step multiplier** for a radial blur and
> has no physical unit. Same word, nothing in common — never merge them.
>
> Verified as a pure move on `light-and-shadow-debug`: the four band means agree to **four decimal
> places** across the change (sky 70.1415 → 70.1416, ground 130.3641 → 130.3644), with the deviation
> the same order as between two runs of the SAME binary. The differing-pixel count sits above a
> single-sample noise floor (480 k against 138 k) while its maximum stays below it and no band mean
> shifts — dithering phase, not a value change.

> [!CAUTION]
> **`fogColor` and the inscatter colour are CHROMATICITIES and must be scaled to nits.** The effect
> composites into the ABSOLUTE-LUMINANCE buffer, before tone mapping. Until Aug 2026 the [0,1]
> parameters were pushed raw, so the fog carried ~0.6 **nits**: with `skyFogEnabled` the sky's fog
> amount saturates (the fictive ray length is `maxDistance`) and the sky was not fogged but
> **overwritten with black**. Measured: sky mean 7.05 with fog off → **1.34** with fog on, while the
> ground did not move — the fog only ever touched the sky.
>
> `Parameters::luminance` (nits) now carries the scale; negative, the default, derives it from the
> scene's main directional light as `L = E · ρ / π` — the same Lambertian relation the engine uses
> for a lit surface, with `E` the illuminance in lux and `ρ` the chromaticity. Same separation
> `VolumetricLight` already makes between `mainLight->color()` and `mainLight->intensity()`.
> `execute()` now also returns `inputColor` untouched when there is no directional light, instead of
> dereferencing a null `mainDirectionalLight()`.
>
> ⚠️ **A [0,1] constant reaching this buffer is always a bug.** It reads black at any real exposure,
> and the mistake is invisible in code review because the numbers look like colours.

See `docs/caution-points.md` for the Y-reconstruction pitfall.

**Code references:**
- `Effects/Framebuffer/AtmosphericFog.hpp` — Parameters, FogPushConstants, API
- `Effects/Framebuffer/AtmosphericFog.cpp` — GLSL shaders, pipeline setup, camera extraction

### RTR (Ray-Traced Reflections)

4-pass pipeline using `GL_EXT_ray_query` in a fragment shader (no RT pipeline required):

1. **Trace** (half-res): Reconstructs world position from depth, traces reflection ray via TLAS,
   samples hit material (bindless albedo texture or scalar), computes Lambert lighting at hit point
2. **Blur H**: Horizontal Gaussian blur
3. **Blur V**: Vertical Gaussian blur
4. **Composite**: Blends blurred reflection with scene color using confidence (alpha)

**Descriptor sets** (trace pass):
- Set 0: RT data from `Renderer::rtDescriptorSet()` — TLAS (binding 0), mesh metadata SSBO (binding 1),
  material data SSBO (binding 2), light array SSBO (binding 3)
- Set 1: Input textures — depth (binding 0), normals (binding 1), environment cubemap (binding 2),
  scene albedo (binding 3 — the effect declares `requiresAlbedo()`)
- Set 2: Bindless textures from `BindlessTextureManager` — sampler2D[] (binding 1)

**Primary-surface Fresnel is a COLOR (fixed Aug 2026):**
`F0 = mix(vec3(0.04), originAlbedo, originMetalness)` — a metal tints its reflection by its
albedo (gold reflects gold, a teal dome reflects teal), a dielectric reflects the physical 4%
head-on. The scalar lobe weight (max component) keeps feeding the premultiplied confidence
pipeline; the NORMALIZED tint multiplies the traced color, on both the hit and the
environment-miss paths.
> [!WARNING]
> The former model — `float F0 = mix(0.15, 0.9, metalness)`, "floor at 0.15 so dielectrics show
> visible reflections" — made every metal a WHITE mirror and boosted dielectrics 4×. Measured on
> DamagedHelmet: the dark-teal dome rendered at ~70 % of the SKY's luminance with the sky's own
> chromaticity, erasing the raster's correctly F0-tinted split-sum IBL through the composite mix.
> A "flashy" reflection is an energy bug, never a look to preserve.

> [!WARNING]
> **The MRT normal-alpha packing (`alpha = roughness + round(metalness) × 2`) quantizes the
> metalness to {0,1} at WRITE time — that quantization is load-bearing.** The decode
> (`metalness = alpha >= 2`) assumes it: a raw fractional metalness (real data since the packed
> metallic-roughness source channels are honored) corrupts BOTH decoded values (0.93 metal /
> 0.4 rough packed raw decodes as 1.0 / 0.26). Write site:
> `Saphir/Generator/SceneRendering.cpp` (MRT normal output).

**Mesh data access via buffer references:**
- `BDA` (buffer device addresses) in mesh metadata SSBO point to vertex/index buffers
- Shader reads vertex normals and UVs via `VertexBuffer`/`IndexBuffer` buffer references
- Byte offsets for normals and UVs computed from geometry flags (tangent space, etc.)

**Self-reflection rejection:** `dot(hitNormal, worldNormal) > 0.9` prevents flat surfaces
from reflecting themselves (e.g. floor reflecting floor).

**Critical synchronization requirements:**
1. Mesh/material SSBOs must be **per-frame** (see Frame Synchronization section)
2. View matrices must use **`readStateIndex`** overloads (see Scenes/AGENTS.md)

**Code references:**
- `Effects/Framebuffer/RTR.hpp` — Parameters, API
- `Effects/Framebuffer/RTR.cpp` — GLSL shaders (inline), descriptor layouts, pipeline creation
- `Scenes/SceneMetaData.hpp` — TLAS, mesh metadata, material data management
- `Scenes/GPUMeshMetaData.hpp` — GPU-side mesh metadata struct layout

### GIDenoiser — the shared GI temporal denoiser component (Aug 2026, SVGF work site)

`Graphics/GIDenoiser.{hpp,cpp}` — the temporal machinery extracted VERBATIM from RTGI
(stage 0 of the SVGF plan; measured non-regression: Sponza corridor ptp 0.733/0.790 within
the 0.67–0.80 baseline envelope, identical GPU timings). **One instance is OWNED by each GI
effect** (RTGI today, SSGI planned): the code is shared, the histories are NOT — two
producers reprojecting into one history would corrupt each other. Like `DenoisePass`, it
extends `IndirectPostProcessEffect` purely to reuse the fullscreen-pass infrastructure and
is never inserted into a stack.

The component owns: the resolved-irradiance history ping-pong (RGBA16F, A = camera
distance), the world-normal history pair, the temporal-resolve and normal-copy pipelines,
and the per-frame `FrameUBOData` UBO (moved from RTGI — the owner binds it into its own
trace pass via `frameUBO(f)` and fills it via `updateFrameData(f, data)`, which also
advances the animated-noise R2 index). Owner-facing flow, in `recordPre/PostDenoisePasses`:

1. `setTemporalEnabled(flag)` then `create(w, h)` at the OWNER's working resolution
   (UBOs always allocated; history VRAM and pipelines only when the temporal chain is on).
2. Trace binds `historyReadTexture()` (multi-bounce feedback) — stable until the flip.
3. `m_combineSource = recordResolve(cb, noisyInput, context)` — records temporal resolve +
   normal history, flips the ping-pong, returns the texture the combine must consume
   (the noisy input unchanged when the temporal chain is off).

The SVGF stages are built INSIDE this component — settings live under the mirrored
`RayTracing/GlobalIllumination/{Temporal,Denoiser}/` +
`ScreenSpace/GlobalIllumination/{Temporal,Denoiser}/` groups (owner decisions,
2026-08-06). **CHANTIER COMPLETE**: RTGI stages 0–4 owner-validated live ("the only thing
left vibrating is the VolumetricLight streak" — a separate subject), SSGI wired the same
day — its FIRST temporal accumulation. The component assembles its own frame UBO
(`updateFrameData(frameIndex, context, FrameInputs)` — matrices from the renderer's view
state, temporal parameters from `GIDenoiser::Parameters`; the producer only supplies its
trace scalars, zeroed when it has no feedback loop / sky term) and serves the debug views
to any producer via `debugCombineContribution(prefix, mode)`.

**SSGI wiring (Aug 2026):** SSGI owns its GIDenoiser instance (histories are per-producer),
gained `requiresVelocity()`, left the shared H/V blur, and its trace noise was upgraded
from the banding-prone `fract(sin(dot))` hash to PCG + the R2 animated sequence
(`noiseFrameIndex` push constant, < 0 = frozen). Measured (Sponza corridor, RT off, double
runs): ptp 0.367–0.395 → **0.306–0.338**, area > 2/255 halved, energy preserved
(mean luma 24.30 vs 24.35). The `ScreenSpace/GlobalIllumination/BlurRadius` key is inert.

**Stage 3 — per-pixel 1/N accumulation counter (Aug 2026):** the temporal blend weight is
`max(1/(age+1), 1/MaxAccumulation)` instead of the fixed `Temporal/Alpha` (flag bit 2,
`Denoiser/AccumulationCounter` default true, `Denoiser/MaxAccumulation` default 64). The
age lives in the moments history B channel (stage 1); the colour resolve samples it at the
reprojected UV so colour and moments integrate with the SAME weight. Fast convergence after
a disocclusion (1, 1/2, 1/3…), steady-state variance leak 1/N ≈ 1.6% at N=64 versus
α/(2−α) ≈ 23% at fixed α=0.1 — the factor that sank the first animated-noise attempt.
`Temporal/Alpha` only rules when the counter is off (A/B lever).

**Stage 4 — animated noise DEFAULT ON (Aug 2026, owner decision):** with the à-trous +
1/N in place, `Temporal/AnimatedNoise` flipped to default true. Measured (Sponza corridor,
double runs): energy restored (mean luma 24.9 vs 22.9 frozen — a frozen pattern turns
stable bright outliers into "converged signal" the luminance guide protects, i.e.
fireflies), best distribution tails (>8/255: 0.33%), ptp mean 0.55–0.57 versus the
0.67–0.83 marbled baseline. Owner-validated live: the GI shimmer is extinguished.

**Stage 2 — SVGF reorder + variance-guided à-trous (Aug 2026):** the GI producers LEFT the
shared H/V `DenoisePass` (`usesSharedDenoise()` back to false — a multi-iteration à-trous
does not fit the two-pass separable MRT shape; RTAO/CS/RTR keep merging theirs) and record
their whole chain in `recordOverlayPasses()`. New order (canonical SVGF): trace → temporal
resolve **of the RAW trace** + moments → à-trous 5×5 B3-spline, `Denoiser/Iterations`
passes (default 4), footprint doubling (stride 1,2,4,8), edge-stopping on depth, view-space
normal and **luminance normalised by the local standard deviation**
(`Denoiser/LuminanceSigma`, default 4 — the SVGF auto-dosage), variance filtered alongside
with the w² rule; first iteration falls back to a 3×3 SPATIAL variance estimate where the
accumulation age < 4 (freshly disoccluded pixels — silhouettes under the TAA jitter, the
animated foliage). The combine consumes the à-trous output; the multi-bounce colour history
remains the TEMPORAL output (v1 — SVGF's first-iteration feedback is a later candidate).
`Temporal/Enabled=false` now means RAW passthrough (diagnostic only: no spatial filter
without its variance guide). The `BlurRadius` key is inert for RTGI.

Measured (Sponza corridor bench, double runs): temporal ptp mean 0.67–0.83 → **0.46–0.50**,
area > 2/255 divided by 4, GPU +2.9 ms half-res on the 3070 Ti (4 iterations, optimisation
candidates: single-channel gathers, fewer taps on late iterations). Two structural findings:
(1) `Temporal/NeighborhoodClamp` **default flipped to false** (owner decision) — clipping
the history against the RAW 3×3 statistics costs ~5% GI energy for no stability gain
(designed for the pre-blurred input that no longer exists; SVGF uses the double disocclusion
validation alone); (2) the FROZEN noise seed turns stable bright outliers into
"converged signal" the luminance guide protects — visible cyan fireflies. Exploratory
stage-4 test (AnimatedNoise=true, settings only): fireflies dissolve, energy restored
(luma 24.8 vs 25.2 old chain), ptp 0.69–0.73 with better tails than baseline — the ×2.4
regression is gone; the residual leak is the fixed-alpha EMA (α/(2−α) ≈ 23%), exactly what
the stage-3 1/N counter replaces (steady-state leak at N=64 ≈ 0.8%).

**Stage 1 — per-pixel moments + variance (Aug 2026):** a third ping-pong pair
(`_GIMoments`, RGBA16F) integrates the first/second raw moments of the **RAW estimate's
luminance** (the owner passes its unfiltered trace as `rawInput` to `recordResolve()` —
variance of the already-blurred signal would underestimate the noise the spatial filter
must remove). Channels: R = m1, G = m2 (temporal variance = `max(m2 − m1², 0)`, derived at
the read site via `momentsTexture()`), B = accumulation age in frames (saturates at 64,
reset on disocclusion — the future 1/N counter), A = camera distance (validity marker).
The moments pass duplicates the colour resolve's velocity reprojection + 3×3 depth-nearest
dilation + double disocclusion test VERBATIM — both passes MUST agree on which pixels have
a valid history; `alpha >= 1.0` also routes to the reset path (covers the first frame
after (re)creation AND a user-set alpha of 1). Measured: +0.196 ms half-res on the 3070 Ti
(RTGIEffect/temporal 0.266 → 0.462 ms), visually neutral (Sponza corridor ptp 0.830 within
the baseline envelope, identical mean luma).

**Denoiser debug views** (`RayTracing/GlobalIllumination/Denoiser/DebugView`, default 0,
read by RTGI at create): the combine draws the denoiser internals INSTEAD of the GI —
1 = temporal variance (amplified ×1e6, bounded — a LINEAR scale is unreadable under the
photometric exposure), 2 = accumulation age (white = young/disoccluded, < 4 frames).
Validated on Sponza: the variance map matches the owner's shimmer cartography
(floor near the lit door + curtains brightest, penumbra structured, true darkness black);
the age map shows saturation on stable surfaces and permanent per-frame resets exactly on
the wind-animated ivy and on silhouette edges under the TAA jitter — the first direct
visualisation of the "jitter is the vibrator" mechanism, and the zone the stage-2 spatial
variance fallback must cover.

### Indirect-diffuse OWNERSHIP — who computes the sky, the raster or the effect (Aug 2026)

> [!CAUTION]
> **A sky-lit scene has TWO subsystems able to compute the same diffuse irradiance, and adding
> both counts the sky twice.** The raster ambient pass adds
> `albedo * (1 - metal) * (1 - F) * iblIrradiance * environmentLuminance` (the baked irradiance
> cubemap, reserved bindless cube slot 1). RTGI's miss branch adds
> `cubemap(dir) * FrameContext::skyLuminance` for every ray that escapes the TLAS — the SAME
> integral, from the SAME cubemap, with the SAME luminance (`background->luminance()`), only with
> real visibility. **Measured** on `asset-loader --demo-options 11,0,1,0,0,0` under
> AutumnFieldPureSky (31 800 nits): the watch read 159/255 with both, 116/255 with SSGI (which has
> no sky term), 109/255 once the ownership contract landed — the 43-point gap WAS the double count.
>
> **The contract.** `PostProcessEffect::providesIndirectDiffuse()` (default `false`) declares an
> effect as the OWNER of the frame's indirect diffuse. `PostProcessStack::hasEnabledIndirectDiffuseProvider()`
> aggregates it, `Scene::updateIBLDiffuseOwnership()` polls that every logic tick and pushes a
> `iblDiffuseWeight` of **0** (an owner is active) or **1** (the raster owns it) into the view UBO;
> the ambient pass declares `iblDiffuseIrradiance = iblIrradiance * iblDiffuseWeight` and every
> DIFFUSE leg reads that one. Same shape as the reflection cost ladder
> (`hasEnabledReflectionProvider()` → the Renderer suspends the continuous probes).
>
> - ⚠️ **The weight scales the DIFFUSE leg ONLY.** The specular IBL — prefiltered reflections and
>   the Fdez-Agüera multi-scatter compensation `iblFmsEms` — keeps the RAW `iblIrradiance`: no
>   post-process replaces it, and zeroing it would darken rough metals. That is why the split-sum
>   branch emits TWO additions where it used to emit one.
> - ⚠️ **The scene's SCALAR ambient is untouched.** A hand-lit scene's `setAmbientLightIntensity()`
>   (Sponza's 200 lx) is the owner's deliberate residual — the "skylight leaking" knob of the
>   reference implementations — not a computed term. Only the sky-derived irradiance changes hands.
> - ⚠️⚠️ **A screen-space effect can NEVER be an owner.** SSGI has no sky term at all (a miss in
>   screen space means "no occluder found in the depth buffer", not "open sky" — implemented,
>   measured and reverted in Jul 2026: mean 96.2 / median 96.0, i.e. zero spatial variation, which
>   is exactly the flat ambient it was meant to replace). SSGI's contribution and the raster's
>   ambient are DISJOINT, so they are legitimately added. `providesIndirectDiffuse()` returning
>   `false` for SSGI is a consequence of what it can measure, not a policy choice.
> - ⚠️⚠️ **The provider must be gated on its ability to RUN.** `RTGI::providesIndirectDiffuse()`
>   repeats the exact gate `PostProcessor` skips the effect on (hardware, `isRayTracingSettingEnabled()`,
>   `isRayTracingReady()`). Claiming ownership while the TLAS is still building would hand the diffuse
>   to an effect that draws nothing — a black flash over the first frames of every scene.
> - ⚠️ **Known limitation until the frame is reordered**: the indirect effects run AFTER the
>   TranslucentGB pass, so a surface seen THROUGH a transmissive material gets no indirect at all
>   (the raster leg is off, and the G-buffer at that pixel belongs to the glass). Item
>   `docs/todo/indirect-diffuse-before-translucency.md`.
>
> **State of the art**: UE5 Lumen — *"Sky lighting is solved as part of Lumen's Final Gather
> process. It includes sky shadowing"*; Unity HDRP — *"SSGI and RTGI replace all lightmap and Light
> Probe data […] Light Probes and the ambient probe stop contributing"* (and its `LightLoop.hlsl`
> applies that replacement to non-transparent surfaces only). Both REPLACE, neither adds.
>
> **Files**: `Graphics/PostProcessEffect.hpp` (the virtual), `Graphics/PostProcessStack.{hpp,cpp}`,
> `Graphics/Effects/Framebuffer/RTGI.{hpp,cpp}`, `Scenes/Scene.lighting.cpp`
> (`updateIBLDiffuseOwnership`, `refreshAmbientLightProperties`), `Scenes/Scene.cpp` (the poll),
> `Saphir/LightGenerator.cpp` (`iblDiffuseIrradiance`), `Saphir/Generator/Abstract.cpp` +
> `Graphics/ViewMatrices{2D,3D,Cascaded}UBO.*` (the UBO lane — it fits in the EXISTING padding
> after `environmentLuminance`, so the UBO size did not move).

### The albedo G-buffer: BASE colour in RGB, DIFFUSE WEIGHT in ALPHA (Aug 2026)

> [!CAUTION]
> **Attachment 3 (`VK_FORMAT_R8G8B8A8_SRGB`) has TWO families of reader, and the two lanes serve
> them apart** (written by `Saphir/Generator/SceneRendering.cpp`, ambient/simple pass only):
>
> | Lane | Content | Readers |
> |------|---------|---------|
> | `.rgb` | the surface **BASE colour** (sRGB-encoded) | **RTR** (`RTR.cpp`, `F0 = mix(0.04, albedo, metalness)`) and **SSR** (`SSR.cpp`, same model) — a metal's reflection carries its own colour |
> | `.a` | the **DIFFUSE WEIGHT** `(1 - metalness) * (1 - transmissionFactor)`, linear (`LightGenerator::diffuseWeightShaderExpression()`) | **SSGI** and **RTGI** combines: `gi *= albedo.rgb * albedo.a` — the energy the diffuse lobe actually receives |
>
> Why the weight exists at all: the GI combines re-modulate a demodulated IRRADIANCE, and the two
> materials that have no diffuse lobe were lit as if they were sheets of paper when the base colour
> alone was applied —
> - a **metal** (gold: baseColor 1.00/0.72/0.32, metalness 1) re-emitted 72 % of the incoming
>   irradiance as diffuse light;
> - a **`KHR_materials_transmission` glass**, whose default base colour is WHITE and which
>   overwrites the G-buffer of everything behind it (translucent materials get `blendEnable = FALSE`
>   on the G-buffer attachments — the "flat water reflections" fix), turned into an opaque milky
>   plate: the watch dial under it was unreadable.
>
> Same convention as NVIDIA NRD (its demodulation albedo is `baseColor * saturate(1 - metalness)`,
> `MathLib::ConvertBaseColorMetalnessToAlbedoRf0`) and as the glTF dielectric BRDF, which MIXES the
> diffuse lobe into the transmission by the transmission factor rather than adding to it.
>
> - ⚠️⚠️ **The one-day regression (2026-08-29 → 30) that dictated the two-lane layout:** the first
>   fix wrote the diffuse albedo INTO the rgb lanes, after a grep for the GI combines' identifier
>   concluded "the only consumers are SSGI and RTGI". The reflections read the same attachment
>   under another name (`albedoTex`): every metal's F0 became 0 and **RTR and SSR stopped
>   reflecting anything on any metal**, in every scene. Found by the first capture of the
>   `post-processor-effect-debug` bench (six white metal panels: flat 95/95/95, no band). **Before
>   changing what an attachment carries, grep for the ATTACHMENT — its binding, `context.albedo`,
>   `requiresAlbedo()`/`needsAlbedo` — never for one consumer's variable name.**
> - ⚠️ A material declaring neither metalness nor transmission writes `a = 1.0`: its GI
>   re-modulation is bit-identical to the pre-Aug 2026 one.
> - ⚠️ The transmission factor is published NOWHERE ELSE in the G-buffer (matprops has no
>   transmission nibble, the normals alpha packs only roughness + a BINARY `round(metalness)`), and
>   the reflectivity nibble is a PARTICIPATION mask `max(metalness, 1 - roughness)` — a smooth
>   dielectric publishes ~1.0 exactly like a metal, so it can never stand in for metalness. The
>   alpha lane is what makes the weight reach the combine at all, with a FRACTIONAL metalness.
> - ⚠️ The RTGI trace applies the SAME rule at BOUNCE HITS (`albedo *= 1 - metalness`, metalness
>   texture included, mirroring RTR): a bounce is a diffuse event, and the multi-bounce feedback is
>   damped by that same albedo product — which is what keeps its geometric series convergent.
> - The unlit path writes `vec4(displayedColour, 1.0)`, the no-material path `vec4(1.0)`; the light
>   passes write nothing (zeroed write mask), the attachment is `LOAD_OP_LOAD` on their pass.

### RTGI (Ray-Traced Global Illumination) — Temporal + Multi-Bounce (Jul 2026)

One traced diffuse bounce per frame, temporally accumulated, with a multi-bounce feedback
loop through the history buffer. Since Aug 2026 everything downstream of the trace lives in
the owned `GIDenoiser` instance (see the section above) and follows the SVGF order —
temporal integration of the RAW trace first, variance-guided à-trous after:

1. **Trace** (half-res, RTGI-owned): cosine-weighted hemisphere rays via TLAS ray queries;
   at each hit, direct lighting (with shadow rays gated on the raster shadow-casting flag)
   PLUS the hit surface's accumulated indirect irradiance from the previous resolved frame
   (multi-bounce feedback, multiplied by the HIT albedo — see energy algebra below). The
   output is **DEMODULATED**: no receiver albedo anywhere in the traced signal (Aug 2026).
2. **Temporal resolve** (half-res, GIDenoiser): reprojects through the velocity buffer
   (3×3 depth-nearest dilation), validates history (camera-distance in history alpha +
   world-normal history), optional variance clipping (`Temporal/NeighborhoodClamp`,
   default OFF since the SVGF reorder), then EMA (`Temporal/Alpha`). Output → history
   ping-pong `[writeIdx]` (also next frame's multi-bounce feedback source).
3. **Moments** (half-res, GIDenoiser): m1/m2 of the raw trace luminance + accumulation age,
   same reprojection/validation — the temporal variance guiding the à-trous.
4. **Normal history** (half-res, GIDenoiser): current view-space normals → world space,
   retained for the next frame's validation.
5. **À-trous** (half-res, GIDenoiser, `Denoiser/Iterations` passes): variance-guided
   edge-avoiding wavelet filter — see the GIDenoiser section.
6. **Apply** (full-res): multiplies by the receiver DIFFUSE albedo (albedo G-buffer
   `emAlbedo.rgb * emAlbedo.a`, `CombineContribution::needsAlbedo`) at FULL resolution, then
   additive blend, emissive-masked via material properties G-buffer.

**Albedo demodulation (Aug 2026):** the denoise/temporal chain carries **irradiance only**
(`E/π`); the receiver albedo is re-applied at full resolution in the combine pass — the
same convention as SSGI, and the standard practice of modern GI denoisers (SVGF, Schied et
al. 2017, HPG; NVIDIA NRD). Rationale: multiplying the albedo at trace time (half-res,
before the bilateral blur) destroyed texture detail exactly where GI dominates the final
pixel (dark areas — direct light ≈ 0, so the blurry `blur(albedo × E)` term visually
REPLACES the pixel). With demodulation the final term is `albedo_fullres × blur(E)`: the
texture stays native-sharp regardless of `GIBlurRadius`/`PixelDoubling`. Validated A/B on
Sponza (energy ratio 0.996, floor texture gradient ×1.64) and the Cornell GI demo
(uniform ≤2% run-to-run drift, colour bleed hue preserved, no multi-bounce runaway).

**Frame UBO instead of push constants:** the trace parameters (invViewProj + prevViewProj +
camera data) exceed the **128-byte Vulkan push constant minimum guarantee**
(`maxPushConstantsSize`). A per-frame UBO (`FrameUBOData`, std140) is shared by the
trace/temporal/normal-history passes — created via
`IndirectPostProcessEffect::createPerFrameUniformBuffers()`, bound through
`getInputLayout(samplerCount, uniformBufferCount)` (samplers first, then UBOs).

**History rectification = variance clipping (Aug 2026):** the temporal resolve bounds the
reprojected history to mean ± gamma × sigma of the current 3×3 neighborhood (Salvi, GDC 2016 —
the same technique as the engine TAA; `Temporal/VarianceGamma`, default 1.0), replacing the
former min/max clamp. Neutral with the static noise (measured), required for any future
animated-noise work.

**Animated noise infrastructure (Aug 2026) — DEFAULT OFF, measured regression:**
`Temporal/AnimatedNoise` advances the per-pixel sample rotation along the R2 sequence
(Roberts 2018) each frame (frame index in `traceParams.w`, flag bit 1 of `temporalParams.w`,
gated on the temporal chain). ⚠ With the fixed-alpha EMA (0.1) this REGRESSED temporal
stability ×2.4 on the Sponza corridor bench (mean peak-to-peak 0.67 → 1.65, >4/255 area ×9)
with NO spatial gain: the EMA leaks ~α/(2−α) ≈ 23% of the injected variance, while a frozen
pattern has near-zero temporal variance by construction — even an NRD-style 1/N accumulation
counter would only reach parity (computed). The winning lever is cutting the per-frame noise
BEFORE the resolve (variance-guided à-trous filter, SVGF) — only then does animated noise
become viable. Owner-isolated context: the TAA jitter is what makes the static pattern
shimmer (TAA→FXAA freezes it); see `docs/caution-points.md` § "Animated GI Noise".

**Multi-bounce energy algebra:** the history stores DEMODULATED indirect irradiance
(`E/π` — receiver albedo deferred to the combine pass), so the feedback IS multiplied by
the HIT surface's albedo at consumption (`albedo * historyFeedback(hitPos)` in the trace) —
the geometric series `1/(1-albedo*strength)` stays damped by physical albedo (< 1)
and converges. `MultiBounce/Clamp` bounds the re-injected irradiance (anti-firefly).
`MultiBounce/Strength` is a continuous bounce-depth dial: 0 = single bounce, 1 = full series.

**History ping-pong correctness:** 2 half-res RGBA16F history targets (+2 normal history).
Frame N reads `[1-w]`, writes `[w]`; safe on a single queue thanks to the IRT's full
(non-by-region) external subpass dependencies **plus the explicit inter-pass barrier emitted
by `recordFullscreenPass()`** (the MoltenVK contract, § 12 "Inter-Pass Synchronization"). `m_historyValid` forces alpha=1 and
strength=0 on the first frame after (re)creation — the ping-pong images load DONT_CARE.
The temporal/normal-copy pipelines are created against the `[0]` targets and record into
`[1]` via render pass compatibility (same trick as the shared blur pipeline).

**Structural limitation (screen-space feedback):** bounce rays only pick up feedback from
surfaces visible ON SCREEN in the previous frame. Light does not propagate around corners
that are never co-visible with lit surfaces (validated in the `global-illumination` demo:
+21% median brightness where lit+penumbra are co-visible, zero effect in the fully
occluded bend). Going further requires a world-space cache (probes / surface cache).

**Static-geometry reprojection (v1):** the temporal reprojection uses `prevViewProj` +
current world position — exact for camera motion over static geometry, wrong for moving
objects (bounded by history validation + neighborhood clamp). Per-object motion vectors
(5th MRT attachment) are a planned follow-up; they require moving scene-pass transforms
out of push constants first (see `docs/caution-points.md`, 128-byte entry).

**Settings** (`Core/Graphics/RayTracing/GlobalIllumination/`): `Temporal/Enabled|Alpha|
DepthTolerance|NormalThreshold|NeighborhoodClamp`, `MultiBounce/Enabled|Strength|Clamp`
(see `SettingKeys.hpp` for defaults and rationale). `Temporal/Enabled=false` skips the
temporal chain entirely (no history VRAM, apply reads blur V — the pre-Jul-2026 flow).

**Code references:**
- `Effects/Framebuffer/RTGI.hpp` — Parameters, owned `GIDenoiser` instance
- `Effects/Framebuffer/RTGI.cpp` — trace GLSL shader (inline), blur snippet, combine snippet
- `Graphics/GIDenoiser.{hpp,cpp}` — `FrameUBOData` (std140), temporal/normal-copy shaders,
  history ping-pong recording
- `Graphics/ViewMatricesInterface.hpp` — frame-history contract (previous view/projection)

### Physical Camera — Camera-Driven Photographic Pipeline (Jul 2026)

The `Scenes::Component::Camera` is the **single source of truth for the photographic
behaviour** of the rendered image, like a real camera body. Owner vision: ultimately ALL
image-rendering effects are camera-manageable (lens effects already are).

#### The near plane is DERIVED from the subject's scale (fixed 2026-08-28)

> [!CAUTION]
> `nearPlane = nearestObjectDistance / sqrt(1 + tan²(fov/2) · (aspectRatio² + 1))`, and
> `nearestObjectDistance` was the **literal constant `0.1F`** in FOUR hand-maintained copies —
> `ViewMatrices2DUBO`, `ViewMatrices3DUBO`, `ViewMatricesCascadedUBO` and `SpotLight`, the last of
> which carried a comment claiming consistency with the first, a coupling kept by hand. Every scene
> therefore got a near plane sized for a decimetre subject, about **0.089 m**.

**It was wrong in both directions, and the second half is the one nobody notices.**

- **Small:** a millimetric subject sits entirely INSIDE the near plane and renders nothing. Measured
  on the Khronos `MetalRoughSpheresNoTextures`, radius 0.00035 m — 89 times inside it. The glTF
  bench had to push the camera out and could only reach **5.5 %** of frame height; `BoomBox`
  (r = 0.0101 m) **14.1 %**. Both now plan at the nominal **38.9 %**.
- **Large:** the depth test is conventional — `VK_COMPARE_OP_LESS_OR_EQUAL`, `D32_SFLOAT`, **no
  reversed-Z anywhere in the engine** — so a float depth buffer concentrates its precision near the
  near plane. A 0.089 m near against a kilometre-scale far spends that precision in the first ten
  centimetres, where an outdoor scene needs it least. Any distant z-fighting should look here first.

The single derivation now lives in `ViewMatricesInterface::computeNearPlane(nearestObjectDistance,
fov, aspectRatio)` — pass `aspectRatio = 1` for a cube face or a spot light, which reduces the
bracket to 2 and reproduces the old hand-written expressions bit for bit. ⚠️ **Do not write the
formula again**; four copies is how it drifted.

The distance is a **camera property**, symmetric with the far distance:
`Camera::setNearestObjectDistance()` → `AbstractVirtualDevice::updateNearestObjectDistance()` (a
defaulted no-op, so the five other video devices and every audio device are untouched) →
`SceneRenderTarget` → `ViewMatricesInterface::setNearestObjectDistance()`. A caller that says nothing
keeps `DefaultNearestObjectDistance` (0.1 m) and behaves exactly as before.

⚠️⚠️ **The property has to be forwarded by FIVE separate owners.** `Vulkan::SwapChain`,
`Graphics::SceneRenderTarget` and the templated `RenderTarget::View`, `Texture` and `ShadowMap` each
hold their own `ViewMatrices2DUBO`; there is no common holder. **The one a scene camera actually
connects to is the SWAP CHAIN** — doing only `SceneRenderTarget` left the near plane at its default
and the two small conformance models rendered entirely empty. `ShadowMap` alone may ignore it (a
light's frustum has no subject).
⚠️⚠️ **`Camera::onOutputDeviceConnected()` is the FIRST push a target receives**, and every camera
setter is guarded by `hasOutputConnected()` — so a value set while the scene is still being built,
which is exactly what a viewer does, is stored and never sent unless that hook pushes it too.

⚠️ **`+ModelViewer` had TWO more decimetre assumptions of its own**, and the near plane alone would
not have fixed it: the framing radius was floored at `0.01F` (so every sub-centimetre asset was
re-framed as if it were a centimetre across — 28× too far for `MetalRoughSpheresNoTextures`), and the
orbit controller's lower distance limit at `max(radius * 0.05F, 0.01F)`, which `setDistance()`
CLAMPS against. Both are now scale-relative. When a scale bug survives one fix, look for the other
floors.

### Background photometric contract — the sky is a light source, authored in the asset (Jul 2026)

A sky EMITS, so it is described by a **luminance in nits**, and that value spans seven orders of
magnitude between noon and midnight: `DaylightSkyLuminance` 8000, `OvercastSkyLuminance` 2000,
`TwilightSkyLuminance` 10, `MoonlitNightSkyLuminance` 1 (`Graphics/Renderable/SkyBoxResource.hpp`).
A background manifest (store `Backgrounds`) declares the FULL photometric description of the sky:

```json
{
	"Cubemap": "BlueSky",
	"Luminance": 8000.0,
	"AverageColor": [0.35, 0.55, 0.85],
	"AmbientIlluminance": 20000.0,
	"Stars": [
		{ "Type": "Sun", "Direction": [0.35, -0.55, 0.63], "Illuminance": 100000.0,
		  "Temperature": 5500, "AngularDiameter": 0.53, "InTexture": true }
	]
}
```

- `Cubemap` (required): resource name in the `Cubemaps` store (the key REPLACED `"Texture"`, Jul 2026).
- `Luminance` (nits, default 8000 = clear day): the emission scale of the LDR source — a night
  cubemap and a noon cubemap are not the same photometric object.
- `AverageColor` (sRGB, optional): authored/cheated average; absent = computed from the source
  (`CubemapResource::averageColor()`).
- `AmbientIlluminance` (lux, optional): what the dome pours on the ground; absent = derived as
  `E = L × factor` where the factor is MEASURED on the actual texels
  (`CubemapResource::hemisphereIlluminanceFactor()`: sRGB-decoded luma × cos(zenith) integrated
  over the sky hemisphere — π only for a uniform dome; Backrooms measures 0.11, its dome is
  mostly dark ceiling). Owner decision (review session Jul 2026): the uniform-dome π over-lit
  every non-uniform sky. HDR sources are calibrated to exactly π by construction.
- `Stars` (optional, 0..N): celestial bodies (`Graphics::CelestialBody`) to derive analytic
  directional lights from — `Direction` points TOWARD the body (engine frame, UP = +Y),
  `Illuminance` in lux, `Temperature` in kelvins (industry-standard authoring; wins over a direct
  `"Color"` when both are present) resolved via `Photometry::colorFromTemperature()` (Planckian
  locus), `AngularDiameter` in degrees, `InTexture` = anti-double-counting flag (informative in
  LDR, structuring for the future HDR pipeline). Zero stars is legitimate: pure ambiance
  (overcast, nebula, cave).

Parsing is CENTRALIZED in `AbstractBackground::parsePhotometry()` — every background type
(`SkyBoxResource`, future `DynamicSkyResource`, `ColorBackgroundResource`) goes through it.

**HDR skies (Jul 2026)**: a Cubemaps manifest with `"FileFormat": "hdr"` + `"Equirectangular"`
loads a Radiance RGBE source through the float pipeline (`CubemapResource::loadEquirectangularHDR`):
- **D6 calibration (Unity-like)**: an HDRI is RELATIVE; the loader measures the illuminance its
  upper hemisphere pours on the ground and normalizes so scale 1 = uniform dome of luminance 1
  (E = pi lux). The Background `Luminance` key then means EXACTLY the same thing for LDR and HDR,
  and the unclipped sun keeps its full relative punch (clamped to 65504, the half-float maximum —
  real blinding specular reflections).
- Faces are stored as raw **RGBA16F** texels (`hdrFaceData()`, `isHDR()`; `faces()` stays empty)
  and uploaded as `VK_FORMAT_R16G16B16A16_SFLOAT` (guaranteed filterable, half the VRAM of 32F).
- ⚠️ **`Base::PixelFactory::Color< float > CLAMPS to [0,1] on construction** — every HDR data
  path must stay on RAW floats (`sampleEquirectangularHDR()` bypasses `linearSample`/`pixel()`).
- ⚠️ **`Vulkan::Image::pixelBytes()` must know the texel format**: it drives the per-layer buffer
  offsets of the cubemap upload. Its missing 16F entries scrambled the faces of the first HDR sky
  (fixed) — extend it whenever a new texel format enters the engine.
- Debug: compile with `EMERAUDE_DEBUG_HDR_FACES` to dump tonemapped face PNGs to `/tmp` at load.
- The average color is computed at load from the calibrated radiances (`averageColor()` override).
- LDR-only consumers (`CubemapMovieResource`) explicitly reject HDR cubemaps.

**The scene consumes this in two ways:**
1. **Always** (automatic): the luminance scales every IBL contribution. It travels through the
   View UBO (`UniformBlock::Component::EnvironmentLuminance`, pushed by
   `Scene::refreshAmbientLightProperties()` to ALL render targets — main, views, textures — at
   LightSet init, on `setBackground()`, and when an async background finishes loading). The
   shader reads it via `LightGenerator::scaledIBLIntensity()`; it is a UNIFORM, not a baked
   literal, so it can change at runtime (day/night) without regenerating programs.
2. **Opt-in**: `Scene::applyBackgroundLighting(options)` derives the scene lighting — LightSet
   ambient = `AverageColor` × `AmbientIlluminance`, one `DirectionalLight` per star (the first
   becomes the main directional light). Shadow mapping is NOT photometric data: it comes from
   `BackgroundLightingOptions` (classic map or CSM). Deferred automatically while the background
   resource is still loading (observer on `LoadFinished`). Scene JSON: `"ApplyLighting": true`
   in the `Background` block. Full manual = don't call it.

⚠️⚠️ **The luminance drives TWO consumers and both must hear about it**: the material's emission
(what you see looking up) AND the IBL scale above. Historically the IBL scale sat on its 8000-nit
daylight default in every scene: a material reflecting 3% of its environment received 240 nits
against 0.1 nit of moonlit diffuse, 2400x too much — Citadel's stone walls read as white neon, and
the relief detail modulating that clipped signal was mistakable for a broken normal map. Fixing
only the material would have corrected what the sky LOOKS like while leaving everything it LIGHTS
wrong.
⚠️ The luminance is also part of the sky material's IDENTITY (it is in the resource name): two
manifests sharing one cubemap at different luminances must not share a material.
⚠️ A copy-paste default of `Roughness 0.5` + `Reflection Automatic 0.1` exists in 54 of the 3917
material manifests; they were all amplifying the IBL the same way. Correct by construction now that
the scale is right, but their satin roughness is still worth reviewing surface by surface.

**Camera presets** (`Scenes/EffectsToolkit/CameraPresets.{hpp,cpp}`): full photographic
packages — optics + exposure + DoF/HDR materialization + lens effects in one call.
`Neutral` (reset), `HighQuality` (f/2.8 full frame, clean), `HumanEye` (f/8, soft peripheral
vignette), `VintageBlackAndWhite` (f/5.6 Super 35 + LensPresets::Hitchcock60s stack),
`Super8` (f/1.9 Super 8 gate, +0.3 EV, coarse grain/jitter/flicker/dust). Applying a preset
REPLACES the camera's photographic setup; two cameras can carry different presets
(active-camera switch = full look switch). Validated on Sponza (Jul 2026).

⚠️ **A style declares a FORMAT, not a lens, and NEVER steals the framing** (owner decision,
Jul 2026, once the focal length started driving the field of view). `CameraStyle::sensorWidth`
replaced `focalLength`, and applying a style reads the current field of view, mounts the format,
then re-derives the EQUIVALENT focal length — the same move a director of photography makes when
changing stock. The shot belongs to whoever placed the camera; what the format changes is the
optical CHARACTER, since the circle of confusion scales as the focal length squared over the
format width. Hence `FullFrameFormat` 36 mm, `Super35Format` 24.89, `BroadcastFormat` 8.8 (2/3"
tube), `CamcorderFormat` 6.4 (1/2"), `Super8Format` 5.79 — and that is *why* 1980s broadcast video
and Super 8 look flat while a full-frame prime separates its subject. This also deleted twelve
arbitrary focal-length values. ⚠️ A style is REFUSED on a technical camera (`isStyleable()` warns):
a cubemap face has no format, no lens and no grading.

**EXTENSION CONTRACT — consumer-defined styles** (`EffectsToolkit::CameraStyle`): an
engine consumer declares its own photographic style as a DATA block (optics, exposure,
DoF/HDR flags, and a lens-stack FACTORY — fresh effect instances per application, no
state sharing between cameras). Usable two ways: `CameraPresets::Apply(camera, style)`
directly (unlimited ad-hoc styles), or registered once behind the `CameraPreset::Custom`
token via `CameraPresets::setCustomStyle(style)` (then usable at Toolkit creation and in
runtime cycles; unset token falls back to Neutral with a warning). The factory may return
effect classes OWNED BY THE APPLICATION (subclass `DirectPostProcessEffect`, override
`generateFragmentShaderCode()` — the whole surface is public/EMEN_API): validated with
projet-alpha's `BitmapMonochromeEffect` (1-bit ordered-dither, its own GLSL — the engine
never knows the type). NOTE: a style with no HDR feeds RAW LINEAR values to the lens
effects — threshold-like effects usually want `HDR = true` so the auto-exposure
normalizes their input.

**Preset TOKEN at creation** (owner-decided idiom): the preset is part of the camera
DEFINITION — `enum class EffectsToolkit::CameraPreset` (13 values: Normal, HighQuality,
HumanEye, VintageBlackAndWhite, Super8, plus the PROMOTED LensPresets catalog —
Analog80s, VHSAnalog80s, SatelliteAnalog80s, VHSPureSignal, SatellitePureSignal,
GoldenHour, BlueHour, Retro8Bits — each with era-consistent optics: video/broadcast =
deep focus, cinema grades = photographic DoF, Retro8Bits = no photometry). Taken by
`Toolkit::generatePerspectiveCamera(..., preset = CameraPreset::Normal)` (perspective
only: the thin-lens DoF model is meaningless under orthographic projection; cubemap
capture cameras are never graded). Runtime re-application goes through
`CameraPresets::Apply(camera, token)` — demo cycle order IS the enum order.
LensPresets:: functions remain the lens-stack building blocks.

**Camera API** (all no-op when the matching effect is absent — the options are retained):
- `enableDepthOfField(bool)` / `enableMotionBlur(bool)` / `enableBloom(bool)` / `enableHDR(bool)`
  — MATERIALIZE the DepthOfField / MotionBlur / Bloom / ToneMapping effect in the scene chain (and
  remove it when disabled). ⚠️ **Canonical order: DepthOfField → MotionBlur → Bloom → ToneMapping**,
  which is the physical order of events: the optics form the image, the motion smears during the
  exposure, the glass scatters what was formed, the sensor responds. All four insert themselves
  ahead of the first `runsAfterToneMapping()` effect.
- ⚠️ **The motion blur has no strength knob, by design**: its length is `setShutterSpeed()` divided
  by the frame duration — the shutter angle, i.e. the fraction of the frame during which light was
  collected (1/48 s at 24 fps is the cinematic 180-degree rule). That is what makes it
  framerate-independent, and it means the exposure time is a SHARED control: it sets the blur AND
  one third of the exposure triad. A demo no longer adds `MotionBlur` to its stack; the quality
  knobs (`Core/Graphics/MotionBlur/SampleCount`, `SoftDepthExtent`) are read by the effect itself,
  like the depth of field's.
- Optics: `setAperture(fStop)`, `setFocalLength(mm)`, `setFocusDistance(m)` (implies
  manual focus, like tapping to focus), `setAutoFocus(bool)`.
- ⚠️⚠️ **THE FIELD OF VIEW IS NOT SETTABLE — it is derived.** `m_focalLength` (+ `m_sensorWidth`)
  is the SINGLE source of truth for the framing; `fieldOfView()` computes `2·atan(h/(2f))` on
  demand, `h` being `sensorHeight()` (`sensorWidth × 2/3`, 3:2 full frame) because the engine's
  field of view is VERTICAL. A camera is configured **like a real appliance** — a lens, a format,
  an aperture, a shutter, an ISO — and the projection matrices follow. `setFieldOfView()` and
  `changeFieldOfView()` **no longer exist**, `setPerspectiveProjection(distance)` no longer takes
  an angle, and `AnimationID::FieldOfView` is gone (animate `FocalLength`: a zoom IS a focal ramp).
  Reference points: 13.096 mm = the historical 85° default, 12 mm = 90°, 20.8 = 60°, 25.7 = 50°,
  50 mm = 27°.
  **Why it was done** (owner, Jul 2026): storing both an angle and a lens meant FOUR writers, and
  one of them — `setPerspectiveProjection(fov, distance)` — updated only the angle and left a stale
  focal length behind, so the panel reported a lens that did not match the image. Deriving removes
  the class of bug rather than one instance of it. Side effect: the 1 mm focal floor caps the
  derived angle at ~171°, replacing a clamp that allowed a geometrically meaningless 360°.
- An ultra-wide has enormous depth of field (the circle of confusion goes as f²), so a game-style
  framing yields a subtle DoF whatever the aperture — that is optics, not a weak effect. Visible
  background separation needs a longer lens, not a smaller f-number.
- `setSensorWidth(mm)` is the FORMAT knob, and the reference length that gives millimetres a
  meaning: it converts the thin-lens circle of confusion (in meters, on the sensor) into a
  fraction of the image, which is why the DoF needs no arbitrary scale. It behaves as a
  **constant lens**: the focal length is kept and the field of view follows, i.e. the physical crop
  factor — 11 mm sees 94.6° on full frame and 71.2° on APS-C. Changing the format therefore
  REFRAMES; it does not merely restyle the blur.
- `setTechnicalFieldOfView(degrees)` is the ONLY way an angle enters a camera, and it is **not
  photographic**: it exists where the field of view is a GEOMETRIC constraint — a cubemap face is
  strictly 90° or the six faces do not join. It stores the focal length that yields the angle (90°
  on a 24 mm-high sensor is exactly 12 mm, the round trip costing ~1e-5°) and raises
  `TechnicalProjection`, which **locks the sensor format** (`setSensorWidth()` warns and returns),
  since reframing is precisely what would break the constraint. `isTechnicalCamera()` reports it.
- Exposure: `setExposureCompensation(EV)`, `setAutoExposure(bool)`.
- ⚠️ **With auto-ISO on (the default), the aperture is not an exposure control.** The
  metering solves directly for the multiplier that puts the scene on middle grey, and the
  aperture only enters through the CLAMPS (what the ISO range permits). So f/11 → f/32
  changes the depth of field and nothing else until the metering saturates — aperture
  priority, exactly as a real body behaves. The aperture reads as stops of brightness only
  in manual mode, where the full APEX exposure applies.
- ⚠️ **EV compensation respects the sensor (2026-07-26)**: with auto-ISO on, the bias shifts
  the METERING TARGET (`keyValue × 2^EC`, applied INSIDE the sensor clamp) instead of
  post-amplifying the clamped result — +3 EV saturates at the same ISO ceiling, exactly as a
  real auto-ISO body. In manual mode it stays a straight EV bias on the APEX exposure.
- **Metered values ARE read back (2026-07-26)**: both GPU-resident measurements come home
  through a per-frame-in-flight host-visible ring (one tiny persistently-mapped slot per
  frame; slot N is read when it comes around again — its fence passed — so the read never
  stalls and costs framesInFlight frames of latency, the standard pattern).
  `ToneMapping::meteredSensitivity()/meteredLuminance()` (ISO + scene average in nits,
  decoded from the RGBA16F adaptation history) and `DepthOfField::meteredFocusDistance()`
  (meters, from the 1x1 RG32F focus history). Access from the panel through
  `PostProcessStack::cameraToneMapping()/cameraDepthOfField()` — RENDER THREAD, inside the
  frame scope, like everything the panel touches.
- ⚠️ **Auto and manual expose IDENTICALLY (2026-07-26)**: the auto-exposure keys on
  `Photometry::MeteredMiddleGrey` (K=12.5 / (MeterCalibration=1.2 · 100) ≈ 0.104), the value the
  manual APEX triad lands a correctly metered scene on. The previous key, Reinhard's 0.18, is a
  display-side grey-card convention — NOT what a K=12.5 meter produces through
  `exposureFromValue100()` — and kept auto mode 0.79 EV hotter than the same scene shot manually,
  shifting the auto-ISO window against its own sensor bounds. The three constants live in
  `Photometry.hpp` (single source); do not reintroduce a literal.
  Two consequences worth knowing: (1) the adaptation now consumes `PushConstants::deltaTime`
  (the chain contract — no self-measured time) and its FIRST execution resets the 1x1 history
  in the shader (`resetHistory` push constant): startup convergence is instantaneous instead of
  a long transient, and a recycled NaN can no longer poison the EMA forever (same guard as the
  DoF focus history); (2) ⚠️ any exposure statistic captured BEFORE that reset existed is
  suspect — the old path could take tens of seconds to converge, and comparisons made against
  such captures mis-attributed the difference (lived: a keyValue A/B read "no change" against a
  reference that was in fact an unconverged transient; the METERED ISO readback below is what
  settled it — gltf-loader meters ~ISO 1000 at f/11 1/250, well inside the sensor bounds).
- ⚠️ **Bloom intensity = the FRACTION of above-threshold energy the lens scatters (2026-07-26)**:
  the glare chain carries the full photometric energy (sunlit stone ~20000 nits over a 1000-nit
  threshold), and the composite is `original + bloom × intensity` in nits — so 1.0 means "the
  glass scatters ALL of it" and sets any daylight scene ablaze. Camera default 0.03 (a clean
  modern lens scatters 2-5%; a hazy vintage one >10%). This became visible when the anti-firefly
  ceiling was fixed: the old fixed `clamp(…, 64)` (LDR-era, four stops BELOW the default
  threshold) crushed every source to identical, near-invisible glare; the ceiling is now
  `max(threshold, 1) × 64` — six stops of differentiation headroom — pushed to EVERY downsample
  mip (`BloomPushConstants::fireflyClamp`).
- **Automatic modes are the default** (auto-focus + auto-exposure ON at construction).
- All optics are ANIMATABLE (`AnimationID::Aperture/FocalLength/FocusDistance/
  ExposureCompensation`) — focus pulls and exposure ramps via the animation system.
- Single-pass lens effects (VHS, grain, B&W...) were ALREADY per-camera via
  `addLensEffect()`; the physical camera extends the model to multi-pass effects.
- ⚠️ **Lens-effect list = PUBLICATION contract (2026-07-26)**: `Camera::lensEffects()`
  returns a `std::shared_ptr< const Graphics::DirectEffectList >` SNAPSHOT (nullptr = no
  effect); every mutation replaces the list wholesale (copy-on-write under the camera's
  internal lock). The renderer RETAINS the snapshot it records per frame in flight
  (`Renderer::m_lensEffectsSnapshots`, indexed by `currentFrameIndex()`), so a removed
  effect survives until the frame slot's fence has passed. This replaced a bare
  `std::vector` the render thread iterated while KeyPad style-cycling mutated it from the
  logic thread (use-after-free one keypress away) — do NOT reintroduce a mutable reference
  accessor, and never destroy a direct effect in place while frames are in flight.

**Camera cut** (`Scene::switchToCamera(std::shared_ptr< Component::Camera >)`): performs a
full cut — the camera becomes the RENDERED point of view (primary video source reroute
through `AVConsole::Manager::switchPrimaryVideoSource()`, which disconnects every other
source feeding the primary output) AND the photographic authority (active camera). One
call: image + look together. Validated on Sponza with fixed showcase cameras carrying
different presets (KeyPad8 cycle in the projet-alpha demos).

⚠️ **Active-camera lifetime contract (2026-07-26)**: the scene holds the photographic
authority as a WEAK reference behind an internal publication mutex.
`Scene::activeCamera()` returns a `std::shared_ptr` — a camera whose entity
self-terminated resolves to nullptr and the effects dematerialize through the regular
per-frame polling; the shared_ptr keeps the component alive for the caller's use even if
the entity dies mid-frame. NEVER cache the raw pointer across frames (that was the
use-after-free this replaced: the render thread could copy the raw pointer, then the
entity's destruction freed the component under it — the old CameraDestroyed clear was
unlocked and could arrive too late). App-side long-lived references (the KeyPad8 player
camera) are captured as `std::weak_ptr` and locked at use.

**Materialization mechanics** (no observer, no cross-thread races):
- `Renderer::renderFrame*()` calls `PostProcessStack::syncCameraEffects(activeCamera,
  renderer)` once per frame on the render thread — a two-boolean comparison when nothing
  changed. Camera switches (`Scene::setActiveCamera`) are therefore handled automatically:
  **each camera keeps its own photographic setup; the active one shapes the pipeline.**
- On change: new effects are created render-side; removed effects retire through
  `Renderer::deferredDestructor()` (frames-in-flight safety); the scene target is retired
  so the lazy configure path rebuilds the pipeline with the new requirements (HDR may
  appear/disappear with the tone mapping).
- The effects READ the camera each frame via `FrameContext::camera` — parameter changes
  (aperture, EV...) apply immediately, zero rebuild. Fallback to the effect's local
  `Parameters` when no camera exists.
- Demos/apps DO NOT add DepthOfField/ToneMapping to their stack anymore — they enable
  them on the camera (see `projet-alpha` `GLTFLoader::onEnabled()`).

**The chain is created ON DEMAND (Jul 2026)** — the camera is never silently ignored:

- `Component::Camera::requiresPostProcessing()` answers "does this camera need the
  pipeline to exist at all" (any of DoF / motion blur / bloom / HDR, or at least one lens
  effect). When it says yes and the scene has no chain, `Renderer::renderFrame()` calls
  `Scenes::Scene::requirePostProcessStack()`, which lazily creates an empty one.
- **The bug this closes:** before, `syncCameraEffects()` only ran when the APPLICATION had
  provided a stack, so `camera->enableHDR(true)` on any other scene was a no-op with no
  diagnostic. The raw photometric radiance then reached an LDR swap-chain — a daylight
  scene came out **pure white**, a night scene **pure black**. In `projet-alpha`, 29 of the
  34 demos were in that state.
- Lifetime/threading are unchanged: the stack still belongs to the `Scene` and dies with
  it. `requirePostProcessStack()` is render-thread (frame scope) or scene-building thread
  BEFORE activation — `Manager::newScene()` does not activate, so the two never overlap.

**Master switch vs. actual work** — two distinct questions, do not conflate them:

| Question | Who answers | API |
|---|---|---|
| Is post-processing ALLOWED? | the user | `PostProcessor::enable()` / `isEnabled()`, **default ON** |
| Is there anything TO run? | the renderer | `Renderer::m_postProcessingActive` / `needsInternalTarget()` |

`m_postProcessingActive` is recomputed once per recorded frame, **after**
`syncCameraEffects()` (so effects materialized this very frame count), as
`isEnabled() && (stack has effects || camera has lens effects)`. Every decision inside the
renderer — scene-target create/destroy, strategy dispatch, direct-path composite, jitter —
keys on it, never on `isEnabled()` alone. A scene with an empty chain therefore stays on
`renderFrameDirect()` and pays nothing, which is what lets the switch default to ON.

> [!CAUTION]
> Do not "fix" a missing effect by calling `postProcessor().enable(true)` from the
> application. That call is gone from `projet-alpha` (`AbstractDemo::createScene()`) on
> purpose: making the application arm the pipeline is what produced the silent no-op above.
> Declare the effect on the camera, or add it to the scene stack — the renderer arms itself.

e### Sprite photometric contract — a flame is authored in nits (Aug 2026)

A sprite is almost always a self-illuminating object (flame, explosion, muzzle flash, neon), it is
rendered UNLIT, and on the unlit path the surface colour IS the emitted radiance — so it needs a
real luminance or it contributes nothing. The manifest carries both halves:

```json
{
	"Type": "AnimatedTexture",
	"Data": { "Name": "fire001" },
	"BlendingMode": "Screen",
	"AutoIllumination": 1.0,
	"EmissiveStrength": 10000.0
}
```

- `AutoIllumination` — the emissive **MASK**, clamped to [0,1]. It cannot carry a brightness.
- `EmissiveStrength` — the **LUMINANCE in cd/m² (nits)**. Same key and same contract as
  `BasicResource` / `StandardResource` and the glTF extension `KHR_materials_emissive_strength`;
  the emitted quantity is `autoIlluminationColor * autoIlluminationAmount * emissiveStrength`.

⚠️ **The key was added in Aug 2026 and its absence was a hard limit, not an oversight to work
around in the application**: before it, `SpriteResource::load()` parsed the amount only, so every
sprite emitted exactly 1 nit and the fire and explosions of `game-logic`-style scenes were
invisible under photometric exposure. Reference values and the full failure mode are in
`docs/caution-points.md` § "The light RADIUS is a culling bound, not a dimmer".

Applied by `Material::Interface::emissionMultiplier()` — see `src/Saphir/AGENTS.md` § "Emission on
the UNLIT path", including why it multiplies here and adds on the lit path, and why it must never
reach the albedo attachment.

### Every ray query judges its alpha-tested candidates — `RTAlphaTestGLSL.hpp` (Aug 2026)

> [!CAUTION]
> **A cutout is a TLAS instance flagged `FORCE_NO_OPAQUE`; a ray launched with
> `gl_RayFlagsOpaqueEXT` overrides that flag and accepts every triangle whole.** A leaf becomes a
> solid quad, a fence a wall. RTGI did it on BOTH its rays (bounce + shadow) and RTR on its
> shadow ray: Sponza's ivy blocked the sky for the GI and cast a solid shadow at every bounce hit
> while the raster drew leaves. Only RTR's reflection ray judged its candidates — with a 60-line
> hand-written loop no other effect had copied.
>
> **The rule now lives in ONE place**: `Effects/Framebuffer/RTAlphaTestGLSL.hpp`, two macros
> holding GLSL string literals (a `constexpr const char *` cannot be spliced into the effects'
> `constexpr` shader literals — a macro can):
> - `EMEN_RT_ALPHA_TEST_GLSL_FUNCTIONS` — `rtHitMaterialIndex()` and `rtCandidateIsSolid()`,
>   the latter sampling the opacity texture, else the albedo alpha, else the scalar alpha, at the
>   candidate's UV, against the material's own `alphaCutoff` (raster parity);
> - `EMEN_RT_CONFIRM_ALPHA_TESTED_CANDIDATE(query)` — the body of a `while (rayQueryProceedEXT)`
>   loop.
>
> Usage: `gl_RayFlagsNoneEXT` (or `TerminateOnFirstHitEXT` alone for a shadow ray — it composes,
> the traversal ends at the first CONFIRMED candidate), then the loop macro. **Never
> `gl_RayFlagsOpaqueEXT` on a scene ray again.**
>
> - ⚠️ The macros rely on names the host shader must declare — `meshSSBO`, `materialSSBO`,
>   `textures2D`, `getMeshAccessor()`, `getHitUV()`, `IsAlphaTest`, `HasOpacityTexture`,
>   `HasAlbedoTexture`. A missing one is a GLSL error at RUNTIME, not at build time.
> - **RTAO and ContactShadows apply it too** (same day). Neither declared the scene data, so the
>   header carries a third macro, `EMEN_RT_SCENE_DATA_GLSL(bindlessSet)`: exactly the SSBO,
>   bindless and UV-fetch declarations the rule needs, for an occlusion-only effect. Both effects
>   gained the bindless set (set 2) in their pipeline layout; ContactShadows dropped its private
>   descriptor set with its own TLAS binding and now uses the Renderer's RT set at 0 like every
>   other RT effect, with a custom three-set recording (the shared fullscreen recorder binds one).
> - ⚠️⚠️ **A candidate-judging ray costs more than an opaque one** — it returns to the shader for
>   EVERY cutout triangle it crosses, with a texture fetch each time — **and the bill scales with
>   the ray count.** Measured on Sponza at the spawn pose (dense ivy in the frame), GPU scopes:
>
>   | scope | opaque flag | alpha-test rule |
>   |---|---|---|
>   | `RTAOEffect/trace`, full-res × 8 spp (owner settings) | 12.3 ms | **26.3 ms** |
>   | `RTAOEffect/trace`, half-res × 8 spp | — | **7.2 ms** |
>   | `ContactShadowsEffect/trace` | 1.45 ms | 2.28 ms |
>   | `RTGIEffect/internal` (half-res × 4 spp, already judging) | 23.5 ms | 24.1 ms |
>
>   RTAO at full resolution DOUBLES under the rule (37 M rays/frame through the foliage); at half
>   resolution the corrected effect costs LESS than the uncorrected full-res one. The resolution is
>   the owner's knob (`RayTracing/AmbientOcclusion/PixelDoubling`), not an engine default to flip
>   behind their back. Visual result of the RTAO/ContactShadows port: 23.5 % of the frame changed
>   by more than 5/255, symmetric (11.7 % brighter — the ivy no longer self-occludes as solid
>   cards; 11.8 % darker), frame mean +0.45.
> - ⚠️ RTR paid the candidate price from the start; RTGI pays it since the same day and did not
>   move (23.5 → 24.1 ms, noise) — it was already judging in both measurements.

### RTR shades its hits with the EFFECTIVE ambient, not the LightSet value (Aug 2026)

> [!CAUTION]
> **`LightSet::ambientLightIntensity()` is NOT what the raster shades with.** When the sky drives
> the ambient (`applyAmbient`), the ambient pass reads the baked irradiance cubemap and the scalar
> pushed to the view UBOs is ZERO — the LightSet keeps the manifest's value (17 000 lx for a
> daylight sky) for whoever reads the sky's photometry. RTR shaded its hit points with the
> LightSet value, so under a sky-driven scene every reflection carried a flat 17 000 lx ambient
> ON TOP of its IBL term — the reflected world was brighter than the world it reflected, which
> breaks the "the reflection matches the raster" contract.
>
> `Scene::effectiveAmbientIlluminance()` is the single site of the rule (the UBO refresh reads it
> too), and it reaches the effects as `FrameContext::ambientIlluminance`. **Anything that SHADES
> its own hit points must read that field, never the LightSet.** RTR's IBL term at hits stays: it
> is its estimate of the indirect light at points RTGI cannot reach, and removing it would make
> the reflections too dark instead of too bright.

### An effect that is not CREATED is never recorded (Aug 2026)

> [!CAUTION]
> **A post-process effect holds per-frame containers that its recording code indexes without
> checking, so an un-created effect in the chain is a SEGFAULT, not a misbehaviour.** RTGI's
> `m_tracePerFrame[frameIndex]` on an empty vector is an out-of-bounds read — no null test would
> have caught it, and the stack trace points at `std::unique_ptr::operator->` with nothing to say.
> It happened twice, for two unrelated reasons:
> - a stack **installed without `createAll()`** — a demo factored its post-process setup and the
>   new code path called `Scene::setPostProcessStack()` while the old one created the effects
>   first (projet-alpha `AbstractDemo::create()`, Aug 2026);
> - a **`resize()` whose `create()` half failed** after its `destroy()` half had already run,
>   leaving the effect in the chain with its resources gone.
>
> **The contract.** `IndirectPostProcessEffect::isCreated()` says whether the effect holds its
> resources; `PostProcessStack` is the ONLY writer (`setCreatedFlag()`), because it drives the
> whole lifecycle — `createAll()`, `resizeAll()`, `destroyAll()`, and the four photographic
> effects it materializes itself in `syncCameraEffects()`. `PostProcessor` skips a
> non-created effect exactly as it skips a disabled one.
>
> - ⚠️ The skip is **silent by design**: the failure is already traced loudly where it happens
>   (`createAll()`/`resizeAll()` name the effect), and repeating it in the executor would print one
>   line per effect per frame.
> - ⚠️ `createAll()` and `resizeAll()` no longer **return on the first failure**: they used to
>   leave every remaining effect un-created, and those were recorded exactly like the one that
>   failed. They now attempt them all and report the aggregate.
> - ⚠️⚠️ **The photographic effects are created in `syncCameraEffects()`, never by `createAll()`.**
>   Forgetting their flag there does not crash — it silently skips the TONE MAPPING and leaves the
>   whole frame in linear HDR.

### The frame is CUT around the translucent pass — indirect diffuse before the glass (Aug 2026)

> [!CAUTION]
> **What is seen THROUGH a transmissive surface must receive its indirect diffuse, and the only
> way is to composite that term BEFORE the material grab pass copies the scene.** The indirect
> chain used to run after the TranslucentGB pass, so a glass transmitted a scene with no indirect
> light in it; once the indirect-diffuse ownership contract switched the raster's own IBL leg off
> under RTGI, the watch dial behind its crystal received no indirect light AT ALL (measured:
> −6.11 on the dial vs −4.64 on the opaque case, RTGI minus no-GI, before the cut).
>
> **The mechanism.** `EffectSlot::isPreTranslucencySlot()` names the slots that run early — ONLY
> `IndirectDiffuse`. `PostProcessStack::hasEnabledPreTranslucencyEffect()` says whether a frame
> has anything to cut. `Renderer::renderFrameWithInternal()` cuts the frame when that is true AND
> the frame contains TranslucentGB objects:
> ```
> opaque + translucent scene pass
>   → PostProcessor::recordBlit  →  executeIndirectPostProcessEffects(PreTranslucency)
>       (the IndirectDiffuse slot, on the OPAQUE G-buffer)  →  recordWriteBack() into the scene colour
>   → material grab pass (now carries the indirect diffuse)  →  TranslucentGB pass
>   → PostProcessor::recordBlit  →  executeIndirectPostProcessEffects(PostTranslucency)  →  composite
> ```
> An uncut frame (no glass, or no enabled indirect-diffuse effect) runs `ChainPhase::Whole`
> exactly as before and pays nothing new.
>
> - ⚠️ **Only the indirect diffuse moves forward.** The ambient occlusion stays in the closing
>   half: its snippet is a global multiply and it must come AFTER the reflections (placed before
>   them it stopped attenuating them — the owner-reported bright patches). It still attenuates the
>   transmitted indirect diffuse, which is already in the colour by then; the price is that AO is
>   evaluated at the glass surface rather than at the surface behind it. The reflections stay after
>   the translucent pass so a water surface keeps its SSR/RTR through its own G-buffer footprint.
> - ⚠️ **The write-back is a blit** (`PostProcessor::recordWriteBack`, same two-barrier discipline
>   as `recordBlit`): chain target SHADER_READ_ONLY → TRANSFER_SRC, scene colour
>   COLOR_ATTACHMENT → TRANSFER_DST, blit, restore both. The scene colour MUST come back to
>   COLOR_ATTACHMENT_OPTIMAL — the material grab pass and the TranslucentGB LOAD pass expect it.
>   Rendering the combine straight into the scene colour would save the blit; not done, measure
>   first.
> - ⚠️ **The closing half owns the frame.** Only `ChainPhase::Whole`/`PostTranslucency` update
>   `m_lastChainFrameTime` (the motion blur's frame duration — the two halves run milliseconds
>   apart) and the final composite descriptor. The pre-translucency half returns right after its
>   write-back.
> - ⚠️ **The RT descriptor set is refreshed BEFORE the pre-translucency half** (it used to be
>   refreshed after the translucent pass): RTGI traces there.
> - ⚠️ **TranslucentGB programs keep their raster IBL diffuse.** The indirect diffuse can no longer
>   reach a surface drawn by that pass, so its own diffuse leg (a frosted glass, a tinted water)
>   is exempt from the ownership weight — decided at shader GENERATION through the grab-pass
>   marker (`LightGenerator::m_transmissionIsSceneRadiance`), zero runtime cost.
> - ⚠️ The swap-chain (direct) path is never cut: an indirect-diffuse effect needs the G-buffer,
>   which forces the internal scene target. `recordWriteBack` refuses to run without it.
> - ⚠️⚠️ **Two usage flags had to be added for the write-back, and a blit without them RUNS anyway
>   on NVIDIA** — with a plausible image and four VUIDs a frame (`oldLayout-01212/01213/01197`,
>   `vkCmdBlitImage-srcImage-00219`/`dstImage-00224`): the CombinePass targets now carry
>   `TRANSFER_SRC` (`CombinePass.cpp`), the scene colour image `TRANSFER_DST`
>   (`SceneRenderTarget.cpp`, colour only). ⚠️ `RenderTarget/Texture.hpp` is NOT the scene target
>   — it is the render-to-texture probe; the flag was first added there by mistake and reverted.
>   The undefined-behaviour blit also poisoned the first measurement (a −2.5 shift on the SKY,
>   which the cut cannot touch): **never read a number off a run that reports VUIDs.**
> - **Measured** (asset-loader ChronographWatch, 2880×1620, RTGI on in both runs, `Core/Graphics/
>   PostProcessing/CutFrameAroundTranslucency` true vs false, run-to-run noise from two identical
>   uncut runs):
>
>   | region | cut − uncut | noise floor |
>   |---|---|---|
>   | dial, behind the crystal | **+3.47** | +0.48 |
>   | case + strap (opaque) | −0.71 | +0.45 |
>   | whole watch | +0.27 | +0.51 |
>   | sky | −0.97 | +0.58 |
>
>   The dial gains its indirect diffuse (7× the noise); everything the cut must not touch stays
>   inside the noise. Cost: frame 5.65 → 6.28 ms (**+0.64 ms**: the pre-translucency half at
>   1.33 ms including its own grab blit, minus the GI that left the closing half, 4.55 → 3.81 ms).
>   0 VUID. The key is an A/B switch for exactly this kind of measurement, not a quality knob:
>   OFF, nothing seen through a glass receives indirect light.

### The chain order is a STRUCTURE, not a call sequence — `EffectSlot` (Aug 2026)

> [!CAUTION]
> **`PostProcessStack` is no longer an insertion-ordered vector.** It is a fixed table of
> CONCEPTS — `Graphics/EffectSlot.hpp` — walked in enum declaration order, and every framebuffer
> effect declares the concept it implements (`IndirectPostProcessEffect::slot()`, **pure
> virtual**). `addEffect()` files the effect into its slot; **the order of the calls has no
> effect whatsoever**. Twelve scenes used to restate the order by hand, three of them
> differently, and a wrong order was silent.
>
> **The canonical order (this IS the enum):**
> ```
> IndirectDiffuse → Reflections → AmbientOcclusion → ContactShadows → Fog
>   → VolumetricLight → LensFlare → Custom → TemporalAA
>   → [camera: DepthOfField → MotionBlur → Glare → ToneMapping] → PostToneMapping
> ```
>
> ⚠️⚠️ **This order CHANGED with the redesign, and the change is a correctness fix.** It used to
> be `Reflections → AmbientOcclusion → IndirectDiffuse`, which broke one thing:
> - **SSR reflected an unlit world.** `SSR` samples the chain colour to fetch what a reflected
>   ray sees (`reflColor = texture(colorTex, traceData.xy).rgb`) and declares
>   `readsChainColorUpstream()` PRECISELY so the pending combine group is flushed before it — but
>   placed first, it had nothing to flush. Since the indirect-diffuse OWNERSHIP contract, an
>   enabled RTGI also switches the raster's ambient IBL leg off, so those reflections carried no
>   sky light at all. `RTR` is immune: it shades its own hits and never reads the chain.
>
> ⚠️⚠️ **THE AMBIENT OCCLUSION IS LAST OF THE THREE, and both neighbours are load-bearing.** The
> members of a combine group emit their snippets into ONE generated pass in slot order, and AO's
> is a GLOBAL MULTIPLY (`em_Color.rgb *= ao;`) while GI's and the reflections' are adds:
> **everything emitted BEFORE the AO is attenuated, everything after is not.**
> - Placed FIRST (the historical order) it multiplied the direct lighting and left the indirect
>   diffuse it exists to occlude untouched.
> - Placed between the GI and the reflections — which is where the redesign put it for a few
>   hours — it stopped attenuating the reflections, and traced reflections came out at FULL
>   strength inside creases and occluded corners. **Owner-reported the same day**: "la réflexion
>   éjecte des gros pâtés lumineux partout". Measured on Sponza at the spawn pose, moving it back
>   after the reflections darkens **8.72 % of the frame by more than 10/255** (2.99 % by more than
>   30, max 204) while the frame mean moves by 0.48 — i.e. it does not darken the image, it
>   restores occlusion where it was missing. The difference map is concentrated on the FOLIAGE and
>   on the arch edges.
> - ⚠️ A global multiply is not the physically right instrument (an occlusion for the diffuse and
>   a specular occlusion for the reflections are different terms) — it is the same simplification
>   UE4 makes. The slot order is the best available placement for it, not a proof that the term
>   is correct.
>
> **A slot holds as many occupants as the application builds** — several RTGI and several SSGI
> with different `Parameters`, all resident so a runtime switch compares them on the very same
> framing — **of which AT MOST ONE IS ENABLED**. That exclusivity is mechanical, not a convention:
> `PostProcessEffect::enable()` is virtual, `IndirectPostProcessEffect` overrides it, and enabling
> an effect asks its stack to `disableSlotSiblings()`. **Enabling one is SELECTING it.**
> `EffectSlot::Custom` is the single multi-occupant slot, the extension point for an application
> effect the engine has no concept for.
>
> - ⚠️ **The requirement aggregation ignores `isEnabled()`** (`requiresAlbedo()`, `requiresHDR()`,
>   …): the attachment snapshot the Renderer takes on the frame the scene target is created must
>   cover EVERY alternative, or selecting a disabled one later would find no attachment. Concrete
>   case: `SSGI` declares `requiresHDR()`, `RTGI` does not. The cost is the attachments of an
>   alternative that never runs — the price of the runtime A/B, and it is what the old
>   "create everything enabled, select after a 200 ms timer" trick was paying blindly.
>   `hasEnabledReflectionProvider()` / `hasEnabledIndirectDiffuseProvider()` keep filtering on
>   `isEnabled()`: they are about what RUNS, not about what is allocated.
> - ⚠️ **The four camera slots are REFUSED to `addEffect()`** with a trace error. `DepthOfField`,
>   `MotionBlur`, `Glare` and `ToneMapping` are materialized by `syncCameraEffects()` from the
>   camera's own switches, which owns their lifetime — an application-added one would be
>   destroyed under its feet at the next camera change. That method lost its
>   erase → `find_if(runsAfterToneMapping)` → insert dance: it assigns four slots.
> - ⚠️ `runsAfterToneMapping()` is now DERIVED (`slot() == EffectSlot::PostToneMapping`) instead
>   of being an independent virtual that could disagree with the effect's position.
> - ⚠️ The back-pointer an effect keeps to its stack is RAW, and safe by who clears it: the stack
>   clears it on removal, in `clearEffects()` and in its own destructor, always while it still
>   holds a `shared_ptr` to the effect. An effect DOES outlive its stack — a demo keeps copies to
>   toggle it.

### Effect Chain Order & Phase Contract (Jul 2026)

The chain has THREE phases, enforced structurally:

1. **Scene effects (HDR, linear)** — declared by the application/demo stack: GI, AO,
   reflections, fog, volumetric light, bloom...
2. **Photographic effects (HDR resolve)** — materialized by the ACTIVE CAMERA
   (`enableDepthOfField()`/`enableHDR()`, see "Physical Camera" below): DepthOfField,
   then ToneMapping (HDR→LDR).
3. **Post-tonemap effects (LDR, display-referred)** — effects overriding
   `IndirectPostProcessEffect::runsAfterToneMapping()` (FXAA, FXAASharpen, Sharpen).

> [!CRITICAL]
> `PostProcessStack::syncCameraEffects()` inserts the camera effects BEFORE the first
> `runsAfterToneMapping()` effect. Running AA/sharpen on linear HDR input produces severe
> posterization and halo streaks (observed live on Sponza, Jul 2026) — any new LDR effect
> MUST override `runsAfterToneMapping()`.

Scene-effect order, as every scene actually builds it:
```
RTR|SSR → RTAO|SSAO → RTGI|SSGI → ContactShadows → AtmosphericFog → VolumetricLight → LensFlare
[camera: DoF → glare/Bloom → ToneMapping] [LDR: FXAASharpen]
```

**Rationale:** RTR first (hardware ray tracing, highest quality reflections), SSR as the fallback
where RTR is unavailable — the two are alternatives, never both. AO next, then GI. ContactShadows
adds fine-detail shadowing from depth. AO darkening the image globally including reflections is
acceptable and matches UE4's approach, where AO is a global multiplier applied after reflection
composition. LensFlare last, from bright light sources.

> [!CAUTION]
> **This list is the one the scenes build; the previous revision matched NO scene and carried a
> rationale the code does not implement.** It read `RTR → SSR → ContactShadows → SSAO →
> AtmosphericFog → VolumetricLight → LensFlare → Bloom`, which differed on three counts, all
> verified against `Sponza`, `Citadel`, `WaterWorld` and `LightAndShadowDebug`:
> - it put **ContactShadows before AO**; every scene puts AO first;
> - it **omitted GI entirely** (`RTGI|SSGI`), which every scene inserts after AO;
> - it ended the SCENE stack with **Bloom**, which is not a scene effect: veiling glare is a LENS
>   phenomenon carried by the active camera (`enableHDR`/glare threshold), and adding it to the
>   scene stack would run it before the defocus.
>
> ⚠️ **"AtmosphericFog before VolumetricLight so god rays bloom through the fog" was FALSE.** The
> shafts cannot be attenuated by an effect placed before them: `VolumetricLight::producesOverlay()`
> is true, its combine snippet is a pure add (`em_Color.rgb += texture(vlightTex, vUV).rgb`), and
> `recordOverlayPasses()` takes its `inputColor` **unnamed and unused** — the shafts are built from
> the depth occlusion mask and the light colour/intensity alone and never see the chain. Placing the
> fog first buys nothing the reverse order would not. If shafts SHOULD extinguish in fog, that is a
> feature to implement (feed the fog's transmittance into the shaft integration), not a line in an
> ordering rationale. **OPEN.**
>
> ⚠️ An effect that overrides `readsChainColorUpstream()` — `LensFlare` does, for its bright pass —
> IS genuinely sensitive to what precedes it: `PostProcessor` flushes the pending combine group so
> the effect sees the real chain colour. That coupling is real, unlike the VolumetricLight one. But
> the conclusion drawn from it — that the LDR fog was starving LensFlare's 2000-nit threshold and
> deleting the flares — is **FALSE, and was disproved by measurement before it could be committed**.
>
> Four states were shot looking straight at the sun, same pose: fog off; fog fixed; fog with its
> luminance forced back to 1.0 (the LDR bug re-emulated); and the fog ALSO given back its unsigned
> `tanHalfFovY`. **The halo ring and the shafts are present in all four.** The fog never gagged
> LensFlare. What hid the flare was the DEMO's exposure: at f/5.6 with auto-exposure, 42 % of the
> frame clipped and a soft grey ring on a white sky is invisible by construction. Pinning the triad
> (`6018bc7`) is what revealed it — the owner had never seen this effect run.
>
> ⚠️ **Two rules.** A plausible causal chain between two real defects is still a guess: "the fog is
> LDR" and "the flare is missing" were both true and unrelated. And when an effect appears to come
> back after a fix, suspect the EXPOSURE before crediting the fix — on an auto-exposing camera the
> sensor is the loudest variable in the frame.

### DepthOfField — Production-Grade Gather DoF (Jul 2026)

7-pass physical camera DoF (technique refs: Jimenez, "Next Generation Post Processing
in COD:AW", SIGGRAPH 2014):

1. **Focus** (1x1 RG32F ping-pong): auto-focus measurement (5x5 Gaussian around screen
   center) + exponential rack focus EMA (R = focus distance, G = timestamp for the frame
   delta; manual focus pulls are smoothed too). First-frame reset flag (DONT_CARE images);
   on the reset frame the shader must NOT read the undefined history — it falls back to the
   manual focus distance (an inherited NaN would survive every later EMA and kill the DoF
   until recreation).
2. **Setup** (half-res RGBA16F): downsampled color + SIGNED thin-lens CoC in alpha
   (positive = far field, negative = near field; sky lands in the far field naturally).
   ⚠️ The alpha is the CoC radius in half-res PIXELS (`sensorFraction * targetWidth / 2`,
   clamped to ±MaxRadius at the source) — NOT a normalized fraction. Every downstream pass
   consumes pixels directly; skipping that conversion once left the physically-correct CoC
   ~80x too weak (the ~0.01 fraction was read as pixels).
3/4. **Near-CoC dilation** (H/V max filter, R16F): spreads the near coverage BEYOND the
   silhouettes — the foreground blur must bleed over the sharp background.
5. **Far gather** (half-res): golden-angle spiral disc (circular bokeh), scatter-as-gather
   weighting (a sample contributes when its own CoC reaches the shaded pixel), near-field
   samples excluded.
6. **Near gather** (half-res): same spiral driven by the DILATED near CoC, no occlusion
   rejection (foreground freely covers the background). Coverage in alpha.
7. **Composite** (full-res): sharp base → far blend by CoC factor → near OVER (bleed),
   modulated by the per-pixel material DoF mask (matprops A low nibble, HUD exemption).

Optics come from the ACTIVE CAMERA (`FrameContext::camera`); quality knobs from
`Core/Graphics/DepthOfField/` settings (`MaxRadius`, `SampleCount`, `AutoFocusSpeed`,
`NearField`). `MaxRadius` (default 32, half-res pixels) is a pure performance/quality
ceiling — the blur AMOUNT is the thin-lens CoC alone, there is no scale factor
(`CoCScale` was removed 2026-07-26; a stale persisted key is ignored, but a persisted
`MaxRadius` from an older settings file still caps the blur). `NearField=false` skips
passes 3/4/6 entirely.

### FrameContext — Effect Chain Context (Jul 2026)

`IndirectPostProcessEffect::execute()` takes `(commandBuffer, inputColor, const
FrameContext &)` — the context groups the G-buffer inputs (depth/normals/matProps/albedo),
the LightSet, the ACTIVE CAMERA and the frame PushConstants. This replaced the former
8-parameter signature across all 16 effects (the GBufferInputs refactor). Any new
per-frame data belongs in FrameContext, NOT in a new parameter.

### IntermediateRenderTarget usage flags (Jul 2026)

`IntermediateRenderTarget::create()` gives every target `COLOR_ATTACHMENT_BIT | SAMPLED_BIT |
TRANSFER_DST_BIT` — enough to render into it, sample it, and clear it. Anything beyond that must
be requested through the trailing `extraUsageFlags` parameter.

**Every IRT starts ZERO-CLEARED (Aug 2026).** `create()` clears the image to `(0,0,0,0)` before
the initial transition to `SHADER_READ_ONLY_OPTIMAL`. This is a hard guarantee, not a nicety:
temporal effects sample their history IRT before the first write (the VolumetricLight
occlusion-mask EMA, the GIDenoiser ping-pong). Fresh device memory is UNDEFINED — desktop
drivers happen to return zeroed pages, **Metal/MoltenVK returns real garbage**, and in float
formats garbage bit patterns contain NaNs. A NaN entering an EMA feedback loop
(`mix(history, current, alpha)`) never leaves it (`NaN * 0 = NaN`) and spreads to the whole
frame through the additive combine. Measured on macOS (Apple M2): R/B channels NaN-flushed to 0
at the UNORM swapchain write — every demo with VolumetricLight rendered as a green-only frame.
A temporal effect whose history carries a validity marker (GIDenoiser: `history.a > 0.0`, false
for NaN) is defended in depth; one that mixes blindly relies entirely on this clear.

> [!WARNING]
> **An image can only be TRANSITIONED to a layout its usage flags support.** If your effect reads
> a target back with `vkCmdCopyImageToBuffer`, it MUST be created with
> `VK_IMAGE_USAGE_TRANSFER_SRC_BIT`, or the barrier to `TRANSFER_SRC_OPTIMAL` is silently
> rejected and every later command runs against a **stale tracked layout** — the failure surfaces
> as four unrelated-looking VUIDs pointing at the copy, not at the creation. `ToneMapping`'s
> auto-exposure adaptation targets and `DepthOfField`'s rack-focus targets (both 1x1 per-frame
> readbacks) are the reference cases; see `docs/caution-points.md` § Vulkan Validation.

```cpp
m_adaptTargets[index].create(renderer, 1, 1, lumFormat, "AdaptLum0", VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
```

The same rule applies outside `IntermediateRenderTarget`: any `SceneRenderTarget` attachment
copied to the grab pass by `PostProcessor::recordBlit()` needs `TRANSFER_SRC_BIT` at creation
(all six attachments — color, normals, material properties, albedo, velocity, depth — have it).

**A second write-direction case (Aug 2026):** the pre-translucency write-back blits a CombinePass
target INTO the scene colour image, so the target needs `TRANSFER_SRC_BIT` and the scene colour
`TRANSFER_DST_BIT` (`SceneRenderTarget.cpp`, colour only). ⚠️ On NVIDIA the blit without them ran
and produced a plausible image — only the four VUIDs told; see § "The frame is CUT around the
translucent pass".

**And it applies in the WRITE direction too (Aug 2026).** `RenderTarget::ShadowMap` clears its
depth image to 1.0 through `TransferManager::clearDepthImage()` at creation, so that image needs
`VK_IMAGE_USAGE_TRANSFER_DST_BIT` alongside `DEPTH_STENCIL_ATTACHMENT_BIT | SAMPLED_BIT`. It was
missing for a while: the clear and both surrounding barriers were rejected, the image never left
`UNDEFINED`, and **`vkQueueSubmit` refused the shadow pass on every frame** — directional shadows
gone, silently, on the NVIDIA driver. Full cascade in `docs/caution-points.md` § Vulkan
Validation; per-target flag table in `docs/render-targets.md`.

**Readback is a declared capability, not a fallback.** `TransferManager::downloadImage()` now
**refuses** an image that lacks `TRANSFER_SRC_BIT` (it used to attempt a blit through
`VK_IMAGE_LAYOUT_GENERAL`, which is invalid by construction — `vkCmdBlitImage` requires the usage
bit on `srcImage` whatever the layout). Consequently `RenderTarget::Texture` declares
`TRANSFER_SRC_BIT` unconditionally on its **color** image so `capture()` and the
`dumpRenderTarget` console command work; its **depth** image deliberately does not, because
nothing reads it back and `TRANSFER_SRC` is the flag that costs depth-compression metadata on
some architectures. Grant the flag at the creation site, never work around its absence.

### Coding Conventions for Effects

**Use engine types for semantic data, raw floats for GPU push constants.**

The engine provides rich types in `Base/` that should be used for all user-facing parameters, member variables, and API signatures. Push constant structs are the only exception — they must remain raw `float` fields for GPU memory layout compliance.

| Semantic | Engine type | Header |
|----------|------------|--------|
| Direction (3D) | `Base::Math::Vector< 3, float >` | `Base/Math/Vector.hpp` |
| Position (3D) | `Base::Math::Vector< 3, float >` | `Base/Math/Vector.hpp` |
| Color (RGB/RGBA) | `Base::PixelFactory::Color<>` | `Base/PixelFactory/Color.hpp` |
| Rotation | `Base::Math::Quaternion< float >` | `Base/Math/Quaternion.hpp` |
| Transform | `Base::Math::Matrix< 4, float >` | `Base/Math/Matrix.hpp` |

**Aliases**: `Vector3F` = `Vector< 3, float >`, `ColorF` = `Color< float >`.

**Rules:**
1. **`Parameters` struct** — Use `Color<>` for colors, `Vector< 3, float >` for directions/positions. Never use `float xxxR, xxxG, xxxB` or `float dirX, dirY, dirZ`.
2. **Member variables** — Same rule: `m_lightDirection` (`Vector< 3, float >`), not `m_lightDirX/Y/Z`.
3. **Setter methods** — Accept engine types: `setLightDirection(const Vector< 3, float > &)`, not `(float x, float y, float z)`.
4. **Push constant structs** — Keep as raw `float` fields (POD with `static_assert` on size). These are GPU-uploaded verbatim.
5. **Populating push constants from engine types** — Use accessors: `color.red()`, `color.green()`, `color.blue()`, `vec.x()`, `vec.y()`, `vec.z()`.
6. **Normalization** — Use `vector.normalized()` instead of manual `sqrt()` + division.
7. **Constructor** — Accept `const Parameters & parameters = {}` to allow inline initialization at construction.

**Example (AtmosphericFog):**
```cpp
// Parameters struct — engine types
struct Parameters {
    float density{0.02F};
    Base::PixelFactory::Color<> fogColor{0.5F, 0.6F, 0.7F};
    // ...
};

// Member — engine type
Base::Math::Vector< 3, float > m_lightDirection{0.0F, -1.0F, 0.0F};

// Setter — engine type
void setLightDirection(const Base::Math::Vector< 3, float > & direction) noexcept;

// Push constants — raw floats (GPU layout)
struct FogPushConstants {
    float fogColorR, fogColorG, fogColorB;
    float lightDirX, lightDirY, lightDirZ;
};

// Populate — accessors
.fogColorR = m_parameters.fogColor.red(),
.lightDirX = lightDir.x(),
```

**Reference implementation:** `Effects/Framebuffer/AtmosphericFog.hpp/cpp`

## 13. Geometry ResourceGenerator: Gem Methods

`ResourceGenerator` provides GPU-ready `IndexedVertexResource` wrappers for all 12 gem cuts. Each method follows the same pattern:

```cpp
std::shared_ptr< IndexedVertexResource > diamondCutGem (
    float radius, float depth, float tableRatio, uint32_t segments,
    std::string resourceName = {}
) const noexcept;
```

### Pattern
1. Auto-generates resource name from class + parameters if empty
2. Calls `ShapeGenerator::generate*CutGem< float, uint32_t >(...)` with `ShapeBuilderOptions`
3. Applies transform matrix if not identity
4. Loads into `IndexedVertexResource` via `getOrCreateResource()`

### Available Methods
`diamondCutGem()`, `emeraldCutGem()`, `asscherCutGem()`, `baguetteCutGem()`, `princessCutGem()`, `trillionCutGem()`, `ovalCutGem()`, `cushionCutGem()`, `marquiseCutGem()`, `pearCutGem()`, `heartCutGem()`, `roseCutGem()`

See: `Graphics/Geometry/ResourceGenerator.hpp`, `Graphics/Geometry/ResourceGenerator.cpp`

## 14. Pipeline Efficiency Objectives

> [!CRITICAL]
> **Baseline established via RenderDoc programmatic analysis** (LightAndShadowDebug, 6 objects, 3 lights).
> Every pipeline modification must be measured against this baseline using `/renderdoc-capture`.

### Current State (Baseline)

| Metric | Value | Assessment |
|--------|-------|------------|
| Draw calls per frame | 86 | Acceptable for 6 objects |
| Render passes per frame | 48 | **High** — 42 post-process + 3 shadow + 2 geometry + 1 overlay |
| Draws per object (geometry) | 4× | **Redundant** — multi-subpass G-buffer |
| Post-process passes | 42 | **Excessive** — 9 effects producing 42 passes |
| Compute dispatches | 0 | **Missing** — all effects use fragment shaders |
| Heaviest mesh | Ground plane, 2M indices × 7 renders = 14M/frame | **Disproportionate** |

### Optimization Roadmap (Priority Order)

| Priority | Objective | Current | Target | Impact |
|----------|-----------|---------|--------|--------|
| **P1** | MRT single-pass deferred | 4 draws/object | 1 draw/object | -75% geometry draws |
| **P2** | Fuse chainable post-process passes | 42 passes | ~15-20 passes | Fewer render pass transitions |
| **P3** | Compute shaders for blur/SSAO | Fragment-only | Compute + Fragment | Shared memory, no RP transitions |
| **P4** | Mesh LOD / tessellation | 2M indices flat ground | Adaptive | Scalable scene complexity |
| **P5** | GPU-driven culling | CPU-side | Compute dispatch | Scalable to large scenes |

> P2 has an **owner-approved phased plan** (2026-08) with per-effect pass counts, merge
> targets and execution order — see
> [`docs/post-processing-pipeline.md`](../../docs/post-processing-pipeline.md) § 5.
> Phases D, B, C, A and E are DONE: batched grab-pass barriers + `offscreenComposite`
> swap-chain pass (D); `Effects::Display::*` folded into the final shader (B);
> ToneMapping applies the bloom itself (C); the nine overlay effects apply through the
> shared generated `CombinePass` (A) and the seven separable-blur effects run their
> blurs through the shared MRT `DenoisePass` (E) — their own apply/composite AND blur
> passes are GONE. Before touching ANY overlay effect's apply or blur math, read
> `docs/post-processing-pipeline.md` § 3b/§ 3c: the math now lives in the effect's
> `combineContribution()` / `denoiseContribution()` GLSL snippets.

### UE5 Comparison (Same Scene)

| Aspect | emeraude-engine | UE5 equivalent |
|--------|----------------|----------------|
| G-buffer | Multi-subpass, 4 draws/object | Single-pass MRT, 1 draw/object |
| Post-process | 42 separate render passes | Fused passes + compute shaders |
| Blur (Bloom, SSAO, DoF) | Fragment shader per pass | Compute shader with shared memory |
| Culling | CPU-side | GPU-driven (compute) |
| Mesh detail | Fixed resolution | Nanite (virtualized geometry) |

### Measurement Protocol

Every pipeline modification **must** follow this protocol:
1. **Before**: Run `/renderdoc-capture` on the test scene, record metrics
2. **Implement**: Make the change
3. **After**: Run `/renderdoc-capture` again, compare metrics
4. **Verify**: Visual output must be identical or improved (read the thumbnail)
5. **Report**: Delta in draw calls, render passes, vertex throughput

No blind optimization. No guesswork. Data drives every decision.

## 15. Instance-Local Program Cache (RenderableInstance)

> [!IMPORTANT]
> **`RenderableInstance::Abstract` has a local `ResolvedProgram` cache (`StaticVector<16>`)
> that avoids per-draw hashtable lookups into `Renderable::Abstract::m_programs`.**

### Architecture

The `Renderable::Abstract` stores compiled shader programs in a `std::unordered_map` keyed
by `ProgramCacheKey` (render pass handle + material hash). Looking up this map every draw call
was measured at 13.32% of CPU time (perf profiling).

**Solution:** Each `RenderableInstance::Abstract` caches resolved programs locally:

```cpp
struct ResolvedProgram {
    ProgramCacheKey key;
    GraphicsPipeline * pipeline;
    PipelineLayout * layout;
};
StaticVector< ResolvedProgram, 16 > m_resolvedPrograms;
```

`resolveProgram(renderPassHandle, layerIndex)` checks the local cache first (linear scan of
up to 16 entries — cache-friendly). On miss, it falls back to the `Renderable`'s map (protected
by `std::shared_mutex` for concurrent reads) and caches the result locally.

**Cache invalidation:** `m_resolvedPrograms` is cleared on swap-chain recreation (render pass
handle changes) via `invalidateProgramCache()`.

**Measured impact:** Cache lookup -83% (from 13.32% to 2.20% of CPU time).

**Thread safety:** `Renderable::Abstract::m_programs` uses `std::shared_mutex` — shared lock
for reads (render thread), exclusive lock for writes (resource loading thread).

**Code references:**
- `RenderableInstance/Abstract.hpp` — `ResolvedProgram`, `m_resolvedPrograms`, `resolveProgram()`
- `RenderableInstance/Abstract.cpp` — `resolveProgram()` implementation, `invalidateProgramCache()`
- `Renderable/Abstract.hpp` — `m_programs`, `m_programsMutex` (`std::shared_mutex`)
- `Renderable/Abstract.cpp` — `findProgram()` (shared_lock read), `cacheProgram()` (unique_lock write)

## 15b. PostProcessStack Race Condition (Fixed Mar 2026)

> [!WARNING]
> **The render thread must NOT create the scene render target before the logic thread has
> set the PostProcessStack.** If created too early, the scene target uses wrong formats
> (no HDR, no depth/normals attachments) because `PostProcessor::requiresHDR()` etc. return
> false when the stack is null.
>
> **Fix:** Defer `SceneRenderTarget` creation until the PostProcessStack is non-null.
> The Renderer checks for a non-null stack before creating the scene target.
>
> **Code references:**
> - `Graphics/Renderer.cpp` — Deferred scene target creation
> - `Graphics/PostProcessor.cpp` — `requiresHDR()` aggregates chain needs

> [!WARNING]
> **Still true with the on-demand chain (Jul 2026), and it constrains WHEN an application
> may hand over a stack.** The render thread can now create an empty stack itself
> (`Scene::requirePostProcessStack()`), and `Scene::setPostProcessStack()` **destroys** the
> stack it replaces — so calling it on a scene that is already being rendered would tear
> down the camera effects under the render thread. Build the scene fully, THEN activate it:
> `Manager::newScene()` deliberately does not activate, which is what keeps the two writers
> apart. Never call `setPostProcessStack()` on the active scene.
>
> Ordering inside the frame is what makes the requirements correct: the scene target is
> created AFTER `syncCameraEffects()`, so a stack the camera just populated already reports
> `requiresHDR()` on the very frame it appears — no one-frame LDR flash.

## 15c-bis. GrabPass — the COLOR grab carries a mip chain, and only it (Aug 2026)

Frosted glass is a `textureLod()` away, provided the levels exist. `GrabPass::create()` gives the
**colour** image a full chain (`1 + floor(log2(max(w, h)))`), and `recordBlit()` generates it by
blitting the image onto itself, level N-1 to level N, right after the swap-chain copy into level 0.
The shader reads the LOD from the material roughness — see `src/Saphir/AGENTS.md` §
"Screen-space refraction".

⚠️ **Three things must agree or the blur is silently absent**, and each was a real trap:

1. The image needs **`VK_IMAGE_USAGE_TRANSFER_SRC_BIT`** on top of `TRANSFER_DST | SAMPLED` — the
   chain reads the image it writes.
2. The image **view** must expose every level (`levelCount = mipLevels`, it was 1).
3. The **sampler's `maxLod`** must reach them. It was pinned at `1.0F`, which would have clamped the
   blur to the first level even with a full chain present — and `Renderer::getSampler()` keys on the
   **NAME**, so the cache would have handed that stale sampler back. The name now carries the level
   count (`"GrabPass-<levels>"`). ⚠️ This is the same name-keyed-cache trap that once let a dummy
   texture win a shadow-map slot; check the key whenever a sampler's parameters depend on anything.

⚠️ **Layouts after the chain are NOT uniform.** Levels `[0, N-1)` end as `TRANSFER_SRC_OPTIMAL`
(they were each the source of the next blit) while only the last is still `TRANSFER_DST_OPTIMAL`.
The post-copy transition is therefore **two** barriers with `targetMipLevel(offset, count)`, not one
whole-image barrier — which would declare the wrong old layout for every level but one, and the
validation layers say so.

⚠️ **The depth / normals / material-properties grabs stay single-level on purpose.** They are read
as exact per-pixel values; a linearly filtered mip of a depth buffer is meaningless.

**Cost, measured by construction rather than profiled**: the chain adds ~1/3 of the base image in
extra writes, once per frame, and only on frames that already pay for the grab pass (it is armed by
`Scene::hasTranslucentGBObjects()`). ⚠️ Every level is generated even when every transmissive
material in the frame is perfectly smooth and reads only level 0 — closing that would require the
renderer to know the roughest grab-pass material of the frame. If this ever shows up in a profile,
that is the lever.

## 15c. GrabPass Destruction Safety (Fixed Mar 2026)

> [!WARNING]
> **`PostProcessor::configure()` must call `device->waitIdle()` before destroying the old
> GrabPass.** In-flight command buffers may still reference the old GrabPass's image/sampler.
> Destroying without waiting causes use-after-free Vulkan validation errors.
>
> **Code reference:** `Graphics/PostProcessor.cpp:configure()`

## 15b. Level of Detail (LOD)

### Settings

LOD behavior is controlled via `Core/Graphics/LOD/` settings in `SettingKeys.hpp`:

| Setting | Default | Purpose |
|---------|---------|---------|
| `EnableAutomaticGeneration` | `false` | Enable/disable automatic LOD mesh generation |
| `MinTriangleCount` | `250` | Minimum triangles a LOD level must produce to be generated |
| `ScreenCoverageThreshold` | `0.75` | Screen-space coverage ratio for LOD 0 → LOD 1 transition |
| `ReductionRatio` | `0.33` | Triangle reduction per LOD level (each level keeps ~33%) |

### LOD Generation Pipeline

When `EnableAutomaticGeneration = true`, LOD meshes are generated automatically in `onDependenciesLoaded()` for both `SimpleMeshResource` and `MeshResource`:

1. Check if source geometry is `IndexedVertexResource` with local data
2. Determine levels to generate based on triangle count and `MinTriangleCount`
3. Submit decimation tasks to engine `ThreadPool` (NOT `std::async`)
4. Each level uses `ShapeDecimator` (QEM) at `ratio^level` reduction
5. LOD 0 renders immediately — generation is non-blocking

### LOD Selection

`Scene::selectLODLevel(distance, objectRadius)` computes LOD from screen-space coverage:
```
screenSize = objectRadius / distance
LODLevel = clamp(MaxLODLevels × (1 - screenSize / threshold), 0, MaxLODLevels-1)
```

The threshold is read from `ScreenCoverageThreshold` at scene init (cached in `m_LODScreenCoverageThreshold`).

**Code references:**
- `Renderable/SimpleMeshResource.cpp:onDependenciesLoaded()` — LOD generation trigger
- `Renderable/MeshResource.cpp:onDependenciesLoaded()` — Same for multi-layer meshes
- `Scenes/Scene.rendering.cpp:selectLODLevel()` — Runtime LOD selection
- `Renderable/Types.hpp` — `MaxLODLevels` (4), legacy constants

## 16. Frame Synchronization — Double-Buffering

> [!CRITICAL]
> **Read [`src/Scenes/AGENTS.md` → Frame Synchronization](../Scenes/AGENTS.md) BEFORE adding
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

The logic/render double-buffer (`readStateIndex`/`writeStateIndex`) tracks **logic ticks**,
NOT rendered frames — if the logic thread ticks twice between two frames, "the other index"
is NOT the previous frame. Temporal effects (RTGI reprojection, future TAA) must use the
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
- `Renderer.hpp:m_currentReadStateIndex` — Double-buffer read state index for current frame
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
- `Saphir/VertexShader.cpp` — MDI paths in all synthesize/prepare methods
- `Renderer.hpp/.cpp` — `m_MDIBatchBuilder`, `m_MDIEnabled`, init in `onSetup()`

### Extension Registration Ordering (Critical)

> [!WARNING]
> **GLSL extensions used by BDA (`GL_EXT_buffer_reference`) must be registered in `enableMDI()`,
> NOT in `onSourceCodeGeneration()`.** The `generateHeaders()` method emits `#extension` directives
> BEFORE `onSourceCodeGeneration()` runs. Registering extensions in the latter causes the
> `PerDrawDataRef` struct to appear before the extension directive → compilation error.

## 18. Navigation

-   **Base Class**: `Renderable::Abstract`
-   **Main Entry**: `Renderer` (Central coordinator)
-   **Scene Bridge**: `Components::Visual`
-   **Shader Cache**: [`src/Saphir/AGENTS.md`](../Saphir/AGENTS.md) - 3-level cache system
-   **On-disk Caches**: See [Section 4](#persistent-on-disk-caches--the-renderer-owns-the-pipeline-cache-io) - `Renderer` does the `VkPipelineCache` disk I/O; SPIR-V binary cache ON by default
-   **Swap-Chain/VSync**: [`src/Vulkan/AGENTS.md`](../Vulkan/AGENTS.md) - Present mode selection
-   **Pattern Examples**: [`docs/development-patterns.md`](../../docs/development-patterns.md)
-   **Material JSON format**: See `docs/development-patterns.md#material-json-format-unified`
-   **Shadow Mapping**: [`docs/shadow-mapping.md`](../../docs/shadow-mapping.md) - PCF, global control, per-light settings
-   **Animated Cubemaps**: See [Section 11](#11-animated-texture-cubemap-system) - CubemapMovieResource + AnimatedTextureCubemap
-   **Post-Processing**: See [Section 12](#12-post-processing-effects) - RTR, SSR, ContactShadows, SSAO, Bloom, DoF, AtmosphericFog, VolumetricLight, LensFlare, ToneMapping
-   **Instance Program Cache**: See [Section 15](#15-instance-local-program-cache-renderableinstance) - Per-instance resolved program cache
-   **Frame Sync**: See [Section 16](#16-frame-synchronization--double-buffering) - Per-frame buffers, view matrix state index
-   **Compute Shaders**: See below - GPU compute pipeline for non-rendering workloads
-   **Raw Geometry**: See [Section 19](#19-raw-geometry-system) - Direct GPU upload from raw buffers

## 19. Raw Geometry System

Two geometry resource classes for loading raw vertex/index data directly to GPU without the `Shape<float>` intermediate.

### Classes

| Class | File | Description |
|---|---|---|
| `RawIndexedVertexResource` | `Graphics/Geometry/RawIndexedVertexResource.hpp` | VBO + IBO from raw data |
| `RawVertexResource` | `Graphics/Geometry/RawVertexResource.hpp` | VBO only from raw data |
| `RawGeometryOptions` | `Graphics/Geometry/RawGeometryOptions.hpp` | Shared options struct (topology, bounding box) |

### Design Principles

- **Zero staging**: No CPU-side copies. Data is uploaded to GPU directly inside `load()`.
- **Zero move**: `const` references/`std::span` — caller retains ownership of its data.
- **`std::span` API**: Accepts `std::vector`, `std::array`, C arrays, or raw `{pointer, size}` pairs.
- **`createOnHardware()` is a no-op**: Upload happens in `load()` via `serviceProvider().graphicsRenderer().transferManager()`. When `onDependenciesLoaded()` fires, `isCreated()` is already `true` — skips upload, builds BLAS.

### Two Load Modes

**1. Pre-interleaved** (caller already packed the vertex buffer):
```cpp
// vertexCount deduced from span.size() / vertexElementCount
res.load(vertexData, vertexElementCount, indices, options);
```

**2. Separate attributes** (engine interleaves for optimal GPU layout):
```cpp
// Strides: positions=3, normals=3, UV=2, colors=4 (RGBA)
// Geometry flags auto-set from non-empty spans
res.load(positions, indices, normals, uvs, colors, options);
```

### Resource Container Registration

Containers registered in `Resources/Manager.cpp` alongside existing geometry types:
- `RawVertexGeometries` — `Container<RawVertexResource>`
- `RawIndexedVertexGeometries` — `Container<RawIndexedVertexResource>`

### Key Constraint

**Must use `getOrCreateResourceSync()`** (not `getOrCreateResource()`) when the caller's data is on the stack. The async version dispatches to a thread pool — stack references would dangle.

## GPU Compute Pipeline (Graphics/Compute/)

### Infrastructure

The engine supports Vulkan compute shaders via:
- `Vulkan::ComputePipeline` — Compute pipeline with `setShaderModule()` for shader stage init
- `Vulkan::CommandBuffer::dispatch()` — Compute shader dispatch (vkCmdDispatch wrapper)
- `Saphir::ShaderManager::getShaderModuleFromSourceCode()` — Runtime GLSL→SPIRV compilation

### Graphics/Compute/XRayAnalyzer

GPU-accelerated volumetric cross-section scanner using Vulkan compute shaders:
- **GLSL compute shader**: 16×16 workgroups, Möller-Trumbore ray-triangle intersection
- **Spatial grid SSBO**: 128² cells with triangle index lookup (avoids brute-force 150K tests per pixel)
- **Bit-packed output**: 32 pixels per uint32 (32× memory reduction vs raw pixels)
- **Device-local output SSBO** + **host-cached staging buffer** for fast PCIe readback
- **Push constants**: Per-slice depth, resolution, grid parameters
- **Pipeline barriers**: compute→transfer→host for correct synchronization
- **Performance**: 46ms/slice at 16K×16K on RTX 3070 Ti (5.5 Gpixels/sec)

Architecture:
```
Triangles SSBO (binding 0) ─┐
Grid Cells SSBO (binding 2) ─┼─→ Compute Shader ─→ Output SSBO ─→ vkCmdCopyBuffer ─→ Staging Buffer ─→ memcpy ─→ CPU
Grid Indices SSBO (binding 3)┘                    (device-local)                      (host-cached)
```

Code references:
- `Graphics/Compute/XRayAnalyzer.hpp` — Public API (addShape, setViewpoint, prepare, scan, scanAll)
- `Graphics/Compute/XRayAnalyzer.cpp` — Vulkan pipeline setup, GLSL source, dispatch loop
- `Vulkan/ComputePipeline.hpp:setShaderModule()` — Shader stage initialization
- `Vulkan/CommandBuffer.hpp:dispatch()` — vkCmdDispatch wrapper
- `Vulkan/Buffer.hpp:setHostReadable()` — HOST_CACHED_BIT for fast CPU reads

### Graphics/Compute/IBLBaker + Graphics/IBLTexture (IBL lot 1, Jul 2026)

The image-based-lighting GPU bricks. `IBLTexture` (a `Vulkan::TextureInterface`) is an
**engine-baked, GPU-only texture**: no CPU pixel data, image created with
`STORAGE_BIT | SAMPLED_BIT`, always `RGBA16F` — the only 16F layout with **mandatory**
`STORAGE_IMAGE` support (`R16G16_SFLOAT` storage is an optional Vulkan feature; never rely
on it cross-platform). Three roles drive dimensions and sampler:

| Role | Image | Sampler (cache name) |
|------|-------|----------------------|
| `BRDFLut` | 2D 128², 1 mip | `IBLBrdfLut` — clamp-to-edge both axes (NdotV × roughness) |
| `IrradianceCubemap` | cube 32², 1 mip | `IBLIrradiance` — bilinear, single mip |
| `PrefilteredCubemap` | cube 128², 6 mips (128→4) | `IBLPrefiltered` — trilinear, `maxLod = VK_LOD_CLAMP_NONE` |

`IBLTexture::storageView(mip)` exposes per-mip **storage views** for compute `imageStore`
(cube roles → `2D_ARRAY` views, 6 layers = faces; LUT → plain 2D).

`Compute::IBLBaker` bakes the content. `generateBRDFLut()` (lot 1): split-sum LUT
(Karis 2013), 1024 Hammersley samples, Smith GGX with the **IBL k remap (`k = a²/2`)** —
never the analytic-light Disney remap. Reconstruction in shaders:
`specular = prefiltered * (F0 * lut.x + lut.y)`; the two channels also feed the
Fdez-Agüera multi-scatter compensation (lot 3) with no extra resource.

`bakeEnvironment(source, irradiance, prefiltered)` (lot 2): per-environment assets in ONE
blocking submission, re-baked at every sky change. Both passes use **filtered importance
sampling** (Křivánek & Colbert, GPU Gems 3 ch. 20): each sample reads the SOURCE mip whose
texel solid angle matches the sample solid angle — this is why environment cubemaps carry
their full mip chain, and why 64-512 samples/texel suffice. Details:
- Prefiltered: GGX importance sampling, N=V=R, cosθ weighting, roughness = mip/(mips−1),
  **mip 0 = direct copy** (roughness-0 shortcut), samples = 64 + 32·mip.
- Irradiance: cosine importance sampling, 512 samples, **+1 FIS mip bias** (without it the
  near-normal samples read the detailed source mips and a sun disc prints the fixed
  Hammersley sequence as a star-shaped artefact), stores **E/π** — ambient shading is then
  `albedo * texture(irradiance, N) * environmentLuminance`, matching the scalar path on a
  uniform sky.
- The baker works entirely in **cubemap space** (identity face mapping, `faceDirection()`);
  the world-to-cubemap Y negation stays a CONSUMER contract.
- Measured on the RTX 3070 Ti: ~1 ms uncontended (submit+wait, 1024² source); up to a few
  ms when the graphics queue is draining a frame — one-shot per sky change. Upgrade path
  for per-frame dynamic skies: fence-polled async submit (documented in the code).
- Debug: compile with `EMERAUDE_DEBUG_IBL_FACES` to dump every baked face as tonemapped
  PNGs to `/tmp/ibl-*.png`.

**Trigger & publication (Scenes side):** `Scene::updateEnvironmentIBL()` polls in
`processLogics` (same pattern as the background photometry poll): when the mutex-protected
`BindlessTextureSet::environmentCubemap()` identity changes (and is not the engine default),
it bakes into a **ping-pong pair** of scene-owned `IBLTexture` (frames in flight keep
sampling the published pair untouched), then publishes via
`BindlessTextureSet::setIrradianceCubemap()/setPrefilteredCubemap()` — mirrored to the
reserved slots by `syncTextureSet()` (UPDATE_AFTER_BIND hot-swap), parked on the default
cubemap by `clearTextureSet()` on scene switch.

**Baking runs on the GRAPHICS queue** (one-shot, `waitIdle` at boot): the images are later
sampled by fragment shaders on that same queue — using the compute queue would demand a
queue-family ownership transfer on an EXCLUSIVE image. Barrier sequence:
`UNDEFINED → GENERAL` (compute write) → dispatch → `GENERAL → SHADER_READ_ONLY_OPTIMAL`
(fragment read), via `Vulkan::Sync::ImageMemoryBarrier`.

Wiring: `Renderer::createDefaultResources()` creates + bakes the LUT once and publishes it
with `updateTexture2D(BindlessTextureManager::BRDFLutSlot, …)`; `Renderer::brdfLUT()`
exposes it for effects binding it through their own descriptor sets.

> [!CRITICAL]
> **Engine cubemap sampling convention (Y-UP, settled Aug 2026):** a world direction `D`
> samples any environment cubemap **RAW — `D` itself, no component negation.** Every
> sampling site obeys it: the skybox (`Material/Helpers.cpp` `checkPrimaryTextureCoordinates`),
> the material reflections (`StandardResource`), `LightGenerator`, SSR, RTGI, RTR, and the
> IBL generation (`IBLBaker`).
>
> ⚠️ **This REPLACES the Jul 2026 rule `vec3(D.x, -D.y, D.z)`**, which existed only because
> the world was Y-down (UP = -Y) while cubemaps are stored Y-up. The Y-up flip removed the
> reason; five of the six negations went with the flip and the sixth — the skybox display —
> followed once measured in the `coordinates-debug` scene. **A negation re-introduced anywhere
> now swaps the +Y/-Y faces and mirrors the four side faces vertically**; measured symptom:
> the magenta `Y-` face of `AxisDebug` at the zenith while the compass sphere there is green.
>
> **The hardware face convention is untouched and NOT negotiable**: the `(dx, dy, dz)` tables
> in `CubemapResource` / `CubemapMovieResource` and the frozen `IBLBaker.cpp:211-216` table
> are the standard Vulkan cube-face mapping. Consequence to know: that convention is
> LEFT-handed, so in this right-handed world **each face image displays mirrored horizontally**
> relative to the stored pixels. The equirectangular loader bakes that in (its
> `u = atan2(dz,dx)/2π + 0.5` output reads non-mirrored on screen); packed / per-face assets
> must therefore be authored the same way — as the standard skybox sets are.

### Environment cubemap mip chains (IBL lot 1, Jul 2026)

`TextureCubemap::createTexture()` now **always builds the full mip chain**
(`Image::getMIPLevels`), ignoring the global `Core/Graphics/Texture/MipMappingLevels`
setting (default 1 — which used to silently disable every cubemap mip in the engine), and
the shared `"Cubemap"` sampler uses `maxLod = VK_LOD_CLAMP_NONE`. Rationale: the IBL
prefiltering (filtered importance sampling) reads the source chain by solid-angle ratio,
and roughness-driven `textureLod()` (PBR transmission) needs real mip content. Memory cost:
+33% on a handful of cubemaps. The upload blit chain (`ImageTransferOperation::finalizeForGPU`,
per-layer × per-mip `vkCmdBlitImage`) is exercised for cubemaps since this change — LDR and
HDR (RGBA16F) validated visually (water-world BlueSky reference frame).

## Compressed image path (KTX2 / `KHR_texture_basisu`, Aug 2026)

### Two ways to reach BC7 — the source decides, not a setting

| | Source | CPU work at load | Disk cache | Uncompressed copy in RAM |
|---|---|---|---|---|
| **(a) Pixel path** | `ImageResource` (PNG, JPEG, procedural) | full BC7 encode (bc7enc, via `renderer.textureCache().getOrCompress()`) | yes (`Graphics::TextureCache`, `.bc7cache` on disk) | **yes** — RGBA8 level 0 |
| **(b) Compressed path** | `CompressedImageResource` (KTX2) | a block→block transcode | no, and none needed | **never** |

The colour space is picked by the **texture's** sRGB flag, never by the source container: path (a)
creates the image as `VK_FORMAT_BC7_SRGB_BLOCK` or `VK_FORMAT_BC7_UNORM_BLOCK`, path (b) runs the
decoded linear format through `KTX2Decoder::sRGBFormat()` — also BC7 for anything the transcoder
touched, see § "Classes" for the pass-through case.

`Texture2D::createTexture()` is now a dispatcher: `createFromCompressedData()` or
`createFromPixelData()`, then the shared image-view + sampler tail. Exactly one of `m_localData`
/ `m_compressedData` is non-null.

**The memory argument, in numbers.** A 4096×4096 texture costs ~22 MiB as BC7 *with its full mip
chain*, against ~89 MiB for the RGBA8 level 0 **alone** that path (a) must materialise before it
can compress anything. On the compressed Sponza (84 images, all 4096², all UASTC+zstd) that is the
difference between a KTX2 payload of 1092 MiB read straight through, and 84 successive 89 MiB
decodes each followed by a bc7enc pass.

### The two BC7 sub-services (Aug 2026) — `TextureCompressor` + `TextureCache`

Both classes used to be **a grouping of statics** — the shape the owner does not want in the engine,
after the earlier pass removing needless singleton logic. They are now real sub-services of
`Graphics::Renderer`:

| Class | ClassId | Member | Reached by |
|---|---|---|---|
| `Graphics::TextureCompressor` | `"TextureCompressorService"` | `Renderer::m_textureCompressor` | `renderer.textureCompressor()` — **const &** |
| `Graphics::TextureCache` | `"TextureCacheService"` | `Renderer::m_textureCache` | `renderer.textureCache()` — **const &** |

Both derive from `EmEn::ServiceInterface`, are **value members** of the `Renderer`, are enrolled in
`Renderer::initializeSubServices()` into `m_subServicesEnabled`, and are terminated in reverse order
with every other sub-service. Neither failing is fatal: the compressor failing means textures are
not BC7-compressed, the cache failing means they are compressed at every launch.

> [!CRITICAL]
> **Declaration order is a constraint, not a style choice.** `m_textureCache` is declared **AFTER**
> `m_textureCompressor` in `Renderer.hpp` and initialised **after** it in
> `initializeSubServices()`, because the cache holds `const TextureCompressor & m_compressor` and
> uses it on every miss. Swapping the two declarations — or moving `m_textureCache` above the
> compressor — binds a reference to a not-yet-constructed member.

**`getOrCompress()` is the entry point — callers no longer orchestrate anything.**

```cpp
/* Graphics/TextureResource/Texture2D.cpp — createFromPixelData() */
const auto compressedMips = renderer.textureCache().getOrCompress(this->name(), m_localData->data(), mipLevels);
```

`TextureCache::getOrCompress(resourceName, pixmap, maxMipLevels)` does the disk lookup, compresses
through the `TextureCompressor` sub-service on a miss, and stores the result. The old
try / compress / store dance at the call site is gone. Two call sites were migrated:
`Texture2D::createFromPixelData()` (the cached mip chain, above) and
`CompressedImageResource::load()`, which builds the **default** 64×64 payload — a single level, no
mip chain — and therefore reaches the compressor directly
(`serviceProvider().graphicsRenderer().textureCompressor().compressSingle(pixmap)`), with no cache,
by design. That call is the ONE bc7enc pass on the compressed side and it encodes a procedural
fallback, not an asset: row (b) of the table above still holds for every real KTX2.

**All mutable static state is gone.** `TextureCompressor::s_initialized`, plus
`TextureCache::s_cacheDirectory` and `s_initialized`, no longer exist. The one-time
`bc7enc_compress_block_init()` moved into `TextureCompressor::onInitialize()`, so a caller can no
longer reach a compression method before the encoder is ready — the old static `initialize()` the
caller had to remember to invoke is **DELETED**, and forgetting it used to produce only a runtime
error log. The cache's directory is the `m_cacheDirectory` member and its readiness is the base
class `usable()` state. The pure private helpers `generateMip()` and `compressLevel()` moved to an
anonymous namespace in `TextureCompressor.cpp`. What remains static in the headers is
`static constexpr` constants plus two functions that are pure on their arguments and hold nothing:
`TextureCompressor::compressedSize()` (block arithmetic) and the private `TextureCache::cacheKey()`
(the content hash).

> [!WARNING]
> **The thread-pool parameter was dead and has been removed.** `compressLevel()` received a
> `Base::ThreadPool` and never used it; `compress()` and `compressSingle()` no longer take one.
> BC7 compression is **sequential per texture** — the parallelism comes from the resource manager
> loading several textures concurrently on different workers. Any document claiming compression is
> "parallelized across blocks using the engine ThreadPool" is **FALSE**; correct it where you find it.

#### The cache key was broken and is fixed (file format Version 1 → 2)

- **BEFORE:** `SHA256(resourceName | sourceFileSize | sourceModTime)`. But the caller passed, as
  `sourceFileSize`, the **decoded pixel byte count**, and as `sourceModTime`,
  `width * 1000000 + height`. The key therefore reduced to **name + dimensions**: repainting a
  texture without changing its size served the stale BC7 blob forever. The class documentation
  claimed file size and modification time — it described a mechanism that was not there.
- **AFTER:** **FNV-1a** (`Base::Hash::FNV1a`) over the **decoded pixels**, folded with `width`,
  `height` and `colorCount`. Content-addressed, correct by construction, and it needs no plumbing
  through `ResourceTrait`.

> [!CAUTION]
> **Changing the key scheme ORPHANS entries, it does not invalidate them.** Their filenames stop
> being produced, so they stay on disk unreachable rather than being detected as stale — no header
> check can catch what is never opened. `--clear-renderer-cache` is the remedy (**40** stale entries
> erased here when the key changed). `Version` stays **1**: the file FORMAT did not change, only the
> key, and a version number that moves for other reasons stops meaning anything.

#### Measured (`material-debug`, all 10 options, RTX 3070 Ti, Release)

| Run | BC7 compressions | Time spent compressing |
|---|---|---|
| Cold cache | **231** mip levels | **7 705 ms** |
| Warm cache | **0** | **0 ms** |

So the texture cache is worth **~7.7 s of load time** — more than the `VkPipelineCache`
(5 702 ms → 31 ms) and about twenty times the SPIR-V binary cache (393 ms → 10.3 ms). **Zero**
compressions on the warm run is also the proof that the content-addressed key is deterministic:
every texture found its entry. Build `-Werror` clean, 1967/1967 emeraude-base unit tests pass.

**On disk:** `~/.cache/<app>/texture-cache/`, extension `.bc7cache`
(`TextureCache::CacheDirectoryName` / `CacheFileExtension`).

> [!WARNING]
> Several documents claim `~/.cache/AppName/TextureCache/`. That path is **wrong and has always
> been wrong** — the code has always used a `texture-cache` sub-directory. Fix it where you see it.

### Classes

- **`Graphics::KTX2Decoder`** — stateless. `isKTX2()` (magic-number probe), `decodeCompressed()`
  → `{mips, VkFormat}`, `decodeToPixmap()` (the no-BC-support fallback), `sRGBFormat()`.
  Transcode target is **BC7**, because that is the one block format the engine supports. A KTX2
  that already carries a real `vkFormat` (nothing to transcode) is passed through untouched.
- **`Graphics::CompressedImageResource`** — an **opaque GPU payload**, deliberately. No
  `averageColor()`, no `isGrayScale()`, no per-pixel access, no `flipNormalMapY()`: answering any
  of them means decoding the blocks, which is exactly what the type exists to avoid. Code that
  needs to *inspect* pixels wants `ImageResource`.

> [!CAUTION]
> **The stored format is always the LINEAR variant, and that is load-bearing.** libktx derives the
> transcoded `vkFormat` from the container's transfer function, so an sRGB-tagged asset comes back
> as `*_SRGB_BLOCK`. The decoder normalises it back to linear, because the colour space is the
> **texture's** call — it knows the usage (albedo and emissive are sRGB, normal and ORM maps are
> not), the container does not. The blocks are bit-identical either way. Taking the container's
> word for it double-applies the sRGB curve on every ORM and normal map of the asset.

> [!WARNING]
> `isGrayScale()` and `averageColor()` **must** null-check `m_localData`, not just `isLoaded()` —
> a texture on the compressed path is fully loaded *and* has no pixmap.

### `Core/Graphics/Texture/MaxDimension` (default 4096)

Largest accepted mip dimension; 0 disables clamping. **Only honored by sources that ship a
ready-made mip chain** (i.e. KTX2), where clamping is free: the top levels are simply not kept,
nothing is resampled. Every halving divides the VRAM footprint by four — Sponza's 84 textures cost
~1.88 GiB at 4096, ~470 MiB at 2048, ~118 MiB at 1024. The default changes no behaviour; the knob
exists for the 8 GiB machine.

### Block formats and `Image::pixelBytes()` / `colorCount()`

Block formats are **absent** from both tables, which return 0 for them. That is not a latent bug
on this path: `Image::createFromCompressed()` never consults them — it is handed explicit per-level
byte counts. Do not "fix" the tables by giving a block format a per-pixel size; the honest answer
for BC7 is 1 byte per pixel *amortised over a 4×4 block*, which is not what those accessors mean.
