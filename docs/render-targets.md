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
- **Shader Access**: `gl_ViewIndex` (0-5) identifies the face.
- **Vulkan Extension**: `VK_KHR_multiview` + `GL_EXT_multiview` GLSL extension.

**UBO Layout:**
```glsl
struct CubemapFace {
    mat4 viewMatrix;              // Per-face view matrix
};

layout(std140, set = 0, binding = 0) uniform CubemapView {
    CubemapFace instance[6];      // Per-face data (indexed by gl_ViewIndex)
    mat4 projectionMatrix;        // Shared: 90° FOV, 1:1 aspect
    vec4 positionWorldSpace;      // Shared: camera position
    vec4 ambientLightColor;       // Shared: lighting
    float ambientLightIntensity;
} ubView;

// Shader access pattern
mat4 view = ubView.instance[gl_ViewIndex].viewMatrix;  // Per-face
mat4 proj = ubView.projectionMatrix;                    // Shared
```

**Matrix Sources in Cubemap Mode:**

| Matrix | Source | Reason |
|--------|--------|--------|
| Projection | UBO (shared) | Same 90° FOV for all faces |
| View | UBO (indexed) | Different orientation per face |
| Model | Push Constant | Per-object transform |

**Push Constant in Cubemap Mode:**
```glsl
// Cubemap mode: ONLY model matrix
layout(push_constant) uniform Matrices {
    mat4 modelMatrix;  // NOT modelViewProjectionMatrix!
} pcMatrices;
```

**Shader MVP Computation:**
```glsl
// Cubemap mode: combine from UBO + push constant
mat4 MVP = ubView.projectionMatrix
         * ubView.instance[gl_ViewIndex].viewMatrix
         * pcMatrices.modelMatrix;
```

**CPU-Side Coordination:**
See `RenderableInstance/Unique.cpp:pushMatricesForRendering()` - pushes only `modelMatrix` when `passContext.isCubemap` is true.

**Saphir Generator Adaptation:**
- `RenderPassContext.isCubemap` flag propagates to shader generators
- `VertexShader.isCubemapModeEnabled()` gates matrix source selection
- All code using view matrix must check: UBO (`ViewUB`) vs push constant (`MatrixPC`)
- See: `src/Saphir/AGENTS.md` section "Cubemap Rendering Mode"

### 4. Shadow Map Render Targets

Shadow maps are depth-only render targets created per light.

**Types by Light:**
- **Directional/Spot:** 2D depth texture
- **Point:** Cubemap depth texture (6 faces)

**Global Control:**
Shadow mapping can be globally disabled via `GraphicsShadowMappingEnabledKey` setting. When disabled:
1. `Scene::renderShadowMaps()` returns early
2. Shadow map images remain in `VK_IMAGE_LAYOUT_UNDEFINED`
3. Lighting passes use `NoShadow` pass types (no shadow descriptor binding)

**Critical:** If shadow maps are not rendered but their descriptors are bound, Vulkan validation errors occur due to layout mismatch.

See [`docs/shadow-mapping.md`](shadow-mapping.md) for complete shadow mapping architecture.

### 5. Recursive Rendering
**Limitation**: A texture showing itself (e.g., TV showing camera filming TV) causes infinite recursion / read-after-write hazard.
**Status**: Triggers validation warning. Future fix: Double buffering.
