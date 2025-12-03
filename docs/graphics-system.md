# Graphics System Architecture

**High-level rendering abstraction layer built on Vulkan.**

## 1. Core Concept: Declarative Rendering

Instead of managing Vulkan state manually, users declare **WHAT** to render:

```
Geometry (Shape) + Material (Look) = Renderable (Object)
```

The system automatically manages:
-   Vulkan Pipelines & Shaders (via [Saphir](saphir-shader-system.md))
-   Memory & Synchronization
-   Instancing & Batching

## 2. Instancing Hierarchy

| Component | Role | Data |
|---|---|---|
| **Geometry** | The Mesh | Vertices, Indices, Format. |
| **Material** | The Look | Textures, Parameters (Roughness, Color). |
| **Renderable** | The Definition | Unique `Geo + Mat` combo. Owns the Pipeline. |
| **Instance** | The Usage | Transform, Flags. |
| **Visual** | The Scene Node | Connects `Instance` to Scene Graph. |

> **Details:** [`docs/renderable-instance-system.md`](renderable-instance-system.md)

## 3. Architecture Map

### The Coordinator
The **Renderer** manages the frame lifecycle, queues, and subsystems.
> **Deep Dive:** [`docs/graphics-subsystems.md`](graphics-subsystems.md)
> *Covers: TransferManager, LayoutManager, SharedUBO, VertexFormats.*

### Render Targets
Support for off-screen rendering, shadow maps, and cubemaps.
> **Deep Dive:** [`docs/render-targets.md`](render-targets.md)
> *Covers: RenderTextures, Image Layouts, Multiview.*

### Scene Integration
**Visual Component (`Visual`)**:
-   Connects Scene Nodes to Graphics.
-   **Observer Pattern**: Automatic registration with Renderer when added to scene.
-   **Lifecycle**: Update Transform (Scene) -> Update Instance (Graphics).

### Texture System
-   **Types**: 2D, 3D (Volumetric), Cubemap.
-   **Resources**: Loaded asynchronously via Resource system.
-   **Fail-Safe**: Returns neutral magenta texture if missing.

## 4. Key Workflows

### Creating Objects
```cpp
// 1. Get Resources
auto geo = resources.get<GeometryResource>("cube");
auto mat = resources.get<MaterialResource>("wood");

// 2. Define Object (Pipeline created here)
auto renderable = Renderable::create(geo, mat);

// 3. Place in Scene (Instance created here)
node->newVisual(renderable);
```

### Instancing
Reuse the `renderable` for multiple nodes. The engine automatically batches them into hardware instancing calls if compatible.

## 5. Critical Constraints

1.  **Coordinate System**: **Y-DOWN**. Never flip Y in shaders or transforms.
2.  **Resource Loading**: Never in the render loop. Load once, reuse.
3.  **Vulkan Access**: Never call `vk*` functions directly. Use the abstraction.
4.  **Tangent Space**: Normal maps require Geometry with Tangents.

## 6. Performance
-   **Sorting**: Opaque (Front-to-Back), Transparent (Back-to-Front).
-   **Culling**: Frustum culling on CPU.
-   **Resize**: Dynamic Viewport/Scissor prevents pipeline recreation.
