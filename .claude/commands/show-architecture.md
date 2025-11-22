---
description: Affiche diagramme/hiérarchie ASCII du système ou sous-système
---

Display ASCII architecture diagrams for Emeraude Engine systems.

**Task:**

If no subsystem specified, show **high-level engine architecture:**

```
Emeraude Engine Architecture

Application Layer
    ↓
┌─────────────────────────────────────────┐
│         High-Level Systems              │
├─────────────────────────────────────────┤
│ Scenes (Nodes, Components, SceneGraph) │
│ Graphics (Geometry, Material, Render)   │
│ Audio (3D Sound, Music, Effects)        │
│ Input (Keyboard, Mouse, Gamepads)       │
│ Overlay (2D UI, ImGui)                  │
└─────────────────────────────────────────┘
    ↓
┌─────────────────────────────────────────┐
│          Core Systems                   │
├─────────────────────────────────────────┤
│ Physics (4-entity simulation)           │
│ Resources (Fail-safe loading)           │
│ Saphir (Shader generation)              │
│ Vulkan (GPU abstraction)                │
│ Net (Download resources)                │
└─────────────────────────────────────────┘
    ↓
┌─────────────────────────────────────────┐
│         Foundation (Libs)               │
├─────────────────────────────────────────┤
│ Math (Vector, Matrix, CartesianFrame)  │
│ IO, Compression, ThreadPool             │
│ Observer/Observable, JSON               │
│ PixelFactory, VertexFactory, Wave      │
└─────────────────────────────────────────┘
    ↓
┌─────────────────────────────────────────┐
│      Platform & External Deps           │
├─────────────────────────────────────────┤
│ PlatformSpecific (OS isolation)         │
│ GLFW, OpenAL, VMA, libsndfile          │
└─────────────────────────────────────────┘
```

**If subsystem specified, show detailed architecture:**

Example for `/show-architecture graphics`:
```
Graphics System Architecture

Graphics::Renderer (Central coordinator)
    ├── TransferManager (CPU ↔ GPU)
    ├── LayoutManager (Pipeline layouts)
    ├── ShaderManager (Saphir integration)
    ├── SharedUBOManager (UBO sharing)
    └── VertexBufferFormatManager (Format registry)

Resource Types:
    Geometry/ (Vertex data, formats)
        ↓
    Material/ (Textures, properties)
        ↓
    Renderable/ (Geometry + Material)
        ↓
    RenderableInstance/ (Per-instance data)
        ↓ Used by
    Scenes::Visual Component

Integration:
    Resources → Geometry/Material loading
    Saphir → Shader generation (Material × Geometry)
    Vulkan → GPU buffers, pipelines, rendering
```

**Subsystems with detailed diagrams:**
- graphics, physics, scenes, resources, saphir
- audio, overlay, input

Read relevant AGENTS.md to extract architecture info. Create clear ASCII diagrams showing:
- Component hierarchy
- Data flow
- Key relationships
- Integration points
