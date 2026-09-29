# Scene Editor System

Context for developing the Emeraude Engine scene editor (picking, gizmos, entity manipulation).

## Module Overview

Standalone editor overlay for real-time scene manipulation. Lives in `Scenes::Editor` namespace. Provides CPU raycasting-based entity picking, three interactive gizmos (translate, rotate, scale), and input handling. Renders in the post-process pass before the overlay. Gizmos are **NOT scene entities** — they manage their own GPU resources.

## Architecture

### Ownership Chain
- `Scenes::Manager` owns `Editor::Manager` — auto-deactivates editor on scene change/destroy
- `Editor::Manager` owns `Gizmo::Translate`, `Gizmo::Rotate`, `Gizmo::Scale` as direct members
- ALL gizmos are pre-created at editor activation (GPU resources uploaded once, reused for every selection)

### Activation Flow
```
Core: Shift+F3 → Scenes::Manager::toggleEditorMode()
  → Gets active scene + first render-to-view's ViewMatricesInterface
  → Editor::Manager::activate(scene, viewMatrices)
    → Registers as KeyboardListenerInterface + PointerListenerInterface
    → Unlocks pointer (absolute mode for clicking)
    → Pre-creates ALL gizmo GPU resources (translate + rotate + scale)
```

### Pipeline Hooks

| Thread | Hook | Location | Purpose |
|--------|------|----------|---------|
| Logic | `editorManager.processLogics()` | `Core::logicsTask()` after scene logic | Update gizmo position, rotation, screen scale |
| Render | `editorManager->render(commandBuffer)` | `Renderer::renderFrame*()` before overlay | Record active gizmo draw commands |

### Render Order
```
Scene (opaque → translucent) → Post-process → Editor Gizmos → Overlay (ImGui/CEF)
```

## Key Files

| File | Purpose |
|------|---------|
| `Manager.hpp/.cpp` | Main editor manager — input, picking, gizmo lifecycle, drag logic, mode switching |
| `Gizmo/Abstract.hpp/.cpp` | Gizmo base class — screen-scale, hit-test interface, world frame, AxisID enum |
| `Gizmo/Translate.hpp/.cpp` | Translation gizmo — cylinder+cone arrows, per-axis drag via closest-point-on-line |
| `Gizmo/Rotate.hpp/.cpp` | Rotation gizmo — torus rings, per-axis drag via angle projection on ring plane |
| `Gizmo/Scale.hpp/.cpp` | Scale gizmo — cylinder+cube branches + gray center cube for uniform scale |

## Picking System

- **Method**: CPU raycasting via `screenToWorldRay()` (inverse VP matrix, perspective divide)
- **Coordinate space (CRITICAL)**: Pointer events are dispatched in **physical framebuffer pixels** on every platform (`Core::updatePointerScaling()` scales Wayland/macOS logical coords). `screenToWorldRay()` therefore reads the **main render target extent at event time** — never the logical window size (off by the content scale on HiDPI: broken picking, regression fixed 2026-07-09) and never a size captured at activation (stale after a window resize)
- **Targets**: Both `Node` and `StaticEntity` with collision models (AABB or Sphere)
- **Intersection**: `Segment-Sphere` and `Segment-AABB` from `Base/Math/Space3D/Intersections/`
- **Selection**: Closest hit by distance to camera
- **Priority**: Active gizmo hit-test checked BEFORE scene picking
- **Camera-carrier exclusion**: Any entity whose collision volume CONTAINS the camera position is skipped (`isColliding(point, volume)` from `Base/Math/Space3D/Collisions/`). Without this, the entity carrying the active camera (e.g., the player actor) catches every click at near-zero distance since the ray origin sits inside its AABB

## Selection, editor states, group transformation, panel (2026-09-29)

→ [`docs/subsystems/scenes/25-editor-selection-states-group-transform-panel.md`](../../../docs/subsystems/scenes/25-editor-selection-states-group-transform-panel.md):
Selection / Transformation states (Shift+Q, Shift+T/R/S), the multi-selection (Shift+click) outlined by the scene,
transformation around the centre of the selection, the replaceable ImGUI panel (`setPanel()`) and its click-through.

## Gizmo System

### Three Gizmo Types

| Gizmo | Visual | Drag Behavior |
|-------|--------|---------------|
| **Translate** | 3 RGB cylinder+cone arrows (gap from center) | Closest-point-on-line projection, `setPosition()` |
| **Rotate** | 3 RGB torus rings (64 slices) | Angle projection on ring plane, `rotate()` |
| **Scale** | 3 RGB cylinder+cube branches + gray center cube | Horizontal+vertical mouse delta, `setScalingFactor()` |

### Standalone Rendering
- Geometry: built manually via `ResourceGenerator` (cylinder, cone, torus, cube) — NO `arrow()` (it adds unwanted white sphere)
- Shader: `Saphir::Generator::GizmoRendering` — vertex color passthrough + highlightFactor
- Pipeline: depth test OFF, depth write OFF, no face culling, alpha blending
- Push constants: mat4 MVP + float frameIndex + float highlightFactor (72 bytes)
- Each sub-element rendered independently (per-axis highlight support)
- `isCreated()` checked before binding geometry (GPU upload may be deferred)

### Constant Screen Size
```
scaleFactor = screenRatio * distance * tan(FOV/2)
```
- `screenRatio`: configurable via `Editor::Manager::setGizmoScreenRatio()` (default `Gizmo::Abstract::DefaultScreenRatio`
  = 0.3: one unit of gizmo length over that fraction of the viewport half-height)
- Updated every frame in `processLogics()`, with the FOV in RADIANS
- ⚠️⚠️ `ViewMatricesInterface::fieldOfView()` returns DEGREES. Fed raw until Sep 2026: tan(42.5 rad) = −11.3 at 85°, a
  NEGATIVE scale that point-mirrored the whole gizmo (X and Y arrows reversed on screen, against the compass; the
  hit-test used the same scale, so the reversed arrows were the clickable ones, but a drag moved AGAINST the arrow
  held) and made its size swing with the FOV. The old default 0.025 × |tan(42.5 rad)| / tan(42.5°) = 0.307 → 0.3
  keeps the size it had at 85°.

### Hit-Test
- **Translate/Scale**: World-space AABBs built from gizmo position + screenScale per axis
- **Rotate**: Flat AABBs per ring plane (thin on rotation axis, wide on ring plane)
- **Scale center cube**: `AxisID::All` — highlights entire gizmo, triggers uniform scale

### Transform Spaces
- **Local** (default): Gizmo aligns with the ACTIVE entity's orientation. One target: rotation uses
  `TransformSpace::Local` with unit axes; a group: see § Group transformation.
- **World**: Gizmo aligned to world axes. Rotation uses `TransformSpace::World` + save/restore position (avoids
  orbit), then the offset from the pivot turns.
- **Parent**: Enum exists, implementation pending.

### Drag Mechanics

**Translation**: `projectMouseOnAxis()` — closest point between mouse ray and world-space axis line.

**Rotation**: `projectMouseAngleOnPlane()` — ray-plane intersection + `atan2` for angle. Incremental delta applied per frame.

**Scale**: Horizontal mouse delta (right=bigger) + vertical delta (up=bigger). Absolute scaling from initial values stored at drag start. Uses `setScalingFactor()` on `LocatableInterface`.

## Input Handling

### Key Bindings (handled in `Editor::Manager::onKeyPress()`)
| Key | Action |
|-----|--------|
| Shift+F3 | Toggle editor mode (handled at Core level) |
| Shift+G | Toggle transform space (Local ↔ World) |
| Shift+Q | Selection state (no gizmo) |
| Shift+T | Transformation state, Translation gizmo |
| Shift+R | Transformation state, Rotation gizmo |
| Shift+S | Transformation state, Scale gizmo |
| Escape | Clear the selection |

### Mouse
- **Hover**: `onPointerMove()` does lightweight hit-test on active gizmo, updates `highlightedAxis` (gizmo shown only)
- **Selection state**: click picks (CPU raycasting) and replaces the selection, Shift+click adds / removes, a click on
  the void clears
- **Transformation state**: a click on the gizmo starts a drag of the whole selection (axis-specific, or uniform for
  the scale centre); anywhere else, nothing
- **Drag**: Applies transformation based on active gizmo mode
- **Release**: Ends drag

## Engine API Additions

- `LocatableInterface::setScalingFactor(Vector3)` — non-pure virtual, implemented in Node, StaticEntity, Particle
- `CartesianFrame::scalingFactor()` — getter (already existed)
- `Gizmo::AxisID::All` — for uniform scale hit-test

## Critical Rules

1. **Gizmos are NOT scene entities** — own GPU resources (VBO, pipeline, program), zero scene graph pollution
2. **ALL gizmos pre-created at activation** — no lazy creation, avoids GPU upload race conditions
3. **Scenes::Manager protects the editor** — `disableActiveScene()` auto-deactivates editor first
4. **Copy-before-iterate** — Input injection copies listener lists before dispatch (prevents UB from listener registration during callbacks)
5. **GizmoRendering generator needs geometry flags** — Must pass `Topology::TriangleList` + `EnableVertexColor`
6. **World rotation save/restore position** — `rotate(TransformSpace::World)` orbits; save position before, restore after
7. **Local rotation uses unit axes** — Pass `(1,0,0)` with `TransformSpace::Local`, NOT the already-transformed world vector
8. **Arrow directions are POSITIVE** — the gizmo arrows are authored along +X/+Y/+Z, and the hit-test AABBs extend in positive directions. The mirror-era negative-axis compensation ("projection pipeline inversion") died with the Y-up flip (stage 4). ⚠️ They did NOT display positive until Sep 2026: the degrees-as-radians FOV gave a negative screen scale (§ Constant Screen Size). Verified since against the compass (`toggleCompass`): red X+ arrow toward the red sphere, green Y+ up.

## Future Work
- Plane handles for 2-axis translation (XY, YZ, XZ)
- Torus arc segments (partial ring for better visual)
- Undo/redo system for transformations
- Stepping / snap-to-grid modes
- Parent transform space implementation
