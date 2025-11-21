# Scene Graph Architecture

This document provides detailed architecture for the Scene Graph system, the organizational backbone for all spatial entities in the engine. The scene graph manages transformations, hierarchy, components, and synchronization between simulation and rendering.

## Quick Reference: Key Terminology

- **Node**: Dynamic entity in the scene graph tree with physics, hierarchy, and double-buffered state. Supports MovableTrait for full physics simulation.
- **StaticEntity**: Immovable entity stored in a flat map (not a tree). Optimized for static geometry like buildings. No physics overhead, but supports components.
- **AbstractEntity**: Common base class for Node and StaticEntity. Manages component attachment and lifecycle.
- **CartesianFrame**: Transformation representation storing position + 3 orthonormal basis vectors (X, Y, Z axes). One axis computed via cross product of the other two.
- **Component**: Modular behavior/data attached to entities. Examples: Visual, Light, Camera, SoundEmitter. Each component has a unique name within its entity.
- **LocatableInterface**: Interface providing position/rotation/scale access. Implemented by both StaticEntity (directly) and MovableTrait (which Node uses).
- **MovableTrait**: Physics-enabled trait adding velocity, forces, mass, and integration. Inherits from LocatableInterface. Used by Nodes for dynamic behavior.
- **Double Buffering**: Technique where each Node/StaticEntity maintains two CartesianFrame states (active + render) for thread-safe rendering.
- **AVConsole**: Per-scene manager that detects Camera and Microphone components, linking them to render targets and audio outputs.
- **LightSet**: Per-scene structure tracking all Light components, synchronizing lighting state with GPU via Uniform Buffer Objects (UBO).

## Design Philosophy: Generic Containers + Modular Components

### Core Principle: Entities Are Generic Until Components Define Them

The scene graph distinguishes between two fundamental container types:
- **Node**: Dynamic, hierarchical, physics-enabled
- **StaticEntity**: Immovable, flat, performance-optimized

Both are **generic containers** that gain meaning through attached **components**:
- Without components → just a position in space
- With Visual component → something to render
- With Light component → illuminates the scene
- With Camera component → defines a viewport
- With SoundEmitter component → plays audio

### Why This Design?

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

### Emeraude's Solution: Composition Over Inheritance

```cpp
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

## Architecture: The Two Entity Types

### Hierarchy Overview

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

### Entity Type 1: Node (Dynamic Hierarchy)

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

### Entity Type 2: StaticEntity (Performance-Optimized)

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

### Comparison Table

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

## Component System: Modular Behaviors

### How Components Work

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
```

### Available Component Types

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

### Component Lifecycle Methods

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

## Transformations: CartesianFrame System

### CartesianFrame Structure

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

### Transformation Hierarchy

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
- Root has identity transform (world origin)

**StaticEntity Transformations:**
- No parent → **local = world** always
- Direct position setting
- Still uses CartesianFrame for consistency

## Double Buffering: Thread-Safe Simulation and Rendering

### The Problem: Two Threads, One Scene

```
Thread 1 (Logic - 60 Hz):     Thread 2 (Rendering - Variable FPS):
- Update physics               - Read positions
- Move entities                - Build draw calls
- Modify transforms            - Submit to GPU
```

**Without synchronization:**
- Renderer reads positions **while** physics is writing them
- Results: Torn reads, visual artifacts, flickering, objects "jumping"

### Emeraude's Solution: Double Buffering

Each Node and StaticEntity maintains **TWO CartesianFrame states**:

```cpp
class Node {
    CartesianFrame m_activeFrame;   // Written by logic thread (60 Hz)
    CartesianFrame m_renderFrame;   // Read by render thread (variable)
};
```

### Update Flow: Logic Thread (60 Hz)

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

3. Scene Graph Update (root-to-leaves traversal)
   For each Node (depth-first):
     a. Update CartesianFrame (combine parent + local)
     b. Call processLogics() on all components
     c. Recurse to children

4. StaticEntity Update (map iteration)
   For each StaticEntity:
     - Call processLogics() on all components

5. Finalization & Swap
   - ATOMIC SWAP: m_renderFrame = m_activeFrame
   - All Nodes and StaticEntities swapped
   - Scene now ready for next render frame
```

## AVConsole and LightSet: Automatic Management

### AVConsole (AudioVideoConsole)

**Purpose:** Automatically detect and link Camera and Microphone components to render targets and audio outputs.

```cpp
class AVConsole::Manager {
    std::vector<Camera*> m_cameras;
    std::vector<Microphone*> m_microphones;
    std::vector<RenderTarget*> m_renderTargets;

    // Automatically called when Camera/Microphone created
    void registerCamera(Camera* cam);
    void registerMicrophone(Microphone* mic);
};
```

### LightSet - Per Scene Lighting State

**Purpose:** Track all Light components, synchronize with GPU via Uniform Buffer Objects (UBO).

```cpp
class LightSet {
    std::vector<DirectionalLight*> m_directionalLights;
    std::vector<PointLight*> m_pointLights;
    std::vector<SpotLight*> m_spotLights;

    VkBuffer m_lightingUBO;  // GPU buffer

    // Called before rendering
    void updateGPUBuffer();
};
```

## Common Usage Patterns

### Pattern 1: Creating a Dynamic Object (Node)

```cpp
// Player character
auto player = scene->root()->createChild("player", initialPos);

// Visual representation
auto mesh = resources.container<MeshResource>()->getResource("character");
player->newVisual(mesh, castShadows, receiveShadows, "body");

// First-person camera
player->newCamera(90.0f, 16.0f/9.0f, 0.1f, 1000.0f, "player_cam");

// Physics configuration
player->bodyPhysicalProperties().setMass(80.0f);  // 80 kg
player->enableSphereCollision(true);
```

### Pattern 2: Creating Static Geometry (StaticEntity)

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

### Pattern 3: Hierarchical Objects (Vehicle with Wheels)

```cpp
// Vehicle body
auto vehicle = scene->root()->createChild("vehicle", vehiclePos);
vehicle->newVisual(carBodyMesh, true, true, "body");

// Wheels (children of vehicle)
auto wheelFL = vehicle->createChild("wheel_FL", localPos_FL);
wheelFL->newVisual(wheelMesh, true, true, "wheel");

// Moving vehicle → wheels move automatically (hierarchy)
vehicle->applyForce(forwardVector * thrust);
```

## Design Principles Summary

| Principle | Description | Benefit |
|-----------|-------------|---------|
| **Generic Containers** | Entities gain meaning through components, not inheritance | Flexibility, composability, no code duplication |
| **Two Entity Types** | Node (dynamic) vs StaticEntity (optimized) | Performance for static world, full features for dynamic |
| **Composition Over Inheritance** | Attach components instead of subclassing | Easy to add/remove behaviors, no rigid hierarchies |
| **Double Buffering** | Separate active and render states | Thread-safe, no visual artifacts, smooth rendering |
| **RAII Hierarchy** | Smart pointers manage lifetime, parent destruction cascades | No memory leaks, no dangling pointers, safe cleanup |
| **Automatic Registration** | Components auto-register with AVConsole and LightSet | No manual tracking, dynamic discovery, zero boilerplate |

### Core Philosophy
> "Entities are positions in space. Components give them meaning. Hierarchy gives them relationships. Double buffering keeps them safe across threads."