# Scene Editor System

Context for developing the Emeraude Engine scene editor (picking, gizmos, entity manipulation).

## Module Overview

Standalone editor overlay for real-time scene manipulation. Lives in `Scenes::Editor` namespace. Provides CPU raycasting-based entity picking, standalone gizmo rendering (not scene entities), and input handling. Renders in the post-process pass before the overlay.

## Architecture

### Ownership Chain
- `Scenes::Manager` owns `Editor::Manager` — auto-deactivates editor on scene change/destroy
- `Editor::Manager` owns `Gizmo::Translate` (and future Rotate/Scale) as direct members
- Gizmos are **standalone**: own geometry, shader program, pipeline — NOT scene entities

### Activation Flow
```
Core: Shift+F3 → Scenes::Manager::toggleEditorMode(viewportW, viewportH)
  → Gets active scene + first render-to-view's ViewMatricesInterface
  → Editor::Manager::activate(scene, viewMatrices, w, h)
    → Registers as KeyboardListenerInterface + PointerListenerInterface
    → Unlocks pointer (absolute mode for clicking)
    → Pre-creates gizmo GPU resources (geometry + shader program + pipeline)
```

### Pipeline Hooks

| Thread | Hook | Location | Purpose |
|--------|------|----------|---------|
| Logic | `editorManager.processLogics()` | `Core::logicsTask()` after scene logic | Update gizmo position + screen scale |
| Render | `editorManager->render(commandBuffer)` | `Renderer::renderFrame*()` before overlay | Record gizmo draw commands |

### Render Order
```
Scene (opaque → translucent) → Post-process → Editor Gizmos → Overlay (ImGui/CEF)
```

## Key Files

| File | Purpose |
|------|---------|
| `Manager.hpp/.cpp` | Main editor manager — input listener, picking, gizmo lifecycle, mode switching |
| `Gizmo/Abstract.hpp/.cpp` | Gizmo base class — screen-scale, hit-test interface, world frame |
| `Gizmo/Translate.hpp/.cpp` | Translation gizmo — 3 colored arrows via `ResourceGenerator::axis()` |

## Picking System

- **Method**: CPU raycasting via `screenToWorldRay()` (inverse VP matrix, perspective divide)
- **Targets**: Both `Node` and `StaticEntity` with collision models (AABB or Sphere)
- **Intersection**: `Segment-Sphere` and `Segment-AABB` from `Libs/Math/Space3D/Intersections/`
- **Selection**: Closest hit by distance to camera
- **Priority**: Gizmo hit-test checked BEFORE scene picking

## Gizmo System

### Standalone Rendering
- Geometry: `ResourceGenerator::axis()` (3 colored arrows + origin sphere)
- Shader: `Saphir::Generator::GizmoRendering` (vertex color passthrough, MVP push constant)
- Pipeline: depth test OFF, depth write OFF, no face culling, alpha blending
- Push constants: mat4 MVP + float frameIndex (68 bytes)
- Rendered directly into command buffer (not through scene batching)

### Constant Screen Size
```
scaleFactor = screenRatio * distance * tan(FOV/2)
```
- `screenRatio`: configurable via `Editor::Manager::setGizmoScreenRatio()` (default: 0.025)
- Updated every frame in `processLogics()`

### Hit-Test
- Each axis = elongated AABB in gizmo local space
- Ray transformed to local space (inverse position + inverse scale)
- Hit tolerance: `HitRadius = 0.08` in local units

## Input Handling

### Key Bindings (handled in `Editor::Manager::onKeyPress()`)
| Key | Action |
|-----|--------|
| Shift+F3 | Toggle editor mode (handled at Core level) |
| Shift+G | Switch to Translate gizmo |
| Shift+R | Switch to Rotate gizmo (TODO) |
| Shift+S | Switch to Scale gizmo (TODO) |
| Escape | Deselect current entity |

### Mouse
- Left click: Pick entity (or gizmo axis if gizmo active)
- Drag on gizmo axis: Transform entity (TODO)

## Critical Rules

1. **Gizmos are NOT scene entities** — they manage their own GPU resources (VBO, pipeline, program)
2. **Scenes::Manager protects the editor** — `disableActiveScene()` auto-deactivates editor first
3. **Copy-before-iterate** — Input injection copies listener lists before dispatch to prevent UB from listener registration during callbacks
4. **GizmoRendering generator needs geometry flags** — Must pass `Topology::TriangleList` + `EnableVertexColor` to the constructor (not the renderableInstance constructor)

## Future Work
- Drag interaction on gizmo axes (translate entity along axis)
- Hover highlight (axis color change on mouse over)
- Rotate and Scale gizmo implementations
- Stepping / freehand modes
- Gizmo Local/World space toggle
