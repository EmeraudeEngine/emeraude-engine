## Scene Class Organization

The Scene class is split into multiple implementation files by concept for easier navigation.

### Scene.hpp Structure (Declaration Order)

**Public Section:**
| Concept | Description |
|---------|-------------|
| Core/Lifecycle | Constructor, destructor, enable/disable, processLogics |
| Managers/Accessors | Accessors for managers (video, audio, physics, resources) |
| Entities | Node tree, static entities, modifiers |
| Rendering | Render targets (shadow maps, textures, views), rendering pipeline |
| Physics | Octree management, collision detection |
| Audio | Ambience management |
| Effects | Visual effects (fog, depth of field) |
| Debug Display | Statistics and debug visualization |

> [!IMPORTANT]
> **Post-process stack ownership (Jul 2026).** The `PostProcessStack` belongs to the Scene and
> dies with it, but the Scene is no longer the only one who may create it:
> - `setPostProcessStack()` — the APPLICATION hands over a chain of SCENE effects (GI, AO,
>   fog). It **destroys** the stack it replaces, so it may only be called while the scene is
>   being built, **before activation**. `Manager::newScene()` deliberately does not activate.
> - `requirePostProcessStack()` — the RENDERER lazily creates an empty one, once per frame, when
>   `Component::Camera::requiresPostProcessing()` says the active camera needs the pipeline.
>   Without this, `camera->enableHDR(true)` on a scene the application never gave a stack was a
>   silent no-op and the photometric radiance reached an LDR swap-chain (white/black screen).
>
> Both writers are safe **only** because they never overlap in time. Never call
> `setPostProcessStack()` on the active scene. See `Graphics/AGENTS.md` § Physical Camera.

**Private Section:**
| Concept | Description |
|---------|-------------|
| Observer | onNotification, checkRootNodeNotification, checkEntityNotification |
| Core/Lifecycle | initializeBaseComponents (⚠️ inspects nodes AND static entities for the camera/microphone — a fixed camera is a static entity; Aug 2026), suspendAllEntities, wakeupAllEntities |
| Entities | checkEntityLocationInOctrees |
| Rendering | Render list population, shadow casting, visual component iteration |
| Physics | sectorCollisionTest, leafSectorCollisionTest, boundary clipping |

### Implementation Files

| File | Concepts | Lines |
|------|----------|-------|
| `Scene.cpp` | Core/Lifecycle, Audio, Octree management | ~875 |
| `Scene.entities.cpp` | Entities (Node/StaticEntity), Observer notifications | ~540 |
| `Scene.lighting.cpp` | `applyBackgroundLighting()` (+ deferred `…Now()`), ambient refresh, CSM cascades, environment IBL | ~290 |
| `Scene.physics.cpp` | Modifiers, Collision detection, Boundary clipping | ~1225 |
| `Scene.rendering.cpp` | Render targets, Shadow casting, Rendering pipeline | ~1890 |
| `Scene.debug.cpp` | Debug displays (compass, ground zero, boundary planes, octrees) | ~340 |
| `Debug/Compass.cpp` | Orientation compass, recorded after the post-process chain | ~215 |

### Debug Helpers and the Exposure Trap

⚠️⚠️ **A debug helper drawn in the scene pass CANNOT keep its authored color.** The scene colour
buffer is an **absolute-luminance** buffer; `ToneMapping` multiplies everything in it by the camera
exposure (`hdrColor *= exposure`, times the auto-exposure factor). An exposure calibrated for a few
thousand nits — Sponza runs `f/11 · 1/250s` — crushes a `1.0` vertex color to black. Disabling
lighting does **not** save it: the former compass was already unlit (`EnableLighting` is off by
default) and still went dark. A reference whose colors depend on the camera settings measures
nothing.

**The contract for anything that must be read as authored** (compass, gizmos):

1. It does **not** live in the scene graph — no `StaticEntity`, no `Component::Visual`.
2. Its pipeline is compiled against `Renderer::overlayFramebuffer()` (which resolves to the
   swap-chain post-process framebuffer — the same in a window-less run, whose swap-chain is headless),
   **not** the scene render target's.
3. It is recorded **after** `PostProcessor::executeDirectPostProcessEffects()`, from the three
   sites in `Graphics/Renderer.cpp` that draw the editor gizmos. Recording it any earlier puts it
   back under the exposure multiply.
4. Depth test and write are disabled, culling is off — it is an instrument, it is always readable.

`Scene::renderDebugOverlay()` is the single entry point the renderer calls; it resolves the main
camera's view matrices from the first render-to-view target, exactly as `Editor::Manager` does for
its gizmos. `Debug::Compass` reuses `Saphir::Generator::GizmoRendering` (same need: unlit
vertex-colored geometry, no depth, no culling — one shader to maintain), and draws through the
**INFINITY** view matrix so the spheres state directions, not places.

**Consequence for the API:** `Scene::enableCompassDisplay()` alone is no longer enough to see the
compass — the renderer must call `renderDebugOverlay()`. Any new render path (a new frame-recording
function in `Renderer`) must add that call or the compass will silently not appear.

### Section Comments Format

Each concept section is marked with:
```cpp
/* ============================================================
 * [CONCEPT: NAME]
 * Description.
 * ============================================================ */
```

This allows quick navigation using search (e.g., `[CONCEPT: RENDERING]`).
