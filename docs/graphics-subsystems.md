# Graphics Subsystems

This document details the internal managers and subsystems that orchestrate the rendering process under the `Renderer` class.

## Renderer: The Central Coordinator

The **Renderer** is the heart of the Graphics system. It coordinates all rendering operations and manages the subsystems.

**Responsibilities:**
1. Coordinate all rendering operations
2. Manage frame lifecycle (begin frame, render, end frame, present)
3. Integrate subsystems (TransferManager, LayoutManager, ShaderManager, etc.)
4. Maintain render queues (opaque, transparent, shadows, post-processing)
5. Observe scene changes (Visual components added/removed via Observer pattern)
6. Execute render passes in correct order
7. Manage synchronization (fences, semaphores for frame pacing)

## Subsystems

### 1. TransferManager: CPU ↔ GPU Data Movement

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

```cpp
// Uploading vertex data
transferManager.uploadToBuffer(
    vertices.data(),
    vertices.size() * sizeof(Vertex),
    vertexBuffer,
    []() { Log::info("Vertices uploaded!"); }
);
```

### 2. LayoutManager: Vulkan Pipeline Layout Management

**Purpose:** Centralize and reuse Vulkan pipeline layouts across the engine.

**Why?**
- Vulkan requires explicit descriptor set layouts and pipeline layouts
- Many shaders share similar layouts (e.g., all PBR shaders)
- Reusing layouts improves performance and reduces memory

**Operations:**
- Register common layout patterns (standard PBR, shadow, overlay)
- Return existing layout if pattern matches (cache lookup)
- Create new layout if unique pattern (cache miss)

### 3. ShaderManager (Saphir Integration)

**Purpose:** Interface between Graphics and Saphir for automatic shader generation.

**Process:**
1.  **Input:** Material requirements + Geometry attributes + Scene context
2.  **Generate:** Calls appropriate Saphir generator (Scene/Overlay/Shadow)
3.  **Compile:** GLSL → SPIR-V via `glslang`
4.  **Create:** Vulkan pipeline with SPIR-V

**Key Features:**
- Transparent Saphir integration (Graphics doesn't know about GLSL details)
- Caching of compiled shaders (avoids redundant work)
- Handles compilation errors gracefully (logs, falls back to neutral)

### 4. SharedUBOManager: Uniform Buffer Sharing

**Purpose:** Share uniform buffer objects (UBOs) between resources that use the same data.

**Shared Data Categories:**
1. **Per-frame data**: View matrix, projection matrix, camera position, time
2. **Lighting data**: Light positions, colors, directions, shadow matrices
3. **Material constants**: Shared material properties

**Key Features:**
- Automatic deduplication (same data → same UBO)
- Efficient updates (update once per frame, not per object)

### 5. VertexBufferFormatManager: Format Registry

**Purpose:** Centralize vertex format definitions and enable reuse.

**Why?**
- Many geometries use the same vertex format (e.g., position + normal + UV)
- Vulkan requires explicit format description
- Saphir needs to query formats for shader generation

**Registry:**
```cpp
FormatID format_P_N_UV = manager.registerFormat({
    {0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position)},
    {1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal)},
    {2, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv)}
});
```

## Dynamic Viewport and Scissor

### Window Resize Optimization

The Graphics system uses **dynamic viewport and scissor states** to avoid pipeline recreation on resize:

```cpp
// Pipeline configuration (at creation time)
graphicsPipeline.configureDynamicStates({
    VK_DYNAMIC_STATE_VIEWPORT,
    VK_DYNAMIC_STATE_SCISSOR
});
```

**Benefits:**
- **Resize performance**: Smooth (no recreation) vs Stutters.
- **Code complexity**: Simple, no refresh logic needed.

**Why It Works:**
Viewport/scissor dimensions are NOT part of RenderPass compatibility. Changing dimensions allows reusing the same compatible pipeline.
