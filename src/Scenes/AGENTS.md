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

### Observer System
- **Automatic registration**: Scene observes Component additions
- Visual → rendering registration
- Camera/Microphone → AVConsole registration
- Lights → LightSet registration
- **NEVER manual registration**

### Spatial Optimization
- **Octrees per Scene**: One for physics, one for rendering
- **Frustum culling**: Active during tree traversal
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
- `Scene.physics.cpp` - Collision detection, boundary clipping
- `Scene.rendering.cpp` - Render targets, shadow casting, rendering pipeline
- `Node.cpp/.hpp` - Hierarchical dynamic entity (tree)
- `StaticEntity.cpp/.hpp` - Optimized static entity (flat map)
- `AbstractEntity.cpp/.hpp` - Common base for Component management
- `LocatableInterface.cpp/.hpp` - Interface for coordinates/movement
- `ToolKit.cpp/.hpp` - Helpers for complex entity construction
- `Component/Abstract.hpp` - Base class for all Components (pure virtual onSuspend/onWakeup)
- `Component/SoundEmitter.cpp/.hpp` - Audio emitter with suspend/wakeup source management
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
- **AABB**: Translation to centroid + scaling by dimensions
- **Capsule**: Translation to center + scaling (diameter, height, diameter)

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

- **Double buffering**: Logic thread writes activeFrame, Render thread reads renderFrame
- **Atomic swap**: m_renderFrame = m_activeFrame at end of each logic frame
- **Smart pointers**: shared_ptr and weak_ptr for automatic hierarchy management
- **Manager and Scene**: Handle fail-safe construction/destruction (in development)
- **Root Node**: Immutable, cannot move nor receive Components
- **Y-down convention**: CartesianFrame uses Y-down everywhere
- **No world cache**: On-demand recalculation (future optimization planned)
- **Observers**: Automatic registration, do not register manually
- **Suspend/Wakeup**: Every new Component MUST implement `onSuspend()`/`onWakeup()` (pure virtual)
- **Friend class**: `AbstractEntity` is friend of `Component::Abstract` to access protected hooks
- **Auto collision models**: Visual components auto-generate collision models - disable for gizmos!

## Detailed Documentation

For complete architecture, diagrams, and advanced patterns:
- @docs/scene-graph-architecture.md
