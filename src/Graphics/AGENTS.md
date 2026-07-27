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
2.  **Y-DOWN**: Strictly Y-down coordinate system.
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
| Samplers | `m_samplers` | Sampler properties | Texture sampler cache |

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
- `m_materialProperties[]`: Float array with material data (ambientColor, diffuseColor, etc.)

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

**StandardResource** stores properties in a float array with these offsets:

| Offset | Property | Type |
|--------|----------|------|
| 0-3 | ambientColor | vec4 |
| 4-7 | diffuseColor | vec4 |
| 8-11 | specularColor | vec4 |
| 12-15 | autoIlluminationColor | vec4 |
| 16 | shininess | float |
| 17 | opacity | float |
| 18 | autoIlluminationAmount | float |
| 19 | normalScale | float |
| 20 | reflectionAmount | float |
| 21 | refractionAmount | float |
| 22 | refractionIOR | float |
| 23 | heightScale | float | 0.0+ (0.02) — POM depth |

**PBRResource** stores properties in a 52-float array (208 bytes, std140):

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
| 36 | attenuationDistance | float | 0.0+ |
| 37 | thicknessFactor | float | 0.0+ |
| 38 | heightScale | float | 0.0+ (0.02) — POM depth |
| 39 | iridescenceFactor | float | 0.0-1.0 (0.0) |
| 40 | iridescenceIOR | float | 1.0+ (1.3) |
| 41 | iridescenceThicknessMin | float | nm (100.0) |
| 42 | iridescenceThicknessMax | float | nm (400.0) |
| 43 | dispersion | float | 0.0+ (0.0) |
| 44-47 | specularColorFactor | vec4 | KHR specular color (white) |
| 48 | emissiveStrength | float | 0.0+ (1.0) — HDR multiplier |
| 49 | clearCoatNormalScale | float | 0.0+ (1.0) — CC normal map intensity |
| 50-51 | padding | float | std140 alignment |

The GLSL struct is generated to match this layout exactly.

### Material Opacity and GrabPass

`Material::Interface` provides two key query methods used by the rendering pipeline for render list dispatch:

- **`isOpaque()`**: Returns `!BlendingEnabled`, but also returns `false` when `requiresGrabPass()` is `true` (a material requiring grab pass is inherently non-opaque).
- **`requiresGrabPass()`**: Virtual method (default `false`). Overridden by `PBRResource` based on material properties (e.g., transmission with screen-space refraction).

These are propagated through `Renderable::Abstract::isOpaque(layerIndex)` and `Renderable::Abstract::requiresGrabPass(layerIndex)` to all concrete renderables, enabling the Scene to dispatch into 3 render categories: Opaque, Translucent, and TranslucentGB.

**Code references:**
- `Material/Interface.hpp:isOpaque()` — non-virtual, checks blending and grab pass
- `Material/Interface.hpp:requiresGrabPass()` — virtual, default false
- `Material/PBRResource.hpp:requiresGrabPass()` — override
- `Renderable/Abstract.hpp:requiresGrabPass()` — pure virtual

### Normal Map Scale

The `normalScale` parameter (offset 19 for Standard, offset 6 for PBR) controls normal map intensity by scaling the tangent-space XY components before re-normalizing:

```glsl
vec3 raw = texture(normalSampler, uv).rgb * 2.0 - 1.0;
vec3 normal = normalize(vec3(raw.xy * ubMaterial.normalScale, raw.z));
```

- `1.0` = full normal map effect (default)
- `0.5` = half intensity (smoother bumps)
- `0.0` = flat surface (normal map ignored)

**Code references:** `StandardResource.cpp:generateFragmentShaderCode()`, `PBRResource.cpp:generateFragmentShaderCode()`

### Parallax Occlusion Mapping (POM)

POM ray-marches through a height map in the fragment shader to create depth/relief illusion on flat surfaces without extra geometry. Uses `ComponentType::Displacement` with height map textures.

**Activation conditions** (all must be true):
1. Material has a Height component (`m_useParallaxOcclusionMapping`)
2. High quality enabled (`EnableHighQualityKey = true`)
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
- `StandardResource.cpp:generateFragmentShaderCode()` — POM GLSL generation
- `PBRResource.cpp:generateFragmentShaderCode()` — POM GLSL generation (+ distance fade)
- `StandardResource.cpp:textCoords()` — UV variable selection
- `PBRResource.cpp:textCoords()` — UV variable selection
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
> **`"Shininess"` in a manifest is a GLOSSINESS in [0,1] — the C++ API takes an EXPONENT.**
> The whole data store was authored as a perceptual glossiness (3834 material files of 3917 hold
> `0.1`), while the shader uniform and every setter carry a real Blinn-Phong exponent. The conversion
> happens at the parse boundary ONLY, in the two specular parse sites of
> `StandardResource::parseSpecularComponent()`:
>
> ```
> exponent = StandardResource::specularExponentFromGlossiness(gloss)   // exp2(1 + 10 * gloss)
> // 0.0 -> 2 | 0.1 -> 4 | 0.2 -> 8 | 0.4 -> 32 | 0.5 -> 45 | 0.9 -> 1024 | 1.0 -> 2048
> ```
>
> **Never apply it anywhere else.** `DefaultShininess` is `32`, `MaxPBRShininess` is `128`, and
> `setRoughness()` reaches `setShininess()` through `pow(1 - roughness, 2) * 128` — all exponents.
> Remapping one of those would produce `exp2(321)`. The absent-key fallback is `DefaultGlossiness{0.4F}`
> for the same reason: it maps back to the historical 32.
>
> A value coming out of JSON is a glossiness; a value held by the resource or reaching the shader is an
> exponent. See `docs/caution-points.md`, "The legacy specular was not energy-normalised, and
> `Shininess` was authored as a glossiness".

**Code references:**
- `Graphics/Material/Helpers.cpp:parseComponentBase()` - Base parsing
- `Graphics/Material/StandardResource.cpp:parseReflectionComponent()` - Automatic handling
- `Graphics/Material/PBRResource.cpp:parseReflectionComponent()` - PBR variant

### Material Types Array

> [!CRITICAL]
> **All material resource types must be registered in `Material::Types`!**
>
> `Materials.hpp` defines the valid material types for JSON validation:
> ```cpp
> constexpr auto Types = std::array< std::string_view, 3 >{
>     BasicResource::ClassId,      // "MaterialBasicResource"
>     StandardResource::ClassId,   // "MaterialStandardResource"
>     PBRResource::ClassId         // "MaterialPBRResource"
> };
> ```
>
> Missing types cause silent fallback to `BasicResource` during mesh loading.

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

## 10. Video Recording (Graphics::Recorder)

Real-time video recording service that captures the Vulkan swap-chain framebuffer and encodes VP8/IVF.

### Pipeline
1. **GPU async readback** (4-slot round-robin) — copies swap-chain image to host-visible staging buffer
2. **Unbounded frame queue** — accumulates BGRA frames for encoding thread
3. **Dedicated encoding thread** — BGRA→I420 conversion (SIMD dispatched: scalar/SSSE3/AVX2), VP8 encoding, IVF container writing

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
- `Recorder.cpp:startRecording()` — VP8 init, async resource creation, thread start
- `Recorder.cpp:encodingThreadFunc()` — BGRA→I420 + VP8 encode loop
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
removed it (see `TODO.md` § "Photometric lighting"), the generated falloff is the clean
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
| **VolumetricLight** | `Effects/Framebuffer/VolumetricLight.hpp/cpp` | Multi-pass | Depth, HDR |
| **AtmosphericFog** | `Effects/Framebuffer/AtmosphericFog.hpp/cpp` | 1-pass | Depth, HDR |
| **RTR** | `Effects/Framebuffer/RTR.hpp/cpp` | 4-pass (Trace→BlurH→BlurV→Composite) | Depth, Normals, RT (TLAS+SSBOs) |
| **RTGI** | `Effects/Framebuffer/RTGI.hpp/cpp` | 6-pass (Trace→BlurH→BlurV→Temporal→NormalHistory→Apply) | Depth, Normals, MaterialProps, Albedo, RT (TLAS+SSBOs) |
| **RTAO** | `Effects/Framebuffer/RTAO.hpp/cpp` | Multi-pass | Depth, Normals, RT (TLAS+SSBOs) |
| **SSGI** | `Effects/Framebuffer/SSGI.hpp/cpp` | Multi-pass | Depth, Normals, MaterialProps, Albedo |
| **ContactShadows** | `Effects/Framebuffer/ContactShadows.hpp/cpp` | Multi-pass | Depth, Normals |
| **LensFlare** | `Effects/Framebuffer/LensFlare.hpp/cpp` | Multi-pass | Depth, HDR |
| **FogEnvironment** | `Effects/Framebuffer/FogEnvironment.hpp/cpp` | 1-pass | Depth |

### SSR (Screen-Space Reflections)

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
- Resolve descriptor set: 5 bindings (color, trace, depth, normals, envCubemap)

**Code references:**
- `Effects/Framebuffer/SSR.hpp` — Parameters, ResolvePushConstants, setEnvironmentCubemap()
- `Effects/Framebuffer/SSR.cpp` — Shader source, descriptor layouts ("SSRResolveInput"), pipeline creation
- `PostProcessEffect.hpp` — Base interface
- `PostProcessor.hpp/cpp` — Chain management, push constants. `configure()` retires its previous grab pass + per-frame descriptor sets through `Renderer::deferredDestructor()` (frames-in-flight safety, no mid-frame `waitIdle`) — see `src/Vulkan/AGENTS.md`, "Deferred destruction contract"

### AtmosphericFog (Exponential Height Fog)

Single-pass analytical fog using closed-form integral (no iterative sampling). Reads depth buffer to reconstruct world-space positions, applies exponential height fog with directional inscattering.

**Algorithm:**
1. Reconstruct world position from depth + camera basis vectors (push constants)
2. Exponential height fog integral: `ρ(y) = density * exp(k * (y - baseHeight))` along the view ray
3. Directional inscattering (simplified Henyey-Greenstein): bright halo when looking toward the sun
4. Sky fog option: when `skyFogEnabled = true`, fog covers skybox pixels using `maxDistance` as fictive distance

**Push constants** (116 bytes): Camera basis (pos, right, forward), depth reconstruction (near, far, tanHalfFovY, aspectRatio), fog params (density, heightFalloff, baseHeight, maxDistance, color), inscatter params (lightDir, exponent, color, intensity), skyFogEnabled.

**Y-DOWN convention:** In Y-DOWN, `+Y = deeper into fog`. The height falloff density function increases with Y. See `docs/caution-points.md` for the critical Y-reconstruction pitfall.

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
- Set 1: Input textures — depth (binding 0), normals (binding 1)
- Set 2: Bindless textures from `BindlessTextureManager` — sampler2D[] (binding 1)

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

### RTGI (Ray-Traced Global Illumination) — Temporal + Multi-Bounce (Jul 2026)

6-pass pipeline: one traced diffuse bounce per frame, temporally accumulated, with a
multi-bounce feedback loop through the history buffer.

1. **Trace** (half-res): cosine-weighted hemisphere rays via TLAS ray queries; at each hit,
   direct lighting (with shadow rays gated on the raster shadow-casting flag) PLUS the hit
   surface's accumulated indirect radiance from the previous resolved frame (multi-bounce
   feedback). Receiver albedo read from the **albedo G-buffer** (no primary ray).
2/3. **Blur H/V** (half-res): bilateral, depth+normal edge-stopping.
4. **Temporal resolve** (half-res): reprojects the pixel's world position through the
   PREVIOUS frame's view-projection, validates history (camera-distance in history alpha +
   world-normal history), optional 3x3 neighborhood clamp, then EMA (`Temporal/Alpha`).
   Output → history ping-pong `[writeIdx]`, also consumed by the apply pass.
5. **Normal history** (half-res): current view-space normals → world space, retained for
   the next frame's validation (the normals MRT is rewritten every frame).
6. **Apply** (full-res): additive blend, emissive-masked via material properties G-buffer.

**Frame UBO instead of push constants:** the trace parameters (invViewProj + prevViewProj +
camera data) exceed the **128-byte Vulkan push constant minimum guarantee**
(`maxPushConstantsSize`). A per-frame UBO (`FrameUBOData`, std140) is shared by the
trace/temporal/normal-history passes — created via
`IndirectPostProcessEffect::createPerFrameUniformBuffers()`, bound through
`getInputLayout(samplerCount, uniformBufferCount)` (samplers first, then UBOs).

**Multi-bounce energy algebra:** the history stores OUTGOING indirect radiance (receiver
albedo applied at trace time), so the feedback is NOT re-multiplied by the hit albedo —
the geometric series `1/(1-albedo*strength)` is naturally damped by physical albedo (< 1)
and converges. `MultiBounce/Clamp` bounds the re-injected radiance (anti-firefly).
`MultiBounce/Strength` is a continuous bounce-depth dial: 0 = single bounce, 1 = full series.

**History ping-pong correctness:** 2 half-res RGBA16F history targets (+2 normal history).
Frame N reads `[1-w]`, writes `[w]`; safe on a single queue thanks to the IRT's full
(non-by-region) external subpass dependencies. `m_historyValid` forces alpha=1 and
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
- `Effects/Framebuffer/RTGI.hpp` — Parameters, `FrameUBOData` (std140), history members
- `Effects/Framebuffer/RTGI.cpp` — 5 GLSL shaders (inline), ping-pong recording
- `Graphics/ViewMatricesInterface.hpp` — frame-history contract (previous view/projection)

### Physical Camera — Camera-Driven Photographic Pipeline (Jul 2026)

The `Scenes::Component::Camera` is the **single source of truth for the photographic
behaviour** of the rendered image, like a real camera body. Owner vision: ultimately ALL
image-rendering effects are camera-manageable (lens effects already are).

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
  directional lights from — `Direction` points TOWARD the body (engine frame, UP = -Y),
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

Recommended scene-effect order (used in the demos):
```
RTR → SSR → ContactShadows → SSAO → AtmosphericFog → VolumetricLight → LensFlare → Bloom
[camera: DoF → ToneMapping] [LDR: FXAASharpen]
```

**Rationale:** RTR first (hardware ray tracing, highest quality reflections). SSR as fallback where RTR is unavailable. ContactShadows adds fine-detail shadowing from depth. SSAO then darkens the image globally including reflections, which is acceptable — this matches UE4's approach where AO is applied as a global multiplier after reflection composition. AtmosphericFog before VolumetricLight so god rays bloom through the fog. LensFlare from bright light sources. Bloom before DoF extracts bright pixels from sharp image (avoids runaway glow from DoF blur spreading HDR values).

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

`IntermediateRenderTarget::create()` gives every target `COLOR_ATTACHMENT_BIT | SAMPLED_BIT` —
enough to render into it and sample it, which is what almost every effect wants. Anything beyond
that must be requested through the trailing `extraUsageFlags` parameter.

> [!WARNING]
> **An image can only be TRANSITIONED to a layout its usage flags support.** If your effect reads
> a target back with `vkCmdCopyImageToBuffer`, it MUST be created with
> `VK_IMAGE_USAGE_TRANSFER_SRC_BIT`, or the barrier to `TRANSFER_SRC_OPTIMAL` is silently
> rejected and every later command runs against a **stale tracked layout** — the failure surfaces
> as four unrelated-looking VUIDs pointing at the copy, not at the creation. `ToneMapping`'s
> auto-exposure adaptation targets are the reference case; see
> `docs/caution-points.md` § Vulkan Validation.

```cpp
m_adaptTargets[index].create(renderer, 1, 1, lumFormat, "AdaptLum0", VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
```

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

**Known limitation**: StandardResource materials without lighting enabled may not render correctly in the Opaque (non-lighted) list. This is a material/demo configuration issue, not an MDI bug.

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
> **Engine cubemap sampling convention (settled Jul 2026):** a world direction `D` samples
> any environment cubemap at **`vec3(D.x, -D.y, D.z)`** — the engine world is Y-down
> (UP = -Y) while cubemaps are stored Y-up. Reference sites: the skybox
> (`Material/Helpers.cpp` `checkPrimaryTextureCoordinates`) and the material reflections
> (`PBRResource`/`StandardResource`), both validated visually (celestial servoing within 1°).
> RTGI/RTR/SSR sampled the RAW direction (sky upside-down in GI bounces and ray-miss
> reflections) — fixed in lot 1. **The IBL generation (lot 2) MUST produce and consume
> cubemaps under this same convention.**

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
