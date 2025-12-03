# RenderableInstance System

This document provides detailed architecture for the RenderableInstance system, which manages the actual rendering of objects in Emeraude Engine's 3D world.

## Quick Reference: Key Terminology

- **RenderableInstance**: An instance of a Renderable ready to be drawn, with its own transformation and rendering parameters.
- **Unique**: A RenderableInstance for classic rendering (one object, push constants for matrices).
- **Multiple**: A RenderableInstance for GPU instancing (N objects, VBO for matrices).
- **ProgramCacheKey**: Key structure identifying a shader program configuration (program type, render pass, layer, flags).
- **RenderPassType**: Type of rendering pass (ambient, directional light, point light, etc.).
- **Layer**: A sub-geometry + material combination within a single Renderable (for multi-material meshes).

## Overview

A **RenderableInstance** represents a concrete occurrence of a Renderable in the 3D world. It's the object that will actually be drawn by the GPU, with its own transformation (position, rotation, scale) and rendering parameters.

### Class Hierarchy

```
RenderableInstance::Abstract (abstract base)
    ├── RenderableInstance::Unique    → Classic rendering (1 instance)
    └── RenderableInstance::Multiple  → GPU instancing (N instances)
```

| Class | Technique | Matrix Storage | Use Case |
|-------|-----------|----------------|----------|
| `Unique` | Classic rendering | Push constants | Single object (character, building) |
| `Multiple` | GPU instancing | Dedicated VBO | Repeated objects (forest, crowd, particles) |

Both classes use push constants, but with different content adapted to their respective rendering technique.

### Correspondence with Scene Components

| Scene Component | RenderableInstance | Description |
|-----------------|-------------------|-------------|
| `Visual` | `Unique` | A single object in the scene |
| `MultipleVisuals` | `Multiple` | Multiple instances of the same object |

This separation reflects responsibilities: Scene handles logic (entities, components), Graphics handles technique (how to render).

## Flag System

Each RenderableInstance has flags that control its rendering behavior:

| Flag | Description |
|------|-------------|
| `EnableLighting` | Generates lighting code in shaders |
| `EnableShadows` | Generates code for casting/receiving shadows |
| `UseInfinityView` | Ignores translation (for skybox) |
| `DisableDepthTest` | Does not read the depth buffer |
| `DisableDepthWrite` | Does not write to the depth buffer |
| `DisableStencilTest` | Does not read the stencil buffer |
| `DisableStencilWrite` | Does not write to the stencil buffer |
| `ApplyTransformationMatrix` | Applies an additional transformation *(Unique only, push constants)* |
| `DisableLightDistanceCheck` | Ignores distance for lighting calculation |
| `DisplayTBNSpaceEnabled` | Debug: displays tangent space vectors |

**Convention**: All flags are `false` by default.

## Layer System

A Renderable can contain multiple **layers**, where each layer corresponds to a sub-geometry with its own material.

**Use case**: A character with multiple material zones (skin, clothing, metallic accessories) in a single mesh.

**Not to be confused with**: Clearly separable objects (sword, hat) which will have their own RenderableInstance attached to child nodes.

```cpp
// Example: Character with 3 layers
Renderable "character":
    Layer 0: skin geometry + skin material
    Layer 1: clothing geometry + fabric material
    Layer 2: armor geometry + metal material

// Each layer has its own shader programs
// All layers are rendered as part of the same RenderableInstance
```

## Program Caching Architecture

Shader programs are cached at the **Renderable** level, not per-instance. This enables efficient sharing across all instances of the same Renderable:

```
Renderable::Abstract
    └── m_programCache: Map<RenderTarget → Map<ProgramCacheKey → Program>>
```

This allows the same object to be rendered in multiple targets (main view, shadow map, security camera) with appropriate programs, while ensuring instances sharing the same Renderable also share cached programs.

### ProgramCacheKey Structure

The cache key uniquely identifies a shader program configuration:

```cpp
struct ProgramCacheKey {
    ProgramType programType;      // Rendering, ShadowCasting, TBNSpace
    RenderPassType renderPassType; // Ambient, directional, point, spot...
    uint32_t layerIndex;          // Material layer index
    bool isInstancing;            // Unique (false) vs Multiple (true)
    bool isLightingEnabled;       // Lighting code enabled
    bool isDepthTestDisabled;     // Depth test disabled flag
    bool isDepthWriteDisabled;    // Depth write disabled flag
};
```

### Why Cache at Renderable Level?

- **Memory efficiency**: Instances sharing a Renderable share programs
- **Lightweight instances**: RenderableInstance has no dynamic allocations
- **Thread-safe**: Cache protected by mutex for concurrent access
- **Automatic sharing**: No manual program management needed

## RenderPass Types

The engine uses **forward rendering multi-pass**. A lit object is rendered multiple times:

```cpp
enum RenderPassType {
    SimplePass = 0,                  // No lighting
    AmbientPass = 1,                 // Ambient light only
    DirectionalLightPass = 2,        // Directional light + shadows
    DirectionalLightPassNoShadow = 3,
    PointLightPass = 4,              // Point light + shadows
    PointLightPassNoShadow = 5,
    SpotLightPass = 6,               // Spot light + shadows
    SpotLightPassNoShadow = 7
};
```

**Example**: An object affected by 3 lights = 4 draw calls:
1. `AmbientPass` (ambient light)
2. `DirectionalLightPass` (sun)
3. `PointLightPass` (lamp 1)
4. `PointLightPass` (lamp 2)

Contributions are accumulated via additive blending.

**Design rationale**: This strategy aims to produce simple and efficient shaders.

## Preparation Flow

The **Scene** orchestrates program generation based on configuration:

```
1. getReadyForShadowCasting(renderTarget, renderer)
   └── For each layer:
       └── Build ProgramCacheKey
       └── Check Renderable cache → if miss: Generator::ShadowCasting → cache result

2. getReadyForRender(scene, renderTarget, renderPassTypes, renderer)
   └── For each requested RenderPassType:
       └── For each layer:
           └── Build ProgramCacheKey
           └── Check Renderable cache → if miss: Generator::SceneRendering → cache result
```

**Dynamic generation**: If a new light appears in the scene, missing programs are generated on the fly. The Scene examines what's active (shadows, light types, render target types) and completes rendering possibilities both upfront and as needed.

**Cache hierarchy**: Programs are first looked up in the Renderable's cache. On miss, they're generated via Saphir and cached for future use by any instance of the same Renderable.

## Render Context System

To support both classic 2D rendering and cubemap multiview rendering, the system uses two POD structures that encapsulate rendering context:

### RenderPassContext

Groups render pass information:

```cpp
struct RenderPassContext final
{
    const Vulkan::CommandBuffer * commandBuffer{nullptr};  // Active command buffer
    const ViewMatricesInterface * viewMatrices{nullptr};   // View/projection data
    uint32_t readStateIndex{0};                            // Double-buffering index
    bool isCubemap{false};                                 // True for cubemap multiview
};
```

### PushConstantContext

Groups push constant parameters:

```cpp
struct PushConstantContext final
{
    const Vulkan::PipelineLayout * pipelineLayout{nullptr};
    VkShaderStageFlags stageFlags{VK_SHADER_STAGE_VERTEX_BIT};
    bool useAdvancedMatrices{false};  // Need separate V and M matrices
    bool useBillboarding{false};      // Billboard/sprite rendering
};
```

### Why Two Structures?

- **Separation of concerns**: Pass context vs push constant context have different lifecycles
- **Performance**: Values like `stageFlags` are computed once per program, not per draw
- **Cubemap support**: `isCubemap` flag enables different push constant strategies
- **Clean interface**: Reduces function parameters from 6+ to 3

## Rendering Flow

```cpp
render(readStateIndex, renderTarget, lightEmitter, renderPassType,
       layerIndex, worldCoordinates, commandBuffer)
{
    1. Build ProgramCacheKey for (renderPassType, layerIndex, instance flags)
    2. Query program from Renderable cache
    3. Bind graphics pipeline
    4. Bind instance VBO/matrices
    5. Build RenderPassContext and PushConstantContext
    6. Push matrices via pushMatricesForRendering(passContext, pushContext, worldCoordinates)
    7. Bind View UBO (descriptor set 0)
    8. Bind Light UBO if present (descriptor set 1)
    9. Bind Material descriptors (descriptor set 1 or 2)
    10. commandBuffer.draw(geometry, layerIndex/frameIndex, instanceCount)
}
```

### Shadow Casting Flow

```cpp
castShadows(readStateIndex, renderTarget, layerIndex, worldCoordinates, commandBuffer)
{
    1. Build ProgramCacheKey for (ShadowCasting, layerIndex, instance flags)
    2. Query program from Renderable cache
    3. Bind graphics pipeline
    4. Set dynamic viewport
    5. Bind View UBO (if instancing enabled)
    6. Bind instance model layer
    7. Build RenderPassContext and PushConstantContext
    8. Push matrices via pushMatricesForShadowCasting(passContext, pushContext, worldCoordinates)
    9. commandBuffer.draw(geometry, layerIndex, instanceCount)
}
```

## Texture Animation

For animated materials (sprite sheets, texture arrays), the system uses `m_frameIndex`:

- **`layerIndex`**: Selects which material layer of the object
- **`m_frameIndex`**: Selects which frame in an animated texture (2D texture array)

The method `updateFrameIndex(sceneTimeMS)` updates the index based on scene time.

```cpp
// If material is animated, frameIndex is used instead of layerIndex
if (material->isAnimated()) {
    commandBuffer.draw(*geometry, m_frameIndex, this->instanceCount());
} else {
    commandBuffer.draw(*geometry, layerIndex, this->instanceCount());
}
```

## Lifecycle and Notifications

```
Renderable (finishes loading)
    → notifies Visual (observer)
        → propagates to Scene
            → triggers GPU upload (programs, textures, mesh...)
                → marks Renderable as ready
                    → RenderableInstance becomes OK for rendering
```

This mechanism enables asynchronous resource loading while keeping the system reactive.

## Unique vs Multiple: Technical Details

### RenderableInstance::Unique

- **Instance count**: Always 1
- **Matrix storage**: Via push constants
- **Model matrices created**: Always true (no GPU buffer needed)
- **Use case**: Individual objects with unique transforms

```cpp
// Unique uses push constants for model matrix
// Supports ApplyTransformationMatrix for additional transform
// Lighter weight for single objects
```

#### Cubemap Rendering Mode

When `passContext.isCubemap` is true:
- Only pushes the Model matrix (M)
- View/Projection matrices are in a UBO indexed by `gl_ViewIndex`
- Enables multiview rendering for all 6 cubemap faces in a single pass

```cpp
// Classic 2D: Push MVP (or V + M for advanced)
// Cubemap: Push only M (VP in UBO[gl_ViewIndex])
```

### RenderableInstance::Multiple

- **Instance count**: N (configurable)
- **Matrix storage**: Dedicated VBO
- **Supports two formats**:
  - Mesh: 25 floats per instance (mat4 model + mat3 normal)
  - Sprite: 6 floats per instance (vec3 position + vec3 scale)

```cpp
// Multiple stores all instance transforms in a VBO
// The Renderable determines which format to use (isSprite())
// Ideal for rendering hundreds/thousands of identical objects

// Constructor with initial positions
Multiple(device, renderable, instanceLocations, flagBits);

// Constructor with just capacity
Multiple(device, renderable, instanceCount, flagBits);

// Update individual instance
updateLocalData(instanceLocation, instanceIndex);

// Update batch of instances
updateLocalData(instanceLocations, instanceOffset);

// Sync to GPU
updateVideoMemory();
```

#### Cubemap Rendering Mode

When `passContext.isCubemap` is true:
- No push constants are needed at all
- Model matrices are already in the VBO (per-instance data)
- View/Projection matrices are in a UBO indexed by `gl_ViewIndex`
- Most efficient path for GPU instancing with cubemap multiview

```cpp
// Classic 2D: Push VP (Model in VBO)
// Cubemap: Push nothing (Model in VBO, VP in UBO[gl_ViewIndex])
```

## Transformation Matrix

The `setTransformationMatrix()` method allows applying an additional transformation just before rendering (e.g., for scaling adjustments).

**Important**: This only works with push constants, meaning it's effective with `Unique` but not with `Multiple`.

```cpp
// Apply additional transform (Unique only)
renderableInstance->setTransformationMatrix(scaleMatrix);
```

## Integration with Other Systems

### Scene Graph Integration

- `Visual` component creates `Unique` instances
- `MultipleVisuals` component creates `Multiple` instances
- Scene observes Renderables and triggers GPU preparation when ready

### Saphir Integration

- Programs are generated via Saphir generators:
  - `Generator::SceneRendering` for main rendering
  - `Generator::ShadowCasting` for shadow maps
  - `Generator::TBNSpaceRendering` for debug visualization

### Resource System Integration

- RenderableInstance holds a shared pointer to the Renderable
- Observes Renderable for loading completion notifications
- Fail-safe: if Renderable fails to load, RenderableInstance is marked as broken

## Common Patterns

### Creating a Visual (Single Object)

```cpp
// In Scene code
auto node = scene->root()->createChild("crate", position);
auto renderable = resources.get<RenderableResource>("wooden_crate");

// Visual internally creates a Unique RenderableInstance
node->newVisual(renderable, castShadows, receiveShadows, "main_visual");
// Automatic registration to Renderer via observers
```

### Creating Multiple Instances (GPU Instancing)

```cpp
// Using MultipleVisuals for repeated objects
auto renderable = resources.get<RenderableResource>("tree");

// Prepare positions for 1000 trees
std::vector<CartesianFrame<float>> treePositions;
for (int i = 0; i < 1000; ++i) {
    treePositions.push_back(randomForestPosition());
}

// MultipleVisuals creates a Multiple RenderableInstance internally
// All 1000 trees rendered efficiently via GPU instancing
```

## Cubemap Multiview Rendering

The RenderableInstance system supports cubemap multiview rendering through the `isCubemap` flag in `RenderPassContext`. This enables efficient 360° environment rendering for:

- **Reflection probes**: Real-time reflections on shiny surfaces
- **Environment maps**: Dynamic skyboxes and ambient lighting
- **Shadow cubemaps**: Omnidirectional shadow maps for point lights

### How It Works

1. **RenderTarget detection**: `renderTarget->isCubemap()` returns true for cubemap targets
2. **Context propagation**: `passContext.isCubemap` is set based on render target type
3. **Push constant adaptation**:
   - `Unique`: Pushes only Model matrix (VP in UBO)
   - `Multiple`: Pushes nothing (Model in VBO, VP in UBO)
4. **Shader access**: `gl_ViewIndex` (0-5) selects the appropriate face's VP matrix

### Matrix Distribution

| Data | 2D Rendering | Cubemap Rendering |
|------|--------------|-------------------|
| Model (M) | Push constant (Unique) or VBO (Multiple) | Same |
| View (V) | Push constant or computed | UBO[gl_ViewIndex] |
| Projection (P) | Push constant or computed | UBO[gl_ViewIndex] |

**Next step**: Adapt Saphir shader generators to produce shaders using `gl_ViewIndex` for cubemap VP matrix access.

## Performance Optimizations

### Zero Heap Allocation in Hot Paths

The RenderableInstance system avoids `std::vector` in rendering code paths, using `StaticVector` instead:

```cpp
// Instead of std::vector (heap allocation per frame)
StaticVector< VkBuffer, 8 > buffers;
StaticVector< VkDeviceSize, 8 > offsets;
```

**Why StaticVector?**
- **Stack allocation**: Fixed capacity storage on the stack, no heap allocation
- **Known bounds**: Maximum buffer/offset count is known at compile time (≤8)
- **Cache friendly**: Contiguous stack memory improves CPU cache utilization
- **Zero overhead**: No allocator calls in the render loop

This optimization is applied throughout the rendering pipeline:
- VBO binding (buffers and offsets)
- Descriptor set binding
- Push constant data assembly

### std::span for Vulkan Command APIs

The `CommandBuffer` class uses `std::span` instead of `std::vector` for barrier and event methods:

```cpp
// Accepts any contiguous container (StaticVector, std::vector, std::array, C-array)
void pipelineBarrier(std::span< const VkImageMemoryBarrier > imageMemoryBarriers, ...);
void waitEvents(std::span< const VkEvent > events, ...);
```

**Why std::span?**
- **Zero-copy view**: No data duplication, just a pointer + size
- **Universal compatibility**: Accepts `StaticVector`, `std::vector`, `std::array`, raw arrays
- **No allocation on call site**: Callers can use stack-allocated containers
- **Automatic conversion**: Existing `std::vector` code works without changes

**Example usage:**
```cpp
// With StaticVector (zero heap allocation)
StaticVector< VkImageMemoryBarrier, 4 > barriers;
barriers.emplace_back(...);
commandBuffer.pipelineBarrier(barriers, srcStage, dstStage);

// With std::vector (still works, automatic conversion)
std::vector< VkImageMemoryBarrier > barriers;
commandBuffer.pipelineBarrier(barriers, srcStage, dstStage);
```

### Pre-computed Context Values

The `PushConstantContext` structure pre-computes values once per program:

```cpp
// stageFlags computed once, not per draw call
.stageFlags = static_cast< VkShaderStageFlags >(
    program->hasGeometryShader() ?
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT :
        VK_SHADER_STAGE_VERTEX_BIT
)
```

This avoids repeated conditional checks during high-frequency draw calls.

## Design Principles Summary

| Principle | Description | Benefit |
|-----------|-------------|---------|
| **Separation of concerns** | Scene components vs Graphics technique | Clean architecture |
| **Forward multi-pass** | One pass per light source | Simple, efficient shaders |
| **Renderable-level caching** | Programs cached on Renderable, not Instance | Memory efficiency, shared programs |
| **Lightweight instances** | RenderableInstance has no dynamic allocations | Fast instantiation, low memory |
| **Flag-based configuration** | Behavior controlled via flags | Flexible, explicit |
| **Zero heap allocation** | StaticVector + std::span in hot paths | No allocator overhead |
| **Context-based rendering** | POD structures for render context | Cubemap support, clean interface |
| **Layer support** | Multi-material meshes | Complex objects in single draw |
| **Dual instancing modes** | Unique vs Multiple | Optimal for each use case |

### Core Philosophy

> "Provide the right rendering technique for each use case: classic rendering for unique objects, GPU instancing for duplicates, with dynamic shader program generation based on actual scene requirements. Programs are cached at the Renderable level to maximize sharing across instances."