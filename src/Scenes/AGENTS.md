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
- **Y-DOWN mandatory** in CartesianFrame
- Local transforms for Nodes (parent-relative)
- World space recalculated on demand (no cache currently)

### Available Components
**Rendering:** Visual, MultipleVisuals
**Lights:** DirectionalLight, PointLight, SpotLight
**Audio:** SoundEmitter, Microphone
**Physics:** DirectionalPushModifier, SphericalPushModifier, Weight
**Utilities:** Camera, ParticlesEmitter

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
- `Scene.physics.cpp` - Collision detection, boundary clipping, sleep/wake collision. See [`@Physics/AGENTS.md`](../Physics/AGENTS.md) for normal convention
- `Scene.rendering.cpp` - Render targets, shadow casting, rendering pipeline
- `Node.cpp/.hpp` - Hierarchical dynamic entity (tree)
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
- `@docs/coordinate-system.md` - Y-down convention (CRITICAL)

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
| `Scene.cpp` | Core/Lifecycle, Audio, Octree management | ~750 |
| `Scene.entities.cpp` | Entities (Node/StaticEntity), Observer notifications | ~480 |
| `Scene.physics.cpp` | Modifiers, Collision detection, Boundary clipping | ~300 |
| `Scene.rendering.cpp` | Render targets, Shadow casting, Rendering pipeline | ~1300 |

### Section Comments Format

Each concept section is marked with:
```cpp
/* ============================================================
 * [CONCEPT: NAME]
 * Description.
 * ============================================================ */
```

This allows quick navigation using search (e.g., `[CONCEPT: RENDERING]`).

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
- `generateCamera<T>(name, fov)`

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
- **Y-down convention**: CartesianFrame uses Y-down everywhere
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

## Ray Tracing Architecture (SceneMetaData)

`SceneMetaData` manages all scene-level RT resources. It is inert when the device lacks RT support.

### Lifecycle
1. **Construction** (`Scene::Scene()`) — Creates `AccelerationStructureBuilder`, registers it with `Geometry::Interface`
2. **Per-frame buffer init** (`Scene::Scene()`) — `initializePerFrameBuffers(framesInFlight())` creates per-frame SSBOs
3. **Per-frame rebuild** (`Scene::prepareRender()`) — `rebuild(renderLists, ..., frameIndex)` collects TLAS instances, uploads SSBOs
4. **Destruction** — Unregisters builder, clears all RT resources

### BLAS Building
- **Centralized** in `Geometry::Interface::onDependenciesLoaded()` — called after `createOnHardware()`
- **On-demand** in `SceneMetaData::rebuild()` — for geometries loaded before the RT builder was set (`const_cast` + `buildAccelerationStructure()`)
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

**Dispatch logic** in `Scene::insertIntoRenderLists()`:
1. `renderable->isOpaque(layerIndex)` → Opaque/OpaqueLighted
2. `renderable->requiresGrabPass(layerIndex)` → TranslucentGB/TranslucentGBLighted
3. Otherwise → Translucent/TranslucentLighted

**Code references:**
- `Scene.hpp` — Constants and `m_renderLists` array (7 elements)
- `Scene.rendering.cpp:insertIntoRenderLists()` — 3-way dispatch
- `Scene.rendering.cpp:populateRenderLists()` — Clear and populate all 6 non-shadow lists

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

## GLTFLoader

### Overview

`GLTFLoader` loads glTF 2.0 files into the scene graph. It is a **stack-allocated utility object** created per-load, not a long-lived service. All resource creation is **fully asynchronous** via `getOrCreateResource()`.

### 5-Phase Async Pipeline

| Phase | Method | Resource Type | Async |
|-------|--------|---------------|-------|
| 1 | `loadImages()` | `ImageResource` | Yes |
| 2 | `loadTextures()` | `TextureResource::Texture2D` | Yes |
| 3 | `loadMaterials()` | `Material::PBRResource` | Yes |
| 4 | `loadMeshes()` | `SimpleMeshResource` + `IndexedVertexResource` | Yes |
| 5 | `load()` | Scene node hierarchy | Sync (tree walk) |

### Coordinate System Conversion

glTF uses **Y-up, right-handed** coordinates. The engine uses **Y-down**. The loader applies a **180° X rotation** to the root node, which inverts Y and Z.

**Winding order compensation:** The 180° rotation flips triangle winding from CCW to CW. The loader swaps indices 1 and 2 during triangle building to restore correct winding:

```cpp
triangles.emplace_back(triBuffer[0], triBuffer[2], triBuffer[1]);  // swap 1↔2
```

### Default Resource on Every Error Path (MANDATORY)

Every resource slot must contain a valid resource — never nullptr. On any loading error, the loader stores the container's default resource and continues:

```cpp
m_images[i] = m_resources.container< ImageResource >()->getDefaultResource();
m_textures[i] = m_resources.container< TextureResource::Texture2D >()->getDefaultResource();
m_materials[i] = m_resources.container< Material::PBRResource >()->getDefaultResource();
m_meshes[i] = m_resources.container< SimpleMeshResource >()->getDefaultResource();
```

This respects the engine's fail-safe philosophy: errors are logged but never break the application.

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

See also: `Resources/AGENTS.md` → Async Lambda Capture Safety

### Performance Optimizations

- **String allocation**: `reserve + append` instead of concatenation temporaries for resource names
- **Tri-buffer streaming**: 3-element stack buffer replaces per-primitive heap-allocated `std::vector<uint32_t>` for index building
- **Move-capture**: `[materialList = std::move(materialList)]` avoids N atomic refcount increments
- **materialList.reserve**: Pre-allocates material vector to primitive count

### Code References

- `Scenes/GLTFLoader.hpp/.cpp` — Main loader class
- `Scenes/GLTFLoader.cpp:loadImages()` — Phase 1: async image loading
- `Scenes/GLTFLoader.cpp:loadTextures()` — Phase 2: async texture creation
- `Scenes/GLTFLoader.cpp:loadMaterials()` — Phase 3: async PBR material creation with pre-resolved textures
- `Scenes/GLTFLoader.cpp:loadMeshes()` — Phase 4: async geometry + mesh creation with tri-buffer

## Detailed Documentation

For complete architecture, diagrams, and advanced patterns:
- @docs/scene-graph-architecture.md
- @docs/shadow-mapping.md - Shadow mapping, PCF, global controls, color projection
