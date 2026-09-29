## Editor — selection, the two states, group transformation, the panel (2026-09-29)

Source: `src/Scenes/Editor/Manager.{hpp,cpp}`, `src/Overlay/Manager*.cpp` (click-through), `src/Core.cpp` (the panel
screen). Router: `src/Scenes/Editor/AGENTS.md`.

### Selection and the two editor states (owner decisions 2026-09-29)

The SELECTION is kept apart from the gizmo: the scene OUTLINES it (`Scene::setHighlightedEntities()`,
graphics doc 34), the gizmo is only a TOOL acting on it. `EditorState`:

| State | Click | Gizmo |
|---|---|---|
| **Selection** (default, Shift+Q) | click = replace, Shift+click = add / remove, click on nothing = clear | hidden |
| **Transformation** (Shift+T / R / S) | only the gizmo reacts; a click away from it changes NOTHING | the current `GizmoMode`, on the whole selection |

- The selection is a `std::vector< std::weak_ptr< AbstractEntity > >` under `m_selectionAccess` (input thread writes,
  logic thread reads), in selection order: the LAST one is the ACTIVE entity — its axes drive the Local space.
  `selection()`, `activeEntity()`; the former raw `AbstractEntity * m_selectedEntity` could dangle.
- Every change calls `publishSelection()` → `Scene::setHighlightedEntities()` (one step: the render thread never
  sees half a set).
- `m_state`, `m_gizmoMode`, `m_transformSpace` are atomics: keys (main thread), panel buttons (render thread) and
  `processLogics()` (logic thread) all touch them. `m_gizmoShown` tells `render()` whether to draw.

### Group transformation (owner decision: "par rapport au centre des objets sélectionnés")
- **Pivot** = the MEAN of the selected entities' world positions (`selectionPivot()`, Blender's "median point"). The
  gizmo sits there; Local orients it like the active entity, World resets its rotation.
- `beginDrag()` captures the pivot and, per DRAG TARGET, the initial position and scaling. An entity whose
  ancestor is also selected is left out (`hasSelectedAncestor()`): it follows its parent, it must not move twice.
- **Translate**: every target at `initialPosition + axis · delta` (absolute from the drag start).
- **Rotate**: each target turns ON ITSELF (`rotate(World)` + position restored), then its offset from the pivot
  turns by `Matrix< 3 >::rotation(delta, axis)` — the matrix `CartesianFrame::rotate()` uses, so spin and orbit
  agree by construction. ONE target in Local space keeps the exact local path (`rotate(unitAxis, Local)`).
  Measured on `geometry-generator`: yaw −71.2°, the active entity's position (0.707, 0, 0.707) → (−0.441, 0, 0.898),
  the same −71.2° about the centre, radius 1.000 → 1.0004.
- **Scale**: each target's scaling on the dragged component (or all, centre cube) × factor, and its offset from the
  pivot scaled along the dragged world direction (× factor for uniform). A rotated entity's per-axis scale is along
  its OWN axis, its offset along the drag's: a non-uniform group scale of rotated entities cannot be exact without
  shear (accepted).
- ⚠️ `Node::rotate(TransformSpace::World)` turns the node's LOCAL frame, i.e. in its PARENT's space: exact for a
  child of the root node only. Engine `docs/todo/editor-world-rotation-of-nested-nodes.md`.

### The panel (optional, replaceable — owner decision)
- `Editor::Manager` holds a draw function (`setPanel()`, `hasPanel()`, `drawPanel()`); Core's ImGUI screen
  `SceneEditorScreen` calls it and is visible exactly while the editor is active (synced every frame in the render
  loop, whoever toggled the editor).
- Default (`IMGUI_ENABLED` only): window "Scene editor", auto-resized — the four tool buttons (Select / Move / Rotate
  / Scale, the current one highlighted, shortcut in the tooltip), Local / World, the selection with the active entity
  marked, the active entity's world position, rotation (ZYX Tait-Bryan from `toQuaternion().eulerAngles()`) and
  scale READ-ONLY, the centre when several are selected.
- An application replaces it with `setPanel(myDraw)` or removes it with `setPanel(nullptr)` — at setup or from the
  render thread (the overlay pass calls it without a lock). Every panel drives the editor through the public API.
- ⚠️ Click-through: ImGUI reads the pointer through its OWN GLFW callbacks, in parallel with the engine's. The overlay
  latches `io.WantCaptureMouse` each ImGUI frame (`Overlay::Manager::m_ImGUICapturesPointer`) and CONSUMES a press or
  a wheel event over a window; a RELEASE is never consumed (a drag started in the scene must end). An INJECTED click
  (`InputManagerService.mouseClick`) reaches the engine callbacks only: it never presses an ImGUI button.
- ⚠️ `CartesianFrame::getPitchAngle()/getYawAngle()/getRollAngle()` are NOT Euler angles (angles between the backward
  axis and −Z/+X/+Y: 180/90/90 for an untouched entity): emeraude-base `docs/todo/cartesian-frame-angle-getters.md`.
