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

| Slot | Constant | Purpose |
|------|----------|---------|
| 0 | `EnvironmentCubemapSlot` | Scene environment cubemap |
| 1 | `IrradianceCubemapSlot` | IBL irradiance map |
| 2 | `PrefilteredCubemapSlot` | IBL prefiltered environment |
| 3 | `BRDFLutSlot` | BRDF lookup texture |
| 16+ | `FirstDynamicSlot` | Dynamic texture allocation |

### Usage

**Registering a texture:**
```cpp
uint32_t index = bindlessManager.registerTexture2D(texture);
// Use 'index' in shader to access the texture
```

**Updating reserved slots:**
```cpp
bindlessManager.updateTextureCube(BindlessTexturesManager::EnvironmentCubemapSlot, envMap);
```

**In GLSL shaders:**
```glsl
layout(set = BINDLESS_SET, binding = 1) uniform sampler2D textures2D[];

// Access with non-uniform index
vec4 color = texture(textures2D[nonuniformEXT(textureIndex)], uv);
```

### Color Projection via Bindless

Light color projection textures are registered in the bindless arrays during `createOnHardware()` or asynchronously via `ObserverTrait` notification when resource loading completes. Each light UBO carries a `ColorProjectionIndex` field (`uint` encoded as `bit_cast<float>`) that indexes into the bindless 2D or Cube array.

- **2D lights** (directional, spot): use `registerTexture2D()` → `sampler2D` array at binding 1
- **Point lights**: use `registerTextureCube()` → `samplerCube` array at binding 3
- **Sentinel value**: `UINT32_MAX` means no color projection texture assigned

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

**Algorithm:** Inverted Voronoi F2-F1 distance (bright at cell edges = caustic lines, dark at cell centers). Temporal animation uses a circular sin/cos path through noise space for seamless looping. See: `Libs/Algorithms/VoronoiNoise.hpp` for the underlying noise.

**Usage pattern:** See `projet-alpha/src/Builtin/PoolRooms.cpp:onSetupLighting()`.

### Code References

- `Graphics/CubemapMovieResource.hpp/cpp` — CPU frame storage, JSON loading, procedural caustics
- `Graphics/TextureResource/AnimatedTextureCubemap.hpp/cpp` — Vulkan cube array texture resource
- `Libs/Algorithms/VoronoiNoise.hpp` — Voronoi noise: `evaluate()`, `caustic()` (F2-F1 clamped)
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
| **DepthOfField** | `Effects/Framebuffer/DepthOfField.hpp/cpp` | Multi-pass | Depth |
| **ToneMapping** | `Effects/Framebuffer/ToneMapping.hpp/cpp` | 1-pass | HDR |
| **VolumetricLight** | `Effects/Framebuffer/VolumetricLight.hpp/cpp` | Multi-pass | Depth, HDR |
| **AtmosphericFog** | `Effects/Framebuffer/AtmosphericFog.hpp/cpp` | 1-pass | Depth, HDR |
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
- `PostProcessor.hpp/cpp` — Chain management, push constants

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

### Effect Chain Order

Recommended order (used in LightAndShadowDebug):
```
SSR → SSAO → AtmosphericFog → VolumetricLight → Bloom → DoF → ToneMapping (always last: HDR→LDR)
```

**Rationale:** SSR first so it samples clean scene color (no SSAO noise amplified in reflections). SSAO then darkens the image globally including reflections, which is acceptable — this matches UE4's approach where AO is applied as a global multiplier after reflection composition. AtmosphericFog before VolumetricLight so god rays bloom through the fog. Bloom before DoF extracts bright pixels from sharp image (avoids runaway glow from DoF blur spreading HDR values).

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

## 15. Navigation

-   **Base Class**: `Renderable::Abstract`
-   **Main Entry**: `Renderer` (Central coordinator)
-   **Scene Bridge**: `Components::Visual`
-   **Shader Cache**: [`src/Saphir/AGENTS.md`](../Saphir/AGENTS.md) - 3-level cache system
-   **Swap-Chain/VSync**: [`src/Vulkan/AGENTS.md`](../Vulkan/AGENTS.md) - Present mode selection
-   **Pattern Examples**: [`docs/development-patterns.md`](../../docs/development-patterns.md)
-   **Material JSON format**: See `docs/development-patterns.md#material-json-format-unified`
-   **Shadow Mapping**: [`docs/shadow-mapping.md`](../../docs/shadow-mapping.md) - PCF, global control, per-light settings
-   **Animated Cubemaps**: See [Section 11](#11-animated-texture-cubemap-system) - CubemapMovieResource + AnimatedTextureCubemap
-   **Post-Processing**: See [Section 12](#12-post-processing-effects) - SSR, Bloom, SSAO, DoF, AtmosphericFog, VolumetricLight, ToneMapping
