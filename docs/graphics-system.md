# Graphics System Architecture

This document provides detailed architecture for the Graphics system, the high-level rendering abstraction layer of Emeraude Engine built on top of Vulkan.

## Quick Reference: Key Terminology

- **Geometry**: GPU-side geometry description (vertices, indices, formats). Defines what shape an object has.
- **Material**: Assembly of colors, textures, and properties. Defines how an object looks.
- **Renderable**: Geometry + Material = a complete unique object ready to render. Analogous to a "mesh" in other engines.
- **RenderableInstance**: An instance of a Renderable with unique transformation and parameters. Enables efficient rendering of duplicates.
- **Renderer**: The central coordinator managing all rendering operations and subsystems.
- **TransferManager**: Handles CPU ↔ GPU data transfers (uploads, downloads, staging buffers).
- **LayoutManager**: Centralized management of Vulkan pipeline layouts.
- **ShaderManager**: Saphir integration for automatic GLSL generation from Material + Geometry.
- **SharedUBOManager**: Manages shared uniform buffer objects between multiple resources.
- **VertexBufferFormatManager**: Centralized registry of vertex formats used throughout the engine.
- **RenderTarget**: High-level abstraction for render-to-texture, shadow maps, and off-screen rendering.
- **Visual Component**: Scene component that holds a RenderableInstance and registers it with the Renderer.

## Design Philosophy: Why Graphics Exists

### The Problem: Vulkan is Too Low-Level

Vulkan provides tremendous control and performance but requires extensive boilerplate:

```cpp
// Raw Vulkan approach (excerpt - 500+ lines for one object)
VkBuffer vertexBuffer;
VkDeviceMemory vertexMemory;
VkBuffer indexBuffer;
VkDeviceMemory indexMemory;
VkDescriptorSetLayout descriptorLayout;
VkPipelineLayout pipelineLayout;
VkPipeline pipeline;
VkDescriptorPool descriptorPool;
VkDescriptorSet descriptorSet;
// ... allocate memory, bind buffers, create pipeline, update descriptors ...
// ... manage synchronization, barriers, layout transitions ...
// ... record commands, submit, present ...
```

**Problems with raw Vulkan:**
- **Verbose**: Hundreds of lines per renderable object
- **Error-prone**: Easy to forget synchronization, leak resources, misconfigure
- **Not declarative**: Can't easily say "render this object with this material"
- **Manual resource management**: Developers must track lifetimes, dependencies

### Graphics Solution: OpenGL-Style Declarative Interface

```cpp
// Graphics approach (5-10 lines)
auto geometry = resources.get<GeometryResource>("cube");
auto material = resources.get<MaterialResource>("wood");
auto renderable = Renderable::create(geometry, material);

// In scene
auto node = scene->root()->createChild("crate", position);
node->newVisual(renderable);  // Automatically registered, rendered
```

**Benefits:**
- ✅ **Declarative** - Say what you want, not how to do it
- ✅ **Automatic management** - Resources, pipelines, synchronization handled internally
- ✅ **Fail-safe integration** - Leverages Resource system's fail-safe philosophy
- ✅ **Instancing built-in** - Duplicates rendered efficiently without code changes
- ✅ **Saphir integration** - Shaders generated automatically
- ✅ **Developer-friendly** - Focus on content, not Vulkan mechanics

## Architecture: The Instancing System

### Core Concepts

The Graphics system is built around an **instancing hierarchy** that separates definition from usage:

```
Geometry (what shape)
    +
Material (how it looks)
    =
Renderable (complete object definition)
    ↓
RenderableInstance (unique transform + parameters)
    ↓
Visual Component (scene integration)
```

### Detailed Breakdown

#### 1. Geometry: Shape Definition

**Purpose:** Describe the geometric shape at GPU level.

**Contents:**
- Vertex data (positions, normals, tangents, UVs, colors)
- Index data (triangle indices for indexed rendering)
- Vertex format (queryable by Saphir for shader generation)
- GPU buffers (managed by Vulkan abstraction)

**Key Features:**
- Loaded via Resource system (fail-safe: neutral geometry if load fails)
- Registered with VertexBufferFormatManager (format reuse across geometries)
- Immutable once uploaded to GPU (CPU copy discarded)
- Thread-safe reference counting (multiple Renderables can share one Geometry)

**Example:**
```cpp
// Cube geometry
Geometry "cube_1m":
    vertices: 24 (8 corners × 3 for sharp edges)
    indices: 36 (6 faces × 2 triangles × 3 vertices)
    format: positions (vec3) + normals (vec3) + uvs (vec2)
    GPU memory: ~2 KB
```

#### 2. Material: Appearance Definition

**Purpose:** Define how surfaces respond to light and how textures are applied.

**Contents:**
- Textures (diffuse, normal, roughness, metallic, emissive)
- Colors (base color, emissive color, etc.)
- Properties (roughness value, metallic value, transparency, blend mode)
- Requirements (what attributes the geometry must have: normals, tangents, UVs, vertex colors)

**Key Features:**
- Declarative requirements (Saphir checks compatibility)
- Loaded via Resource system (fail-safe: neutral material if load fails)
- Can reference other resources (textures) via dependency system
- Thread-safe once finalized

**Example:**
```cpp
// Wood material
Material "wood_planks":
    requires: [Normals, TangentSpace, TextureCoordinates2D]
    textures:
        diffuse: "wood_planks_diffuse.png"
        normal: "wood_planks_normal.png"
        roughness: "wood_planks_roughness.png"
    properties:
        baseColor: (1.0, 1.0, 1.0)  // White (texture provides color)
        roughness: 0.8  // Modulated by roughness map
        metallic: 0.0  // Wood is non-metallic
```

#### 3. Renderable: Complete Object

**Purpose:** Combine Geometry + Material into a complete, renderable object.

**Creation Process:**
```cpp
// When Renderable is finalized (onDependenciesLoaded)
1. Check compatibility: Material requirements vs Geometry attributes
   ✓ Material needs: [Normals, TangentSpace, TextureCoordinates2D]
   ✓ Geometry has: [Positions, Normals, Tangents, UVs]
   → Compatible!

2. Generate shader via Saphir (ShaderManager)
   → GLSL source generated based on Material + Geometry
   → Compiled to SPIR-V

3. Create Vulkan pipeline (via Vulkan abstraction)
   → Pipeline layout, descriptor sets, graphics pipeline

4. Register with Renderer
   → Renderable ready for rendering
```

**If Incompatible:**
```cpp
✗ Material needs: [Normals, TangentSpace, TextureCoordinates2D]
✗ Geometry has: [Positions, Normals, UVs]  // NO TANGENTS!
→ Incompatible - cannot use normal map without tangent space

Result:
- onDependenciesLoaded() returns false
- Renderable fails to load
- Neutral renderable used instead (fail-safe)
- Application continues running
- Log explains what's missing
```

**Key Features:**
- Immutable once created (Geometry + Material fixed)
- Single Vulkan pipeline (optimized for this exact combination)
- Can be instanced many times (see RenderableInstance)
- Shared by all instances (memory efficient)

#### 4. RenderableInstance: Positioned Object

**Purpose:** Represent a specific occurrence of a Renderable in the scene with unique transformation.

**Contents:**
- Reference to parent Renderable (shared)
- Transformation matrix (position, rotation, scale)
- Per-instance parameters (custom colors, material overrides if supported)
- Rendering flags (cast shadows, receive shadows, visible, etc.)

**Key Features:**
- Lightweight (only stores transform + flags, shares everything else)
- Many instances → one Renderable (efficient instancing)
- Updated independently (each instance has its own transform)
- Automatically batched by Renderer (GPU instancing where possible)

**Example:**
```cpp
Renderable "wooden_crate" (shared):
    Geometry: "cube_1m"
    Material: "wood_planks"
    Pipeline: [generated for this combination]

RenderableInstance #1:
    Renderable: "wooden_crate"
    Transform: position (10, 0, 5), rotation (0°, 45°, 0°), scale 1.0
    Flags: castShadows=true, receiveShadows=true

RenderableInstance #2:
    Renderable: "wooden_crate"
    Transform: position (15, 0, 8), rotation (0°, 90°, 0°), scale 1.2
    Flags: castShadows=true, receiveShadows=false

// ... up to thousands of instances ...

Result:
- 1000 crates rendered efficiently
- Single Geometry, Material, Pipeline (shared)
- 1000 unique transforms (GPU instancing)
```

## Renderer: The Central Coordinator

### Overview

The **Renderer** is the heart of the Graphics system. It coordinates all rendering operations and manages the subsystems.

**Responsibilities:**
1. Coordinate all rendering operations
2. Manage frame lifecycle (begin frame, render, end frame, present)
3. Integrate subsystems (TransferManager, LayoutManager, ShaderManager, etc.)
4. Maintain render queues (opaque, transparent, shadows, post-processing)
5. Observe scene changes (Visual components added/removed via Observer pattern)
6. Execute render passes in correct order
7. Manage synchronization (fences, semaphores for frame pacing)

### Subsystems

#### TransferManager: CPU ↔ GPU Data Movement

**Purpose:** Handle all data transfers between CPU and GPU safely and efficiently.

**Operations:**
- **Upload**: CPU data → GPU buffer/image (e.g., vertex data, textures)
- **Download**: GPU buffer/image → CPU data (e.g., readback for debugging)
- **Staging**: Automatic use of staging buffers for large transfers
- **Synchronization**: Fences to ensure transfer completion before use

**Key Features:**
- Batches small transfers for efficiency
- Automatic staging buffer management (reuses buffers)
- Thread-safe (can be called from resource loading threads)
- Async uploads with completion callbacks

**Example:**
```cpp
// Uploading vertex data
std::vector<Vertex> vertices = loadFromFile("mesh.obj");
transferManager.uploadToBuffer(
    vertices.data(),
    vertices.size() * sizeof(Vertex),
    vertexBuffer,
    []() { Log::info("Vertices uploaded!"); }
);
```

#### LayoutManager: Vulkan Pipeline Layout Management

**Purpose:** Centralize and reuse Vulkan pipeline layouts across the engine.

**Why?**
- Vulkan requires explicit descriptor set layouts and pipeline layouts
- Many shaders share similar layouts (e.g., all PBR shaders)
- Creating layouts is relatively expensive
- Reusing layouts improves performance and reduces memory

**Operations:**
- Register common layout patterns (standard PBR, shadow, overlay)
- Return existing layout if pattern matches (cache lookup)
- Create new layout if unique pattern (cache miss)
- Track layout lifetimes (reference counting)

**Key Features:**
- Reduces Vulkan object count (fewer VkPipelineLayout objects)
- Automatic deduplication
- Descriptor set compatibility ensured

#### ShaderManager (Saphir Integration)

**Purpose:** Interface between Graphics and Saphir for automatic shader generation.

**Process:**
```cpp
// When Renderable needs shader
Material material = renderable->getMaterial();
Geometry geometry = renderable->getGeometry();
SceneContext scene = renderer->getSceneContext();

// ShaderManager calls appropriate Saphir generator
if (renderableType == Scene3D) {
    glsl = saphir.sceneGenerator()->generate(material, geometry, scene);
} else if (renderableType == Overlay2D) {
    glsl = saphir.overlayGenerator()->generate(material, geometry);
} else if (renderableType == Shadow) {
    glsl = saphir.shadowManager()->generate(geometry);
}

// Compile GLSL → SPIR-V
spirv = glslang.compile(glsl);

// Create Vulkan pipeline with SPIR-V
pipeline = vulkan.createPipeline(spirv, ...);
```

**Key Features:**
- Transparent Saphir integration (Graphics doesn't know about GLSL details)
- Caching of compiled shaders (avoids redundant work)
- Handles compilation errors gracefully (logs, falls back to neutral)

#### SharedUBOManager: Uniform Buffer Sharing

**Purpose:** Share uniform buffer objects (UBOs) between resources that use the same data.

**Why?**
- Many objects use the same global data (view matrix, projection matrix, lights)
- Duplicate UBOs waste memory and bandwidth
- Shared UBOs = update once, use everywhere

**Shared Data Categories:**
1. **Per-frame data**: View matrix, projection matrix, camera position, time
2. **Lighting data**: Light positions, colors, directions, shadow matrices
3. **Material constants**: Shared material properties (e.g., all objects use same roughness)

**Key Features:**
- Automatic deduplication (same data → same UBO)
- Efficient updates (update once per frame, not per object)
- Vulkan descriptor set compatibility (objects with same layout share descriptors)

#### VertexBufferFormatManager: Format Registry

**Purpose:** Centralize vertex format definitions and enable reuse.

**Why?**
- Many geometries use the same vertex format (e.g., position + normal + UV)
- Vulkan requires explicit format description (VkVertexInputAttributeDescription)
- Defining formats repeatedly is error-prone and wasteful
- Generators (Saphir) need to query formats

**Registry:**
```cpp
// Common formats registered at startup
FormatID format_P_N_UV = manager.registerFormat({
    {0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position)},  // vec3 position
    {1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal)},    // vec3 normal
    {2, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv)}            // vec2 UV
});

// Geometries reference registered formats
Geometry cube;
cube.setFormat(format_P_N_UV);

// Saphir can query format to generate compatible shaders
if (geometry->hasNormals()) { ... }
if (geometry->hasTangentSpace()) { ... }
```

**Key Features:**
- Format deduplication (same layout → same ID)
- Queryable by Saphir (Material requirements matching)
- Vulkan format descriptors precomputed (performance)

## RenderTarget: Render-to-Texture and Off-Screen Rendering

### Purpose

Enable rendering to textures instead of the screen for advanced effects:
- Shadow maps (render depth from light's perspective)
- Post-processing (render scene, apply effects, display result)
- Reflections (render scene from mirror's perspective, apply to mirror surface)
- Multi-camera setups (render multiple views, composite final image)

### Abstraction Level

**High-level interface** (Graphics layer):
```cpp
// Declare a render target
auto shadowMap = RenderTarget::createDepthOnly(2048, 2048);
auto reflectionTarget = RenderTarget::createColorDepth(1920, 1080, format);

// Use in render pass
renderer.beginRenderPass(shadowMap);
renderer.renderShadowCasters();
renderer.endRenderPass();

// Shadow map texture now contains depth from light POV
```

**Low-level implementation** (Vulkan layer):
- Vulkan framebuffers, render passes, image views managed internally
- Automatic layout transitions (e.g., depth write → shader read)
- Synchronization handled (e.g., wait for shadow map before using in lighting pass)

### Integration with Scene

RenderTargets are used by:
1. **ShadowManager** (part of Saphir): Renders shadow maps for each light
2. **AVConsole** (Audio-Video Console): Manages cameras rendering to textures
3. **Post-processing pipeline**: Applies effects to rendered scene

## Integration with Scene Graph

### Visual Component: Bridge Between Scene and Graphics

The **Visual component** connects scene nodes to the rendering system.

**Lifecycle:**
```cpp
// 1. Create node in scene
auto node = scene->root()->createChild("crate", position);

// 2. Attach Visual component with RenderableInstance
auto renderable = resources.get<RenderableResource>("wooden_crate");
auto visual = node->newVisual(renderable);

// 3. Visual registers itself with Renderer (via Observer pattern)
// Renderer is observing Scene → automatically notified of new Visual

// 4. Each frame:
//    - Scene updates node transforms (physics, animations)
//    - Visual reads node transform, updates RenderableInstance
//    - Renderer renders all RenderableInstances
```

**Observer Pattern:**
```cpp
Scene (Observable)
    ↓ notifies
Renderer (Observer)
    ↓ reacts
"New Visual component added"
    → Add RenderableInstance to render queue
"Visual component removed"
    → Remove RenderableInstance from render queue
```

**Automatic Registration:**
- Developers don't manually register/unregister with Renderer
- Adding/removing Visual from scene = automatic rendering
- Clean separation: Scene manages hierarchy, Graphics manages rendering

### MultipleVisuals: Multiple RenderableInstances

Some objects need multiple visual elements (e.g., character = body + clothing + weapon):

```cpp
auto characterNode = scene->root()->createChild("player");

// Body
auto body = resources.get<RenderableResource>("human_body");
characterNode->newVisual(body, true, true, "body");

// Clothing
auto shirt = resources.get<RenderableResource>("shirt");
characterNode->newVisual(shirt, true, true, "shirt");

// Weapon (attached to hand bone)
auto sword = resources.get<RenderableResource>("sword");
auto handNode = characterNode->findChild("hand_bone");
handNode->newVisual(sword, true, false, "sword");

// All three rendered together, positioned relative to skeleton
```

## Coordinate System Integration

### Y-Down Convention (CRITICAL)

The Graphics system strictly follows the Y-down coordinate convention throughout the engine:

**Why Y-Down?**
1. **Consistency**: Physics, Audio, Scenes all use Y-down
2. **Natural for 2D**: UI elements naturally flow downward (Y increases down screen)
3. **Vulkan native**: Vulkan's viewport Y-axis increases downward by default
4. **No conversions**: Eliminates error-prone coordinate flips

**Implementation:**
```cpp
// Projection matrices configured for Y-down
Matrix4 perspective = Matrix4::perspectiveProjection(
    fovY,
    aspect,
    nearPlane,
    farPlane,
    CoordinateSystem::Y_DOWN  // Explicit flag
);

// Vulkan viewport configured for Y-down
VkViewport viewport;
viewport.x = 0;
viewport.y = 0;  // Top-left corner (Y increases down)
viewport.width = width;
viewport.height = height;
viewport.minDepth = 0.0f;
viewport.maxDepth = 1.0f;
```

**Critical Rule:**
> **NEVER flip Y coordinates in shaders, transforms, or user code.** The engine is Y-down at all levels. Flipping breaks physics, audio spatialization, and causes subtle bugs.

See @docs/coordinate-system.md for comprehensive details.

## Resource Loading and Fail-Safe Integration

### Graphics Resources as ResourceTrait

All Graphics resources (Geometry, Material, Texture, Renderable) inherit from `ResourceTrait` and follow the fail-safe philosophy:

**Neutral Resources:**
1. **Neutral Geometry**: Simple cube or quad (depends on context)
2. **Neutral Material**: Unlit white material (no textures, no lighting requirements)
3. **Neutral Texture**: 1×1 magenta pixel (makes missing textures obvious)
4. **Neutral Renderable**: Neutral Geometry + Neutral Material (always displays something)

**Loading Process:**
```cpp
// User requests renderable
auto renderable = resources.get<RenderableResource>("wooden_crate");

// If load succeeds:
//   → renderable points to fully loaded "wooden_crate"
// If load fails (file missing, OOM, incompatible Material+Geometry):
//   → renderable points to neutral renderable (white cube)
//   → Log error explaining failure
//   → Application continues running

// User code is identical in both cases:
node->newVisual(renderable);  // Always works (never nullptr)
```

**Benefits:**
- Application never crashes due to missing/broken resources
- Visual indication of problems (magenta textures, white cubes)
- Logs provide diagnostic information
- Development continues smoothly even with incomplete assets

See @docs/resource-management.md for complete fail-safe architecture.

## Rendering Pipeline Overview

### Frame Lifecycle

```
1. BEGIN FRAME
   - Wait for previous frame to complete (fence)
   - Acquire swapchain image (present queue)
   - Reset command buffers

2. UPDATE PHASE
   - Scene updates node transforms (physics, animations)
   - Visuals update RenderableInstance transforms
   - Frustum culling (determine what's visible)
   - Build render queues (opaque, transparent, shadows)

3. SHADOW PASS (if shadows enabled)
   - For each shadow-casting light:
       - Render depth to shadow map RenderTarget
       - Use ShadowManager shaders (minimal, depth-only)

4. MAIN PASS (scene rendering)
   - Bind main framebuffer (or RenderTarget if off-screen)
   - Render opaque objects (front-to-back, depth test)
   - Render transparent objects (back-to-front, blending)
   - Sky/Environment rendering

5. POST-PROCESSING (optional)
   - Render scene to texture
   - Apply effects (bloom, tone mapping, anti-aliasing)
   - Render final result to screen

6. UI/OVERLAY PASS
   - Render 2D elements (HUD, menus, debug overlays)
   - Use OverlayGenerator shaders (no lighting, screen-space)

7. END FRAME
   - Submit command buffers (graphics queue)
   - Present swapchain image (present queue)
   - Signal frame completion (fence)
```

## Common Workflows

### Adding a New Visual to Scene

```cpp
// 1. Load resources (happens once, cached)
auto geometry = resources.get<GeometryResource>("helmet");
auto material = resources.get<MaterialResource>("sci_fi_metal");

// 2. Create renderable (combines geometry + material)
auto renderable = Renderable::create(geometry, material);
// Saphir generates shader, Vulkan pipeline created

// 3. Add to scene
auto node = scene->root()->createChild("helmet", Vector3(0, 5, 10));
auto visual = node->newVisual(renderable);
// Visual automatically registered with Renderer

// 4. Rendering happens automatically each frame
// No further code needed - helmet appears in scene
```

### Instancing Many Objects

```cpp
// 1. Load once
auto treeRenderable = resources.get<RenderableResource>("tree_oak");

// 2. Create many instances
for (int i = 0; i < 1000; ++i) {
    Vector3 position = randomForestPosition();
    auto node = scene->root()->createChild("tree_" + std::to_string(i), position);
    node->newVisual(treeRenderable);  // Shares geometry, material, pipeline
}

// Result: 1000 trees rendered efficiently
// - Single Geometry (shared)
// - Single Material (shared)
// - Single Vulkan pipeline (shared)
// - 1000 transforms (GPU instanced if supported)
```

### Custom Material with Normal Mapping

```cpp
// Material declares requirements
Material customMaterial;
customMaterial.addTexture("diffuse", "metal_diffuse.png");
customMaterial.addTexture("normal", "metal_normal.png");
customMaterial.addTexture("roughness", "metal_roughness.png");
customMaterial.setRequirements({
    MaterialRequirement::Normals,
    MaterialRequirement::TangentSpace,  // Normal mapping needs tangents!
    MaterialRequirement::TextureCoordinates2D
});

// Geometry MUST provide tangent space
Geometry customGeometry;
customGeometry.setVertices(vertices);  // Must include tangents
customGeometry.setFormat(format_P_N_T_B_UV);  // Position, Normal, Tangent, Bitangent, UV

// Combine
auto renderable = Renderable::create(customGeometry, customMaterial);
// Saphir checks: Geometry has tangents? YES → generate shader with normal mapping
```

## Performance Considerations

### GPU Instancing

**When to use:**
- Many identical objects (same Geometry + Material)
- Only transforms differ between instances
- Objects visible in same frame

**Automatic optimization:**
```cpp
// User code (same as before)
for (int i = 0; i < 10000; ++i) {
    node->newVisual(crateRenderable);
}

// Renderer automatically:
// 1. Detects: 10000 instances of same Renderable
// 2. Batches transforms into GPU buffer
// 3. Issues single instanced draw call
// Result: 10000 crates rendered in ~1 draw call (huge performance win)
```

### Batching and Sorting

**Render queue sorting:**
1. **Opaque objects**: Front-to-back (minimize overdraw, early depth test)
2. **Transparent objects**: Back-to-front (correct blending order)
3. **State changes minimized**: Group by pipeline, material, geometry

**Automatic by Renderer:**
- Users don't manually batch or sort
- Renderer analyzes scene each frame
- Optimal draw order computed automatically

### Culling

**Frustum culling:**
- Renderer performs view frustum culling each frame
- Objects outside camera view skipped (not submitted to GPU)
- Implemented in CPU (cheap test before expensive GPU work)

**Occlusion culling (future):**
- Detect objects fully blocked by other objects
- Skip rendering if not visible
- Requires occlusion queries (Vulkan feature)

## Common Pitfalls and Best Practices

### ❌ Common Mistakes

1. **Forgetting tangent space for normal maps**
   ```
   Material: Uses normal map (needs TangentSpace)
   Geometry: Only has normals (NO tangents)
   → FAILURE - Add tangents or remove normal map
   ```

2. **Loading resources in render loop**
   ```cpp
   // BAD: Loading in loop
   for (int i = 0; i < 1000; ++i) {
       auto renderable = resources.get<RenderableResource>("tree");  // Loads 1000 times!
       node->newVisual(renderable);
   }

   // GOOD: Load once, reuse
   auto renderable = resources.get<RenderableResource>("tree");  // Load once
   for (int i = 0; i < 1000; ++i) {
       node->newVisual(renderable);  // Reuse 1000 times
   }
   ```

3. **Calling Vulkan directly from user code**
   ```cpp
   // NEVER DO THIS
   vkCmdBindPipeline(commandBuffer, ...);  // Breaks abstraction!

   // Instead: Use Graphics abstractions
   auto renderable = Renderable::create(geometry, material);  // Graphics handles Vulkan
   ```

### ✅ Best Practices

1. **Design Geometry and Material together**
   - Material declares requirements → Geometry provides attributes
   - Test compatibility early (logs explain mismatches)

2. **Leverage instancing automatically**
   - Reuse Renderables for duplicate objects
   - Renderer handles instancing optimization

3. **Use fail-safe resources**
   - Don't check for nullptr (resources never null)
   - Neutral resources provide visual feedback for errors

4. **Follow Y-down convention**
   - Never flip Y coordinates in code
   - Engine is Y-down throughout (Physics, Graphics, Audio)

5. **Trust the abstractions**
   - Don't try to "optimize" by calling Vulkan directly
   - Graphics system is designed for performance
   - Premature optimization = bugs

## Integration with Other Systems

### Physics

**Scene Graph Synchronization:**
- Physics updates node transforms (positions, rotations)
- Graphics reads transforms, updates RenderableInstance matrices
- No direct coupling (Scene mediates)

**Coordinate System:**
- Both use Y-down (gravity = +9.81 along Y)
- No conversion needed

### Audio

**3D Spatial Audio:**
- Audio uses node positions for sound spatialization
- SoundEmitter component reads node transform (same as Visual)
- Same coordinate system (Y-down)

### Resources

**Dependency Management:**
- Material depends on Textures (loaded first)
- Renderable depends on Geometry + Material (loaded after dependencies)
- Fail-safe: Missing dependencies → neutral resources

### Saphir

**Shader Generation:**
- Graphics provides Material + Geometry to Saphir
- Saphir generates GLSL, compiles to SPIR-V
- Graphics creates Vulkan pipeline with SPIR-V
- Seamless integration (user doesn't see Saphir details)

## Design Principles Summary

| Principle | Description | Benefit |
|-----------|-------------|---------|
| **High-Level Abstraction** | OpenGL-style declarative interface | Developer-friendly, less boilerplate |
| **Instancing Architecture** | Separate definition (Renderable) from usage (RenderableInstance) | Memory efficient, performance optimized |
| **Fail-Safe Resources** | Neutral resources for missing/broken assets | Application never crashes, visual debugging |
| **Automatic Management** | Observer pattern for registration, automatic batching/sorting | No manual bookkeeping, fewer bugs |
| **Strict Compatibility** | Material requirements must match Geometry attributes | Prevents rendering errors, clear diagnostics |
| **Y-Down Everywhere** | Consistent coordinate system across engine | No conversions, fewer bugs |
| **Vulkan Abstraction** | Never expose Vulkan details to user code | Maintainable, portable, safe |

### Core Philosophy

> "Provide an intuitive, high-level interface that handles Vulkan complexity internally, follows fail-safe principles, automatically optimizes rendering, and integrates seamlessly with the Scene, Resource, and Saphir systems."
