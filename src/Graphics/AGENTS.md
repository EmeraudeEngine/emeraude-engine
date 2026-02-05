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

## 9. Shadow Mapping Global Control

The `Renderer` provides a global shadow mapping enable/disable via `isShadowMapsEnabled()`.

**Setting key:** `GraphicsShadowMappingEnabledKey` (`Core/Graphics/Renderer/ShadowMappingEnabled`)

**Implementation:**
```cpp
bool Renderer::isShadowMapsEnabled() const noexcept
{
    return m_shadowMapsEnabled;  // Cached from settings
}
```

**Integration with Scene:**
The Scene checks this setting when selecting render pass types for lighting. When disabled, all lights use `NoShadow` pass types to avoid binding shadow map descriptors.

**Why this matters:**
Without this check, disabling shadow mapping via settings caused Vulkan validation errors because shadow map images remained in `VK_IMAGE_LAYOUT_UNDEFINED` but descriptor sets still tried to bind them.

**Code references:**
- `Renderer.hpp:isShadowMapsEnabled()` - Global accessor
- `Scenes/Scene.rendering.cpp:978` - Scene-side check
- `SettingKeys.hpp:GraphicsShadowMappingEnabledKey` - Setting key

See [`docs/shadow-mapping.md`](../../docs/shadow-mapping.md) for complete shadow mapping architecture.

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

### Compile-Time Guard
- Enabled via `EMERAUDE_VIDEO_RECORDING_ENABLED` (requires libvpx)
- When disabled, all methods are no-op stubs returning false

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

## 11. Navigation

-   **Base Class**: `Renderable::Abstract`
-   **Main Entry**: `Renderer` (Central coordinator)
-   **Scene Bridge**: `Components::Visual`
-   **Shader Cache**: [`src/Saphir/AGENTS.md`](../Saphir/AGENTS.md) - 3-level cache system
-   **Swap-Chain/VSync**: [`src/Vulkan/AGENTS.md`](../Vulkan/AGENTS.md) - Present mode selection
-   **Pattern Examples**: [`docs/development-patterns.md`](../../docs/development-patterns.md)
-   **Material JSON format**: See `docs/development-patterns.md#material-json-format-unified`
-   **Shadow Mapping**: [`docs/shadow-mapping.md`](../../docs/shadow-mapping.md) - PCF, global control, per-light settings
