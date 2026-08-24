# Scene Graph System

Context for developing the Emeraude Engine hierarchical scene graph system.

## Module Overview

Scene graph system based on composition architecture (Entity-Component) with two entity types: hierarchical dynamic **Nodes** and optimized flat **StaticEntities**. Double buffering for thread-safety between simulation and rendering.

## Scenes-Specific Rules

### Philosophy: Composition Over Inheritance
- **Generic entities**: Node and StaticEntity are position containers
- **Components give meaning**: Visual, Light, Camera, SoundEmitter, etc.
- **NEVER subclass**: Use Component composition instead of Player extends Node
- **Maximum flexibility**: Add/remove behaviors dynamically

### Architecture: Two Entity Types

**Node (Dynamic)**: Hierarchical tree with physics, parent-relative transforms
**StaticEntity (Static)**: Optimized flat map, no physics, absolute transforms

See @docs/scene-graph-architecture.md for complete details.

### Coordinate Convention
- **Y-UP mandatory** in CartesianFrame — `localYAxis()` is `m_upward`, and `downwardVector()` is its INVERSE (they are opposites, not aliases)
- Local transforms for Nodes (parent-relative)
- World space recalculated on demand (no cache currently)

### Available Components
**Rendering:** Visual, MultipleVisuals
**Lights:** DirectionalLight, PointLight, SpotLight
**Audio:** SoundEmitter, Microphone
**Physics:** DirectionalPushModifier, SphericalPushModifier, Weight
**Utilities:** Camera, ParticlesEmitter

### Editor Subsystem

Standalone scene editor for entity picking and gizmo manipulation. See [`Editor/AGENTS.md`](Editor/AGENTS.md).

- **Owned by**: `Scenes::Manager` (auto-deactivates on scene change)
- **Namespace**: `Scenes::Editor` (Manager) + `Scenes::Editor::Gizmo` (Abstract, Translate)
- **Activation**: Shift+F3 via Core → `Scenes::Manager::toggleEditorMode()`
- **Rendering**: Standalone pipeline (not scene entities), renders before overlay

### Level Interfaces (Ground & Sea)

Two interfaces define scene-wide physical levels for gameplay queries:

| Interface | Purpose | Implementations |
|-----------|---------|-----------------|
| `GroundLevelInterface` | Ground/terrain queries | `BasicGroundResource`, `TerrainResource` |
| `SeaLevelInterface` | Water surface queries | `BasicSeaResource` |

**GroundLevelInterface** (`Scenes/GroundLevelInterface.hpp`):
- `getLevelAt(worldPosition)` - Ground height at position
- `getLevelAt(x, z, deltaY)` - Returns position with Y = ground level + delta
- `getNormalAt(worldPosition)` - Surface normal at position
- `updateVisibility(cameraPosition)` - LOD/visibility hint

**SeaLevelInterface** (`Scenes/SeaLevelInterface.hpp`):
- `getLevel()` - Constant water height
- `getLevelAt(worldPosition)` - Water height at position (flat = constant)
- `getLevelAt(x, z, deltaY)` - Returns position with Y = water level + delta
- `getNormalAt(worldPosition)` - Water surface normal (flat = {0,1,0})
- `isSubmerged(worldPosition)` - True if position.Y < water level
- `getDepthAt(worldPosition)` - Depth below water (positive = submerged)
- `updateVisibility(cameraPosition)` - Visibility hint

**Scene accessors:**
```cpp
scene->groundPhysics()     // Returns GroundLevelInterface*
scene->seaLevelPhysics()   // Returns SeaLevelInterface*
```

**Code references:**
- `Scenes/GroundLevelInterface.hpp` - Ground interface definition
- `Scenes/SeaLevelInterface.hpp` - Sea level interface definition
- `Graphics/Renderable/BasicGroundResource.hpp` - Flat ground implementation
- `Graphics/Renderable/BasicSeaResource.hpp` - Flat water implementation
- `Graphics/Renderable/TerrainResource.hpp` - Heightmap terrain implementation

### Modifier System & Influence Areas

Modifiers (DirectionalPushModifier, SphericalPushModifier) apply forces to entities within their influence area.

**Influence Area Types:**
- `SphericalInfluenceArea`: Sphere with inner/outer radius for falloff. See `SphericalInfluenceArea.cpp`
- `CubicInfluenceArea`: Oriented box with local space transformation. See `CubicInfluenceArea.cpp`

**Modifier API (Semantic Dispatch):**

Two overloads with clear semantic separation:

```cpp
// For entities (Node, StaticEntity) - encapsulates collision model lookup
Vector<3,float> getForceAppliedTo(const LocatableInterface& entity) const noexcept;

// For particles/points - direct position with optional bounding radius
Vector<3,float> getForceAppliedTo(const CartesianFrame<float>& worldPosition, float radius = 0.0F) const noexcept;
```

**Entity overload internals** - Dispatches based on `CollisionModelType`:
- `Point` → uses `influenceStrength(position)` (point-based)
- `Sphere` → creates Sphere from `getRadius()`, uses Sphere overload
- `AABB/Capsule` → uses `getAABB(worldCoordinates)`, uses AACuboid overload
- No collision model → fallback to point-based

**Particle/Point overload**:
- `radius > 0.0F` → creates Sphere on the fly
- `radius == 0.0F` (default) → uses point-based influence

**Influence Area Interface:**

Three overload families for different use cases:
```cpp
// Bounding volume tests (entities with collision models)
float influenceStrength(const CartesianFrame<float>&, const Sphere<float>&);
float influenceStrength(const CartesianFrame<float>&, const AACuboid<float>&);

// Point test (particles, fallback for entities without collision)
float influenceStrength(const Vector<3,float>& worldPosition);
```

**How modifiers work:**
1. `Scene::forEachModifiers()` iterates all modifiers
2. For entities: calls `modifier->getForceAppliedTo(*this)` - entity passed directly
3. For particles: calls `modifier->getForceAppliedTo(worldCoordinates, m_size * 0.5F)` - radius passed
4. Modifier internally dispatches to correct `influenceStrength()` overload
5. Returns force vector applied to entity's physics

**Code references:**
- `InfluenceAreaInterface.hpp` - Pure virtual interface (Sphere, AABB, Point overloads)
- `SphericalInfluenceArea.cpp:influenceStrength()` - Distance-based falloff (inner/outer radius)
- `CubicInfluenceArea.cpp:influenceStrength()` - Local space box containment test
- `AbstractModifier.hpp:getForceAppliedTo()` - Virtual interface (entity vs particle)
- `SphericalPushModifier.cpp:getForceAppliedTo()` - Radial force with type dispatch
- `DirectionalPushModifier.cpp:getForceAppliedTo()` - Directional force with type dispatch
- `Node.cpp:879` - Entity call site (passes `*this`)
- `Particle.cpp:404` - Particle call site (passes `worldCoordinates, m_size * 0.5F`)

**Future improvement:** Modifiers should be integrated into physics octree for O(log n) lookups instead of O(n) iteration.

### Observer System
- **Automatic registration**: Scene observes Component additions
- Visual → rendering registration
- Camera/Microphone → AVConsole registration
- Lights → LightSet registration
- **NEVER manual registration**

### Spatial Optimization
- **Octrees per Scene**: One for physics, one for rendering
- **Frustum culling**: Active during tree traversal. **Sprites are excluded** from frustum culling because billboard rotation (vertex shader) changes the screen-space extent, but culling uses CPU-side AABB from the flat quad geometry (Z=0). See: `Scene.rendering.cpp` frustum check.
- **Depth limit**: `DefaultMaxDepth` (16 levels) prevents infinite subdivision when entities cluster
- Future optimization: Culling by Octree sector

## Development Commands

```bash
# Scene graph tests
ctest -R Scenes
./test --filter="*Scene*"
```

## Important Files

- `Manager.cpp/.hpp` - SceneManager, multiple Scenes management + ActiveScene
- `Scene.hpp` - Scene class declaration (~2260 lines), organized by concept
- `Scene.cpp` - Core lifecycle, audio, octree management
- `Scene.entities.cpp` - Node tree, static entities, modifiers
- `Scene.lighting.cpp` - `applyBackgroundLighting()` and its deferred apply, ambient light properties, CSM cascades, environment IBL
- `Scene.physics.cpp` - Collision detection, boundary clipping, sleep/wake collision. See [`@Physics/AGENTS.md`](../Physics/AGENTS.md) for normal convention
- `Scene.rendering.cpp` - Render targets, shadow casting, rendering pipeline
- `Scene.debug.cpp` - Debug displays (compass, ground zero, boundary planes, octrees)
- `Debug/Compass.cpp/.hpp` - Orientation compass, drawn **after** the post-process chain. ⚠️ **NOT a scene entity** — see "Debug Helpers and the Exposure Trap"
- `Node.cpp/.hpp` - Hierarchical dynamic entity (tree)
- `NodeCrawler.hpp` - Header-only tree iterator. ⚠️ **Never yields the base node** — see "Node Tree Iteration — NodeCrawler Contract"
- `StaticEntity.cpp/.hpp` - Optimized static entity (flat map)
- `AbstractEntity.cpp/.hpp` - Common base for Component management
- `LocatableInterface.cpp/.hpp` - Interface for coordinates/movement
- `Toolkit.cpp/.hpp` - High-level scene construction helper. See [`@docs/toolkit-system.md`](../../docs/toolkit-system.md)
- `Component/Abstract.hpp` - Base class for all Components (pure virtual onSuspend/onWakeup)
- `Component/SoundEmitter.cpp/.hpp` - Audio emitter with suspend/wakeup source management
- `InfluenceAreaInterface.hpp` - Pure virtual interface for modifier influence zones
- `SphericalInfluenceArea.cpp/.hpp` - Spherical influence with inner/outer radius falloff
- `CubicInfluenceArea.cpp/.hpp` - Oriented box influence with local space transform
- `Component/SphericalPushModifier.cpp/.hpp` - Radial push force modifier
- `Component/DirectionalPushModifier.cpp/.hpp` - Directional push force modifier
- `@docs/scene-graph-architecture.md` - **Complete detailed architecture**
- `@docs/coordinate-system.md` - Y-up convention (CRITICAL)

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
| Core/Lifecycle | initializeBaseComponents, suspendAllEntities, wakeupAllEntities |
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
   swap-chain post-process framebuffer, or the windowless view's framebuffer in windowless mode),
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

## Node Tree Iteration — NodeCrawler Contract

`NodeCrawler< node_t >` (`Scenes/NodeCrawler.hpp`) is the only way the Scene walks the node tree —
**12 call sites**: `Scene.cpp` ×2, `Scene.entities.cpp` ×5, `Scene.rendering.cpp` ×5. The API is
`bool fetchNextNode()` plus the accessor `const std::shared_ptr< node_t > & currentNode()`; it
replaced `std::shared_ptr< node_t > nextNode()`.

> [!CAUTION]
> **⚠️⚠️ The iteration NEVER yields the base node.**
> - **Before** the first `fetchNextNode()`, `currentNode()` **is the base node** — the constructor
>   seats it through `populateStack()`.
> - **After** `fetchNextNode()` returns `false`, `currentNode()` is `nullptr` — so a while-loop body
>   never sees a null node.
>
> Consequence for callers, and this is the whole point of the contract:
> ```cpp
> while ( crawler.fetchNextNode() ) { use(crawler.currentNode()); }   // DESCENDANTS ONLY
> ```
> A caller that must also process the base node handles it **before** the loop. Two sites in
> `Scene.entities.cpp` do exactly that: **`getNodeStatistics()`** (the root's children count and
> depth) and the `showTree` dump in **`getNodeSystemStatistics()`** (the root's own line). Both were
> `do/while` loops and are now "process the base node, then `while (...)`", with the body factored
> into a lambda so nothing is duplicated, each carrying a ⚠️ comment on the pre-loop call. The
> conversion was forced by clang-tidy's `cppcoreguidelines-avoid-do-while`; the contract itself did
> not change, only the shape of those two call sites.

### ⚠️⚠️ Caution: The Stale-Local Bug (IT COMPILED CLEANLY)

When the crawler lost its return value, all 12 call sites kept the local they used to assign:

```cpp
// BROKEN — compiles, zero warnings, wrong on every iteration
const auto currentNode = m_rootNode;          // leftover of the old form:
while ( crawler.fetchNextNode() )             //   while ( (currentNode = crawler.nextNode()) != nullptr )
{
    currentNode->doSomething();               // ALWAYS THE ROOT NODE
}
```

The loops ran the correct NUMBER of times but operated **on the root node every iteration**.
Per-site effect:

| Site | Effect of the stale local |
|------|---------------------------|
| `Scene::processLogics()` | ran N times on the root, never on any other node |
| `Scene::findNode()` | could only ever match the root |
| `getNodeStatistics()` / tree dump | counted the root's children N times |
| Camera / microphone detection | inspected the root only |
| The 5 rendering crawlers (`Scene.rendering.cpp`) | would have rendered nothing node-attached |

**Why it was nearly invisible on `doom-loader`:** the Doom level is a `StaticEntity` — the
`m_staticEntities` path, which does not use the crawler at all — and the only `Node` is the player
actor, which carries no visible mesh. `doom-loader` is therefore a **weak target** for this and must
not be used as the witness; a demo with node-attached visuals is the real test.

**Validated at runtime on the `animation-debug` demo (Aug 2026):** node-attached visuals render and
animate correctly. That witness is decisive, not merely reassuring — with the stale local the
crawlers only ever saw the ROOT node, so node-attached meshes rendered **not at all** (last row of
the table above). Seeing them render *and* animate proves the five rendering crawlers
(`Scene.rendering.cpp`) **and** the `Scene::processLogics()` crawler are repaired.

### ⚠️ Dead End: Never Make `m_currentNode` a Reference Member

An intermediate attempt declared the member as a reference to the caller's smart pointer. Two
reasons never to retry it:

1. **It does not compile.** Binding `shared_ptr< const Node > &` / `shared_ptr< Node > &` to const
   lvalues → *"discards qualifiers"*, 6 instantiations.
2. **It would have been wrong anyway.** Assigning through the reference writes into the **CALLER's**
   variable, and two sites pass `m_rootNode` directly — the terminal `m_currentNode = nullptr` at
   the end of the iteration would have **nulled the scene's root node**.

**Residual nits deliberately left alone:** `populateStack()` also assigns `m_currentNode` (the
constructor DEPENDS on that side effect to seat the base node — an implicit coupling), and the class
constrains `requires std::is_class_v< Node >` instead of `node_t` (inert — true either way).

## Development Patterns

### Creating a Dynamic Object (Node)
```cpp
// Create as child of existing Node
auto player = scene->root()->createChild("player", initialPos);

// Add Components
player->newVisual(meshResource, castShadows, receiveShadows, "body");
player->newCamera(90.0f, 16.0f/9.0f, 0.1f, 1000.0f, "player_cam");

// Configure physics
player->bodyPhysicalProperties().setMass(80.0f);
player->enableSphereCollision(true);
```

### Creating Static Geometry (StaticEntity)
```cpp
// Create via Scene
auto building = scene->createStaticEntity("building_01");
building->setPosition(worldPos);

// Add Visual and Light
building->newVisual(buildingMesh, true, true, "main");
building->newPointLight(Color::Warm, 100.0f, 20.0f, "lamp");
```

### Hierarchy (vehicle with wheels)
```cpp
// Parent vehicle
auto vehicle = scene->root()->createChild("vehicle", vehiclePos);
vehicle->newVisual(carBodyMesh, true, true, "body");

// Child wheels (automatically follow parent)
auto wheelFL = vehicle->createChild("wheel_FL", localPos_FL);
wheelFL->newVisual(wheelMesh, true, true, "wheel");

// Move vehicle → wheels automatically follow
vehicle->applyForce(forwardVector * thrust);
```

### Toolkit — Entity Generation & Node Hierarchies

The `Toolkit` class (`Scenes/Toolkit.hpp`) provides high-level entity construction helpers. It manages a cursor position, generation policies, and material/geometry creation.

**Core workflow:**
1. `setCursor(x, y, z)` — Position for the next entity
2. `generateCuboidInstance<entity_t>(name, size, material)` — Creates geometry + material + renderable + visual component
3. Returns `BuiltEntity<entity_t, Component::Visual>` with `.entity()` and `.component()` accessors

**Generation policies (`GenPolicy`):**

| Policy | Behavior |
|--------|----------|
| `Simple` (default) | Creates a standalone entity under the scene root |
| `Parent` | Creates the next Node as a **child** of a previously set parent node |
| `Reusable` | Reuses an existing entity for the next component attachment |

**Node hierarchy creation:**
```cpp
// Create parent node at world position
const auto parent = toolkit
    .setCursor(0.0F, -1.0F, 0.0F)
    .generateCuboidInstance< Node >("Parent", 2.0F, material);

// Create child — cursor is now in parent's local space
const auto child = toolkit
    .setParentNode(parent.entity())
    .setCursor(6.0F, 0.0F, 0.0F)
    .generateCuboidInstance< Node >("Child", 2.0F, material);

// Create grandchild — cursor in child's local space
const auto grandchild = toolkit
    .setParentNode(child.entity())
    .setCursor(6.0F, 0.0F, 0.0F)
    .generateCuboidInstance< Node >("GrandChild", 2.0F, material);

// IMPORTANT: Reset to default after building hierarchy
toolkit.clearGenerationParameters();
```

**Key methods:**
- `setParentNode(shared_ptr<Node>)` — Next generated Node becomes a child of this parent
- `setReusableNode(shared_ptr<Node>)` — Attaches next component to an existing Node (no new entity)
- `setReusableStaticEntity(shared_ptr<StaticEntity>)` — Same for static entities
- `clearGenerationParameters()` — Resets policy to `Simple`, clears parent/reusable refs, resets cursor

**Available generators:**
- `generateCuboidInstance<T>(name, size, material)` / `generateCuboidInstance<T>(name, {w,h,d}, material)`
- `generateSphereInstance<T>(name, radius, material)`
- `generateRenderableInstance<T>(name, renderable)` — Generic, from pre-built renderable
- `generateEntity<T>(name)` — Empty entity (no visual)
- `generateDirectionalLight<T>(name, color, intensity, shadowRes, range)`
- `generatePointLight<T>(name, color, range, intensity, shadowRes)`
- `generateSpotLight<T>(name, color, range, intensity, angle, shadowRes)`
- `generatePerspectiveCamera<T>(name, focalLengthMM, lookAt, primary, showModel, preset)` — ⚠️ the
  framing is a LENS in millimetres, never an angle (13.096 mm = the historical 85° default;
  12 mm = 90°, 20.8 = 60°, 25.7 = 50°, 50 = 27°). See `Component/Camera.hpp`.
- `generateOrthographicCamera<T>(name, size, ...)` — unaffected: no lens under an orthographic
  projection.
- `generateCubemapCamera<T>(name, ...)` / `generateEnvironmentCubemapRenderer<T>(...)` — go through
  `setTechnicalFieldOfView(90)`, a cube face being a geometric constraint rather than a lens choice.

All generators support `<Node>` or `<StaticEntity>` as template parameter (default: `StaticEntity`).

### Creating a New Component
1. Inherit from `Component::Abstract` (Abstract.hpp)
2. Implement `processLogics()` if per-frame logic needed
3. Implement `move()` if reaction to entity movement needed
4. Implement `onSuspend()`/`onWakeup()` (pure virtual, mandatory)
5. Register with Scene if automatic observation needed

### Suspend/Wakeup System (Scene Manager Level)
When Scene Manager changes active scene, entities and their components are suspended/woken up to release pooled resources (e.g., OpenAL audio sources).

**Architecture (Template Method Pattern):**

1. **AbstractEntity** (`AbstractEntity.hpp/.cpp`):
   - `suspend()` / `wakeup()` - Public non-virtual methods
   - Call entity's `onSuspend()`/`onWakeup()` then iterate components
   - `onSuspend()`/`onWakeup()` - Protected virtual hooks (default empty)

2. **Component::Abstract** (`Component/Abstract.hpp`):
   - `onSuspend()` / `onWakeup()` - Pure virtual protected (mandatory contract)
   - Called by `AbstractEntity` (friend class)
   - Each component must implement (even if empty)

**Call flow:**
```
Scene::disable() → entity->suspend() → entity->onSuspend()
                                     → component->onSuspend() (for each)

Scene::enable()  → entity->wakeup()  → entity->onWakeup()
                                     → component->onWakeup() (for each)
```

**Existing implementations:**
- `SoundEmitter`: Releases/reacquires audio source, remembers playing state
- Other components: Empty implementation (no pooled resources)

See `Scene.cpp:enable()`, `Scene.cpp:disable()`, `AbstractEntity.cpp:suspend()`, `AbstractEntity.cpp:wakeup()`

## Multi-Scene Lifecycle (Active / Dormant / Deleted)

> [!CRITICAL]
> Several scenes can be loaded at once; **exactly one is ACTIVE**. Read
> [`docs/multi-scene-resource-ownership.md`](../../docs/multi-scene-resource-ownership.md) — it is
> the code-generation doctrine for any code touching GPU resources. The four Jun 2026 fixes
> (view-matrices, bindless, sampler cache, AS builder) all came from wiring a shared/global
> resource per-scene.

**Three states (the application designer decides transitions — the engine only offers them):**

- **Active** — the **only** scene that is rendered (`Renderer::renderFrame`) AND ticked
  (`processLogics`). The global services (bindless table, env cubemap, AS builder) mirror it.
- **Loaded-Dormant** — loaded but not active: **not rendered, not ticked**. It therefore does
  **NOT** contaminate the active scene's rendering (a common misconception — the renderer only
  reads the *active* scene's LightSet / post-process / render targets). Its pooled resources
  (audio sources) are released via Suspend/Wakeup. It still holds its per-scene GPU resources, so
  keep the dormant set small (memory; the RTX 3070 Ti 8 GB constraint is deliberate). Re-activating
  re-syncs the bindless table in ~1 frame.
- **Deleted** — destroyed; per-scene state dies with it, global services keep running referencing
  nothing of it.

> [!IMPORTANT]
> **`BindlessTextureSet` slot capacities are device-dependent.** `Scene`'s constructor pushes the GPU
> table capacities into its set (`setCapacities(maxTextures2D, maxTexturesCube, maxTexturesCubeArray)`,
> read from `Renderer::bindlessTextureManager()`). They are resolved at renderer initialization from
> the device's update-after-bind budget and are **lower than the `DesiredMaxTextures*` constants on
> MoltenVK** (2D[768]). Never bound a slot with the desired constants: a set handing out a slot beyond
> the table would have its descriptor write rejected by the manager and the texture would simply never
> appear. See [`Graphics/AGENTS.md`](../Graphics/AGENTS.md) → "Table Capacities Are Device-Dependent".

**Disable contract** (`Manager::disableActiveScene`, under the exclusive `m_activeSceneSharedAccess` lock):
editor deactivate → `BindlessTextureManager::clearTextureSet` (overwrites this scene's dynamic
slots with dummies — **hitch-free, NO waitIdle**) → `Scene::disable` (suspend entities, node
controller). The scene stays loaded (dormant) unless also deleted.

**Delete contract** (`Manager::deleteScene`): if active, `disableActiveScene` first; then
**`device->waitIdle()` (the drain lives HERE, before `m_scenes.erase`)** so in-flight frames that
referenced the scene's textures/buffers complete before destruction. The drain is on delete, not
disable, precisely so scene **switching stays seamless**.

**Jun 2026 audit (from the Scene Manager outward):** no remaining multi-scene-hazardous global
statics; the cached-shared-resource-destruction class was fully swept (all `TextureResource` types
+ `Overlay::Surface` now release the cache-owned sampler instead of destroying it). Dormant scenes
do not contaminate rendering; suspend/wakeup already releases their pooled (audio) resources.

## Octree Depth Limit

The OctreeSector class has a maximum subdivision depth (`DefaultMaxDepth = 16`) to prevent infinite recursion when many entities occupy the same position.

**Problem solved:**
When entities cluster at the same point (e.g., physics simulation causing all balls to converge), the octree would subdivide infinitely trying to separate them.

**Solution:**
- `OctreeSector::isStillLeaf()` checks `getDistance() < DefaultMaxDepth` before calling `expand()`
- At max depth, sector remains a leaf with all elements (O(n²) collision checks, but no infinite loop)

**Code references:**
- `OctreeSector.hpp:DefaultMaxDepth` - Constant (16 levels)
- `OctreeSector.hpp:isStillLeaf()` - Depth check before expansion
- `OctreeSector.hpp:getDistance()` - Calculates current depth from root

**Performance note:**
At depth 16 with a 200-unit root sector, minimum sector size ≈ 0.003 units. This is smaller than any realistic entity radius, so the depth limit rarely triggers in normal gameplay.

## Visual Debug System

Entities support visual debugging through `enableVisualDebug()` with different visualization types.

### Debug Types

| Type | Purpose | Mesh Used |
|------|---------|-----------|
| `Axis` | Show entity orientation | RGB axis lines |
| `Velocity` | Show movement direction | Arrow |
| `BoundingShape` | Show collision model | Shape-specific mesh |
| `Camera` | Show camera frustum | Camera model |

### BoundingShape Visualization

The debug system visualizes all collision model types with appropriate transformations:

- **Point**: Identity transform (axis gizmo used)
- **Sphere**: Uniform scaling by diameter
- **AABB**: World-space axis-aligned box (always aligned to scene axes, not entity rotation)
- **Capsule**: Translation to center + scaling (diameter, height, diameter)

**AABB debug shows the world AABB**, not the local one. For rotated entities, the world AABB
is larger than the geometry. The instance transform uses `inverseEntityMatrix * translation(worldAABBCentroid) * scaling(worldAABBDims)` to counter-rotate the debug mesh so it remains axis-aligned in world space.

See: `AbstractEntity.debug.cpp:enableVisualDebug()`, `AbstractEntity.debug.cpp:updateVisualDebug()`

### The TWO extents, and why a debug helper must not touch either (Aug 2026)

> [!CRITICAL]
> An entity carries **two different extents**, and confusing them has already cost a session:
>
> | | Component accessor | Entity member | Consumer |
> |---|---|---|---|
> | **RENDER** | `renderBoundingBox()` / `renderBoundingSphere()` | `m_renderBoundingBox` | frustum culling, rendering octree |
> | **PHYSICS** | `localBoundingBox()` / `localBoundingSphere()` | `m_collisionModel` | collision, physics octree |
>
> ⚠️ **The physics one INHERITS the render one by default** — `Component::Abstract::localBoundingBox()`
> literally returns `renderBoundingBox()`. That default is right for real content and wrong for an
> overlay, and it is the whole reason the two get confused.
>
> **`Component::Abstract::setContributesToEntityExtents(false)` excludes a component from BOTH.**
> Every visual debug helper sets it (in `enableVisualDebug()`). A gizmo is DEBUG LOGIC: it must never
> move an engine-level measurement of the content it only describes. A helper excluded from the
> render extent may be culled a touch early — deliberate, and cheaper than a falsified extent.
>
> ⚠️⚠️ **Set the flag from the builder's `setup()` callback, NEVER after `build()`.** `build()` links
> the component and linking runs `updateEntityProperties()` immediately, so a flag set afterwards
> arrives one accumulation too late.
>
> **The defect this fixed, and its signature:** the axis gizmo is generated at extent 1.0 AND is drawn
> scaled by the collider radius. Left contributing, it merged into the collider of whatever it
> annotated — a 1 m cube got its collider nearly doubled by the mere act of looking at it — and the
> bigger collider then drew a bigger gizmo, which is how it was spotted (arrows overshooting the
> object). In `geometry-debug` it read as "the AABB never shrinks when I switch object", because the
> helper's constant raw box floored it. **No assertion can see this**; the detector is
> `coordinates-debug` with both box helpers on.
>
> **Two helpers, on purpose**: `VisualDebugType::CollisionShape` (vertex-coloured) draws the PHYSICS
> extent, `VisualDebugType::RenderBoundingBox` (flat cyan) the RENDER one. Enable both to see them
> apart — on content with no scaled instance they must coincide.
> ⚠️ `CollisionShape` needs a collision model, so it silently traces an error and draws NOTHING if
> enabled before the entity has any component: enable helpers AFTER building the visual.
> ⚠️ Only the AXIS gizmo overshoots (`AxisDebugRadiusOvershoot`, 1.5x) — a shape helper reports its
> extent truthfully or it is not an instrument.

### ⚠️ OPEN: the extents ignore the renderable instance transform

> [!WARNING]
> `Component::Visual::renderBoundingBox()` returns `renderable()->boundingBox()` — the RAW mesh box,
> **not** transformed by the instance matrix set through `RenderableInstance::setTransformationMatrix()`.
> Both extents therefore ignore any per-instance scale. Content sites that DO set one:
> `DefinitionResource.cpp:457,545` (scene JSON scale), `Manager.console.cpp:547` (`addMesh` scale),
> `projet-alpha` `Actor/Fox.cpp:107` (**0.01**) and `Builtin/GameLogic.cpp:200,274` (0.01 / 0.5).
> At scale 0.01 the collider and the culling box would be 100x too large.
>
> **Status: code-derived, NOT reproduced on screen** (the actor gizmo is gated behind an
> `EnableVisualDebug` flag that no current demo sets, so the Fox shows no box to measure). Do not
> quote it as measured until someone enables that flag and captures it.

### Collision Model Auto-Creation

**CRITICAL BUG PATTERN**: Visual components with meshes trigger automatic collision model creation.

When creating debug/gizmo entities (e.g., sun position markers):
1. `generateSphereInstance()` creates a visual mesh
2. `updateEntityProperties()` auto-generates AABB from mesh bounds
3. This collision model interferes with physics!

**Solution**: Disable physics on gizmo entities:
```cpp
// Option 2: Set null collision model after creation
entity->setCollisionModel(nullptr);
```

See: `AbstractEntity.cpp:updateEntityProperties()` for auto-AABB creation logic.

## Critical Points

- **Smart pointers**: shared_ptr and weak_ptr for automatic hierarchy management
- **Manager and Scene**: Handle fail-safe construction/destruction (in development)
- **Root Node**: Immutable, cannot move nor receive Components
- **NodeCrawler**: the iteration NEVER yields the base node; `currentNode()` is the base node before the first `fetchNextNode()` and `nullptr` after the last. Process the base node BEFORE the loop when you need it
- **Y-up convention**: CartesianFrame uses Y-up everywhere — `localYAxis()` for any STRUCTURAL read (the basis Y column), `upwardVector()`/`downwardVector()` reserved for code actually talking about gravity
- **No world cache**: On-demand recalculation (future optimization planned)
- **Observers**: Automatic registration, do not register manually
- **Suspend/Wakeup**: Every new Component MUST implement `onSuspend()`/`onWakeup()` (pure virtual)
- **Friend class**: `AbstractEntity` is friend of `Component::Abstract` to access protected hooks
- **Auto collision models**: Visual components auto-generate collision models - disable for gizmos!

## Frame Synchronization — Double-Buffering Contract

> [!CRITICAL]
> **ANY data that flows from the Logic thread to the Renderer MUST be double-buffered
> (one copy per frame-in-flight).** Failure to respect this causes GPU read / CPU write
> race conditions that manifest as flickering, tearing, or corrupted data.

### How It Works

The engine uses **frames-in-flight** (typically 2-3) to keep the GPU busy while the CPU
prepares the next frame. Each frame-in-flight has its own fence, command buffer, and
descriptor sets. The logic thread and render thread run concurrently.

**Synchronization mechanism:**
- `m_renderStateIndex` (`std::atomic<uint32_t>`) — Written by the logic thread after
  updating entity transforms, read by the render thread via `std::memory_order_acquire`.
- Each entity stores **two copies** of its world coordinates (indexed by state index).
- The logic thread writes to `activeStateIndex`, the render thread reads from
  `m_preparedReadStateIndex` (captured at `prepareRender()` time).

**Code references:**
- `Scene.rendering.cpp:prepareRender()` — `m_preparedReadStateIndex = m_renderStateIndex.load()`
- `Scene.hpp` — `m_renderStateIndex` atomic, `m_preparedReadStateIndex`
- `Renderer.hpp` — `m_currentFrameIndex`, `framesInFlight()`

### Per-Frame GPU Resources

Any GPU buffer (SSBO, UBO) that is **updated every frame** must have one instance per
frame-in-flight. Otherwise, the CPU overwrites the buffer while the GPU is still reading
the previous frame's data.

**Already double-buffered:**
| Resource | Owner | Indexed by |
|----------|-------|------------|
| Entity world coordinates | `LocatableInterface` | `m_renderStateIndex` |
| RT mesh metadata SSBOs | `SceneMetaData` | `m_currentFrameIndex` |
| RT material data SSBOs | `SceneMetaData` | `m_currentFrameIndex` |
| RT descriptor sets | `Renderer` | `m_currentFrameIndex` |
| Light UBOs | `LightSet` | Dynamic offset |
| Instance transforms SSBOs | `SceneInstanceTransforms` | `m_currentFrameIndex` |

### Rules When Adding New GPU Data

1. **If you create a new SSBO/UBO that is written every frame**, create `framesInFlight()` copies.
2. **Index them by `m_currentFrameIndex`** (from `Renderer::currentFrameIndex()`).
3. **Update the descriptor set for the current frame only** — never write to all descriptor sets.
4. **Use `SceneMetaData::initializePerFrameBuffers()` as a reference** for the pattern.
5. **If in doubt, look at how `m_meshMetaDataSSBOs` works** — it was the fix for RT reflection flickering.

**Anti-pattern (causes flickering):**
```cpp
// WRONG: Single buffer overwritten every frame
m_ssbo->mapMemory();
memcpy(dst, data, size);
m_ssbo->unmapMemory();
```

**Correct pattern:**
```cpp
// RIGHT: Per-frame buffer, only the current frame's copy is written
m_ssbos[frameIndex]->mapMemory();
memcpy(dst, data, size);
m_ssbos[frameIndex]->unmapMemory();
```

### View Matrix State Index — Critical Trap

> [!CRITICAL]
> **Post-process effects that reconstruct world positions from the depth buffer MUST use
> the `readStateIndex` overloads of `viewMatrix()` and `projectionMatrix()`, NOT the
> default overloads.**

The `ViewMatricesInterface` provides two families of overloads:
- `viewMatrix(bool infinity, size_t viewIndex)` → reads `m_logicState` (current logic tick)
- `viewMatrix(uint32_t readStateIndex, bool infinity, size_t viewIndex)` → reads `m_renderState[readStateIndex]` (stable render snapshot)

The scene rendering pipeline uses `m_renderState[readStateIndex]` to compute the depth buffer.
If a post-process effect reconstructs world positions using `m_logicState` (the default overload),
the logic thread may have already advanced to the next tick. The matrices will disagree with the
depth buffer → **world position mismatch → flickering**.

**Fix pattern (used in RTR):**
```cpp
const auto readStateIndex = m_renderer->currentReadStateIndex();
const auto & viewMat = viewMatrices.viewMatrix(readStateIndex, false, 0);
const auto & projMat = viewMatrices.projectionMatrix(readStateIndex);
```

**Code references:**
- `Renderer.hpp:currentReadStateIndex()` — Getter for the stable read state index
- `Renderer.cpp:renderFrameWithPostProcessing()` — Captures `scene->preparedReadStateIndex()` before post-processing
- `Effects/Framebuffer/RTR.cpp:execute()` — Uses `readStateIndex` for NDC → world reconstruction
- `ViewMatrices3DUBO.cpp:viewMatrix()` — Two overloads: `m_logicState` vs `m_renderState[idx]`

## Instance Transforms (SceneInstanceTransforms)

`SceneInstanceTransforms` owns the per-scene **InstanceTransforms SSBO** (one buffer per
frame-in-flight, `SceneMetaData::initializePerFrameBuffers()` pattern). It is the B1 step of
the motion-vectors chain (see engine `TODO.md`): move the non-instanced model matrices out of
push constants into a per-instance SSBO indexed by `gl_BaseInstance`, and carry
`{model, previousModel}` per entry for temporal effects (TAA, RTGI reprojection, motion blur).

**GPU layout** (std430, header `static_assert`-ed at 128 B):
`Header {mat4 previousViewProjection; mat4 previousViewProjectionInfinity;}` followed by
`Entry {mat4 model; mat4 previousModel;}[]`. The header is reserved for the motion-vector pass and
written **only** by the primary view target (`RenderTargetType::View`); the regular matrix path
keeps pushing the view-projection matrix through push constants (MDI precedent) and only reads the
entries. The second matrix serves the renderables drawn with the **translation-free infinity
view** (the sky): their current clip position comes from the pushed infinity view, so their
previous one must match, or the velocity is off by the whole camera translation even on a static
camera. The header used to hold the CURRENT view-projection in that slot — read 0 times by the
generated GLSL, hence recycled at no size cost.

⚠️ **Both header matrices MUST be UNJITTERED** — stage them from
`ViewMatricesInterface::unjitteredProjectionMatrix()`, never from `projectionMatrix()`, which
serves the jittered form while TAA is active. The velocity clip positions are built from this
header, and the TAA sub-pixel offset is applied to `gl_Position` alone through a per-draw push
constant. A short-lived revision carried the current/previous jitters in a third `vec4` member
(header at 144 B) so the vertex shader could subtract them back — that design required the
jitter to also sit in the single-buffered view UBO, which raced the GPU; see engine
`docs/caution-points.md` § "Sub-pixel projection jitter raced the single-buffered view UBO".

**Frame contract (frame-linear slots):**
1. The **Renderer** calls `Scene::beginRenderFrame()` once per rendered frame (both windowed
   and windowless flows), BEFORE any `Scene::prepareRender()` — this resets the staging cursor
   and targets the current frame-in-flight buffer.
2. Every `prepareRender()` of the frame (render-to-textures first, main view last) stages one
   entry per **visible non-instanced** instance (`!useModelVertexBufferObject()`) via
   `RenderableInstance::Abstract::stageInstanceTransforms()` — a mirror of
   `Unique::pushMatricesForRendering()`'s model matrix computation — then uploads the whole
   staged range (`updateVideoMemory()`, cumulative and idempotent within the frame).
3. The instance retains its slot (`instanceTransformsSlot()`) for the draws recorded until the
   next `prepareRender()`. The same instance may hold a different slot per render target within
   one frame — REQUIRED for sprites, whose model matrix depends on the camera position.
4. Buffers grow on demand (power of two); the old buffer is retired through the
   `Vulkan::DeferredDestructor`.

**Descriptor binding (milestone 2):** the SSBO is exposed through a DEDICATED descriptor set
owned by `SceneInstanceTransforms` (one set per frame-in-flight, shared renderer descriptor
pool, layout cached under UUID `InstanceTransformsSSBO` via
`SceneInstanceTransforms::getDescriptorSetLayout()`), at the dynamic set index
`Saphir::SetType::PerSceneTransforms`. NOT inside the per-render-target view UBO set — the
SSBO is per-scene/per-frame while view sets are per-target and frame-agnostic (incompatible
lifecycles; same reasoning as the skinning SSBO's dedicated PerModel set). On buffer growth,
the current frame's set is rewritten in place (legal: the frame fence guarantees no in-flight
reference).

**Consumption (milestone 3):** the classic non-instanced scene path (non-MDI, non-cubemap,
non-advanced) READS the SSBO: push constants shrink to VP + jitter + frameIndex (76 B, or
68 B before the TAA jitter member), the model
matrix comes from `instanceMatrices[gl_InstanceIndex * 2]`, the slot travels through the
`firstInstance` draw parameter (`CommandBuffer::drawWithFirstInstance()` — instanceCount
is 1, so `gl_InstanceIndex == firstInstance`, no shaderDrawParameters feature needed).
The descriptor set is passed from `Scene::prepareRender()`'s cached
`m_preparedInstanceTransformsDS` down through `render()`. Full shader-side contract
(incl. the ⚠️ two-condition binding rule): `src/Saphir/AGENTS.md` § "InstanceTransforms
SSBO Path". Advanced/lighted, cubemap/CSM and shadow paths still push their matrices
(milestone 4).

**Status:** B1 (e080399e) + B2 (4d500626) + B3 (velocity MRT + RTGI dilation consumption) + B4 (double skinning: the skinning SSBO interleaves {current, previous} bone matrices, stride 2 — limb-level velocity)
DONE 2026-07-25 — the header {VP, previousVP} is now CONSUMED by the velocity vertex
shaders and the entries' previousModel by the same path. B1 details: — classic AND advanced
paths consume the SSBO (advanced pushes V + frameIndex = 68 B, killing the historical 132 B
min-spec violation). Cubemap/shadow/CSM paths stay on push constants (owner decision —
min-spec clean, no motion data needed). Validated: `doom-loader` (unlit classic path),
`global-illumination` A/B pixel diff vs pre-B1 baseline within the stochastic noise floor +
M4 BIT-IDENTICAL to M3 (deterministic console camera), `basic-scenery` (skybox infinity view
+ sprites + instanced + shadow-receiving).

**Previous model matrices (motion vectors B2, 2026-07-25):**
- **Unique (non-instanced)**: `previousModel` is REAL — `Abstract::m_lastModelMatrix` holds
  the matrix staged at the previous rendered frame; only the PRIMARY view staging advances
  it (`advanceHistory` parameter, gated on `RenderTargetType::View` in
  `insertIntoRenderLists()`). First staging (and post-culling reappearance) falls back to
  `previousModel == model` (zero object velocity beats a bogus one).
- **Multiple (instanced)**: opt-in `RenderableInstanceFlagBits::EnableInstanceMotionHistory`
  (MUST be set at construction — it fixes the VBO stride at
  `MeshVBOWithHistoryElementCount` = 16+9+16 floats). `updateLocalData()` archives the
  current model matrix into the previous slot before overwriting (one history step per
  logic update); ⚠️ the flag rides the whole chain: generator flag
  `IsInstanceMotionHistoryEnabled` → `VertexShader::enableInstanceMotionHistory()` →
  `VertexBufferFormatManager` (declare-or-jump `PreviousModelMatrixR0..R3`) →
  `ProgramCacheKey::isInstanceMotionHistory`. Breaking ANY link desynchronizes the pipeline
  vertex input stride from the actual VBO. No demo content uses it yet.
- ⚠️ **A/B capture protocol**: the RTGI accumulation converges asymptotically after a
  camera move — A/B pixel diffs are only valid at IDENTICAL post-placement timing
  (a 0.5-1 s window difference showed up as ~3/255 RMSE of pure reconvergence residual).

## Ray Tracing Architecture (SceneMetaData)

`SceneMetaData` manages all scene-level RT resources. It is inert when the device lacks RT support.

### Lifecycle
1. **Construction** (`Scene::Scene()`) — **Borrows** the single renderer-owned `AccelerationStructureBuilder` (`graphicsRenderer.accelerationStructureBuilder()`, created once at renderer init when RT is enabled; null otherwise) **and the renderer-owned `Vulkan::DeferredDestructor`** (`graphicsRenderer.deferredDestructor()`). `SceneMetaData` does **NOT** create or own either. `isRayTracingEnabled()` == (borrowed builder pointer != null).
2. **Per-frame buffer init** (`Scene::Scene()`) — `initializePerFrameBuffers(framesInFlight())` creates per-frame SSBOs
3. **Per-frame rebuild** (`Scene::prepareRender()`) — `rebuild(renderLists, ..., frameIndex)` collects TLAS instances, uploads SSBOs
4. **Destruction** — **Retires** this scene's RT resources (per-frame SSBOs, TLAS, pending build request) through the `DeferredDestructor`: a scene can be deleted at runtime while frames are still in flight. The builder is renderer-owned — nothing to unregister.

> [!CRITICAL]
> **All runtime destruction of TLAS/build requests goes through `Vulkan::DeferredDestructor`**
> (2026-07-05): the transiently-empty instance list retires (never destroys in place) the live
> TLAS, and each recorded rebuild retires its previous request frame-stamped — the old
> count-capped deque ("keep at most 3") under-covered rebuild bursts and caused GPU
> use-after-free (Xid 109 DEVICE_LOST). See `src/Vulkan/AGENTS.md`, "Deferred destruction
> contract".

> [!CRITICAL]
> **The `AccelerationStructureBuilder` is owned ONCE by the Renderer, never per-scene.** BLAS are
> built for SHARED geometries that outlive any scene; a per-scene builder (the old design, via a
> `Geometry::Interface` global static) got destroyed/nulled when a scene was deleted and broke the
> active scene's BLAS. See [`docs/multi-scene-resource-ownership.md`](../../docs/multi-scene-resource-ownership.md).

### BLAS Building
- **Centralized** in `Geometry::Interface::onDependenciesLoaded()` — called after `createOnHardware()`. The geometry fetches the builder from `serviceProvider().graphicsRenderer().accelerationStructureBuilder()` (skips if null = RT off). The returned `AccelerationStructure` is owned by the geometry; the builder only builds it.
- **TriangleStrip support** — `generateTriangleListIndicesForRT()` virtual method converts strip+primitive restart to triangle list. Persistent `m_rtIndexBufferObject` stored in `Geometry::Interface` for shader access to converted indices.
- **Subclasses**: `VertexGridResource` overrides `generateTriangleListIndicesForRT()` for strip conversion

### TLAS Async Build (Inline Recording)

> [!CRITICAL]
> **TLAS builds are recorded inline into the render command buffer via `recordTLASBuild()`.**
> The old synchronous `buildTLAS()` (fence wait per frame) has been removed.

**Two-phase API:**
1. `SceneMetaData::rebuild(renderLists, ..., frameIndex)` — Collects TLAS instances, calls `AccelerationStructureBuilder::prepareTLAS()` (CPU-side buffer preparation)
2. `Scene::recordTLASBuild(commandBuffer)` → `SceneMetaData::recordTLASBuild(commandBuffer)` → `AccelerationStructureBuilder::recordTLASBuild(commandBuffer, request)` — Records build commands into the render command buffer

**Call site in Renderer:**
```
prepareRender() → scene->recordTLASBuild(commandBuffer) → beginRenderPass()
```

### TLAS Buffer Lifetime & Retirement

TLAS buffers (TLAS + instance buffer + scratch buffer) are **per-request**, not persistent.
Each `TLASBuildRequest` owns its buffers. After recording, the request is retired into a
`std::deque`. Requests are popped from the front when the deque exceeds `framesInFlight()`
entries. This prevents use-after-free where a persistent buffer was written by the CPU
while the GPU was still reading it from a previous frame's command buffer.

### Pre-Allocated Rebuild Vectors

`SceneMetaData::rebuild()` reuses persistent vectors as class members (`m_instances`,
`m_meshMetaDataEntries`, `m_materialDataEntries`) instead of per-frame heap allocations.
These are cleared and refilled each frame without deallocating.

### Key Files
- `Scenes/SceneMetaData.hpp/.cpp` — TLAS, per-frame SSBOs, texture registration cache, `recordTLASBuild()`
- `Scenes/GPUMeshMetaData.hpp` — GPU-side struct (VB/IB addresses, stride, offsets, material index)
- `Graphics/Geometry/Interface.hpp/.cpp` — `buildAccelerationStructure()`, `generateTriangleListIndicesForRT()`, `m_rtIndexBufferObject`
- `Graphics/Geometry/VertexGridResource.cpp` — Strip→TriangleList conversion
- `Vulkan/AccelerationStructureBuilder.hpp/.cpp` — BLAS/TLAS building, `TLASBuildRequest`, `prepareTLAS()`, `recordTLASBuild()`, retired request deque
- `Graphics/Renderer.hpp/.cpp` — **owns** the single `AccelerationStructureBuilder` (`accelerationStructureBuilder()` getter); `SceneMetaData` and `Geometry::Interface` borrow it

## Render List Categories

The Scene dispatches renderable layers into 7 render lists (defined in `Scene.hpp`):

| Index | Constant | Sort Order | Description |
|-------|----------|------------|-------------|
| 0 | `Opaque` | State-sorted (pipeline\|material\|geometry\|distance) | Opaque objects, no lighting. Special objects (sprites, InfinityView, depth-disabled) use distance-only fallback |
| 1 | `Translucent` | Back-to-front | Translucent objects (no grab pass), no lighting |
| 2 | `OpaqueLighted` | State-sorted | Opaque objects, with lighting. Same special-object fallback |
| 3 | `TranslucentLighted` | Back-to-front | Translucent objects (no grab pass), with lighting |
| 4 | `Shadows` | Distance | Shadow-casting objects |
| 5 | `TranslucentGB` | Back-to-front | Translucent objects requiring grab pass, no lighting |
| 6 | `TranslucentGBLighted` | Back-to-front | Translucent objects requiring grab pass, with lighting |

**Rendering order**: Opaque → Translucent → TranslucentGB (grab pass capture happens between Translucent and TranslucentGB).

**The background is drawn FIRST, and without depth.** `Scene::registerSceneVisualComponents()` gives
the background visual (`m_sceneVisualComponents[BackgroundVisualIndex]`, index 0)
`setUseInfinityView(true)` + `disableDepthTest(true)` + `disableDepthWrite(true)` (plus shadow
casting and receiving OFF). `populateRenderLists()` walks the scene visual components before the
nodes and the static entities and inserts them at distance `0.0F`; the background takes the
`isSpecial` branch of `insertIntoRenderLists()` (distance-only key, NOT the state-sorted composite),
so its key is `0` in the `Opaque` multimap — head of the list, drained first. The geometry is a
512 m cuboid (`Renderable::AbstractBackground::SkySize`) with flipped winding drawn on the
translation-free infinity view: it fills every pixel, and the level geometry drawn afterwards with
depth test + write ON simply overwrites it.

That is what lets a loader emit **no geometry at all** where the sky must show through — the `F_SKY1`
sectors of a Doom map are holes on purpose, no stencil and no portal involved. With no background
installed those pixels are opaque black, never garbage. See [`@Scenes/Loaders/AGENTS.md`](../Scenes/Loaders/AGENTS.md)
→ WADLoader.

**Dispatch logic** in `Scene::insertIntoRenderLists()`:
1. `renderable->isOpaque(layerIndex)` → Opaque/OpaqueLighted
2. `renderable->requiresGrabPass(layerIndex)` → TranslucentGB/TranslucentGBLighted
3. Otherwise → Translucent/TranslucentLighted

**Code references:**
- `Scene.hpp` — Constants and `m_renderLists` array (7 elements)
- `Scene.rendering.cpp:insertIntoRenderLists()` — 3-way dispatch
- `Scene.rendering.cpp:populateRenderLists()` — Clear and populate all 6 non-shadow lists

**Populate gate exclusions** (`checkRenderableInstanceForRendering()`, Aug 2026): before the
readiness checks, an instance is skipped for a given target when (1) the caller registered it in
the target's manual exclusion list (`RenderTarget::Abstract::excludeFromRendering()`), or (2) —
**automatic, no registration** — its material `samplesTexture()` the Texture/Cubemap target being
populated. Rule 2 is what keeps a probe self-sampling feedback loop structurally impossible: on
Apple Silicon that loop is a GPU fault → `DEVICE_LOST`, not a mere artifact. See engine
`docs/caution-points.md` § "Probe self-sampling" and `docs/reflection-pipeline.md` § 2.3 fix 4.

## LightSet & Background-Derived Lighting (Jul 2026)

**The "static lighting" mode was REMOVED** (`Saphir::StaticLighting`, `enableAsStaticLighting()`,
`isUsingStaticLighting()`, the `SimplePass` remap and its program-cache-key bit). It was a
performance shortcut (one forward pass with a single light baked as GLSL literals) predating the
photometric migration; owner decision: more trouble than it was worth. `RenderPassType::SimplePass`
is now strictly UNLIT (light set disabled or instance lighting disabled). The `LightSet` is a pure
aggregator: lights + photometric ambient (`setAmbientLightColor()` sRGB +
`setAmbientLightIntensity()` in LUX).

**Sky → LightSet bridge (OPT-IN)**: `Scene::applyBackgroundLighting(BackgroundLightingOptions)`
derives the scene lighting from the background photometric manifest — ambient = average color ×
ambient illuminance, plus one `StaticEntity` + `DirectionalLight` per declared celestial body
(`Graphics::CelestialBody`; the entity sits at `direction × 1000`, the component default shines
along `-normalize(position)`). The first star becomes `mainDirectionalLight`. Options carry the
NON-photometric choices only (shadow map resolution, classic coverage vs CSM cascades). Scene
JSON opt-in: `"ApplyLighting": true` in the `Background` block; console:
`setBackground(name, true)`.

⚠️ **Threading contract (rewritten Jul 2026 after two live crashes)**: the entry point only
RAISES a request (`m_backgroundLightingRequested`, atomic) — it may be called from ANY thread
(console TCP thread, input callbacks, demo constructors). The actual application
(`applyBackgroundLightingNow()`: entity/light creation, LightSet, view UBOs) happens exclusively
at the top of `Scene::processLogics()` (logic thread), which POLLS the request and honors it once
the background resource `isLoaded()` — no observer, no notification race. Re-application
(background switch) first removes the star entities recorded in `m_backgroundStarEntities`, so
switching skies never stacks directional lights.

**Environment luminance is a View UBO value, not a baked literal**:
`Scene::refreshAmbientLightProperties()` pushes ambient color + intensity + background luminance
to EVERY render target's view UBO (main, render-to-view, render-to-texture — offsets
`EnvironmentLuminanceOffset` in the three `ViewMatrices*UBO`). Called at `LightSet::initialize()`,
`Scene::setBackground()`, background `LoadFinished`, and by `applyBackgroundLightingNow()`.
See `src/Graphics/AGENTS.md` § "Background photometric contract" for the manifest schema.

**IBL ambient replaces the scalar under the applyAmbient contract (Jul 2026, IBL lot 3)**:
when `applyBackgroundLighting({applyAmbient: true})` runs, `refreshAmbientLightProperties()`
pushes a **ZERO** scalar ambient intensity to the view UBOs (the baked irradiance cubemap
takes over in the ambient pass — a directional E(n) and a flat scalar would double-count
the same sky) while the LightSet keeps the photometric values for effects reading it
directly. With `applyAmbient: false` (RTGI demos, manual ambient) the irradiance slot is
NOT published (parked on the default black cubemap) and the scalar path stands alone —
`Scene::updateEnvironmentIBL()` re-evaluates that publication every tick, so a scene can
switch lighting modes after the bake. `Scene::setBackground()` now writes the bindless SET
directly when the cubemap is already created (the per-frame sync mirrors it; the late
adoption covers async loads) — the IBL re-bake is keyed on that set identity.

**Environment IBL follows the adopted cubemap (Jul 2026, IBL lot 2)**:
`Scene::updateEnvironmentIBL()` is polled every `processLogics` tick (idle cost: one mutex
lock + a pointer compare). When the identity of `BindlessTextureSet::environmentCubemap()`
changes — `setBackground()` can run on any thread, the late adoption of an async-loaded
cubemap runs on the render thread, so the mutex-protected set is the source of truth — the
scene bakes irradiance + prefiltered cubemaps through `Renderer::iblBaker()` (blocking GPU
job, ~1 ms) into a scene-owned **ping-pong pair** of `Graphics::IBLTexture` (frames in
flight keep sampling the published pair), then publishes via
`setIrradianceCubemap()/setPrefilteredCubemap()` (reserved bindless slots 1 and 2). The
engine default black cubemap is never baked. Failures mark the source as attempted — no
retry storm. See `src/Graphics/AGENTS.md` § "Graphics/Compute/IBLBaker".

## EffectsToolkit/FX — photometric since Aug 2026

`EffectsToolkit::FX::createFlashEffect()` was the LAST authoring entry point still taking raw
candela after the photometric migration converted `Toolkit::generate{Point,Spot,Directional}Light()`
to authoring units. It now takes **lumens** and converts once (the keyframes drive `setIntensity()`,
which is candela, so converting per interpolated frame would be waste).

Its keyframes are also the reference shape for a **detonation**, and the reason is a contract
change nobody sees coming:

> [!CAUTION]
> **Do not shape a light effect with its RADIUS.** Under the windowed inverse square the radius is
> `saturate(1 - (d/r)^4)^2`, a culling window that sits at 1.0 over almost the whole range; the
> falloff is `1 / (d^2 + 1)`, distance only. Animating the radius — the natural move under the old
> `max(1 - (d/r)^2, 0)` falloff — brightens nothing and just pops the hard cut in and out. The
> envelope belongs in the intensity. Full write-up in `docs/caution-points.md` § "The light RADIUS
> is a culling bound, not a dimmer".

The flash also ramps its COLOUR (white hot → yellows → the caller's settling tint), which
`Component::PointLight` supports natively: `playAnimation()` handles the `Color` id and `Sequence`
interpolates `Variant`s of type `Color` (linear and cosine).

## Shadow Mapping Integration

The Scene handles shadow map rendering and lighting pass selection. See [`docs/shadow-mapping.md`](../../docs/shadow-mapping.md) for complete shadow mapping architecture.

### Pass Type Selection (Shadow + Color Projection)

Each light's `RenderPassType` is selected at render time based on 4 conditions:

```cpp
const bool useShadow = shadowMapsEnabled
    && light->isShadowCastingEnabled()
    && light->hasShadowDescriptorSet()
    && instance->isShadowReceivingEnabled();
const bool useColorProjection = light->hasColorProjectionTexture();

// 4-branch selection per light type:
if ( useShadow && useColorProjection )
    passType = RenderPassType::SpotLightPassFull;
else if ( useShadow )
    passType = RenderPassType::SpotLightPassShadowMap;
else if ( useColorProjection )
    passType = RenderPassType::SpotLightPassColorMap;
else
    passType = RenderPassType::SpotLightPass;
```

Same pattern applies to directional (with CSM variants) and point lights.

**Why this matters:** Without the global shadow check, disabling shadows via settings caused Vulkan validation errors because shadow map images remained in `VK_IMAGE_LAYOUT_UNDEFINED` but descriptor sets still tried to bind them.

### Descriptor Set Architecture

Each light creates a descriptor set with 2 bindings:

| Binding | Content | Inactive fallback |
|---------|---------|-------------------|
| 0 | Light UBO (dynamic offset) | Always present |
| 1 | Shadow map sampler | Not created (no shadow descriptor set) |

Lights without shadow use only the shared UBO descriptor set (binding 0). Shadow-enabled lights get a dedicated descriptor set with both bindings.

**Color projection uses the global bindless system** — the light UBO carries a `uint` bindless index (`ColorProjectionIndex`, encoded as `bit_cast<float>`). The texture is registered in `BindlessTextureManager` via `ObserverTrait` notification when async loading completes. See: `Saphir/AGENTS.md` → Bindless Color Projection Sampling.

**Code references:**
- `Scene.rendering.cpp:renderLightedSelection()` - Pass type selection logic
- `Component/SpotLight.cpp:createShadowDescriptorSet()` - 2-binding shadow descriptor
- `Component/DirectionalLight.cpp:createShadowDescriptorSet()` - 2-binding shadow descriptor
- `Component/PointLight.cpp:createShadowDescriptorSet()` - 2-binding shadow descriptor
- `Component/AbstractLightEmitter.cpp:registerColorProjectionInBindless()` - Bindless registration
- `Component/AbstractLightEmitter.cpp:onNotification()` - Async texture load callback

## GLTFLoader → Scenes::Loaders (Refactored)

> **MOVED:** `Scenes::GLTFLoader` has been refactored into `Scenes::Loaders::GLTFLoader` (`src/Scenes/Loaders/`).
> The loader no longer depends on Scenes/ types. See [`@Scenes/Loaders/AGENTS.md`](../Scenes/Loaders/AGENTS.md) for the full loader documentation.
>
> Scene-side consumption is now handled by `Scenes::SceneDataConsumer`.

### Overview

`SceneDataConsumer` (`Scenes/SceneDataConsumer.hpp`) builds scene objects from an `Scenes::Loaders::SceneData`.

### Two Operating Modes

`SceneDataConsumer::build()` operates in one of two modes:

| Mode | Condition | Entity Type | Use Case |
|------|-----------|-------------|----------|
| **StaticEntity** | `parentNode == nullptr` | `StaticEntity` (flat, AABB culling) | Static scene geometry (buildings, props) |
| **Node** | `parentNode != nullptr` | `Node` (hierarchical, parent-relative) | Animated models, attachments, dynamic objects |

```cpp
// Step 1: Load resources (no Scene dependency)
Scenes::Loaders::GLTFLoader loader{act.resourceManager()};
Scenes::Loaders::SceneData sceneData;
loader.load(gltfPath, sceneData);

// Step 2: Build scene hierarchy
Scenes::SceneDataConsumer consumer;
consumer.setCreateLights(true);                  // OFF by default — see the warning below
consumer.build(sceneData, scene);                // StaticEntity mode
consumer.build(sceneData, scene, parentNode);    // Node mode
```

> [!WARNING]
> **`setCreateLights()` is OFF by default, deliberately** (owner decision, 2026-08-08). A demo
> that lights its own scene must never have an asset's emitters appear behind its back: a
> photometric calibration is a whole, and uninvited lights silently rebalance the exposure the
> scene was tuned for. Turn it on only when the asset **is** the lighting authority.
>
> Two consequences inside the consumer, both easy to break:
> - a **light-only node survives flattening** — dropping it would drop the emitter with it;
> - in `StaticEntity` mode a node with a light but **no mesh still gets an entity**, since the
>   emitter needs an owner.
>
> Cameras declared by an asset are **never instantiated** — they stay data in `SceneData`.

### Configuration Options

**On the loader** (affects resource loading):

| Setter | Default | Effect |
|--------|---------|--------|
| `LoaderOptions::skipSkinning` | `false` | Skip phases 4-5, ignore bone weights (load as static mesh) |
| `LoaderOptions::excludedNodeNames` | empty | Skip named nodes and their subtrees entirely |

**On the consumer** (affects scene building):

| Setter | Default | Effect |
|--------|---------|--------|
| `setFlattenHierarchy(true)` | `false` | Skip intermediate nodes, attach all meshes directly to parent |

### Lighting Is Carried By the Descriptor (`MeshDescriptor::lightingEnabled`)

`SceneDataConsumer` used to call `visual.getRenderableInstance()->enableLighting()`
**unconditionally** at FIVE sites (both operating modes, hierarchy and flatten paths). All five now
honour `Scenes::Loaders::MeshDescriptor::lightingEnabled`, through
`Graphics::RenderableInstance::Abstract::setLightingState(bool)` — the symmetric form of
`enableLighting()`, implemented with `enableFlag`/`disableFlag` because
`Base::FlagTrait< uint32_t >` offers no `setFlag(flag, state)`, and adding one to emeraude-base is
barred by the "Ave robustus!" feature freeze.

- **Default is `true`** (boolean last in the struct layout) → **glTF and FBX behaviour is
  unchanged**: a mesh coming from a lit format expects the light set, the ambient pass and the
  environment IBL.
- `Scenes::Loaders::WADLoader` sets `lightingEnabled = false` on its level mesh — the Doom sector light
  levels are already baked into the vertex colors, so re-lighting would double-count them.

> [!WARNING]
> **This fixes a LATENT defect and is INERT today.** The dispatch test is
> `m_lightSet.isEnabled() && renderableInstance->isLightingEnabled()` (`Scene.rendering.cpp`), and
> the light set is only ever enabled by `Scene::applyBackgroundLighting()` (or a
> `DefinitionResource`, or the console). The WAD demo installs a background but never calls it, so
> the flag currently changes nothing that reaches the screen — **no measurement demonstrates it**,
> and it must not be presented as if one did. It would bite the moment any demo enabled the light
> set with a WAD level loaded. Owner decision: keep it, and document it as latent.

### Node Mode Behavior

**Default (hierarchy preserved):** `processNodeAsNode()` recursively walks the glTF node tree. Automatic **identity flattening** skips nodes that have no mesh and no transform, reducing unnecessary depth.

**Flatten mode:** All meshes attach directly to the `parentNode`, ignoring intermediate glTF structural nodes. The first mesh attaches to the parent itself; subsequent meshes create children.

**Joint node skipping:** Nodes that are skeleton joints (but carry no mesh) are skipped — their transforms are driven by `SkeletalAnimator`, not the scene graph.

> [!WARNING]
> **Node mode entities are Nodes, not StaticEntities.** Code that uses `findStaticEntity()` will NOT
> find entities created in Node mode. Use `scene.root()->findChild(name)` instead.

### Coordinate System Conversion — there is NONE (since Aug 2026)

glTF uses **Y-up, right-handed, `-Z` forward** coordinates. So does the engine. **The import is the
IDENTITY**: no rotation, no mirror, no per-asset flag.

> [!CAUTION]
> **Do not reintroduce the 180° X rotation, and do not reintroduce the winding swap.** Until Aug 2026
> the engine was Y-DOWN, the consumer applied a 180° X rotation to the root, and all four loaders
> swapped triangle indices 1↔2 — the latter justified by the claim that *"the 180° rotation flips the
> winding"*. That claim is **false**: a rotation has determinant +1 and NEVER inverts a winding. The
> swap was compensating an orientation-reversing projection, which is the defect the Y-up flip fixed
> at its root. Both the rotation and the swap are **deleted**.

If an imported asset looks mirrored or inside-out, the cause is **not** here — measure it
(`docs/coordinate-system.md` § *The measurement that proves it*) before compensating anywhere.

### Resource Naming Convention

All resources use a prefix derived from the filename: `glTF:{stem}/`

| Category | Pattern | Example |
|----------|---------|---------|
| Images | `glTF:Fox/Image/{name}` | `glTF:Fox/Image/Texture` |
| Materials | `glTF:Fox/Material/{name}` | `glTF:Fox/Material/fox_material` |
| Geometry | `glTF:Fox/Geometry/{name}` | `glTF:Fox/Geometry/fox1` |
| Meshes | `glTF:Fox/Mesh/{name}` | `glTF:Fox/Mesh/fox1` |
| Nodes | `glTF:Fox/Node/{name}` | `glTF:Fox/Node/root` |
| Skeletons | `glTF:Fox/skeleton/{name}` | `glTF:Fox/skeleton/Armature` |
| Animations | `glTF:Fox/animation/{name}` | `glTF:Fox/animation/Run` |

When a glTF object has no name, the numeric index is used as fallback.

### Default Resource on Every Error Path (MANDATORY)

Every resource slot must contain a valid resource — never nullptr. On any loading error, the loader stores the container's default resource and continues. This respects the engine's fail-safe philosophy.

### Lambda Capture Safety (CRITICAL)

GLTFLoader is stack-allocated and destroyed when `onBuilding()` returns. Async lambdas passed to `getOrCreateResource()` execute on the thread pool **after** the loader may be destroyed.

**Rules:**
1. **NEVER capture `this`** in async lambdas
2. **Pre-resolve** all `shared_ptr` data before the lambda
3. **Copy scalars by value** (colors, factors, indices)
4. **Move-capture** vectors of shared_ptr to avoid atomic refcount overhead

```cpp
// WRONG — dangling this
->getOrCreateResource(name, [this, idx] (auto & res) {
    return res.load(m_images[idx]);  // this is dead!
});

// CORRECT — self-contained lambda
->getOrCreateResource(name, [image = m_images[idx]] (auto & res) {
    return res.load(image);
});
```

### PBR Material Features

Textures are created **on-demand during material loading** with the correct sRGB flag based on material semantic. Supported components:
- Albedo (sRGB), Metallic-Roughness, Normal, Ambient Occlusion, Emissive (sRGB)
- Clear coat (KHR_materials_clearcoat), Sheen (KHR_materials_sheen)
- Transmission (KHR_materials_transmission), Iridescence (KHR_materials_iridescence)
- Alpha mode: OPAQUE / MASK / BLEND

### Performance Optimizations

- **String allocation**: `reserve + append` instead of concatenation temporaries
- **Tri-buffer streaming**: 3-element stack buffer replaces per-primitive heap vector for index building
- **Move-capture**: `[materialList = std::move(materialList)]` avoids N atomic refcount increments
- **Two-pass shape building**: first pass counts vertices/triangles, second pass fills

### Code References

- `Loaders/GLTFLoader.hpp/.cpp` — Resource loading (phases 1-6). See [`@Scenes/Loaders/AGENTS.md`](../Scenes/Loaders/AGENTS.md)
- `Loaders/SceneData.hpp` — Common intermediate format (NodeDescriptor, MeshDescriptor — the latter carries `lightingEnabled`, default `true`)
- `Graphics/RenderableInstance/Abstract.hpp:setLightingState()` — Symmetric form of `enableLighting()`, honoured at the consumer's five visual-setup sites
- `Loaders/Interface.hpp` — Loader interface + LoaderOptions
- `Scenes/SceneDataConsumer.hpp/.cpp` — Scene builder (StaticEntity/Node modes, Y-up conversion)
- `Graphics/Renderable/SimpleMeshResource.cpp:load(path)` — Transparent single-mesh glTF loading
- `Graphics/Renderable/MeshResource.cpp:load(path)` — Transparent multi-material glTF loading

## Detailed Documentation

For complete architecture, diagrams, and advanced patterns:
- @docs/scene-graph-architecture.md
- @docs/shadow-mapping.md - Shadow mapping, PCF, global controls, color projection

## Bounding volumes — render vs collision (2026-08-09)

`Component::Abstract` exposes **two** extents. They differ in CARDINALITY, not merely in value —
that is what makes them two contracts rather than two names:

| | Cardinality | Why |
|---|---|---|
| **Render** | **one** general box / sphere | Culling asks a single question: *is any part of this inside the frustum?* One wide envelope answers it exactly, in one test. An envelope that is too large costs a few useless draw calls. |
| **Collision** | **a group** of primitives | Collision asks *where* and *against what*. One wide envelope answers WRONG — the tree whose canopy you hit at ankle height. An envelope that is too large produces incorrect behaviour, not wasted work. |

> [!IMPORTANT]
> **Target contract**: `renderBoundingBox()` stays singular; the collision side becomes a LIST of
> primitives. The single-box collision accessors below are the INTERIM state — correct for
> simple objects, insufficient for anything with a real silhouette.

Current state:

| Accessor | Meaning | `Visual` | `MultipleVisuals` |
|----------|---------|----------|-------------------|
| `localBoundingBox()` / `localBoundingSphere()` | **PHYSICAL** — what the collision model is built from | renderable's box | box of a **single** instance |
| `renderBoundingBox()` / `renderBoundingSphere()` | **VISUAL** — what frustum culling and the rendering octree must use | renderable's box | **union of every instance** |

> [!CAUTION]
> **The rendering octree inserted every entity as a POINT at its origin.** Only the physics
> octree (`enable_volume == true`) ever used a volume; the rendering branch called
> `insertWithPrimitive(element, element->getWorldCoordinates().position())` and nothing else. A
> 250-unit terrain tile, or a cell holding a thousand instanced trees, was therefore culled by
> whether its ORIGIN landed in a visible sector — it vanished while filling half the screen.
>
> **Fixed (2026-08-09):** both `insert()` and `update()` use `getWorldRenderBoundingBox()` when
> the element exposes one, detected with `if constexpr ( requires { ... } )` so the octree stays
> generic and element types that know nothing about rendering keep the point path.
>
> ⚠️ The `update()` point path has a "still inside its last subsector, nothing to do" shortcut.
> That shortcut is **unsound for volumes**: an element whose origin has not moved can still have
> grown — an animated pose, a rebuilt instance set — and now span sectors it is not registered
> in. It is skipped whenever a render box is available.
>
> Widening the COLLISION extent to fix any of this would have been the wrong cure: a forest cell
> would become one solid block. Hence the split.

> [!NOTE]
> `MultipleVisuals` caches its visual extent and **recomputes it when the renderable finishes
> loading** — the renderable's own box is empty before that, so a union built at construction
> is empty too, and the component would be culled as a point anyway. The eight corners are
> transformed individually: under rotation, transforming only the min/max pair yields a box that
> does not contain the shape.

### Open: compound collision shapes

A collision extent is currently **one** box or sphere per component, and `AbstractEntity` builds
a single `AABBCollisionModel` from it. Real objects need several: a tree is a narrow trunk at
ground level and a wide canopy above it — one AABB around both makes you collide with foliage at
ankle height. Moving the collision extent to a **list** of primitives is a separate project: it
touches the collision model, the broad phase and the narrow phase, all of which assume one shape
per entity. Owner-identified, not scheduled.

## Instance clustering — `Scenes/InstanceCluster.hpp` (2026-08-09)

`buildInstanceClusters()` splits an instance set into a fixed metric grid, one entity per
non-empty cell, transforms stored RELATIVE to the cell centroid. The rendering octree then culls
whole cells with no new culling path. Cells are anchored on the centroid rather than the grid
intersection, so relative coordinates stay small — precision no longer depends on how far the
scene sits from the origin.

> [!NOTE]
> **RESOLVED (2026-08-09) — it was never the instance objects.**
>
> | Cell size | Cells | RSS before | RSS after |
> |-----------|-------|------------|-----------|
> | 32 units | 4006 | **104 GB** | **3279 MB** |
> | 500 units | 16 | 3.5 GB | — |
>
> Same 20 000 instances, same geometry, no change to `RenderableInstance::Multiple`. The cost was
> the octree's **all-levels storage**: an element was copied into every sector it touched, at
> every depth. A cell spanning a boundary near the root multiplied itself down whole subtrees.
> Small cells span more boundaries, hence the ratio that looked like a per-instance cost.
>
> Cell size can now be chosen for culling quality rather than dictated by the memory budget.
> See "Octree storage and traversal" below — the fix moved elements to a single sector, which
> **changes what a traversal must read**.
>
> Eliminated along the way, and still true: VMA dedicated allocations (`Buffer::createWithVMA`
> uses `VMA_MEMORY_USAGE_AUTO` with no dedicated bit, and suballocates — measured 277 MiB in 4056
> allocations over 10 blocks); `SceneInstanceTransforms` (a `Scene` member, one per scene, not per
> entity); the VBO size itself.

> [!WARNING]
> Any test that creates a number of objects proportional to a parameter must run capped:
> `systemd-run --user --scope -p MemoryMax=8G -p MemorySwapMax=0 --quiet -- <cmd>`.
> The uncapped first run took the machine to 1 GB available and made it unusable.

## Octree storage and traversal — `Scenes/OctreeSector.hpp` (2026-08-09)

**Storage invariant: one element, one sector — the deepest that FULLY CONTAINS it.**
`insertWithPrimitive()` descends only into a subsector that entirely contains the primitive
(`isFullyContaining()`); otherwise the element stays where it is. It replaces the former
*all-levels* storage, where an element was copied into every sector it touched at every depth —
the cause of the 104 GB above.

> [!CAUTION]
> **The invariant changes what a traversal must read, and getting that wrong is silent.**
>
> An element that straddles a boundary now sits on an INNER node. The bigger it is, the higher it
> sits: **a ground plane straddles every boundary and lives at the ROOT.** Two consequences, both
> of which shipped as bugs before being caught:
>
> 1. **Reading `sector.elements()` of leaves only misses everything large.** The physics broad
>    phase did exactly that (`forLeafSectors` + `elements()`), so no body was ever offered the
>    ground as a collider — **you fall through the floor**, with no error anywhere.
> 2. **`m_elements.empty()` no longer proves an empty subtree.** An inner node can hold nothing
>    while its children are full, so the old early-exit prunes populated branches.
>
> **Contract:** the candidate set of a leaf is `leaf.elements() ∪ elements of ALL its ancestors`.
> `forLeafSectors()` accumulates that chain on the way down into a single reused buffer and hands
> it to the callback as `(leaf, candidates, ownedOffset)`. Never call `elements()` on a sector to
> build a query result.
>
> ⚠️ `ownedOffset` is not decoration. Candidates before it are INHERITED, and any per-sector
> predicate is unsound for them — `isTouchingRootBorder()` in particular: an inherited element may
> straddle the world edge while first being met in a leaf that does not touch it. Only elements
> the leaf OWNS are fully inside it.
>
> ⚠️ An element held by an ancestor is offered to EVERY leaf below it. A caller acting **per pair**
> is usually already safe (`testedEntityPairs`). A caller acting **per element** must deduplicate —
> `resolveCollisions()` phase 1 keeps a `correctedEntities` set, without which a straddling body
> receives its position correction once per leaf of the subtree. *(This also fixes a pre-existing
> defect: under all-levels storage the same body was corrected once per leaf it touched.)*

**Rendering is not concerned — and that is itself worth knowing.** `Scene.rendering.cpp` never
queries the rendering octree: it iterates the entities and tests `distance > viewDistance ||
!isVisibleTo(frustum)`. The rendering octree is built and maintained for nothing. Culling by
octree sector remains an open optimization (see "Performance Notes").

> [!CAUTION]
> `StaticEntity::isVisibleTo()` tests the **collision model** AABB, or a bare point when there is
> none. The `renderBoundingBox()` introduced for exactly this purpose is read by no culling path
> yet — the render/collision split is only half wired.
