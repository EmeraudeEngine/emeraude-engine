# Render Targets & Off-Screen Rendering

This document details the architecture for rendering to off-screen targets, including textures, shadow maps, and cubemaps.

## Render Target Types

| Type | Vulkan Image Type | Use Case |
|---|---|---|
| **Texture** | `2D` | Security cameras, portals, mirrors (Color + Depth). |
| **ShadowMap** | `2D` | Depth-only rendering from light perspective. |
| **View** | `Swapchain` | Main window rendering. |
| **Cubemap** | `Cube` | 360° environment, reflection probes (6 faces). |

```cpp
// Example: Security camera feed
auto cameraFeed = RenderTarget::Texture::create(
    "security_camera", 512, 512, precisions, viewDistance, isOrthographic
);
material->setTexture("diffuse", cameraFeed->colorTexture());
```

## Architecture

### 1. Image Layout Management
Render targets transition layouts automatically during the frame:
1.  **UNDEFINED** → Initial state.
2.  **SHADER_READ_ONLY_OPTIMAL** → Before/After render (Reading as texture).
3.  **COLOR_ATTACHMENT_OPTIMAL** → During render (Writing).

### 2. GPU Synchronization
Main scene rendering must wait for off-screen rendering to complete.
- **Semaphores**: Each RT has a semaphore signaled on completion.
- **Wait Stages**: Main render waits at `FRAGMENT_SHADER_BIT` (sampling) for these semaphores.

### 3. Cubemap Rendering (Multiview)

**Goal**: Render 6 faces of a cubemap in a single pass.

**Traditional vs Multiview:**
- **Traditional**: 6 Render Passes. Expensive.
- **Multiview**: **1 Render Pass**. GPU broadcasts geometry to 6 layers.

**Implementation:**
- **Layered Framebuffer**: 6 layers (+X, -X, +Y, -Y, +Z, -Z).
- **Shader Access**: `gl_ViewIndex` identifies the face.
- **Uniform Blocks**: Array of 6 view matrices.

```glsl
// Uniform block layout
layout(std140, set = 0, binding = 0) uniform CubemapView {
    CubemapFace faces[6];         // Projection/View per face
    vec4 ambientLightColor;       // Shared data
} ubView;

// Access
mat4 projection = ubView.faces[gl_ViewIndex].projectionMatrix;
```

**Variable Synthesis:**
- `RenderPassContext` flags `isCubemap`.
- If `isCubemap`: Push constants disabled (matrices are in UBO array).
- Saphir generators adapted to use `gl_ViewIndex`.

### 4. Recursive Rendering
**Limitation**: A texture showing itself (e.g., TV showing camera filming TV) causes infinite recursion / read-after-write hazard.
**Status**: Triggers validation warning. Future fix: Double buffering.
