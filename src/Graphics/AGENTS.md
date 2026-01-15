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

`StandardResource` stores properties in a float array with these offsets:

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

The GLSL struct is generated to match this layout exactly.

## 6. Frame Rate Limiter

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

## 6. Navigation

-   **Base Class**: `Renderable::Abstract`
-   **Main Entry**: `Renderer` (Central coordinator)
-   **Scene Bridge**: `Components::Visual`
-   **Shader Cache**: [`src/Saphir/AGENTS.md`](../Saphir/AGENTS.md) - 3-level cache system
-   **Swap-Chain/VSync**: [`src/Vulkan/AGENTS.md`](../Vulkan/AGENTS.md) - Present mode selection
-   **Pattern Examples**: [`docs/development-patterns.md`](../../docs/development-patterns.md)
