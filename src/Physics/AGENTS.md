# Physics System

Context for developing the Emeraude Engine physics system.

## Module Overview

The Emeraude Engine physics system implements a 4-entity type architecture with differentiated collision handling to balance realism, performance, and game design.

## Physics-Specific Rules

### CRITICAL Coordinate Convention
- **Y-DOWN mandatory** in all physics calculations
- Gravity: `+9.81` on Y axis (pulls downward)
- Jump impulse: negative Y value (pushes upward)
- Forward thrust: negative Z value

### Entity Types (4 distinct types)
1. **Boundaries**: Game constraints (invisible walls)
2. **Ground**: Hybrid physical surfaces (stability + realism)
3. **StaticEntity**: Static objects with defined mass
4. **Nodes**: Full dynamic entities (via MovableTrait)

### Node-Centric Correction Philosophy

**CRITICAL**: Only Nodes receive corrections. All other entity types are passive influences.

| Collision Type | Who is corrected? | Influence |
|----------------|-------------------|-----------|
| Node ↔ Boundary | Node only | Infinite mass (100% absorption) |
| Node ↔ Ground | Node only | Infinite mass (100% absorption) |
| Node ↔ StaticEntity | Node only | Infinite mass (100% absorption) |
| Node ↔ Node | Both Nodes | Mass-proportional distribution |

**Mental model**: Instead of "resolve collision between A and B", think:
> "What correction to apply to THIS Node, given what it touches?"

The Node is the active subject; other entities are parameters influencing the correction.

### Correction Priority Order

**CRITICAL**: Process corrections from MOST constraining to LEAST constraining.

```
1. Boundaries    → ABSOLUTE constraints (world limits, inviolable)
2. Ground        → BASE constraints (where entities can exist)
3. StaticEntity  → FIXED obstacles (walls, rocks, structures)
4. Node ↔ Node   → DYNAMIC interactions (negotiable, flexible)
```

**Why this order matters**:
- If Node↔Node corrected BEFORE Ground → Node could be pushed INTO the ground
- If Ground corrected BEFORE Boundaries → Node could be pushed OUT of the world
- Each pass respects constraints established by previous passes
- Later passes adapt to the "remaining space" after hard constraints

### Physics Execution Pipeline
1. Force integration (gravity, drag, accumulated forces)
2. Broad phase (spatial partitioning via octree)
3. Narrow phase (detailed collision detection)
4. **Correction passes (in priority order)**:
   - 4a. Boundary corrections
   - 4b. Ground corrections
   - 4c. StaticEntity corrections
   - 4d. Node↔Node corrections

## Development Commands

```bash
# Physics-specific tests
ctest -R Physics
./test --filter="*Physics*"
```

## Important Files

- `CollisionModelInterface.hpp` - Abstract interface for all collision models
- `PointCollisionModel.hpp/.cpp` - Zero-volume point collision
- `SphereCollisionModel.hpp/.cpp` - Sphere collision primitive
- `AABBCollisionModel.hpp/.cpp` - Axis-aligned bounding box
- `CapsuleCollisionModel.hpp/.cpp` - Swept sphere (capsule) collision
- `CollisionDetection.cpp` - Collision detection algorithms
- `Particle.hpp/.cpp` - Physics particle with collision model
- `@docs/physics-system.md` - Detailed architecture
- `@docs/coordinate-system.md` - Y-down convention (CRITICAL)

## Development Patterns

### Adding a New Collision Type
1. Define the method in `Collider`
2. Create appropriate manifolds
3. Test with all 4 entity types
4. Verify Y-down coordinate consistency

### Modifying the Solver
1. Maintain 8 velocity iterations, 3 position iterations
2. Apply impulses according to entity type
3. Respect entity/ground separation
4. Preserve boundary separation (no manifolds)

## Critical Points

- **NEVER** convert Y coordinates
- Calculate `penetrationDepth` BEFORE hard clipping (ground)
- Mass matters for StaticEntity (no infinite mass)
- **4 correction passes** in priority order (Boundaries → Ground → StaticEntity → Node↔Node)
- **Scenes integration**: Scene graph Nodes inherit MovableTrait for physics
- **Spatial octree**: Scene owns Octree for physics broad-phase

## CollisionModelInterface

The collision system uses a unified `CollisionModelInterface` for all collision primitives. This stateless design injects world positions at test time, enabling sharing between identical entities.

### Collision Model Types

| Model | Class | Use Case |
|-------|-------|----------|
| Point | `PointCollisionModel` | Raycasting endpoints, triggers |
| Sphere | `SphereCollisionModel` | Simple entities, particles, projectiles |
| AABB | `AABBCollisionModel` | Static objects, boxes, triggers |
| Capsule | `CapsuleCollisionModel` | Characters, elongated objects |

### Key Interface Methods

| Method | Purpose |
|--------|---------|
| `modelType()` | Returns enum for double dispatch |
| `isCollidingWith()` | Tests collision with another model |
| `getAABB()` | Returns local or world-space bounding box |
| `getRadius()` | Returns maximum bounding radius for the shape |
| `overrideShapeParameters()` | Manually set shape dimensions (marks as overridden) |
| `areShapeParametersOverridden()` | Check if manually configured |
| `mergeShapeParameters()` | Expand shape to encompass new bounds |
| `resetShapeParameters()` | Reset to empty state before recalculation |

### getRadius() Implementation

Returns the bounding sphere radius for each model type:
- **Point**: `0.0F`
- **Sphere**: `m_radius`
- **AABB**: `max(width, height, depth) * 0.5F`
- **Capsule**: `halfAxisLength + radius`

See: `CollisionModelInterface.hpp:getRadius()`

### Convenient Constructors

```cpp
// AABB with separate half-extents
AABBCollisionModel(halfWidth, halfHeight, halfDepth, parametersOverridden = false)

// Vertical capsule from radius and total height
CapsuleCollisionModel(radius, height, parametersOverridden = false)
```

### Auto-AABB Creation

When an entity has visual components but no collision model:
1. `AbstractEntity::updateEntityProperties()` iterates all components
2. Merges component bounding boxes into a single AABB
3. Creates `AABBCollisionModel` automatically if none exists

**CRITICAL**: If `areShapeParametersOverridden()` returns true, auto-merge is skipped.

See: `AbstractEntity.cpp:updateEntityProperties()`

## Entity Physics Flags

AbstractEntity uses minimal flags for physics control:

| Flag | Purpose | Check Method |
|------|---------|--------------|
| `IsCollisionDisabled` | Skip collision detection | `isCollidable()` (inverted) |
| `IsSimulationPaused` | Skip gravity/drag | `isSimulationPaused()` |

**Removed flags** (now derived from actual state):
- ~~`HasBodyPhysicalProperties`~~ → Use `bodyPhysicalProperties().mass() > 0`

**Physics participation conditions:**
```cpp
hasMovableAbility()                    // Node (not StaticEntity)
&& !isSimulationPaused()               // Simulation active
&& getMovableTrait()->isMovable()      // Movement enabled
&& hasCollisionModel()                 // Has collision primitive
&& isCollidable()                      // Collision not disabled
```

See: `AbstractEntity.hpp:IsCollisionDisabled`, `AbstractEntity.hpp:IsSimulationPaused`

## Available Collision Primitives

The physics system uses collision primitives from `Libs/Math/Space3D`:

| Primitive | Description | Use Case |
|-----------|-------------|----------|
| `Sphere` | Center + radius | Simple entities, particles |
| `Capsule` | Axis segment + radius | Characters, elongated objects |
| `AACuboid` | Axis-aligned box | Static objects, triggers |
| `Triangle` | 3 vertices | Terrain mesh collision |

**Capsule** is ideal for character collision:
- Better fit for humanoid shapes than spheres
- Handles slopes and stairs naturally
- See: `@src/Libs/AGENTS.md` → "Math/Space3D: Capsule Primitive"

## Detailed Documentation

For complete physics system architecture:
- @docs/physics-system.md - Detailed 4-entity architecture

Related systems:
- @docs/coordinate-system.md - Y-down convention (CRITICAL)
- @src/Scenes/AGENTS.md - Nodes with MovableTrait for physics
- @src/Libs/AGENTS.md - Math (Vector, Matrix, collision primitives including Capsule)
