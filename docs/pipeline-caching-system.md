# Pipeline Caching System

Complete reference for the Emeraude Engine's multi-level graphics pipeline caching architecture.

## Overview

The engine uses a **3-level caching system** to avoid redundant shader generation and pipeline creation. This is critical for performance as shader compilation and Vulkan pipeline creation are expensive operations.

## The Problem This Solves

Without caching:
- Every `RenderableInstance` would trigger full shader generation
- Every draw call would require a new Vulkan pipeline
- Frame times would be measured in seconds, not milliseconds

With caching:
- Identical configurations reuse existing programs/pipelines
- Typical scene: ~50 programs built, ~1500 reused
- Frame-time impact: negligible after initial load

## Cache Levels

```
┌────────────────────────────────────────────────────────────────┐
│                    LEVEL 1: ShaderModule                       │
│                    Location: ShaderManager                     │
│                    Key: Source code hash                       │
│                                                                │
│  Benefit: Reuse compiled SPIR-V between programs               │
│  Example: Two materials using same lighting → share modules    │
└────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌────────────────────────────────────────────────────────────────┐
│                    LEVEL 2: Program                            │
│                    Location: Renderer::m_programs              │
│                    Key: Generator::computeProgramCacheKey()    │
│                                                                │
│  Benefit: Skip entire shader generation (BIGGEST GAIN)        │
│  Example: Same material on different meshes → share program    │
└────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌────────────────────────────────────────────────────────────────┐
│                    LEVEL 3: GraphicsPipeline                   │
│                    Location: Renderer::m_graphicsPipelines     │
│                    Key: GraphicsPipeline::getHash(renderPass)  │
│                                                                │
│  Benefit: Reuse Vulkan pipeline objects                        │
│  Example: Same program with same states → share pipeline       │
└────────────────────────────────────────────────────────────────┘
```

## Complete Pipeline Selection Flow

When a `RenderableInstance` needs to be drawn:

### Step 1: Render Request
```
RenderableInstance wants to draw on a RenderTarget
File: Graphics/RenderableInstance/Abstract.cpp
```

### Step 2: Renderable-Level Cache (ProgramCacheKey)
```cpp
// File: Graphics/RenderableInstance/Abstract.cpp
ProgramCacheKey key;
key.programType = ProgramType::Rendering;
key.renderPassType = RenderPassType::SimplePass;
key.renderPassHandle = framebuffer->renderPass()->handle();  // CRITICAL!
key.layerIndex = 0;
key.isInstancing = false;
// ...

// Lookup in Renderable::m_programCache
auto* program = renderable->findCachedProgram(renderTarget, key);
if (program) {
    // HIT: Skip to Step 5
}
```

### Step 3: Renderer-Level Program Cache
```cpp
// File: Graphics/Renderer.cpp
size_t cacheKey = generator->computeProgramCacheKey();

// Lookup in Renderer::m_programs
auto it = m_programs.find(cacheKey);
if (it != m_programs.end()) {
    // HIT: Skip to Step 5
}
```

### Step 4: Shader Program Generation
```cpp
// Files: Saphir/Generator/*.cpp
generator->onGenerateShadersCode(program);    // Create GLSL
generator->onCreateDataLayouts(...);          // Descriptor layouts
// Compile to SPIR-V
// Store in Renderer::m_programs
```

### Step 5: Renderer-Level Pipeline Cache
```cpp
// File: Graphics/Renderer.cpp → finalizeGraphicsPipeline()
const auto& renderPass = renderTarget.framebuffer()->renderPass();
size_t hash = graphicsPipeline->getHash(*renderPass);  // Includes renderPass!

// Lookup in Renderer::m_graphicsPipelines
auto it = m_graphicsPipelines.find(hash);
if (it != m_graphicsPipelines.end()) {
    // HIT: Use existing pipeline
}
```

### Step 6: Vulkan Pipeline Creation
```cpp
// File: Vulkan/GraphicsPipeline.cpp
vkCreateGraphicsPipelines(..., renderPass.handle(), ...);
// Store in Renderer::m_graphicsPipelines
```

### Step 7: Draw
```cpp
vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
vkCmdDraw(commandBuffer, ...);
```

## Critical Rule: renderPassHandle

> **Vulkan pipelines are tied to specific render passes.**

A pipeline created for render pass A **CANNOT** be used with render pass B if they have different:
- Sample count (1 vs 4 samples)
- Attachment formats (BGRA vs RGBA, different depths)
- Attachment count
- Load/store operations

### The Bug This Prevents

Without `renderPassHandle` in cache keys:

1. Offscreen rendering creates pipeline for RenderPass A (1 sample, RGBA)
2. Main view rendering requests same material
3. Cache returns pipeline from step 1 (WRONG!)
4. Vulkan validation error: "sample count mismatch"

### Required Implementation

**ALL THREE cache levels MUST include renderPassHandle:**

#### 1. ProgramCacheKey (Renderable level)
```cpp
// File: Graphics/Renderable/ProgramCacheKey.hpp
struct ProgramCacheKey {
    uint64_t renderPassHandle{0};  // MANDATORY
    // ... other fields
};
```

#### 2. computeProgramCacheKey() (Generator level)
```cpp
// File: Saphir/Generator/*.cpp
size_t MyGenerator::computeProgramCacheKey() const noexcept
{
    size_t hash = Hash::FNV1a(ClassId);

    // MANDATORY: First hash component
    if (const auto* fb = this->renderTarget()->framebuffer(); fb != nullptr)
    {
        hashCombine(hash, reinterpret_cast<size_t>(fb->renderPass()->handle()));
    }

    // ... other components
    return hash;
}
```

#### 3. getHash() (Pipeline level)
```cpp
// File: Vulkan/GraphicsPipeline.cpp
size_t GraphicsPipeline::getHash(const RenderPass& renderPass) const noexcept
{
    size_t hash = 0;

    // MANDATORY: First hash component
    hashCombine(hash, reinterpret_cast<uintptr_t>(renderPass.handle()));

    // ... other components
    return hash;
}
```

## Required Includes

When implementing `computeProgramCacheKey()` in generators:

```cpp
#include "Vulkan/Framebuffer.hpp"
#include "Vulkan/RenderPass.hpp"
```

## Cache Key Components by Generator

### SceneRendering
| Component | Purpose |
|-----------|---------|
| `renderPassHandle` | Pipeline compatibility |
| `isCubemap` | Render target type |
| `renderableName` | Geometry + material identity |
| `layerIndex` | Material layer |
| `renderPassType` | Ambient, directional, point, spot |
| `flags` | Instancing, lighting, facing camera |
| `staticLighting` | Scene lighting state |

### ShadowCasting
| Component | Purpose |
|-----------|---------|
| `renderPassHandle` | Pipeline compatibility |
| `isCubemap` | Point light vs directional |
| `renderableName` | Geometry identity |
| `layerIndex` | Material layer |
| `flags` | Instancing, facing camera |

### OverlayRendering
| Component | Purpose |
|-----------|---------|
| `renderPassHandle` | Pipeline compatibility |
| `isCubemap` | Render target type |
| `premultipliedAlpha` | Blend state |
| `isBGRASurface` | Fragment shader swizzle |

### TBNSpaceRendering
| Component | Purpose |
|-----------|---------|
| `renderPassHandle` | Pipeline compatibility |
| `isCubemap` | Render target type |
| `renderableName` | Geometry identity |
| `layerIndex` | Material layer |
| `flags` | Instancing, facing camera |

## Debugging Cache Issues

### Symptoms of Missing renderPassHandle

```
VUID-VkGraphicsPipelineCreateInfo-renderPass-00xxx
Validation Error: sample count mismatch
Validation Error: format mismatch
Validation Error: VkRenderPass 0x... is not compatible
```

### Diagnosis Steps

1. **Check all three cache levels** include renderPassHandle
2. **Verify includes** are present in generator .cpp files
3. **Add logging** to see cache hits/misses:
   ```cpp
   Tracer::debug(ClassId, "Cache key: {}, hit: {}", cacheKey, wasHit);
   ```

### Statistics

At shutdown, `Renderer` logs cache statistics:
```
Programs built: 47
Programs reused: 1482
Pipelines built: 89
Pipelines reused: 3241
```

Low reuse counts indicate cache key issues.

## File Reference

| File | Purpose |
|------|---------|
| `Graphics/Renderable/ProgramCacheKey.hpp` | Cache key structure |
| `Graphics/RenderableInstance/Abstract.cpp` | Renderable-level cache lookup |
| `Graphics/Renderer.cpp` | Renderer-level caches |
| `Saphir/Generator/Abstract.cpp` | Base generator with cache integration |
| `Saphir/Generator/SceneRendering.cpp` | 3D rendering generator |
| `Saphir/Generator/ShadowCasting.cpp` | Shadow map generator |
| `Saphir/Generator/OverlayRendering.cpp` | 2D overlay generator |
| `Saphir/Generator/TBNSpaceRendering.cpp` | Debug TBN visualization |
| `Vulkan/GraphicsPipeline.cpp` | Pipeline hash computation |

## See Also

- [Graphics System](graphics-system.md) - High-level rendering architecture
- [Saphir Shader System](saphir-shader-system.md) - Shader generation details
- [Render Targets](render-targets.md) - Offscreen rendering setup
