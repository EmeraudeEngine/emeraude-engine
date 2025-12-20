# Physics System Architecture

This document provides detailed architecture for the physics system, including entity types, collision semantics, and execution flow.

## Quick Reference: Key Terminology

- **Contact Manifold**: Data structure containing collision information (contact points, penetration depth, collision normal) between two entities.
- **Constraint Solver**: Iterative physics solver (Sequential Impulse method) that resolves collisions by computing and applying impulses to entities.
- **Hard Clipping**: Direct position correction that prevents entities from penetrating boundaries or ground, ensuring absolute constraint enforcement.
- **Velocity Inversion**: Manual velocity reversal applied for game design purposes (e.g., bouncing off invisible boundaries).
- **Impulse**: Instantaneous change in momentum applied to an entity (force × time), used to resolve collisions realistically.
- **Restitution (Bounciness)**: Material property coefficient [0.0-1.0] that determines energy preservation in collisions (0 = no bounce, 1 = perfect bounce).
- **GroundResource**: Interface that provides ground height queries at any 2D coordinate: `(X, Z) → Y height`.

## Design Philosophy: Why 4 Entity Types?

The physics system distinguishes between **four fundamental entity types** to balance realism, performance, and game design needs:

1. **Boundaries** → Pure game design (invisible walls for player containment)
2. **Ground** → Hybrid approach (stability + realism)
3. **StaticEntity** → Performance optimization (immovable objects with simplified physics)
4. **Nodes** → Full realism (complete Newtonian physics simulation)

This separation allows each type to have **clear, non-overlapping collision semantics** while maintaining **uniform behavior within each type**.

## Entity Type 1: Scene Boundaries (World Cube Limits)

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

## Entity Type 2: Ground (Scene Area Resource)

**Category:** Physical Surface (Hybrid Approach)

**Purpose:** Defines the terrain/floor height at any 2D coordinate `(X, Z)`. The `GroundResource` interface provides height queries: `getHeightAt(x, z) → y`.

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
float groundY = ground->getHeightAt(entity.x, entity.z);
float penetrationDepth = entity.y - groundY;  // Calculate BEFORE clip
if (penetrationDepth > 0.0F) {
    // 1. Hard clip for stability
    entity.setYPosition(groundY);

    // 2. Create manifold for solver (preserves penetration depth for velocity correction)
    ContactManifold manifold{entity, ground, penetrationDepth, normal};
    groundManifolds.push_back(manifold);
}
```

## Entity Type 3: StaticEntity (Kinematic Deflector)

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

## Entity Type 4: Nodes (Dynamic Entities)

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

## Physics Execution Order

The physics system executes in two phases per fixed timestep. See `Scene.physics.cpp:simulatePhysics()`.

### Phase 1: Static Collisions (Per-Entity Accumulation)

For each movable entity in the physics octree:

```
1. Accumulate corrections from all static sources:
   a. Boundary corrections (if sector touches world border)
   b. Ground corrections (track separately for grounded state)
   c. StaticEntity corrections (track dominant collision entity)

2. Track dominant collision source:
   → Which source had deepest penetration?
   → Store GroundedSource + entity pointer for grounded state

3. Apply combined position correction:
   → Single moveFromPhysics() call with total correction

4. Apply collision response:
   → Velocity bounce based on dominant normal
   → Set grounded state with appropriate source
   → Priority: Ground > Boundary > Entity
```

### Phase 2: Dynamic Collisions (Node ↔ Node)

```
1. Iterate octree leaf sectors
   → For each movable pair in same sector

2. Detect collisions:
   → detectCollisionMovableToMovable() creates ContactManifolds
   → Track involved entities for boundary re-clipping

3. Resolve via Sequential Impulse Solver:
   → ConstraintSolver::solve(manifolds, dt)
   → 8 velocity iterations, 3 position iterations
   → Impulses applied to both bodies (mass-proportional)
   → Grounded state set with GroundedSource::Entity

4. Re-clip involved entities:
   → Impulse resolution may push entities outside boundaries
   → clipInsideBoundaries() for all involved entities
```

### Key Implementation Details

- **Pair deduplication:** `createEntityPairKey()` prevents testing same pair twice across sectors
- **Grounded marking:** Only mark grounded if collision normal is ~vertical (Y > 0.7 threshold)
- **Static-only grounding:** ConstraintSolver only grounds against non-movable bodies
- **Boundary re-clip:** Critical to prevent impulse resolution pushing entities out of world

## Design Principles Summary

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

## GroundedSource System

The physics system tracks not just WHETHER an entity is grounded, but WHAT TYPE of surface it's grounded on. This enables differentiated behavior for gravity and friction.

### GroundedSource Enum

```cpp
enum class GroundedSource : uint8_t {
    None,      // Not grounded
    Ground,    // On terrain (GroundResource)
    Boundary,  // On world boundary
    Entity     // On StaticEntity or Node
};
```

### Differentiated Gravity Behavior

| Grounded On | Gravity Applied? | Rationale |
|-------------|------------------|-----------|
| Ground | No | Terrain is stable, infinite mass |
| Boundary | No | World limits are absolute constraints |
| Entity | **Yes** | Can walk off platforms, entities can move |
| None | Yes | Freefall |

**Key Design Decision:** When grounded on an Entity (StaticEntity or Node), gravity continues to apply. This prevents the "floating platform" bug where entities would remain suspended in air after walking off a platform edge due to the grounded grace period.

### Implementation

```cpp
// MovableTrait.cpp:updateSimulation()
const bool isOnStableSurface = this->isGroundedOnTerrain() || this->isGroundedOnBoundary();

// Gravity only blocked for stable surfaces
const bool shouldApplyGravity = !isOnStableSurface && !this->isFreeFlyModeEnabled() && !objectProperties.isMassNull();
```

### Grace Period

Grounded state persists for 15 frames (`GroundedGracePeriod`) after losing contact to prevent jitter. The grace period only decrements when Y velocity is significant (> 0.001 m/s).

See: `MovableTrait.hpp`, `MovableTrait.cpp:updateGroundedState()`

## Physics Modifiers (Force Zones)

Modifiers are components that apply forces to entities within a defined influence area. Unlike collisions, modifiers don't prevent movement but push/pull entities.

### Available Modifiers

| Modifier | Effect | Use Cases |
|----------|--------|-----------|
| `SphericalPushModifier` | Radial force outward from center | Explosions, repulsors |
| `DirectionalPushModifier` | Constant force in one direction | Wind, conveyor belts |

### Influence Area System

Modifiers use an influence area to define their zone of effect and force falloff:

**SphericalInfluenceArea** (`SphericalInfluenceArea.cpp/.hpp`):
- Defines sphere with inner and outer radius
- Inside inner radius: full influence (1.0)
- Between inner and outer: linear falloff
- Outside outer radius: no influence (0.0)
- Supports both Sphere and AABB collision models

**CubicInfluenceArea** (`CubicInfluenceArea.cpp/.hpp`):
- Defines oriented box in modifier's local space
- Uses parent entity's world matrix for transformation
- Returns full influence (1.0) for entities inside, 0.0 outside
- Supports both Sphere and AABB collision models

### Modifier Execution Flow

```
Scene::applyModifiers() [Scene.physics.cpp]
├── For each Node with physics:
│   ├── Get entity's AABB in world coordinates
│   ├── For each modifier in m_modifiers:
│   │   ├── Call modifier->getForceAppliedToEntity(worldCoords, aabb)
│   │   │   ├── Check influenceArea()->influenceStrength()
│   │   │   └── Return force vector scaled by influence
│   │   └── Accumulate force
│   └── Apply total force to entity
```

### CRITICAL: World Coordinates Convention

**The `worldBoundingBox` parameter passed to influence area methods is ALREADY in world coordinates.**

When `CollisionModel::getAABB(worldFrame)` is called, it returns an AABB with min/max positions that are absolute world positions, NOT relative offsets.

**Correct usage:**
```cpp
// worldBoundingBox.minimum() and maximum() are WORLD positions
const auto & boxMin = worldBoundingBox.minimum();
const auto & boxMax = worldBoundingBox.maximum();
// Use directly for intersection tests
```

**WRONG (historical bug):**
```cpp
// DO NOT add entity position - it's already included!
const auto worldMin = targetPosition + worldBoundingBox.minimum(); // WRONG!
```

### Sphere-AABB Intersection (SphericalInfluenceArea)

Algorithm: Find closest point on AABB to sphere center, check distance.

```cpp
// Find closest point on box to sphere center
Vector<3> closestPoint(
    std::clamp(sphereCenter[X], boxMin[X], boxMax[X]),
    std::clamp(sphereCenter[Y], boxMin[Y], boxMax[Y]),
    std::clamp(sphereCenter[Z], boxMin[Z], boxMax[Z])
);
float distance = (closestPoint - sphereCenter).length();
bool intersects = distance <= sphereRadius;
```

### AABB-vs-Oriented-Box Intersection (CubicInfluenceArea)

Algorithm: Transform AABB center to modifier local space, expand bounds by half-extents.

```cpp
// Transform world AABB center to modifier's local space
const auto center = (boxMin + boxMax) * 0.5F;
const auto localPos = getPositionInModifierSpace(center); // Uses inverted model matrix

// Test with expanded bounds (accounting for AABB size)
bool intersectsX = localPos[X] + halfExtentX >= -m_xSize && localPos[X] - halfExtentX <= m_xSize;
// ... similar for Y, Z
```

### Performance Note

Currently, modifiers are stored in a flat set (`m_modifiers`) and iterated for every entity each frame. This is O(entities × modifiers).

**Future optimization:** Integrate modifiers into the physics octree for O(log n) spatial queries, similar to entity collision detection.