## Saphir-Specific Rules

### Generation Philosophy
- **Parametric generation**: Shaders created from unknowns (material + geometry + scene)
- **STRICT compatibility checking**: Material requirements MUST match geometry attributes
- **Graceful failure**: If incompatible → resource loading fails → application continues
- **Aggressive caching**: 3-level cache avoids redundant generation and compilation

### 3-Level Cache System (IN-MEMORY, one process run)

⚠️ These three levels are process-lifetime hash maps. They are **not** the three ON-DISK cache
stages (source dump / SPIR-V binary cache / `VkPipelineCache`) — see
"The three shader-cache stages" at the end of this file.

| Level | Object | Location | Cache Key | Benefit |
|-------|--------|----------|-----------|---------|
| 1 | `ShaderModule` | `ShaderManager` | Source code hash | Reuse compiled SPIR-V between programs |
| 2 | `Program` | `Renderer::m_programs` | `computeProgramCacheKey()` | Skip entire shader generation |
| 3 | `GraphicsPipeline` | `Renderer::m_graphicsPipelines` | `getHash(renderPass)` | Reuse Vulkan pipeline objects |

**Program cache** (Level 2) provides the biggest gain - it short-circuits before any shader code generation when an identical configuration exists. See: `Generator::Abstract::generateShaderProgram()`

```
RenderableInstance created
    │
    ▼
computeProgramCacheKey() ──► Cache hit? ──► Return cached Program (skip all generation)
    │                              │
    │ miss                         │ hit (1482 reuses typical)
    ▼                              │
Generate GLSL code                 │
Compile ShaderModules              │
Create Program                     │
Cache Program ◄────────────────────┘
```

> [!CRITICAL]
> **computeProgramCacheKey() MUST include renderPassHandle!**
>
> Each generator's `computeProgramCacheKey()` MUST include the render pass handle as the FIRST
> hash component. This is required because Vulkan pipelines are tied to specific render passes.
>
> Without renderPassHandle, pipelines created for offscreen rendering (1 sample) would be
> incorrectly reused for main view rendering (4 samples), causing validation errors.

### computeProgramCacheKey() Requirements

Every generator must implement `computeProgramCacheKey()` with these MANDATORY components:

```cpp
size_t MyGenerator::computeProgramCacheKey () const noexcept
{
    size_t hash = Hash::FNV1a(ClassId);  // Generator type identifier

    // 1. MANDATORY: Render pass handle (pipeline compatibility)
    if ( const auto * framebuffer = this->renderTarget()->framebuffer(); framebuffer != nullptr )
    {
        hashCombine(hash, reinterpret_cast< size_t >(framebuffer->renderPass()->handle()));
    }

    // 2. MANDATORY: Render target type (cubemap vs single layer)
    hashCombine(hash, static_cast< size_t >(this->renderTarget()->isCubemap()));

    // 3. Generator-specific parameters...
    // (renderable name, layer index, flags, etc.)

    return hash;
}
```

**Required includes** for accessing render pass handle:
```cpp
#include "Vulkan/Framebuffer.hpp"
#include "Vulkan/RenderPass.hpp"
```

### Generator Types
1. **SceneGenerator**: 3D objects with full lighting, materials, effects
2. **OverlayRendering**: 2D elements (UI, HUD, text, sprites) - generates 4 program variants
3. **ShadowManager**: Minimal shaders for shadow map generation
4. **GizmoRendering**: Editor gizmos (vertex color, no lighting, depth test OFF). See `Scenes/Editor/AGENTS.md`

### OverlayRendering Generator

Generates 4 shader program variants based on:
- **Premultiplied alpha**: Affects blend state configuration
- **BGRA source**: Affects fragment shader (`.bgra` swizzle or not)

Cache key includes: render target type + premultiplied alpha + BGRA source format.
See: `OverlayRendering.cpp:computeProgramCacheKey()`

### Compatibility Checking
```cpp
Material requirements: [Normals, TangentSpace, TextureCoordinates2D]
Geometry attributes:   [positions, normals, uvs]  // NO tangents!
→ FAILURE with detailed log
```
