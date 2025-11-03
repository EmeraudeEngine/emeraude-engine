# AGENTS.md - AI Agent Guidelines for Emeraude Engine

This file follows the AGENTS.md standard convention for AI coding agents and defines the rules and conventions for any AI agent interacting with the `Emeraude Engine` project.

## General Rules

1.  **Language:** All code, code comments, and the content of this rules file must be written in standard English.
2.  **Cross-Platform:** All code must be written to be cross-platform compatible, avoiding platform-specific APIs or features unless absolutely necessary and properly abstracted.
3.  **Code Priorities:** Code should prioritize the following, in order: readability, maintainability, scalability, and finally, performance.
4.  **Performance:** Performance should only be prioritized in critical parts of the codebase.
5.  **C++ Standard & Targets:** The code must adhere to the C++20 standard. Existing code can be updated to this standard while respecting cross-platform compatibility. The general target platforms are Windows 11, Debian 13/Ubuntu 24.04 (as a Linux base), and macOS SDK 12.0.
6.  **Code Formatting:** All C++ code must be formatted using the project's `.clang-format` file. All code should also be compliant with the checks defined in the `.clang-tidy` file.
7.  **Documentation:** All public APIs (classes, methods, functions in header files) must be documented using Doxygen-style comments (`/** ... */` or `///`). The documentation should explain the purpose of the code, its parameters, and its return value.
8.  **Dependencies:** Do not add any new third-party libraries or dependencies to the project without explicit prior approval.
9.  **Licensing:** This project is open-source and licensed under the LGPLv3. All new code contributions must be compatible with this license.

---

## Project Specific Rules

1.  **Framework Architecture:** This project is a multimedia framework with the following technical specifications:
    *   **Graphics & Compute:** Exclusively uses the Vulkan API (1.2+).
    *   **Shaders:** Shaders are written in GLSL and compiled at runtime using GLSLang.
    *   **Memory Management:** Uses the Vulkan Memory Allocator (VMA) library for GPU memory allocation.
    *   **Audio:** Uses the OpenAL library for 3D positional audio.

2.  **Vulkan Integration:**
    *   All rendering must go through the Vulkan abstraction layer in `Vulkan/`.
    *   Never call Vulkan functions directly from high-level code; use the provided abstractions in `Graphics/`.
    *   Synchronization primitives (fences, semaphores) must be managed carefully to avoid deadlocks.
    *   All Vulkan resources must be properly destroyed when no longer needed.

3.  **Coordinate System Convention:**
    *   **CRITICAL:** The entire engine uses Vulkan's coordinate convention throughout all systems.
    *   This convention is used **consistently in ALL engine systems** (physics, rendering, scene graph, audio, etc.).
    *   **Coordinate System (Right-Handed, Y-Down):**
        ```
        From the player/camera perspective in world space:

        +Y (DOWN)     -Z (FORWARD)
           ↓         ↗
           |       /
           |     /
           |   /
           | /
           └────────→ +X (RIGHT)

        • X-axis: +X = RIGHT,  -X = LEFT
        • Y-axis: +Y = DOWN,   -Y = UP
        • Z-axis: +Z = BACK,   -Z = FORWARD
        ```
    *   **Rationale:** This design decision was made to:
        -   Eliminate coordinate conversions between engine and Vulkan API (zero-cost abstraction)
        -   Avoid performance overhead from constant Y-axis flips in critical paths
        -   Prevent subtle bugs caused by mixing conventions across subsystems
        -   Maintain consistency and clarity throughout the codebase
    *   **Important:** While counter-intuitive (humans naturally think "up = positive"), this convention must be respected in ALL code. Never introduce Y-axis conversions or assumptions based on traditional math coordinates (Y-up).
    *   **Movement Examples (Player/Object perspective):**
        -   Moving upward: `velocity.y < 0` (toward -Y)
        -   Falling downward: `velocity.y > 0` (toward +Y)
        -   Moving right: `velocity.x > 0` (toward +X)
        -   Moving left: `velocity.x < 0` (toward -X)
        -   Moving forward: `velocity.z < 0` (toward -Z)
        -   Moving backward: `velocity.z > 0` (toward +Z)
    *   **Normal Vector Examples:**
        -   Ground normal (pointing up): `(0, -1, 0)`
        -   Ceiling normal (pointing down): `(0, +1, 0)`
        -   Right wall normal (pointing left): `(-1, 0, 0)`
        -   Left wall normal (pointing right): `(+1, 0, 0)`
        -   Front wall normal (pointing back): `(0, 0, +1)`
        -   Back wall normal (pointing forward): `(0, 0, -1)`
    *   **Physics Examples:**
        -   Gravity acceleration: `+9.81` on Y-axis (pulls down, positive Y)
        -   Jump impulse: negative Y value (pushes up, negative Y)
        -   Forward thrust: negative Z value (pushes forward, negative Z)

4.  **Saphir Shader System:**
    *   Saphir automatically generates GLSL shader code at runtime based on material properties, geometry attributes, and scene context.
    *   Eliminates the need for hundreds of manually-written shader variants.
    *   Performs strict compatibility checking: if material requirements don't match geometry attributes, resource loading fails gracefully with explanatory logs.
    *   Multiple specialized generators: SceneGenerator (3D with lighting), OverlayGenerator (2D UI), ShadowManager (shadow maps).
    *   Do not manually write shader code unless creating new Saphir features or custom generators. See "Saphir Shader Generation System" section for details.

5.  **Scene Graph Architecture:**
    *   The scene graph is the primary organizational structure for game objects.
    *   All scene objects (nodes) inherit from a base node class.
    *   Parent-child relationships define transformations and lifecycle dependencies.
    *   Scene graph updates happen in a specific order (transform update, then render).

6.  **Resource Management:**
    *   All resources (textures, models, sounds) must be loaded through the resource management system.
    *   Resources are reference-counted and automatically unloaded when no longer used.
    *   Resource loading is asynchronous by design, allowing immediate usage while loading in background.
    *   The system ALWAYS provides a valid resource (never nullptr), using default/neutral resources as fallbacks.
    *   Client code never checks for loading errors or nullptr - the engine handles all failure modes gracefully. See "Resource Management System" section for details.

7.  **Physics System:**
    *   Physics simulation is decoupled from rendering; it runs at a fixed timestep.
    *   Physics objects are synchronized with scene graph nodes.
    *   Collision detection uses spatial acceleration structures (octree, BVH).
    *   Physics queries (raycasting, shape casting) should be efficient and thread-safe.
    *   The engine distinguishes 4 entity types with different collision semantics: Boundaries, Ground, StaticEntity, and Nodes. See "Physics System Architecture" section for details.

8.  **Audio System:**
    *   Audio uses OpenAL for 3D positional audio.
    *   Audio sources are attached to scene nodes for automatic position updates.
    *   Support streaming for large audio files (music, ambient sounds).
    *   Provide audio source pooling for efficient runtime performance.

9.  **Input System:**
    *   Input is abstracted to support multiple platforms (keyboard, mouse, gamepad).
    *   Input events are processed through a centralized input manager.
    *   Support for input mapping and remapping at runtime.
    *   Handle platform-specific input quirks transparently.

10. **Platform Abstraction:**
    *   Platform-specific code must be isolated in `PlatformSpecific/` directory.
    *   Use platform-independent interfaces; implement platform-specific backends.
    *   File paths should use forward slashes internally; convert to native format when needed.
    *   Handle platform differences in window management, file systems, and system APIs.

11. **Service Interfaces:**
    *   Core systems are exposed through service interfaces defined in `ServiceInterface.hpp`.
    *   Services follow a consistent initialization and shutdown pattern.
    *   Services should be loosely coupled and communicate through well-defined interfaces.

---

## Physics System Architecture

This section provides detailed architecture for the physics system, including entity types, collision semantics, and execution flow.

### Quick Reference: Key Terminology

- **Contact Manifold**: Data structure containing collision information (contact points, penetration depth, collision normal) between two entities.
- **Constraint Solver**: Iterative physics solver (Sequential Impulse method) that resolves collisions by computing and applying impulses to entities.
- **Hard Clipping**: Direct position correction that prevents entities from penetrating boundaries or ground, ensuring absolute constraint enforcement.
- **Velocity Inversion**: Manual velocity reversal applied for game design purposes (e.g., bouncing off invisible boundaries).
- **Impulse**: Instantaneous change in momentum applied to an entity (force × time), used to resolve collisions realistically.
- **Restitution (Bounciness)**: Material property coefficient [0.0-1.0] that determines energy preservation in collisions (0 = no bounce, 1 = perfect bounce).
- **SceneAreaResource**: Interface that provides ground height queries at any 2D coordinate: `(X, Z) → Y height`.

### Design Philosophy: Why 4 Entity Types?

The physics system distinguishes between **four fundamental entity types** to balance realism, performance, and game design needs:

1. **Boundaries** → Pure game design (invisible walls for player containment)
2. **Ground** → Hybrid approach (stability + realism)
3. **StaticEntity** → Performance optimization (immovable objects with simplified physics)
4. **Nodes** → Full realism (complete Newtonian physics simulation)

This separation allows each type to have **clear, non-overlapping collision semantics** while maintaining **uniform behavior within each type**.

### Entity Type 1: Scene Boundaries (World Cube Limits)

**Category:** Logical/Technical Constraint (Game Design)

**Purpose:** Define the playable area; nothing can exit this cube. Size is variable and fixed at scene loading time.

**Why It Exists:** Prevents entities from escaping the game world. Since invisible walls don't exist in reality, we fake physics for better game feel rather than attempting realistic simulation.

**Collision Behavior:**
- **Hard clipping:** Position is corrected to prevent escape (absolute constraint).
- **Velocity inversion:** Velocity is manually inverted or dampened to avoid jarring stops (e.g., hitting at 250 km/h).
- **NO manifolds:** Boundaries do NOT generate contact manifolds for the physics solver.
- **NO realistic physics:** This is purely for game design.

**Implementation Guidelines:**
- Check boundaries AFTER ground collision.
- Apply hard position correction: `entity->clampToBoundary(boundary)`.
- Manually invert/dampen velocity: `velocity = -velocity * dampening`.
- Never route boundary collisions through the constraint solver.

### Entity Type 2: Ground (Scene Area Resource)

**Category:** Physical Surface (Hybrid Approach)

**Purpose:** Defines the terrain/floor height at any 2D coordinate `(X, Z)`. The `SceneAreaResource` interface provides height queries: `getHeightAt(x, z) → y`.

**Why It Exists:** Ground is a physical surface that should behave realistically (bounce, friction), but we add hard clipping for guaranteed stability (prevents tunneling through terrain).

**Collision Behavior:**
- **Hard clipping:** Guarantees entities never penetrate below ground (stability).
- **+ Contact manifolds:** Creates manifolds for realistic physics resolution by the solver.
- **Restitution/friction:** Solver applies material properties (bounce, friction) based on surface and entity properties.
- **Realistic physics:** Behaves like any other physical surface after clipping.

**Implementation Guidelines:**
- **CRITICAL:** Calculate `penetrationDepth` BEFORE applying hard clip (otherwise penetration = 0 and solver won't work).
- Apply hard clip for stability: `if (entity.y > groundY) entity.setY(groundY);`.
- Create manifold with correct penetration depth for solver velocity correction.
- Check ground collision FIRST (before boundaries).

**Example:**
```cpp
float groundY = sceneArea->getHeightAt(entity.x, entity.z);
float penetrationDepth = entity.y - groundY;  // Calculate BEFORE clip
if (penetrationDepth > 0.0F) {
    // 1. Hard clip for stability
    entity.setYPosition(groundY);

    // 2. Create manifold for solver (preserves penetration depth for velocity correction)
    ContactManifold manifold{entity, ground, penetrationDepth, normal};
    groundManifolds.push_back(manifold);
}
```

### Entity Type 3: StaticEntity (Kinematic Deflector)

**Category:** Physical Object with Kinematic Constraint (Performance Optimization)

**Purpose:** Static world objects that affect physics but never move themselves (e.g., buildings, rocks, walls).

**Why It Exists:** Performance and design simplification. In most games, static objects don't need full physics simulation (force accumulation, integration). StaticEntity has defined mass for correct collision response but is kinematically constrained (cannot move).

**Example Use Cases:**
- Normal game: House = StaticEntity (has mass ~50000 kg, never moves).
- Destructible game: House = Node (has mass, can be destroyed/moved).

**Collision Behavior:**
- **Contact manifolds:** Full manifold creation with defined mass (NOT infinite mass).
- **Impulse application:** Solver applies impulses ONLY to the moving Node, NOT to StaticEntity.
- **Mass matters:** StaticEntity mass affects collision response calculation (heavier StaticEntity = less bounce for the Node).
- **Simplified model:** No force accumulation or velocity integration for StaticEntity.

**Implementation Guidelines:**
- StaticEntity has `m_mass` defined (e.g., 50000 kg for a house, 10000 kg for a boulder).
- Flag check: `entity->isStatic()` returns true → solver skips impulse application to this entity.
- StaticEntity mass is used when calculating impulse magnitude on the colliding Node.
- StaticEntity position never changes during physics simulation.

### Entity Type 4: Nodes (Dynamic Entities)

**Category:** Full Physics Actors (Complete Realism)

**Purpose:** All dynamic game objects that move and interact through physics (players, projectiles, vehicles, movable objects).

**Why It Exists:** These are the primary simulation subjects. Nodes receive full Newtonian physics treatment with no compromises or simplifications.

**Collision Behavior:**
- **Contact manifolds:** Full manifold creation with both entities' masses considered.
- **Bilateral impulses:** Solver applies impulses to BOTH Nodes in a collision (Newton's third law).
- **Material properties:** Friction, restitution, drag, density all affect behavior.
- **Complete simulation:** Position integration, velocity integration, force accumulation, torque, angular velocity.

**Implementation Guidelines:**
- All Nodes participate equally in physics simulation (no special cases).
- Node↔Node collision: both receive impulses proportional to their masses.
- Integration: `position += velocity * dt; velocity += (forces / mass) * dt`.
- No exceptions or shortcuts; pure physics simulation.

### Physics Execution Order

The physics system executes in this precise order each fixed timestep:

```
1. Integrate forces
   → Update velocities and positions for all Nodes
   → Apply gravity, drag, user forces
   → Forward integrate: position += velocity * dt

2. Broad phase collision detection
   → Octree spatial queries to find potentially colliding pairs
   → Cull pairs that are too far apart

3. Narrow phase collision detection
   → Precise collision detection (sphere-sphere, AABB-AABB, etc.)
   → Generate contact manifolds for all colliding pairs

4. Solve inter-entity collisions (FIRST solver call)
   → Process manifolds: Node↔Node, Node↔StaticEntity
   → Apply velocity and position corrections via impulses
   → Iterative solving (8 velocity iterations, 3 position iterations)

5. Ground collision check
   → Query SceneAreaResource for ground height
   → Calculate penetration depth BEFORE clipping
   → Apply hard clip: entity.y = groundY (stability guarantee)
   → Create ground manifolds (preserve penetration depth)

6. Boundary collision check
   → Check if entity exceeds scene cube boundaries
   → Apply hard clip: clamp position to boundary
   → Manually invert/dampen velocity (NO manifolds)

7. Solve ground collisions (SECOND solver call)
   → Process ground manifolds from step 5
   → Apply velocity corrections for realistic bounce/friction
   → Iterative solving (same iteration counts)
```

**Key Points:**
- Two separate solver calls: inter-entity collisions first, then ground collisions.
- Ground penetration depth calculated BEFORE hard clipping (critical for solver).
- Boundaries use manual velocity handling (no solver involvement).

### Design Principles Summary

| Entity Type     | Physics Approach        | Collision Method              | Purpose                          |
|-----------------|-------------------------|-------------------------------|----------------------------------|
| **Boundaries**  | Fake physics            | Hard clip + velocity inversion | Game design constraint          |
| **Ground**      | Hybrid                  | Hard clip + manifolds         | Stability + realism             |
| **StaticEntity**| Simplified realistic    | Manifolds (no impulse recv)   | Performance optimization        |
| **Nodes**       | Full realistic          | Manifolds (bilateral impulse) | Pure Newtonian simulation       |

**Core Principles:**
- **Separation:** Each type has clear, non-overlapping collision semantics.
- **Consistency:** Within each type, behavior is uniform and predictable.
- **No leakage:** Implementation details of one type don't affect others.
- **Explicit tradeoffs:** Each type represents an explicit design decision (realism vs performance vs game feel).

---

## Resource Management System

This section provides detailed architecture for the resource management system, including the fail-safe design philosophy, dependency tracking, and asynchronous loading mechanisms.

### Quick Reference: Key Terminology

- **Manager**: Central service that provides access to all resource Containers (one per resource type). Handles global operations like garbage collection.
- **Container\<resource_t\>**: Template class serving as a "Store" for one specific resource type. Manages caching, loading, and lifecycle of resources. Also called "Store" in context.
- **ResourceTrait**: Base interface for all loadable resources. Provides dependency tracking, loading lifecycle states, and observable notifications.
- **Top-Resource**: A resource at the top of a dependency chain (e.g., MeshResource that depends on MaterialResource, which depends on TextureResources). Has no parent resources waiting for it.
- **Dependency Chain**: Hierarchical relationship where resources depend on sub-resources. Loading propagates from leaves to root.
- **Neutral Resource**: Default/fallback version of a resource created without loading external data. Always functional and identifiable (e.g., checkerboard texture, gray cube).
- **LoadingRequest**: Internal wrapper for asynchronous loading operations. Handles file loading, network downloads, and caching.
- **Reference Counting**: Automatic memory management using `std::shared_ptr`. Resources are kept in memory as long as they're referenced.
- **Garbage Collection**: Process of unloading resources when `use_count() == 1` (only Container holds reference).

### Design Philosophy: Fail-Safe Architecture

The resource management system's **fundamental responsibility** is to **always provide a valid, usable resource**, regardless of any failure condition.

**Core Principle: Never Crash, Always Provide**

The system is designed so that:
- Client code **never receives nullptr**
- Client code **never checks for loading errors**
- Client code **never handles failure modes**
- Application **never crashes** due to missing/corrupt resources
- Errors are **logged** but **don't propagate** as exceptions
- Default/neutral resources serve as **fail-safe fallbacks**

**Why This Design?**

In production game engines and real-time applications:
- Better to show a placeholder (pink checkerboard texture) than crash
- Gameplay code should focus on gameplay, not resource error handling
- Developers identify issues through logs and visual placeholders
- "The show must go on" - application continues running even with broken assets

**Design Tradeoffs:**
- **Robustness** over correctness: Graceful degradation preferred
- **Simplicity** for client code: Zero error handling required
- **Transparency**: Loading is asynchronous and automatic
- **Identifiability**: Placeholders are obvious to developers

### Architecture: Three-Layer System

**Layer 1: Manager (Central Coordinator)**
```cpp
// src/Resources/Manager.hpp
class Manager : public ServiceInterface, public ServiceProvider {
    std::map<std::type_index, std::unique_ptr<ContainerInterface>> m_containers;
    std::unordered_map<std::string, shared_ptr<...>> m_localStores;

    // Access a specific resource type's Container
    template<typename T>
    Container<T>* container();

    // Global operations
    size_t memoryOccupied() const;
    size_t unusedMemoryOccupied() const;
    size_t unloadUnusedResources();
};
```

**Responsibilities:**
- Provides access to type-specific Containers
- Coordinates garbage collection across all resource types
- Manages resource index files (stores)
- Reports global memory usage

**Layer 2: Container\<resource_t\> (Type-Specific Store)**
```cpp
// src/Resources/Container.hpp
template<typename resource_t>
class Container : public ContainerInterface, public ObserverTrait {
    std::unordered_map<std::string, std::shared_ptr<resource_t>> m_resources;
    std::shared_ptr<std::unordered_map<std::string, BaseInformation>> m_localStore;

    // Core API
    std::shared_ptr<resource_t> getResource(const std::string& name, bool asyncLoad = true);
    std::shared_ptr<resource_t> getOrCreateResource(const std::string& name, ...);
    std::shared_ptr<resource_t> getDefaultResource();

    // Management
    bool preloadResource(const std::string& name);
    size_t unloadUnusedResources();
};
```

**Responsibilities:**
- **Caching**: Keeps `shared_ptr` to all loaded resources (fast reuse)
- **Loading**: Triggers async or sync loading when resources are requested
- **Default fallback**: Always returns valid resource (real or default)
- **Garbage collection**: Unloads resources with `use_count() == 1`
- **Thread safety**: Protects resource map with mutex

**Layer 3: ResourceTrait (Base Interface)**
```cpp
// src/Resources/ResourceTrait.hpp
class ResourceTrait : public std::enable_shared_from_this<ResourceTrait>,
                      public NameableTrait, public FlagTrait, public ObservableTrait {
    std::vector<std::shared_ptr<ResourceTrait>> m_parentsToNotify;
    std::vector<std::shared_ptr<ResourceTrait>> m_dependenciesToWaitFor;
    Status m_status;  // Unloaded, Loading, Loaded, Failed

    // Loading lifecycle
    virtual bool load(ServiceProvider&) = 0;  // Neutral resource (no params)
    virtual bool load(ServiceProvider&, const std::filesystem::path&) = 0;
    virtual bool load(ServiceProvider&, const Json::Value&) = 0;

    // Dependency management
    bool addDependency(const std::shared_ptr<ResourceTrait>& dependency);
    virtual bool onDependenciesLoaded();  // Finalization hook

    // State queries
    bool isLoaded() const;
    bool isLoading() const;
    Status status() const;
};
```

**Responsibilities:**
- **Dependency tracking**: Maintains parent-child relationships
- **Loading states**: Tracks Unloaded → Loading → Loaded/Failed transitions
- **Event propagation**: Notifies parents when dependencies complete
- **Finalization**: Provides hook (`onDependenciesLoaded`) for GPU upload, etc.

### The Neutral Resource Pattern (Critical!)

**Every resource type MUST implement `load(ServiceProvider&)` without parameters.**

This method creates a **neutral/default** version of the resource that:
- **Always succeeds** (no file I/O, no network, no dependencies)
- **Is immediately usable** (valid GPU resources, valid data structures)
- **Is easily identifiable** by developers (visual/audio placeholders)

**Examples:**

```cpp
// TextureResource - Procedural checkerboard
class TextureResource : public ResourceTrait {
    bool load(ServiceProvider& provider) override {
        // Generate 64×64 checkerboard pattern (pink/black)
        m_pixels = generateCheckerboard(64, 64, Color::Pink, Color::Black);
        m_width = 64;
        m_height = 64;
        return true;  // Always succeeds
    }

    bool load(ServiceProvider& provider, const std::filesystem::path& path) override {
        // Load real texture from file
        if (!loadImageFile(path, m_pixels, m_width, m_height)) {
            return false;  // Can fail
        }
        return true;
    }

    bool onDependenciesLoaded() override {
        // Upload to GPU (same for neutral or real)
        m_vkImage = vulkan.createImage(m_pixels, m_width, m_height);
        return true;
    }
};

// MeshResource - Procedural gray cube
class MeshResource : public ResourceTrait {
    bool load(ServiceProvider& provider) override {
        // Generate unit cube geometry
        m_vertices = generateCubeVertices(1.0F);
        m_indices = generateCubeIndices();

        // Neutral gray material (no dependencies)
        m_material = provider.container<MaterialResource>()->getDefaultResource();

        return true;  // Always succeeds
    }

    bool load(ServiceProvider& provider, const Json::Value& data) override {
        // Load real mesh from data
        m_vertices = loadVertices(data);
        m_indices = loadIndices(data);

        // Add material dependency
        auto material = provider.container<MaterialResource>()->getResource(data["material"]);
        addDependency(material);  // ← Keeps resource in Loading state

        return true;
    }

    bool onDependenciesLoaded() override {
        // ← ALL dependencies loaded! Material + textures ready
        // Upload complete data to GPU
        m_vertexBuffer = vulkan.createBuffer(m_vertices);
        m_indexBuffer = vulkan.createBuffer(m_indices);
        return true;
    }
};
```

**Neutral Resource Guarantees:**
- Created once per Container, cached as "Default"
- Zero dependencies (self-contained)
- No external data required
- Never fails to load
- Immediately usable after creation

### Resource Lifecycle and Dependency Chain

**Complete Resource Lifecycle:**

```
┌─────────────┐
│  Unloaded   │  Initial state
└──────┬──────┘
       │ getResource() or addDependency()
       ▼
┌─────────────┐
│   Loading   │  load() called, dependencies declared
└──────┬──────┘
       │ All sub-resources finish
       ▼
┌─────────────┐
│onDependencies│  Finalization hook (GPU upload, etc.)
│   Loaded()  │
└──────┬──────┘
       │ Finalization succeeds
       ▼
┌─────────────┐
│   Loaded    │  Resource ready for use
└─────────────┘

       OR (if fails)
       ▼
┌─────────────┐
│   Failed    │  Container returns Default instead
└─────────────┘
```

**Dependency Chain Example: MeshResource → MaterialResource → TextureResources**

```
User requests: container->getResource("character.mesh")

Step 1: Container creates MeshResource
   ┌──────────────────┐
   │  MeshResource    │ Status: Loading
   │  "character"     │
   └────────┬─────────┘
            │ load() calls addDependency(materialResource)
            ▼
Step 2: MeshResource depends on MaterialResource
   ┌──────────────────┐
   │  MeshResource    │ Status: Loading, waiting for 1 dependency
   │  "character"     │
   └────────┬─────────┘
            │
   ┌────────▼─────────┐
   │ MaterialResource │ Status: Loading
   │  "char_skin"     │
   └────────┬─────────┘
            │ load() calls addDependency(texture1), addDependency(texture2)
            ▼
Step 3: MaterialResource depends on TextureResources
   ┌──────────────────┐
   │  MeshResource    │ Status: Loading, waiting for 1 dependency
   │  "character"     │
   └────────┬─────────┘
            │
   ┌────────▼─────────┐
   │ MaterialResource │ Status: Loading, waiting for 2 dependencies
   │  "char_skin"     │
   └────────┬─────────┘
            │
     ┌──────┴──────┐
     ▼             ▼
┌─────────┐   ┌─────────┐
│Texture  │   │Texture  │ Status: Loading (async)
│"diffuse"│   │"normal" │
└────┬────┘   └────┬────┘
     │             │
     │ Loading finishes
     ▼             ▼
┌─────────┐   ┌─────────┐
│Texture  │   │Texture  │ Status: Loaded
│"diffuse"│   │"normal" │ → Notifies parent (MaterialResource)
└────┬────┘   └────┬────┘
     │             │
     └──────┬──────┘
            ▼
Step 4: MaterialResource checks dependencies
   ┌──────────────────┐
   │ MaterialResource │ checkDependencies() → all loaded!
   │  "char_skin"     │ onDependenciesLoaded() → upload to GPU
   └────────┬─────────┘ Status: Loaded → Notifies parent (MeshResource)
            │
            ▼
Step 5: MeshResource checks dependencies
   ┌──────────────────┐
   │  MeshResource    │ checkDependencies() → all loaded!
   │  "character"     │ onDependenciesLoaded() → upload vertex/index buffers
   └──────────────────┘ Status: Loaded → READY FOR USE!
```

**Key Implementation Details:**
```cpp
// Inside resource load()
bool MeshResource::load(ServiceProvider& provider, const Json::Value& data) {
    // 1. Load immediate data
    m_vertices = loadVertices(data);
    m_indices = loadIndices(data);

    // 2. Declare dependencies
    auto material = provider.container<MaterialResource>()->getResource(data["material"]);
    addDependency(material);  // ← Resource stays in Loading state

    return true;  // load() succeeded, but resource not Loaded yet
}

// Called automatically when all dependencies finish
bool MeshResource::onDependenciesLoaded() {
    // 3. Finalize with complete data
    // At this point: geometry + material + textures ALL ready
    m_vertexBuffer = vulkan.createBuffer(m_vertices);
    m_indexBuffer = vulkan.createBuffer(m_indices);

    // 4. Resource transitions to Loaded state
    return true;
}
```

### Return Scenarios: Real, Default (Not Found), Default (Failed)

The Container **always** returns a valid `std::shared_ptr<resource_t>`. Three scenarios:

**Scenario 1: Resource Found and Loaded Successfully**
```cpp
auto tex = container->getResource("logo.png");
// → Container finds "logo.png" in store
// → Creates TextureResource, loads from file
// → Returns shared_ptr (Status: Loading → Loaded)
// Result: Real texture displayed
```

**Scenario 2: Resource Not Found in Store**
```cpp
auto tex = container->getResource("missing.png");
// → Container checks store, not found
// → Returns container->getDefaultResource()
// Result: Pink checkerboard displayed, warning logged
```

**Scenario 3: Resource Loading Fails**
```cpp
auto tex = container->getResource("corrupt.png");
// → Container finds "corrupt.png" in store
// → Creates TextureResource, attempts load
// → load() returns false (file corrupt, parse error, etc.)
// → Container returns getDefaultResource() instead
// Result: Pink checkerboard displayed, error logged
```

**Implementation in Container:**
```cpp
template<typename resource_t>
std::shared_ptr<resource_t> Container<resource_t>::getResource(const std::string& name, bool asyncLoad) {
    // Check if already loaded (cache)
    if (m_resources.contains(name)) {
        return m_resources[name];  // Fast return
    }

    // Check if exists in store
    if (!m_localStore->contains(name)) {
        // NOT FOUND → Return Default
        return getDefaultResource();
    }

    // Create and load resource
    auto resource = std::make_shared<resource_t>(name);
    m_resources[name] = resource;  // Add to cache immediately

    // Async loading
    threadPool->enqueue([this, resource, filepath]() {
        if (!resource->load(serviceProvider, filepath)) {
            // LOAD FAILED → Replace with Default
            m_resources[resource->name()] = getDefaultResource();
        }
    });

    return resource;  // Return immediately (may still be Loading)
}

template<typename resource_t>
std::shared_ptr<resource_t> Container<resource_t>::getDefaultResource() {
    // Check if Default already exists
    if (m_resources.contains("Default")) {
        return m_resources["Default"];
    }

    // Create neutral resource (always succeeds)
    auto defaultResource = std::make_shared<resource_t>("Default");
    defaultResource->load(m_serviceProvider);  // ← No parameters!
    m_resources["Default"] = defaultResource;

    return defaultResource;
}
```

### Client Code Usage: Zero Error Handling

**Complete Example: Loading and Using a Mesh**
```cpp
// In projet-alpha (client application)

// 1. Request resource - ALWAYS succeeds, NEVER nullptr
auto mesh = resources.container<MeshResource>()->getResource("character");

// 2. Attach to scene node - ALWAYS safe
auto node = scene->createChild("player");
node->attachMesh(mesh);  // No checks needed!

// 3. Rendering - Engine handles state
// Frame 1-50: mesh is Loading → Renderer skips (not displayed yet)
// Frame 51+: mesh is Loaded → Renderer draws
// If load failed: Default mesh displayed (gray cube)

// Client code never checks anything:
// - No if (mesh != nullptr)
// - No if (mesh->isLoaded())
// - No try/catch
// - Just use it!
```

**Advanced Usage: Custom Resource Creation**
```cpp
// Create procedural resource (not from file)
auto proceduralMesh = resources.container<MeshResource>()->getOrCreateResource(
    "ProceduralTerrain",
    [](MeshResource& mesh) {
        // Custom generation function
        mesh.generateTerrain(1024, 1024, heightmapData);
        return mesh.setManualLoadSuccess(true);
    }
);
```

### Thread Safety and Garbage Collection

**Thread Safety:**
- All Container methods are protected by `std::mutex m_resourcesAccess`
- Async loading happens in thread pool, results synchronized via mutex
- ResourceTrait has `std::mutex m_dependenciesAccess` for dependency list
- Client code doesn't need to worry about threading

**Garbage Collection:**
```cpp
// Each Container tracks usage
size_t Container<resource_t>::unusedMemoryOccupied() const {
    size_t bytes = 0;
    for (const auto& resource : m_resources | std::views::values) {
        if (resource.use_count() == 1) {  // Only Container holds it
            bytes += resource->memoryOccupied();
        }
    }
    return bytes;
}

// Unload unused resources
size_t Container<resource_t>::unloadUnusedResources() {
    size_t unloaded = 0;
    for (auto it = m_resources.begin(); it != m_resources.end(); ) {
        if (it->second.use_count() == 1) {  // Only we hold it
            it = m_resources.erase(it);  // Destroy resource
            unloaded++;
        } else {
            ++it;
        }
    }
    return unloaded;
}

// Manager can trigger global GC
size_t Manager::unloadUnusedResources() {
    size_t total = 0;
    for (auto& [type, container] : m_containers) {
        total += container->unloadUnusedResources();
    }
    return total;
}
```

**When to Trigger GC:**
- After scene transitions (old scene resources released)
- When memory pressure is detected
- Manual trigger via console command
- Periodic cleanup (e.g., every 60 seconds)

### Implementing a New Resource Type

**Step-by-Step Guide:**

1. **Inherit from ResourceTrait:**
```cpp
class MyResource : public ResourceTrait {
public:
    static constexpr auto ClassId{"MyResource"};
    static constexpr DepComplexity Complexity = DepComplexity::Simple;

    MyResource(const std::string& name, uint32_t flags = 0)
        : ResourceTrait(name, flags) {}
};
```

2. **Implement Neutral Resource (REQUIRED):**
```cpp
bool load(ServiceProvider& provider) override {
    // Create default/neutral version (no external data)
    // MUST always succeed!
    m_data = createDefaultData();
    return true;
}
```

3. **Implement File/Data Loading:**
```cpp
bool load(ServiceProvider& provider, const std::filesystem::path& path) override {
    // Load from file (CAN fail)
    if (!loadFromFile(path, m_data)) {
        return false;  // Container will use Default instead
    }
    return true;
}

bool load(ServiceProvider& provider, const Json::Value& data) override {
    // Load from JSON (CAN fail, CAN have dependencies)
    m_data = parseData(data);

    // Declare dependencies if needed
    if (data.isMember("dependency")) {
        auto dep = provider.container<OtherResource>()->getResource(data["dependency"]);
        addDependency(dep);
    }

    return true;
}
```

4. **Implement Finalization Hook:**
```cpp
bool onDependenciesLoaded() override {
    // Called when ALL dependencies are loaded
    // Perfect place for GPU uploads, processing, etc.
    m_gpuResource = uploadToGPU(m_data);
    return true;
}
```

5. **Register with Manager:**
```cpp
// In Manager::onInitialize()
auto store = getLocalStore("MyResources");
auto container = std::make_unique<Container<MyResource>>(
    MyResource::ClassId,
    m_primaryServices,
    *this,
    store
);
m_containers.emplace(typeid(MyResource), std::move(container));
```

### Design Principles Summary

| Principle | Description | Benefit |
|-----------|-------------|---------|
| **Always Valid** | Never return nullptr, always provide usable resource | Client code simplicity, zero crashes |
| **Fail-Safe** | Default resources as fallbacks for all failures | Application continues running |
| **Async by Design** | Immediate return, background loading | No blocking, smooth experience |
| **Dependency Aware** | Automatic tracking and propagation | Correct load order guaranteed |
| **Observable** | Event notifications for load state | Engine can react, client doesn't need to |
| **Cached** | `shared_ptr` in Container for fast reuse | Performance optimization |
| **Self-Cleaning** | Garbage collection based on `use_count()` | Automatic memory management |
| **Identifiable** | Visual/audio placeholders for missing resources | Developer-friendly debugging |

**Core Philosophy:**
> "The resource manager's job is to provide a resource, no matter what. The client's job is to use it. That's it."

---

## Saphir Shader Generation System

This section provides detailed architecture for Saphir, the automatic GLSL shader generation system that eliminates the need for hundreds of manually-written shader variants.

### Quick Reference: Key Terminology

- **Generator**: A specialized shader factory that produces GLSL code based on inputs (unknowns). Examples: SceneGenerator, OverlayGenerator, ShadowManager.
- **Unknowns**: The parameters a generator needs to produce a complete shader: Material properties, Geometry attributes, Scene context.
- **Material Requirements**: What a material needs from geometry to function (normals, tangent space, UVs, vertex colors).
- **Geometry Attributes**: What a geometry provides (positions, normals, tangents, UVs, colors) - the vertex format.
- **Compatibility Check**: Strict verification that material requirements match available geometry attributes. Failure = resource loading fails gracefully.
- **GLSL Source**: String-based shader code generated by combining hard-coded GLSL portions with variables.
- **GLSLang**: The compiler that converts generated GLSL source to SPIR-V binary for Vulkan.
- **Cache**: System that stores compiled shaders (keyed by source code hash) to avoid regeneration and recompilation.
- **Vertex Format**: Structure describing all attributes available in a geometry (queryable by generators).

### Design Philosophy: Why Saphir Exists

**The Problem: Combinatorial Explosion**

Traditional approach to shaders requires manually writing every variant:

```
Manual shader files:
- shader_diffuse.glsl
- shader_diffuse_normal.glsl
- shader_diffuse_normal_specular.glsl
- shader_diffuse_normal_specular_emissive.glsl
- shader_pbr_simple.glsl
- shader_pbr_full.glsl
- shader_textured.glsl
- shader_vertexcolored.glsl
... (hundreds of permutations)
```

**Problems with manual approach:**
- **Maintenance nightmare**: Code duplication across variants
- **Error-prone**: Easy to forget variants or introduce inconsistencies
- **Inflexible**: Adding a new feature requires modifying many files
- **Costly**: Disk space, compile time, cognitive load

**Saphir's Solution: Parametric Generation**

```
Saphir approach:
Material + Geometry + Scene Context
    ↓
Generator.generate()
    ↓
GLSL source (perfectly adapted)
    ↓
GLSLang.compile() → SPIR-V
    ↓
Vulkan pipeline
```

**Benefits:**
- ✅ **Zero maintenance** - Code exists once in generator
- ✅ **Always optimal** - Only includes needed features
- ✅ **No forgotten variants** - Generated on demand
- ✅ **Flexible** - New features added in one place
- ✅ **Efficient** - Cache avoids redundant work

### Architecture: Generators and Unknowns

**Core Concept: Generators Are Shaders Awaiting Their Unknowns**

A generator is essentially a **parameterized shader template**. It has "holes" (unknowns) that must be filled to produce a complete shader.

```cpp
// Conceptual model
class Generator {
    // The unknowns (parameters)
    Material material;
    Geometry geometry;
    SceneContext scene;

    // The output
    string generateGLSL();
};
```

**The Three Unknowns:**

1. **Material (What the shader does)**
   - Which textures are used? (diffuse, normal, roughness, emissive)
   - Material model? (PBR, Phong, unlit)
   - Transparency? Blending mode?
   - Special effects? (emissive, refraction)

2. **Geometry (What data is available)**
   - Vertex format: positions, normals, tangents, UVs, colors
   - Single or multiple UV sets?
   - 2D or 3D texture coordinates?
   - Complete tangent space or normals only?

3. **Scene Context (Environment requirements)**
   - Number and types of lights (directional, point, spot)
   - Shadows enabled?
   - Post-processing requirements?
   - Render pass type (forward, deferred, shadow map)

**Output:**
- **GLSL source code** (string) - directly compilable
- **Parameters list** (optional) - for OpenGL compatibility (not needed for Vulkan)

### Material Requirements and Geometry Attributes

**How Materials Declare Requirements**

Materials declare what they **need** from geometry using a basic "plan" (internal format readable by all generators):

**Four Basic Requirements:**
1. **Normals** → For lighting calculations
2. **Tangent Space** → For normal mapping (requires tangents + bitangents)
3. **Texture Coordinates** → Primary UVs (2D or 3D)
4. **Vertex Colors** → Per-vertex color attributes

**Example Material Plans:**
```cpp
// Simple diffuse material
Material simpleDiffuse {
    requires: [Normals, TextureCoordinates2D]
    textures: [diffuse]
};

// PBR with normal mapping
Material pbrFull {
    requires: [Normals, TangentSpace, TextureCoordinates2D]
    textures: [diffuse, normal, roughness, metallic]
};

// Vertex painted
Material vertexPainted {
    requires: [Normals, VertexColors]
    textures: []
};
```

**How Geometries Provide Attributes**

Geometries provide their **vertex format** - what attributes they contain:

```cpp
// Geometry is queryable in two ways:

// 1. Get complete structure
VertexFormat format = geometry->getVertexFormat();
// format.hasPositions = true
// format.hasNormals = true
// format.hasTangents = false
// format.hasUVs = true (2D)
// format.hasColors = false

// 2. Query individual attributes
if (geometry->hasNormals()) { ... }
if (geometry->hasTangentSpace()) { ... }
if (geometry->hasTextureCoordinates()) { ... }
if (geometry->hasVertexColors()) { ... }
```

### Strict Compatibility Checking (Critical!)

**Core Rule: Material Requirements MUST Match Geometry Attributes**

When a visual resource enters `onDependenciesLoaded()` and needs a rendering program:

```
1. Generator receives Material + Geometry
2. Generator checks: Does Geometry have ALL Material requirements?
   ✓ YES → Generate shader, compile, create pipeline
   ✗ NO  → FAIL immediately with explanatory log
3. If FAIL → onDependenciesLoaded() returns false
4. Resource never completes loading → never displayed
5. Application continues running (fail-safe)
```

**Example: Incompatible Combination**
```cpp
// Material wants normal mapping
Material material {
    requires: [Normals, TangentSpace, TextureCoordinates2D],
    textures: [diffuse, normal]
};

// Geometry doesn't have tangent space
Geometry geometry {
    attributes: [positions, normals, uvs]  // NO tangents!
};

// Result: FAILURE
Generator::check(material, geometry):
    Material requires: TangentSpace
    Geometry provides: positions, normals, uvs
    Missing: TangentSpace

    Log: "Shader generation failed for geometry 'SimpleBox' with material 'PBR_Metal':
          Required attribute missing: tangent space.
          Add tangents to geometry or use simpler material."

    return false;  // onDependenciesLoaded() returns false
    // Object never displayed, but application continues
```

**Example: Compatible Combination**
```cpp
// Material wants texturing
Material material {
    requires: [Normals, TextureCoordinates2D],
    textures: [diffuse]
};

// Geometry has everything
Geometry geometry {
    attributes: [positions, normals, uvs]  // All requirements met!
};

// Result: SUCCESS
Generator::check(material, geometry):
    Material requires: Normals ✓, TextureCoordinates2D ✓
    Geometry provides: normals ✓, uvs ✓

    return true;  // Proceed to shader generation
```

**Why Strict Checking?**
- **Explicit failures** - Better to fail clearly than render incorrectly
- **Developer-friendly** - Logs explain exactly what's wrong
- **Fail-safe** - Application continues, object just not displayed
- **Forces correctness** - Cannot accidentally use incompatible combinations

### The Generation Process: Hard-Coded Portions + Variables

**Generation Method: Hybrid String Assembly**

Saphir uses a technique similar to old-school PHP/HTML mixing: hard-coded GLSL portions combined and filled with variables.

```cpp
// Conceptual example of how SceneGenerator works

string SceneGenerator::generateVertexShader(Material material, Geometry geometry) {
    string shader = R"(
#version 450

// Always include positions
layout(location=0) in vec3 aPosition;
)";

    // Conditionally add attributes based on material needs + geometry availability
    int location = 1;

    if (material.needsNormals() && geometry.hasNormals()) {
        shader += "layout(location=" + std::to_string(location++) + ") in vec3 aNormal;\n";
    }

    if (material.needsTangentSpace() && geometry.hasTangentSpace()) {
        shader += "layout(location=" + std::to_string(location++) + ") in vec3 aTangent;\n";
        shader += "layout(location=" + std::to_string(location++) + ") in vec3 aBitangent;\n";
    }

    if (material.needsUVs() && geometry.hasUVs()) {
        shader += "layout(location=" + std::to_string(location++) + ") in vec2 aTexCoord;\n";
    }

    // Uniforms (always included)
    shader += R"(
layout(set=0, binding=0) uniform SceneUniforms {
    mat4 viewProjection;
    vec3 cameraPosition;
};

layout(set=1, binding=0) uniform ObjectUniforms {
    mat4 model;
    mat4 normalMatrix;
};
)";

    // Outputs to fragment shader
    shader += "layout(location=0) out vec3 vWorldPosition;\n";

    if (material.needsNormals() && geometry.hasNormals()) {
        shader += "layout(location=1) out vec3 vNormal;\n";
    }

    if (material.needsTangentSpace() && geometry.hasTangentSpace()) {
        shader += "layout(location=2) out mat3 vTBN;\n";  // Tangent-Bitangent-Normal matrix
    }

    if (material.needsUVs() && geometry.hasUVs()) {
        shader += "layout(location=3) out vec2 vTexCoord;\n";
    }

    // Main function
    shader += R"(
void main() {
    vec4 worldPos = model * vec4(aPosition, 1.0);
    vWorldPosition = worldPos.xyz;
    gl_Position = viewProjection * worldPos;
)";

    if (material.needsNormals() && geometry.hasNormals()) {
        shader += "    vNormal = normalize((normalMatrix * vec4(aNormal, 0.0)).xyz);\n";
    }

    if (material.needsTangentSpace() && geometry.hasTangentSpace()) {
        shader += R"(
    vec3 T = normalize((normalMatrix * vec4(aTangent, 0.0)).xyz);
    vec3 B = normalize((normalMatrix * vec4(aBitangent, 0.0)).xyz);
    vec3 N = normalize((normalMatrix * vec4(aNormal, 0.0)).xyz);
    vTBN = mat3(T, B, N);
)";
    }

    if (material.needsUVs() && geometry.hasUVs()) {
        shader += "    vTexCoord = aTexCoord;\n";
    }

    shader += "}\n";

    return shader;
}
```

**Fragment Shader Generation** (similar approach):
```cpp
string SceneGenerator::generateFragmentShader(Material material, SceneContext scene) {
    string shader = R"(
#version 450

layout(location=0) in vec3 vWorldPosition;
)";

    // Add inputs based on what vertex shader outputs
    int location = 1;
    if (material.needsNormals()) {
        shader += "layout(location=" + std::to_string(location++) + ") in vec3 vNormal;\n";
    }

    if (material.needsTangentSpace()) {
        shader += "layout(location=" + std::to_string(location++) + ") in mat3 vTBN;\n";
    }

    if (material.needsUVs()) {
        shader += "layout(location=" + std::to_string(location++) + ") in vec2 vTexCoord;\n";
    }

    // Scene uniforms
    shader += R"(
layout(set=0, binding=0) uniform SceneUniforms {
    vec3 cameraPosition;
)";

    // Add lighting uniforms based on scene context
    shader += "    int numDirectionalLights;\n";
    shader += "    int numPointLights;\n";
    if (scene.shadowsEnabled) {
        shader += "    mat4 shadowMatrix;\n";
    }
    shader += "};\n";

    // Material textures
    int binding = 0;
    if (material.hasDiffuseTexture()) {
        shader += "layout(set=2, binding=" + std::to_string(binding++) + ") uniform sampler2D diffuseMap;\n";
    }
    if (material.hasNormalMap()) {
        shader += "layout(set=2, binding=" + std::to_string(binding++) + ") uniform sampler2D normalMap;\n";
    }

    // Output
    shader += "layout(location=0) out vec4 outColor;\n\n";

    // Main function
    shader += "void main() {\n";

    // Sample textures if available
    if (material.hasDiffuseTexture() && material.needsUVs()) {
        shader += "    vec4 diffuse = texture(diffuseMap, vTexCoord);\n";
    } else {
        shader += "    vec4 diffuse = vec4(0.8, 0.8, 0.8, 1.0);\n";  // Default gray
    }

    // Normal mapping
    if (material.hasNormalMap() && material.needsTangentSpace()) {
        shader += R"(
    vec3 normalMap = texture(normalMap, vTexCoord).xyz * 2.0 - 1.0;
    vec3 normal = normalize(vTBN * normalMap);
)";
    } else if (material.needsNormals()) {
        shader += "    vec3 normal = normalize(vNormal);\n";
    }

    // Lighting calculations (based on scene.numLights)
    shader += "    vec3 lighting = vec3(0.0);\n";
    shader += "    vec3 viewDir = normalize(cameraPosition - vWorldPosition);\n";

    for (int i = 0; i < scene.numDirectionalLights; i++) {
        shader += "    // Directional light " + std::to_string(i) + "\n";
        shader += "    lighting += calculateDirectionalLight(...);\n";
    }

    shader += "    outColor = vec4(diffuse.rgb * lighting, diffuse.a);\n";
    shader += "}\n";

    return shader;
}
```

**Key Points:**
- Hard-coded GLSL portions are combined conditionally
- Variables (locations, bindings, names) filled dynamically
- Only includes necessary features (optimal shaders)
- Similar to PHP/HTML mixing, but for shader code

### The Three Active Generators

**1. SceneGenerator** (3D Scene Objects)

**Purpose:** Generate shaders for 3D objects in the scene with full lighting, materials, and effects.

**Unknowns:**
- Material: Textures, blending, transparency, material model (PBR, Phong)
- Geometry: Vertex format (normals, tangents, UVs, colors)
- Scene Context: Number/types of lights, shadows enabled, ambient lighting

**Features:**
- Full lighting calculations (directional, point, spot lights)
- Normal mapping support (if tangent space available)
- PBR or Phong shading models
- Shadow receiving (if shadows enabled in scene)
- Texture sampling (diffuse, normal, roughness, metallic, emissive)
- Vertex colors support
- Transparency and blending modes

**Example Generated Shader:**
```
Material: PBR with diffuse + normal + roughness
Geometry: positions + normals + tangents + UVs
Scene: 2 directional lights, shadows enabled

→ Generates vertex shader with TBN matrix calculation
→ Generates fragment shader with:
  - Normal mapping (using TBN)
  - PBR lighting for 2 lights
  - Shadow sampling and application
  - Texture sampling (diffuse, normal, roughness)
```

**2. OverlayGenerator** (2D UI/HUD)

**Purpose:** Generate shaders for on-screen 2D elements (UI, HUD, text, sprites).

**Unknowns:**
- Material: Textures, blending mode (alpha, additive, multiply)
- Geometry: Positions, UVs, vertex colors
- Scene Context: Screen dimensions, multi-layer blending requirements

**Features:**
- ❌ NO lighting - 2D screen space
- ❌ NO normal mapping - not relevant for UI
- ❌ NO shadows - 2D elements
- ✅ Screen-space positioning (2D transforms)
- ✅ Multi-layer alpha blending
- ✅ Texture sampling (UI elements, fonts, icons)
- ✅ Vertex colors for tinting
- ✅ Clipping regions

**Example Generated Shader:**
```
Material: Textured UI element with alpha blending
Geometry: positions + UVs + colors
Scene: 1920×1080 screen

→ Generates vertex shader with 2D screen-space transform
→ Generates fragment shader with:
  - Texture sampling
  - Vertex color multiplication (tinting)
  - Alpha blending output
  - No lighting calculations
```

**3. ShadowManager** (Shadow Map Generation)

**Purpose:** Generate minimal shaders to render depth for shadow mapping.

**Unknowns:**
- Geometry: Positions only (nothing else needed)
- Scene Context: Light view-projection matrix, shadow map resolution

**Features:**
- ❌ NO materials - don't care about appearance
- ❌ NO textures - only depth matters
- ❌ NO lighting - generating shadows, not receiving them
- ❌ NO colors - depth only
- ✅ Minimal vertex shader (position transform only)
- ✅ Empty or minimal fragment shader (depth written automatically)

**Example Generated Shader:**
```
Geometry: positions only
Scene: Directional light, 2048×2048 shadow map

→ Generates minimal vertex shader:
  - Transform position to light space
  - Output gl_Position
→ Generates empty fragment shader:
  - Depth written automatically by GPU
  - No color output needed
```

### Cache System: Current and Planned

**Current Implementation: Post-Generation Cache**

```cpp
// Current caching approach (basic)

string glslSource = generator.generate(material, geometry, scene);  // Always generate

// Hash the generated source code
string cacheKey = simpleHash(glslSource);  // e.g., SHA256 or FNV1a

if (shaderCache.contains(cacheKey)) {
    return shaderCache[cacheKey];  // Return cached SPIR-V
}

// Cache miss: compile and store
auto spirv = GLSLang.compile(glslSource);
shaderCache[cacheKey] = spirv;
return spirv;
```

**Benefits:**
- ✅ Avoids GLSLang compilation (slow - GLSL → SPIR-V)
- ✅ Avoids GPU pipeline creation (slow - SPIR-V → VkPipeline)
- ✅ Simple implementation (hash of string)

**Limitations:**
- ❌ Still generates GLSL source every time (wasted work)
- ❌ Only caches exact matches (minor differences = cache miss)
- ❌ No cache size management (memory leak potential)
- ❌ Hash collisions possible (though rare with good hash function)

**Planned Improvement: Pre-Generation Cache**

```cpp
// Planned caching approach (optimized)

// Hash inputs BEFORE generation
string cacheKey = hash(material, geometry, scene);

if (shaderCache.contains(cacheKey)) {
    return shaderCache[cacheKey];  // Return cached SPIR-V - skip generation!
}

// Cache miss: generate, compile, store
string glslSource = generator.generate(material, geometry, scene);
auto spirv = GLSLang.compile(glslSource);
shaderCache[cacheKey] = spirv;
return spirv;
```

**Additional Benefits:**
- ✅ Skips GLSL generation (fast path for cache hits)
- ✅ Can identify similar shaders before generating
- ✅ More efficient memory usage (hash is smaller than GLSL source)

**Future Enhancements:**
- LRU cache eviction policy (manage memory)
- Persistent cache to disk (survive application restarts)
- Pre-compilation of common shader combinations
- Shader similarity detection (merge nearly-identical shaders)

### Integration with Resource Loading

**Saphir is a Tool, Not a System Component**

Saphir is a **service/tool** used by resources during finalization, similar to how:
- JPEG decoder is used by TextureResource
- OBJ parser is used by MeshResource
- **Saphir** is used by visual resources (MeshResource with MaterialResource)

**When Saphir is Called:**

```cpp
// Inside VisualComponent or MeshResource
bool onDependenciesLoaded() override {
    // At this point: geometry loaded + material loaded + textures loaded

    // 1. Call Saphir to generate shader
    auto shaderSource = saphir.generate(m_material, m_geometry, sceneContext);
    if (!shaderSource) {
        // Generation failed (incompatibility, error)
        Log::error("Shader generation failed for '{}' with material '{}'",
                   name(), m_material->name());
        return false;  // Resource fails to load
    }

    // 2. Compile with GLSLang
    auto spirv = glslang.compile(shaderSource.value());
    if (!spirv) {
        // Compilation failed (invalid GLSL)
        Log::error("Shader compilation failed:\n{}", shaderSource.value());
        return false;  // Resource fails to load
    }

    // 3. Create Vulkan pipeline
    m_pipeline = vulkan.createPipeline(spirv.value(), ...);
    if (!m_pipeline) {
        // Pipeline creation failed
        return false;  // Resource fails to load
    }

    // 4. Success - resource is now ready for rendering
    return true;
}
```

**Failure Handling:**

If any step fails (generation, compilation, pipeline creation):
- `onDependenciesLoaded()` returns `false`
- Resource never completes loading
- Container returns Default resource instead
- Object never displayed (but application continues)
- Detailed logs explain what went wrong

**This fits perfectly with the Resource Management fail-safe architecture!**

### Common Pitfalls and Best Practices

**❌ Common Mistakes:**

1. **Using PBR material on simple geometry without tangents**
   ```
   Material: "PBR_Metal" (needs tangent space)
   Geometry: "SimpleBox" (only has normals)
   → FAILURE - Add tangents or use simpler material
   ```

2. **Forgetting to export tangents from Blender/Maya**
   ```
   Exported mesh has normals but no tangents
   → Normal mapping won't work
   → Either re-export with tangents or don't use normal maps
   ```

3. **Assuming all geometries have vertex colors**
   ```
   Material: "VertexPainted" (needs vertex colors)
   Most geometries don't have colors
   → FAILURE - Only use with painted meshes
   ```

4. **Using 3D texture coordinates when only 2D available**
   ```
   Material wants 3D UVs (e.g., procedural texturing)
   Geometry only has 2D UVs
   → FAILURE - Incompatible texture coordinate dimensions
   ```

**✅ Best Practices:**

1. **Design geometries and materials together**
   - Know what attributes your materials need
   - Ensure geometries provide those attributes
   - Document requirements clearly

2. **Use default materials for testing**
   - Simple diffuse material works with minimal geometry
   - Test geometry loading before applying complex materials

3. **Check logs when objects don't display**
   - Saphir provides detailed error messages
   - Logs explain exactly what's missing
   - Fix geometry or simplify material based on logs

4. **Leverage Saphir's flexibility**
   - Don't write custom shaders unless absolutely necessary
   - Saphir handles most common cases automatically
   - Extend generators for special cases

5. **Cache-friendly material design**
   - Similar materials → similar shaders → better cache hit rate
   - Avoid unnecessary variations
   - Reuse materials across objects

### Future Improvements and Extensibility

**Planned Features:**

1. **External Shader Support**
   - Ability to provide custom GLSL shaders
   - Bypass Saphir for specialized effects
   - Integrate custom shaders with engine pipeline

2. **PostProcessGenerator Rebuild**
   - Outdated under Vulkan
   - Needs full rewrite for fullscreen post-effects
   - Modern post-processing pipeline integration

3. **Improved Cache System**
   - Pre-generation caching (hash inputs)
   - Persistent disk cache
   - Cache size management (LRU eviction)
   - Shader similarity detection

4. **Custom Generators**
   - API for creating specialized generators
   - Examples: ParticleGenerator, TerrainGenerator, WaterGenerator
   - Plugin system for third-party generators

**Extensibility:**

Creating a new generator requires:
1. Inherit from base Generator class
2. Implement `check(material, geometry)` - compatibility verification
3. Implement `generate()` - GLSL source generation
4. Register with Saphir system

### Design Principles Summary

| Principle | Description | Benefit |
|-----------|-------------|---------|
| **Parametric Generation** | Shaders generated from parameters (unknowns) | Eliminates manual variant explosion |
| **Strict Compatibility** | Material requirements must match geometry attributes | Prevents rendering errors, clear failures |
| **Fail-Safe Integration** | Failures logged, resource not displayed, app continues | Robustness and developer-friendly debugging |
| **Conditional Assembly** | Only include needed features in generated code | Optimal shader performance |
| **Cache Optimization** | Avoid redundant generation and compilation | Performance and efficiency |
| **Separation of Concerns** | Generators for different use cases (3D, 2D, shadows) | Clarity and maintainability |
| **Hybrid Generation** | Hard-coded portions + dynamic variables | Balance between control and flexibility |

**Core Philosophy:**
> "Generate the perfect shader for each combination, check compatibility strictly, fail gracefully if incompatible, cache aggressively to avoid redundant work."

---

## Scene Graph Architecture

This section provides detailed architecture for the Scene Graph system, the organizational backbone for all spatial entities in the engine. The scene graph manages transformations, hierarchy, components, and synchronization between simulation and rendering.

### Quick Reference: Key Terminology

- **Node**: Dynamic entity in the scene graph tree with physics, hierarchy, and double-buffered state. Supports MovableTrait for full physics simulation.
- **StaticEntity**: Immovable entity stored in a flat map (not a tree). Optimized for static geometry like buildings. No physics overhead, but supports components.
- **AbstractEntity**: Common base class for Node and StaticEntity. Manages component attachment and lifecycle.
- **CartesianFrame**: Transformation representation storing position + 3 orthonormal basis vectors (X, Y, Z axes). One axis computed via cross product of the other two.
- **Component**: Modular behavior/data attached to entities. Examples: Visual, Light, Camera, SoundEmitter. Each component has a unique name within its entity.
- **LocatableInterface**: Interface providing position/rotation/scale access. Implemented by both StaticEntity (directly) and MovableTrait (which Node uses).
- **MovableTrait**: Physics-enabled trait adding velocity, forces, mass, and integration. Inherits from LocatableInterface. Used by Nodes for dynamic behavior.
- **Double Buffering**: Technique where each Node/StaticEntity maintains two CartesianFrame states (active + render) for thread-safe rendering.
- **AVConsole (AudioVideoConsole)**: Per-scene manager that detects Camera and Microphone components, linking them to render targets and audio outputs.
- **LightSet**: Per-scene structure tracking all Light components, synchronizing lighting state with GPU via Uniform Buffer Objects (UBO).

### Design Philosophy: Generic Containers + Modular Components

**Core Principle: Entities Are Generic Until Components Define Them**

The scene graph distinguishes between two fundamental container types:
- **Node**: Dynamic, hierarchical, physics-enabled
- **StaticEntity**: Immovable, flat, performance-optimized

Both are **generic containers** that gain meaning through attached **components**:
- Without components → just a position in space
- With Visual component → something to render
- With Light component → illuminates the scene
- With Camera component → defines a viewport
- With SoundEmitter component → plays audio

**Why This Design?**

Traditional approaches create specialized classes:
```
BAD: class Player : public Node { ... }
BAD: class Lamp : public Node { ... }
BAD: class Tree : public StaticEntity { ... }
```

**Problems:**
- Rigid inheritance hierarchies
- Code duplication across similar entities
- Hard to compose behaviors (what if Player needs light?)
- Difficult to add features dynamically

**Emeraude's Solution: Composition Over Inheritance**
```
Node playerNode = scene->createChild("player", position);
playerNode->newVisual(meshResource, ...);       // Can be rendered
playerNode->newCamera(...);                     // Has viewpoint
playerNode->newSoundEmitter(...);              // Emits audio

StaticEntity lamp = scene->createStaticEntity("lamp");
lamp->newVisual(lampMesh, ...);                // Can be rendered
lamp->newPointLight(...);                      // Emits light
// Static → never moves, but still functional
```

**Benefits:**
- ✅ **Flexible**: Any entity can have any combination of components
- ✅ **Composable**: Add/remove behaviors at runtime
- ✅ **No code duplication**: Component logic exists once
- ✅ **Performance**: Pay only for what you use (StaticEntity skips physics)

### Architecture: The Two Entity Types

**Hierarchy Overview:**
```
LocatableInterface (position/rotation/scale interface)
├── MovableTrait (physics: velocity, forces, mass)
│   └── Node (dynamic entity in tree hierarchy)
│
└── StaticEntity (immovable entity in flat map)

AbstractEntity (component management)
├── Node (inherits AbstractEntity + MovableTrait)
└── StaticEntity (inherits AbstractEntity + LocatableInterface)
```

**Entity Type 1: Node (Dynamic Hierarchy)**

**Purpose:** Dynamic game objects that move, interact with physics, and form parent-child relationships.

**Characteristics:**
- ✅ **Hierarchical**: Forms tree structure with parent-child relationships
- ✅ **Physics-enabled**: MovableTrait provides velocity, forces, mass
- ✅ **Double-buffered**: Active state (logic) + Render state (rendering)
- ✅ **Movable**: Position changes via physics or direct manipulation
- ✅ **Recursive updates**: Parent update propagates to children

**Storage:** Tree structure starting from root node (world origin)

**Creation:**
```cpp
// Create as child of existing node (usual case)
auto childNode = parentNode->createChild("objectName", initialPosition);

// Access root (immutable, represents world origin)
auto root = scene->root();
```

**Use Cases:**
- Player characters
- Projectiles
- Vehicles
- Movable objects (crates, barrels)
- Animated characters
- Cameras that follow players

**Entity Type 2: StaticEntity (Performance-Optimized)**

**Purpose:** Immovable world objects that don't need physics simulation overhead.

**Characteristics:**
- ✅ **Flat storage**: No hierarchy, stored in map keyed by name
- ❌ **No physics**: No MovableTrait (no velocity, forces, integration)
- ✅ **Double-buffered**: Same as Nodes for thread-safety consistency
- ✅ **Technically mutable**: Can change position via LocatableInterface, but intended to stay static
- ✅ **Fast access**: Direct lookup by name (no tree traversal)

**Storage:** `std::unordered_map<std::string, StaticEntity>` in Scene

**Creation:**
```cpp
auto building = scene->createStaticEntity("building_23");
building->setPosition(worldPos);  // Set once during scene setup
```

**Use Cases:**
- Buildings and architecture
- Terrain decorations (rocks, trees)
- Static lights (street lamps, ceiling lights)
- Skybox
- Static cameras (security cameras)
- Background geometry

**Comparison Table:**

| Feature | Node | StaticEntity |
|---------|------|--------------|
| **Storage** | Tree hierarchy | Flat map |
| **Physics** | Full (MovableTrait) | None |
| **Hierarchy** | Parent-child | Independent |
| **Performance** | Moderate overhead | Minimal overhead |
| **Use Case** | Dynamic objects | Static world |
| **Access** | Tree traversal | Direct by name |
| **Position** | Changes frequently | Set once |
| **Double Buffering** | Yes | Yes (for consistency) |

### Component System: Modular Behaviors

**How Components Work:**

Components attach to entities (Node or StaticEntity) via AbstractEntity interface:

```cpp
// AbstractEntity manages components uniformly
class AbstractEntity {
    std::map<std::string, std::unique_ptr<Component>> m_components;

    // Generic component creation
    template<typename ComponentType>
    ComponentType* newComponent(const std::string& name, ...args);

    // Query
    Component* getComponent(const std::string& name);
    bool containsComponent(const std::string& name);
};

// Concrete usage (via specialized newXXX methods)
node->newVisual(meshResource, castShadows, receiveShadows, "main_visual");
node->newPointLight(color, intensity, range, "torch_light");
node->newCamera(fov, aspectRatio, nearPlane, farPlane, "player_cam");
```

**Available Component Types:**

**Rendering Components:**
1. **Visual** - Renders a mesh with material
   - Single mesh + material combination
   - Shadow casting/receiving flags
   - Attached to rendering pipeline

2. **MultipleVisuals** - Renders multiple meshes from one entity
   - For complex objects (character = body + clothing + accessories)
   - Each visual has independent mesh/material

**Lighting Components:**
3. **DirectionalLight** - Sun-like infinite light
   - Direction, color, intensity
   - Affects entire scene uniformly

4. **PointLight** - Omnidirectional light with falloff
   - Position, color, intensity, range
   - Spherical attenuation

5. **SpotLight** - Cone-shaped light (flashlight)
   - Position, direction, color, intensity, cone angle, falloff
   - Directional attenuation

**Audio Components:**
6. **SoundEmitter** - 3D positional audio source
   - Audio buffer, volume, pitch, loop
   - Position automatically synced from entity

7. **Microphone** - Audio listener (usually attached to camera)
   - Receives audio from scene
   - Position/orientation affect audio perception

**Physics Modifiers:**
8. **DirectionalPushModifier** - Applies constant directional force
   - Wind, current, conveyor belt effect
   - Affects entities in volume

9. **SphericalPushModifier** - Radial force (explosion, gravity well)
   - Center, strength, radius
   - Push away or pull toward center

**Utility Components:**
10. **Weight** - Mass/weight representation
    - Used for gameplay logic (weighted buttons, scales)
    - Separate from physics mass

11. **ParticlesEmitter** - Software particle system
    - Each particle has its own LocatableInterface
    - CPU-based particle simulation

12. **Camera** - Viewport and projection
    - Defines rendering perspective
    - Connected to render targets via AVConsole

**Component Lifecycle Methods:**

```cpp
class Component {
    // Called every logic frame (60 Hz)
    virtual void processLogics();

    // Called when parent entity moves (event callback)
    virtual void move(const CartesianFrame& newParentLocation);
};
```

**Usage Notes:**
- **Multiple components allowed**: Same entity can have multiple Visuals, Lights, etc. (each with unique name)
- **processLogics()**: Override for per-frame logic (most components don't need it)
- **move()**: Override if component needs to react to entity movement (most don't need it)
- **Optional implementation**: Both methods have empty default implementations

### Transformations: CartesianFrame System

**CartesianFrame Structure:**

```cpp
struct CartesianFrame {
    Vector3 position;          // Origin point in space
    Vector3 rightVector;       // +X axis (normalized)
    Vector3 downwardVector;    // +Y axis (normalized)
    Vector3 forwardVector;     // One of these is computed
    Vector3 backwardVector;    // via cross product of other two

    // Quaternion representation also stored for rotations
};
```

**Key Points:**
- **Orthonormal basis**: All axis vectors are unit length and perpendicular
- **Computed axis**: One axis derived from cross product (redundancy elimination)
- **Y-down convention**: Consistent with Vulkan throughout engine
- **Efficient representation**: Avoids matrix storage, computed when needed

**Transformation Hierarchy:**

```
Root (world origin - immutable)
  └─ Node "vehicle" (local pos: 10, 0, 5)
      ├─ Node "wheel_FL" (local pos: 1, 1, 1)
      ├─ Node "wheel_FR" (local pos: -1, 1, 1)
      └─ Node "camera" (local pos: 0, -2, -3)
```

**World Position Calculation:**
- Each Node stores **local** CartesianFrame (relative to parent)
- **World** CartesianFrame computed on-demand by traversing parent chain
- Recursive: `worldFrame = parent.worldFrame * localFrame`
- Root has identity transform (world = local)

**StaticEntity Transformations:**
- No parent → **local = world** always
- Direct position setting
- Still uses CartesianFrame for consistency

### Double Buffering: Thread-Safe Simulation and Rendering

**The Problem: Two Threads, One Scene**

```
Thread 1 (Logic - 60 Hz):     Thread 2 (Rendering - Variable FPS):
- Update physics               - Read positions
- Move entities                - Build draw calls
- Modify transforms            - Submit to GPU
```

**Without synchronization:**
- Renderer reads positions **while** physics is writing them
- Results: Torn reads, visual artifacts, flickering, objects "jumping"

**Emeraude's Solution: Double Buffering**

Each Node and StaticEntity maintains **TWO CartesianFrame states**:

```cpp
class Node {
    CartesianFrame m_activeFrame;   // Written by logic thread (60 Hz)
    CartesianFrame m_renderFrame;   // Read by render thread (variable)
};
```

**Flow:**
```
Logic Thread (60 Hz):
1. Update physics
2. Write to m_activeFrame
3. Process components (processLogics)
4. Modify entity positions → m_activeFrame
5. End of frame → SWAP
   m_renderFrame = m_activeFrame;

Render Thread (variable FPS):
1. Read m_renderFrame only
2. Compute world transforms from m_renderFrame
3. Build and submit draw calls
4. Never touches m_activeFrame
```

**Swap Timing:**
- Happens **once per logic frame** (60 Hz)
- Controlled by **logic thread**
- Atomic operation (instant pointer/value swap)
- Render thread always sees **consistent snapshot** of entire scene

**Benefits:**
- ✅ **Zero race conditions**: Render never reads data being written
- ✅ **Consistent state**: Entire scene swapped atomically
- ✅ **No visual artifacts**: Renderer sees temporally coherent snapshot
- ✅ **Independent framerates**: Logic 60 Hz, render can be 30, 144, 1000+ FPS
- ✅ **Smooth rendering**: High FPS renders same state multiple times (no jitter)

**Why StaticEntity Also Double-Buffers:**
- **Uniformity**: Same interface as Node (no special cases in render code)
- **Thread-safety**: Can modify StaticEntity position on logic thread safely
- **Consistency**: Scene-wide swap ensures everything coherent
- **Future-proof**: If StaticEntity ever needs to move, system already supports it

### Update Flow: Logic Thread (60 Hz)

**Complete Frame Lifecycle:**

```
┌─────────────────────────────────────────┐
│ Logic Thread - 60 Hz Fixed Timestep    │
└─────────────────────────────────────────┘

1. Physics Integration (before collisions)
   - Apply forces to Nodes
   - Integrate velocities: position += velocity * dt
   - Apply gravity, drag, user forces

2. Collision Detection & Resolution
   - Broad phase (octree queries)
   - Narrow phase (precise intersection tests)
   - Constraint solver (impulse-based resolution)
   - Ground collision (hard clipping + manifolds)
   - Boundary collision (hard clipping + velocity inversion)

3. Scene Graph Update (root-to-leaves traversal)
   For each Node (depth-first):
     a. Update CartesianFrame (combine parent + local)
     b. Call processLogics() on all components
     c. Recurse to children

4. StaticEntity Update (map iteration)
   For each StaticEntity:
     - Call processLogics() on all components
     - (No physics, no hierarchy traversal)

5. Event Propagation
   - MovableTrait detects position changes
   - Calls AbstractEntity::onContainerMove()
   - Entity calls move() on all components
   - Components react to movement (if needed)

6. Finalization & Swap
   - Logic frame complete
   - ATOMIC SWAP: m_renderFrame = m_activeFrame
   - All Nodes and StaticEntities swapped
   - Scene now ready for next render frame
```

**Key Update Methods:**

```cpp
// Called every logic frame on components
void Component::processLogics() {
    // Per-frame logic
    // Most components: empty (no per-frame work)
    // Example: ParticlesEmitter updates particles here
}

// Called when entity moves (event-driven)
void Component::move(const CartesianFrame& newParentLocation) {
    // React to movement
    // Most components: empty (position irrelevant)
    // Example: SoundEmitter might update OpenAL source position
}
```

**Order Guarantees:**
- **Physics before scene update**: Positions finalized before rendering
- **Parent before children**: Hierarchy updated top-down
- **Components after transform**: processLogics() sees updated position
- **Swap at end**: Render thread never sees partial update

### Rendering Integration: Scene Filtering and Delegation

**Rendering Flow (Render Thread - Variable FPS):**

```
┌─────────────────────────────────────────┐
│ Render Thread - Variable FPS           │
└─────────────────────────────────────────┘

1. Core requests render
   Core → GraphicsRenderer::render(activeScene)

2. Scene filters renderable objects
   Scene iterates:
     - All Nodes (tree traversal)
     - All StaticEntities (map iteration)

   For each entity:
     - Query components: component->isRenderable()
     - Visual, MultipleVisuals → true
     - Camera, Light, SoundEmitter → false

3. Frustum Culling (currently broken, being fixed)
   - Test entity bounding volumes against camera frustum
   - Cull objects outside view
   - PLANNED: Use octree for efficient spatial queries

4. Distance Sorting
   For each renderable:
     - Compute distance to camera
     - Sort by distance:
       * Opaque objects: near → far (front-to-back)
         Benefits: Early Z-test rejection, optimal for depth buffer
       * Transparent objects: far → near (back-to-front)
         Required: Correct alpha blending order

5. Delegate to Graphics Namespace
   Scene passes filtered/sorted list to Graphics system
   Scene says: "Draw resource ID X at world position Y"
   Graphics system:
     - Resolves resource ID
     - Checks GPU availability
     - Submits Vulkan draw commands
```

**Scene Responsibilities:**
- ✅ Spatial organization (what exists, where)
- ✅ Filtering (what should be rendered)
- ✅ Culling (what's visible)
- ✅ Sorting (in what order)
- ❌ **NOT** rendering (doesn't know Vulkan)
- ❌ **NOT** resource management (delegates to Graphics)

**Graphics System Responsibilities:**
- ✅ Vulkan command submission
- ✅ GPU resource management
- ✅ Shader program application (via Saphir)
- ✅ Verifying resources loaded
- ❌ **NOT** scene organization (receives filtered list)

**Separation of Concerns:**
```
Scene: "Here's what to draw and where"
       ↓
Graphics: "I'll draw it using Vulkan"
```

### AVConsole and LightSet: Automatic Management

**AVConsole (AudioVideoConsole) - Per Scene Manager**

**Purpose:** Automatically detect and link Camera and Microphone components to render targets and audio outputs.

**Architecture:**
```cpp
class Scene {
    std::unique_ptr<AVConsole::Manager> m_avConsole;
};

class AVConsole::Manager {
    std::vector<Camera*> m_cameras;
    std::vector<Microphone*> m_microphones;
    std::vector<RenderTarget*> m_renderTargets;  // Main screen, offscreen, cubemaps

    // Automatically called when Camera/Microphone created
    void registerCamera(Camera* cam);
    void registerMicrophone(Microphone* mic);
};
```

**How It Works:**
1. Node/StaticEntity creates Camera or Microphone component
2. Component constructor notifies Scene's AVConsole
3. AVConsole registers component automatically
4. AVConsole links component to appropriate render target or audio output

**Use Cases:**
- **Main camera** → Linked to main screen render target
- **Security camera** → Linked to offscreen texture (in-game monitor)
- **Cubemap camera** → Linked to cubemap faces (environment mapping)
- **Player microphone** → Linked to audio listener (3D audio perception)

**Benefits:**
- ✅ Automatic discovery (no manual registration)
- ✅ Unified management (audio + video)
- ✅ Multi-viewport support (split-screen, picture-in-picture)
- ✅ Dynamic targets (add/remove cameras at runtime)

**LightSet - Per Scene Lighting State**

**Purpose:** Track all Light components, synchronize with GPU via Uniform Buffer Objects (UBO).

**Architecture:**
```cpp
class Scene {
    std::unique_ptr<LightSet> m_lightSet;
};

class LightSet {
    std::vector<DirectionalLight*> m_directionalLights;
    std::vector<PointLight*> m_pointLights;
    std::vector<SpotLight*> m_spotLights;

    VkBuffer m_lightingUBO;  // GPU buffer

    // Automatically called when Light component created
    void registerLight(Light* light);

    // Called before rendering
    void updateGPUBuffer();
};
```

**How It Works:**
1. Node/StaticEntity creates Light component (Directional, Point, Spot)
2. Component constructor notifies Scene's LightSet
3. LightSet registers light automatically
4. Before rendering: LightSet uploads all light data to GPU UBO
5. Shaders access UBO for lighting calculations

**GPU Synchronization:**
```glsl
// In fragment shader (generated by Saphir)
layout(set=0, binding=1) uniform LightingData {
    int numDirectionalLights;
    DirectionalLight directionalLights[MAX_LIGHTS];

    int numPointLights;
    PointLight pointLights[MAX_LIGHTS];

    int numSpotLights;
    SpotLight spotLights[MAX_LIGHTS];
};
```

**Benefits:**
- ✅ Automatic discovery (no manual registration)
- ✅ Efficient GPU upload (single UBO for all lights)
- ✅ Dynamic lighting (add/remove lights at runtime)
- ✅ Shader integration (Saphir knows light count for generation)

### Scene Graph Invariants and Rules

**Fundamental Rules:**

1. **Tree Structure (Nodes Only)**
   - ✅ Each Node has exactly **one parent** (or is root)
   - ✅ Node can have **multiple children**
   - ✅ Root node is immutable (world origin, never moves, never destroyed)
   - ❌ **Cycles impossible** by design (parent set via createChild, not settable)
   - ❌ **No orphans**: Destroying parent destroys all descendants (RAII cascade)

2. **Parentage By Design**
   - ❌ Cannot call `node->setParent()` (method doesn't exist)
   - ✅ Parent set implicitly via `parent->createChild()`
   - ✅ Smart pointer ownership: parent holds `shared_ptr<Node>` to children
   - ✅ Child holds `weak_ptr<Node>` to parent (prevents circular references)
   - ✅ RAII: Parent destruction → children automatically destroyed

3. **StaticEntity Independence**
   - ✅ No parent, no children
   - ✅ Stored in flat map (direct lookup by name)
   - ✅ Independent lifecycle (destroyed individually)
   - ✅ Name uniqueness enforced (map key)

4. **Component Uniqueness**
   - ✅ Multiple components of same type allowed on one entity
   - ✅ Each component must have **unique name** within that entity
   - ❌ Cannot have two components with same name (even different types)
   - ✅ Access by name: `entity->getComponent("name")`

5. **Threading Rules**
   - ✅ Logic thread (60 Hz) modifies **m_activeFrame** only
   - ✅ Render thread (variable) reads **m_renderFrame** only
   - ✅ Swap happens atomically at end of logic frame
   - ❌ **Never** modify scene structure during rendering
   - ❌ **Never** modify scene structure during physics simulation
   - ✅ Safe window: After swap, before next logic frame

6. **Transform Consistency**
   - ✅ Local transforms always relative to parent
   - ✅ World transforms computed on-demand (not stored)
   - ✅ Root has identity local transform (world origin)
   - ✅ StaticEntity: local = world (no parent)

7. **Update Order (Guaranteed)**
   - ✅ Physics integration → Collision resolution → Scene update
   - ✅ Parent updated before children (depth-first traversal)
   - ✅ Transform updated before processLogics()
   - ✅ Swap at end (all updates complete)

### Common Usage Patterns

**Pattern 1: Creating a Dynamic Object (Node)**

```cpp
// Player character
auto player = scene->root()->createChild("player", initialPos);

// Visual representation
auto mesh = resources.container<MeshResource>()->getResource("character");
player->newVisual(mesh, castShadows, receiveShadows, "body");

// First-person camera
player->newCamera(90.0f, 16.0f/9.0f, 0.1f, 1000.0f, "player_cam");

// Footstep sounds
player->newSoundEmitter(footstepBuffer, volume, pitch, loop, "footsteps");

// Physics (MovableTrait already present, configure properties)
player->bodyPhysicalProperties().setMass(80.0f);  // 80 kg
player->enableSphereCollision(true);
```

**Pattern 2: Creating Static Geometry (StaticEntity)**

```cpp
// Building
auto building = scene->createStaticEntity("building_downtown_01");
building->setPosition(worldPos);

// Visual
auto buildingMesh = resources.container<MeshResource>()->getResource("building_01");
building->newVisual(buildingMesh, castShadows, receiveShadows, "main_visual");

// Street lamp attached to building
building->newPointLight(Color::Warm, 100.0f, 20.0f, "street_light");
```

**Pattern 3: Hierarchical Objects (Vehicle with Wheels)**

```cpp
// Vehicle body
auto vehicle = scene->root()->createChild("vehicle", vehiclePos);
vehicle->newVisual(carBodyMesh, true, true, "body");

// Wheels (children of vehicle)
auto wheelFL = vehicle->createChild("wheel_FL", localPos_FL);
wheelFL->newVisual(wheelMesh, true, true, "wheel");

auto wheelFR = vehicle->createChild("wheel_FR", localPos_FR);
wheelFR->newVisual(wheelMesh, true, true, "wheel");

// Moving vehicle → wheels move automatically (hierarchy)
vehicle->applyForce(forwardVector * thrust);
```

**Pattern 4: Multiple Components of Same Type**

```cpp
// Character with multiple visuals
auto character = scene->root()->createChild("npc", pos);

character->newVisual(bodyMesh, true, true, "body");
character->newVisual(helmetMesh, true, true, "helmet");
character->newVisual(weaponMesh, true, true, "weapon");

// Each visual independently rendered, same entity
```

**Pattern 5: Physics-Driven Movement**

```cpp
// Apply force (physics-based)
node->applyForce(Vector3(100.0f, 0.0f, 0.0f));  // Push right

// Apply impulse (instant velocity change)
node->applyImpulse(Vector3(0.0f, -50.0f, 0.0f));  // Jump up

// Direct position (bypass physics - teleport)
node->setPosition(newPosition);  // Forces position (via LocatableInterface)
```

**Pattern 6: Event-Driven Component Logic**

```cpp
class MyCustomComponent : public Component {
    void processLogics() override {
        // Called every logic frame (60 Hz)
        // Use for continuous logic
        m_timer += deltaTime;
        if (m_timer > threshold) {
            triggerAction();
        }
    }

    void move(const CartesianFrame& newLocation) override {
        // Called when parent entity moves
        // Use for reactive logic
        updateSoundPosition(newLocation.position);
    }
};
```

### Design Principles Summary

| Principle | Description | Benefit |
|-----------|-------------|---------|
| **Generic Containers** | Entities gain meaning through components, not inheritance | Flexibility, composability, no code duplication |
| **Two Entity Types** | Node (dynamic) vs StaticEntity (optimized) | Performance for static world, full features for dynamic |
| **Composition Over Inheritance** | Attach components instead of subclassing | Easy to add/remove behaviors, no rigid hierarchies |
| **Double Buffering** | Separate active and render states | Thread-safe, no visual artifacts, smooth rendering |
| **RAII Hierarchy** | Smart pointers manage lifetime, parent destruction cascades | No memory leaks, no dangling pointers, safe cleanup |
| **Parentage By Design** | Children created via parent, not settable afterward | Impossible to create cycles, clear ownership |
| **Automatic Registration** | Components auto-register with AVConsole and LightSet | No manual tracking, dynamic discovery, zero boilerplate |
| **Separation of Concerns** | Scene organizes, Graphics renders | Clear responsibilities, easy to maintain |
| **Thread-Safe Update** | Logic modifies active, render reads render state | No race conditions, independent framerates |

**Core Philosophy:**
> "Entities are positions in space. Components give them meaning. Hierarchy gives them relationships. Double buffering keeps them safe across threads."

---

## Coding Conventions

1.  **Naming Conventions:**
    *   **Classes:** PascalCase (e.g., `VulkanDevice`, `SceneNode`, `ResourceManager`)
    *   **Methods/Functions:** camelCase (e.g., `updateTransform()`, `loadTexture()`)
    *   **Member Variables:** camelCase with `m_` prefix (e.g., `m_device`, `m_swapchain`)
    *   **Constants:** UPPER_SNAKE_CASE (e.g., `MAX_FRAMES_IN_FLIGHT`, `DEFAULT_RESOLUTION`)
    *   **Namespaces:** PascalCase (e.g., `EmEn`, `Vulkan`, `Graphics`)
    *   **Private Methods:** Use same convention as public methods; no special prefix required.

2.  **File Organization:**
    *   Header files (`.hpp`) contain class declarations, inline functions, and template implementations.
    *   Implementation files (`.cpp`) contain method definitions.
    *   Platform-specific implementations use suffixes: `.linux.cpp`, `.windows.cpp`, `.mac.cpp`, `.mac.mm` (Objective-C++).
    *   Keep related functionality grouped in subdirectories (e.g., `Vulkan/`, `Graphics/`, `Audio/`).
    *   One class per file pair (`.hpp`/`.cpp`) unless classes are tightly coupled.

3.  **Error Handling:**
    *   Use exceptions for critical errors that cannot be recovered (e.g., Vulkan device creation failure).
    *   Return error codes or `std::optional` for expected failures (e.g., resource not found).
    *   Log errors with appropriate severity levels (ERROR, WARNING, INFO, DEBUG).
    *   Validate all public API inputs and provide clear error messages.
    *   Never silently ignore errors; always log or propagate them.

4.  **Memory Management:**
    *   Prefer smart pointers (`std::unique_ptr`, `std::shared_ptr`) over raw pointers.
    *   Follow RAII (Resource Acquisition Is Initialization) principles strictly.
    *   Avoid manual memory management (`new`/`delete`) when possible.
    *   GPU memory allocations must go through VMA; never use `vkAllocateMemory` directly.
    *   Be mindful of resource lifetimes; ensure proper cleanup order (children before parents).

5.  **Headers and Includes:**
    *   Use `#pragma once` in all header files (preferred over include guards).
    *   Include only what is necessary; use forward declarations when possible to reduce compile times.
    *   Group includes in the following order:
        1. Standard library headers (`<vector>`, `<memory>`, etc.)
        2. Third-party library headers (`<vulkan/vulkan.h>`, `<AL/al.h>`, etc.)
        3. Engine headers from other modules (`"Graphics/Types.hpp"`, etc.)
        4. Headers from the same module
    *   Use angle brackets (`<>`) for external libraries, quotes (`""`) for engine headers.

6.  **Threading and Concurrency:**
    *   Be explicit about thread safety in documentation.
    *   Use mutexes, locks, and atomic operations appropriately.
    *   Avoid global mutable state; prefer thread-local storage or explicit synchronization.
    *   Rendering commands must be recorded from the main thread unless explicitly designed for multi-threading.
    *   Physics and resource loading can run on separate threads.

7.  **Vulkan-Specific Conventions:**
    *   Always check Vulkan function return codes; wrap in helper functions if needed.
    *   Use validation layers during development; provide option to disable in release builds.
    *   Properly transition image layouts before use.
    *   Batch command buffer submissions when possible to reduce overhead.
    *   Use descriptor sets efficiently; avoid creating new descriptors per frame.

---

## Project Structure

The `emeraude-engine` source code (`src/`) is organized as follows:

- `Core.hpp/cpp`: The engine's core components, including the main loop, application lifecycle, and initialization sequence.
  - Manages the main update loop and frame timing.
  - Coordinates service initialization and shutdown.
  - Provides base class for applications to inherit from.

- `ServiceInterface.hpp`: Defines abstract interfaces for core engine services.
  - Provides a consistent pattern for service initialization, update, and cleanup.
  - Allows loose coupling between engine subsystems.

- `User.hpp`: Defines user-specific data and preferences.

- `Vulkan/`: Contains all Vulkan-specific rendering code (low-level abstraction).
  - Device management (physical device selection, logical device creation).
  - Swapchain creation and presentation.
  - Command buffer allocation and management.
  - Synchronization primitives (fences, semaphores).
  - Pipeline creation and management.
  - Descriptor set management.
  - Memory allocation through VMA.
  - Render pass and framebuffer management.

- `Saphir/`: Automatic shader source code generation system.
  - Analyzes material properties and geometry requirements.
  - Generates GLSL shader code dynamically.
  - Compiles shaders at runtime using GLSLang.
  - Manages shader variants and permutations.
  - Handles shader compilation errors and notifications.

- `Graphics/`: High-level graphics abstractions, built upon the Vulkan layer.
  - Material system for defining surface properties.
  - Mesh and geometry management.
  - Camera and viewport management.
  - Lighting system (directional, point, spot lights).
  - Shadow mapping and shadow casters.
  - Render queue and draw call management.
  - Post-processing effects.
  - Texture and image management.

- `Audio/`: OpenAL-based audio implementation.
  - Audio source management (3D positional audio).
  - Audio listener (camera-attached).
  - Audio buffer loading and streaming.
  - Audio source pooling for performance.
  - Distance attenuation and Doppler effects.
  - Audio format support (WAV, OGG, etc.).

- `Window/`: Cross-platform window creation and management.
  - Window creation, resizing, and destruction.
  - Event handling (close, minimize, maximize, focus).
  - Fullscreen and windowed mode switching.
  - Multi-monitor support.
  - Platform-specific window handles for Vulkan surface creation.

- `Input/`: Handles keyboard, mouse, and other input devices.
  - Keyboard input (key press, release, repeat).
  - Mouse input (movement, buttons, scroll).
  - Gamepad/controller support.
  - Input mapping and action bindings.
  - Input state queries (is key pressed, mouse position, etc.).
  - Platform-independent input abstraction.

- `Scenes/`: Manages the scene graph and game objects.
  - Scene node base class with transformation hierarchy.
  - Scene graph traversal and updates.
  - Node lifecycle management (creation, destruction, parenting).
  - Component-based architecture for attaching behaviors to nodes.
  - Visibility culling and spatial queries.

- `Resources/`: Handles loading and management of assets.
  - Resource loading (synchronous and asynchronous).
  - Resource caching and reference counting.
  - Support for multiple asset formats (textures, models, audio, etc.).
  - Resource path resolution and virtual file systems.
  - Streaming for large assets.

- `Physics/`: Physics engine integration.
  - Rigid body dynamics.
  - Collision detection (broad phase and narrow phase).
  - Collision response and contact resolution.
  - Spatial acceleration structures (octree, BVH).
  - Physics queries (raycasting, shape casting, overlap tests).
  - Physics material properties (friction, restitution).
  - Constraint and joint systems.

- `Animations/`: Manages animations and skeletal systems.
  - Skeletal animation (bones, joints, skinning).
  - Animation clips and blending.
  - Animation state machines.
  - Morph target animation.
  - Procedural animation.

- `PlatformSpecific/`: Contains platform-specific implementations.
  - `SystemInfo.hpp/cpp`: System information queries (OS, CPU, RAM, etc.).
  - `UserInfo.hpp/cpp`: User information (username, home directory, etc.).
  - `Helpers.linux.cpp`, `Helpers.windows.cpp`: Platform-specific utility functions.
  - Platform-specific file system operations.
  - Platform-specific threading primitives.

- `Net/`: Networking functionalities.
  - `Manager.hpp/cpp`: Network manager for HTTP/HTTPS requests.
  - `DownloadItem.hpp`: Download queue management.
  - `CachedDownloadItem.hpp`: Cached download with local storage.
  - HTTP client for downloading resources.
  - Network error handling and retry logic.

- `Console/`: In-game console and logging system.
  - `Controller.hpp/cpp`: Console controller and command execution.
  - `Controllable.hpp/cpp`: Interface for objects controllable through console.
  - `Command.hpp`: Console command definition.
  - `Expression.hpp/cpp`: Expression parsing and evaluation.
  - `Argument.hpp/cpp`: Command argument parsing.
  - `Output.hpp`: Console output formatting.
  - Logging with severity levels (DEBUG, INFO, WARNING, ERROR).
  - Command registration and auto-completion.

- `Overlay/`: UI overlay system for in-game UI elements.
  - 2D rendering on top of 3D scene.
  - UI widgets and controls.
  - Text rendering.
  - UI layout and positioning.

- `Libs/`: Internal helper libraries or embedded third-party code.
  - Utility functions and data structures.
  - Math libraries (vectors, matrices, quaternions).
  - String utilities.
  - File I/O helpers.

- `Tool/`: Utility tools for development or debugging.
  - Asset converters and processors.
  - Debug visualization tools.
  - Performance profiling utilities.

- `Testing/`: Unit tests and testing infrastructure.
  - Test fixtures and utilities.
  - Automated tests for engine components.
  - Integration tests.

---

## Testing and Debugging

1.  **Vulkan Validation Layers:** Always enable Vulkan validation layers during development to catch API misuse.
2.  **Console Commands:** Use the in-game console to inspect engine state and execute debug commands.
3.  **Logging System:** Use appropriate log levels (DEBUG, INFO, WARNING, ERROR) for different types of messages.
4.  **Debug Visualization:** Use debug rendering tools to visualize physics colliders, scene graph hierarchy, etc.
5.  **Performance Profiling:** Profile rendering performance regularly to identify bottlenecks.
6.  **Unit Tests:** Run unit tests regularly to catch regressions.
7.  **Memory Debugging:** Use tools like Valgrind (Linux), Dr. Memory (Windows), or Address Sanitizer to detect memory leaks.

---

## Build System Notes

1.  **CMake Configuration:** The engine uses CMake 3.25.1+ with modern CMake practices.
2.  **Parallel Builds:** Compilation automatically uses all available CPU cores.
3.  **Build Types:** Support for Debug, Release, and RelWithDebInfo configurations.
4.  **Third-Party Dependencies:** Managed through CMake's FetchContent or find_package.
5.  **Code Formatting:** Use the `format-code` script (if available) to format all C++ files before committing.
6.  **Vulkan SDK:** Requires Vulkan SDK 1.2+ to be installed on the system.
7.  **Platform-Specific Builds:**
    - Linux: Requires X11 or Wayland development libraries.
    - Windows: Requires MSVC 2019+ or MinGW with C++20 support.
    - macOS: Requires Xcode with MoltenVK for Vulkan support.

---

## Version Control

1.  **Commit Messages:** Write clear, descriptive commit messages in English using imperative mood.
2.  **Branch Strategy:**
    - `main`: Stable releases only.
    - `develop`: Active development branch.
    - Feature branches: For new features or significant changes.
3.  **Code Review:** All changes should be reviewed before merging to `main`.
4.  **Breaking Changes:** Document any API breaking changes clearly in commit messages and changelogs.
5.  **License Compliance:** Ensure all contributions are compatible with LGPLv3 license.

---

## AI Agent Specific Guidelines

1.  **Vulkan Complexity:** Be aware that Vulkan is verbose and complex. Always verify synchronization and resource lifetime management.
2.  **Cross-Platform Testing:** When modifying platform-specific code, consider the impact on all supported platforms.
3.  **Performance Implications:** Engine code runs in performance-critical paths. Be mindful of allocations, copies, and redundant operations.
4.  **Shader Generation:** Saphir is a complex system. Changes to shader generation require careful testing across multiple material types.
5.  **Scene Graph Integrity:** Maintain scene graph invariants (e.g., no cycles, proper parent-child relationships).
6.  **Resource Lifetimes:** Be extremely careful with resource lifetimes, especially Vulkan resources that may be in use by the GPU.
7.  **Thread Safety:** Document thread safety guarantees for all public APIs. Assume nothing is thread-safe unless explicitly stated.
8.  **Backward Compatibility:** As a library, maintain API backward compatibility when possible. Deprecate old APIs before removing them.
9.  **Documentation:** Engine APIs must be well-documented since they will be used by client applications like `projet-alpha`.
10. **Testing:** When adding new features, suggest appropriate tests to validate functionality.

---

## Framework Migration Philosophy

**"We code well, but we code safe!"**

When modifying or extending the `emeraude-engine` framework, follow these principles for safe, gradual migration:

1.  **Smooth Migration:** Never break existing functionality. New systems must coexist with old ones during transition periods.

2.  **Dual Path Evolution:** Use preprocessor macros (e.g., `ENABLE_NEW_PHYSICS_SYSTEM`) to allow instant switching between old and new implementations. This enables:
    *   Easy comparison of both systems
    *   Validation of new features without risk
    *   Gradual transition with zero regression
    *   Clear rollback path if issues arise

3.  **Clear Marking:** Tag all new system code with identifiable markers (e.g., `[PHYSICS-NEW-SYSTEM]` comments) to:
    *   Make the scope of changes immediately visible
    *   Allow easy searching across the codebase
    *   Help understand the impact of each modification
    *   Facilitate future cleanup when old systems are removed

4.  **Validation First:** Always verify that the existing system remains fully functional before testing new implementations.

5.  **No Alien Code:** Changes should be understandable and reviewable. Avoid creating "black box" modifications that are difficult to audit or understand.

6.  **Step-by-Step Testing:**
    *   Test old system first (regression check)
    *   Test new system in isolation
    *   Compare behaviors and performance
    *   Document differences and advantages

7.  **Clean Separation:** Use `#if/#else/#endif` blocks to completely separate old and new code paths. Never mix the two in a way that could cause interference.

This methodology ensures that framework modifications are:
*   **Auditable:** Changes can be reviewed and understood
*   **Reversible:** Can roll back instantly if needed
*   **Safe:** No risk to production functionality
*   **Gradual:** Transition happens at a controlled pace
*   **Professional:** Suitable for enterprise-level development

Remember: The goal is not just to add new features, but to do so in a way that maintains trust, stability, and code quality throughout the migration process.
