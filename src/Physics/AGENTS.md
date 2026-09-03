# Physics System

Context for developing the Emeraude Engine physics system.

## Module Overview

The Emeraude Engine physics system implements a 4-entity type architecture with differentiated collision handling to balance realism, performance, and game design.

## Physics-Specific Rules

### CRITICAL Coordinate Convention
- **Y-UP mandatory** in all physics calculations
- Gravity: a **VECTOR**, not a signed scalar — `EnvironmentPhysicalProperties::DownDirection{0,-1,0}` scaled by the surface gravity magnitude (`Physics::Gravity::Earth = 9.807`). Read the direction from the environment properties, never hard-code a Y sign
- Jump impulse: POSITIVE Y value (pushes upward)
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

### Physics Execution Pipeline (`Scene.physics.cpp:simulatePhysics()`)

**Phase 1: Static Collisions** (per-entity accumulation)
1. Accumulate boundary corrections (if sector at border)
2. Accumulate ground corrections (track separately for grounded state)
3. Accumulate StaticEntity corrections
4. Apply combined position correction
5. Apply velocity bounce + set grounded state with source

**Phase 2: Dynamic Collisions** (Node ↔ Node)
1. Iterate octree leaf sectors
2. Skip non-movable entities; skip pairs where **both** are simulation-paused
3. Test movable pairs via `detectCollisionMovableToMovable()`
4. Collect ContactManifolds
5. Resolve via `ConstraintSolver::solve()` (Sequential Impulse)
6. Re-clip involved entities to boundaries

**Sleep/Wake behavior:** Nodes at rest are paused by `checkSimulationInertia()`. A paused
Node is still a solid body — active entities (with velocity) collide against it normally.
Only pairs where both are paused are skipped (optimization for scenes with many resting
objects, e.g., settled balls). When a collision impulse is applied, `addForce()` resumes
simulation automatically via `pauseSimulation(false)`.

See: `Scene.physics.cpp:simulatePhysics()`

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
- `ConstraintSolver.hpp/.cpp` - Sequential Impulse solver for collision resolution
- `ContactManifold.hpp/.cpp` - Collision contact data structure
- `MovableTrait.hpp/.cpp` - Movement physics trait with GroundedSource tracking
- `Particle.hpp/.cpp` - Physics particle with velocity, lifetime, and modifier integration
- `@docs/physics-system.md` - Detailed architecture
- `@docs/coordinate-system.md` - Y-UP convention (CRITICAL)

## Critical: Collision Normal Convention

> [!CRITICAL]
> **`isCollidingWith()` returns `m_impactNormal` pointing from B toward A** (direction to push A out of B).
> **`m_MTV`** follows the same convention (Minimum Translation Vector to separate A from B).
>
> The `ConstraintSolver` expects normals **from A toward B** (standard physics convention:
> `relativeVelocity = velocityB - velocityA`, separating impulse when `Vn < 0`).
>
> **Consequence:** All code passing normals to `ContactManifold::addContact()` must **negate** the normal.
>
> | Function | Normal passed to manifold | Why |
> |----------|---------------------------|-----|
> | `detectCollisionMovableToMovable()` | `-results.m_impactNormal` | Solver expects A→B |
> | `detectCollisionMovableToStatic()` | `-results.m_impactNormal` | Same convention |
> | `accumulateStaticEntityCorrections()` | `-results.m_impactNormal` (as `dominantNormal`) | For velocity bounce |
>
> **Bug pattern (fixed Mar 2026):**
> ```cpp
> // BROKEN - normal points B→A, solver pushes A INTO B (attraction loop)
> manifold.addContact(results.m_contact, results.m_impactNormal, results.m_depth);
>
> // CORRECT - negate to get A→B convention
> manifold.addContact(results.m_contact, -results.m_impactNormal, results.m_depth);
> ```
>
> **Code references:**
> - `CollisionDetection.cpp:detectCollisionMovableToMovable()` - Negates normal
> - `CollisionDetection.cpp:detectCollisionMovableToStatic()` - Negates normal
> - `Scene.physics.cpp:accumulateStaticEntityCorrections()` - Negates for bounce
> - `Base/Math/Space3D/Collisions/SamePrimitive.hpp:isColliding(AACuboid, AACuboid)` - MTV pushes A out of B

## Critical: Ground and Boundary Conventions (Y-UP, Aug 2026)

> [!CRITICAL]
> **The static-collision normal points FROM the body INTO the surface it penetrates.** That is what
> `applyCollisionResponse()` reads (`vn = dot(velocity, normal) > 0` means "moving into the surface")
> and what makes `positionCorrection -= normal * penetration` push the body OUT. It is the opposite
> of a surface's outward normal, hence the deliberate negation of `getNormalAt()` in
> `Scene::accumulateGroundCorrection()`.
>
> **Consequences now that +Y is up:**
> - A GROUND contact normal is `{0,-1,0}` (downward), never `{0,+1,0}`.
> - "Is this surface a floor?" is therefore `normal[Y] < -0.7`, **not** `> +0.7`. Three sites test
>   it (boundary, static entity, `applyCollisionResponse`) and all three are correct as written.
> - A body is below ground when `position[Y] < groundLevel`; the penetration is
>   `groundLevel - corner[Y]`; a sphere's lowest point is `position[Y] - radius`; clipping out of
>   the ground moves towards **+Y**.
>
> ⚠️ **The ground block is DUPLICATED THREE TIMES** (`clipAboveGround`, `detectGroundCollision`,
> `accumulateGroundCorrection` in `Scene.physics.cpp`) and the corner CHOICE (`minY*`) and the
> penetration ARITHMETIC are ONE decision. The Y-up flip moved the corners and left the subtraction
> reversed, so every airborne body reported a collision — a half-migration that landed INSIDE the
> comment warning against it. Only `accumulateGroundCorrection` is currently wired; the other two
> have no caller today.
>
> **The grounded micro-bounce clamp — it ate the jump (fixed Aug 2026).**
> `MovableTrait::updateSimulation()` zeroes the vertical velocity of a body resting on a stable
> surface, to kill micro-bounces. The test must clamp the **DOWNWARD** velocity, which is
> **NEGATIVE** now that +Y is up: `m_linearVelocity[Y] < 0.0F`. It read `> 0.0F` under Y-down,
> where positive meant sinking — left as it was, it clamped every UPWARD velocity, and since a
> body is still grounded on the very frame it pushes off, **the player's jump was destroyed
> before it could move**: the force was applied, the velocity went positive, and this line zeroed
> it every tick.
> ⚠️⚠️ **Signature to recognise, it is unmistakable**: `velocity[Y]` reads exactly ONE frame's
> worth of impulse instead of an accumulating value (here `cyclesLeft × 0.03`, decreasing with the
> ramp), while the POSITION never moves. That means the velocity is being wiped between frames —
> not that the force is missing. Measured against a healthy jump: velocity accumulates 0.48 → 4.05
> over the 16 push frames, `isGrounded` drops on frame 17, then gravity decays it by exactly
> 9.81/60 per frame.
>
> ⚠️⚠️ **BOUNDARIES CARRY NO CONVENTION.** The world box is `[-boundary, +boundary]³` and
> `clipInsideBoundaries()` / `accumulateBoundaryCorrection()` treat the three axes identically —
> there is nothing to migrate there, and a Y-sign "fix" applied to them is a regression. The only
> Y-dependent boundary decision is which wall grounds an entity, and it lives in
> `Scene::resolveCollisions()`.

## Critical: AABB World Transform with OrientedCuboid

> [!CRITICAL]
> **`AABBCollisionModel::getAABB(worldFrame)` must handle rotation and scale, not just translation.**
>
> Rotated entities need their local AABB transformed through the full model matrix.
> `OrientedCuboid` transforms all 8 corners, then `getAxisAlignedBox()` rebuilds the
> world-space axis-aligned bounding box from the transformed corners.
>
> **Code references:**
> - `AABBCollisionModel.hpp:getAABB()` - Uses `OrientedCuboid` for full transform
> - `Base/Math/OrientedCuboid.hpp:getAxisAlignedBox()` - Rebuilds AABB from transformed corners
> - `Scenes/AbstractEntity.debug.cpp` - Visual debug uses inverse entity matrix to show world AABB

## Development Patterns

### Adding a New Collision Type
1. Define the method in `Collider`
2. Create appropriate manifolds
3. Test with all 4 entity types
4. Verify Y-UP coordinate consistency (gravity is -Y; a ground contact normal points DOWN)

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

## GroundedSource System

MovableTrait tracks not just WHETHER an entity is grounded, but WHAT it's grounded on. This enables differentiated physics behavior.

### GroundedSource Enum (`MovableTrait.hpp:GroundedSource`)

| Source | Description | Gravity | Friction |
|--------|-------------|---------|----------|
| `None` | Not grounded | Applied | No |
| `Ground` | On terrain (GroundResource) | Blocked | Yes |
| `Boundary` | On world boundary | Blocked | Yes |
| `Entity` | On StaticEntity or Node | **Applied** | Yes |

### Key Insight: Entity Grounding

When grounded on an Entity (StaticEntity or another Node), gravity is **still applied**. This is because:
- Entities can move (Nodes) or you can walk off them (StaticEntity)
- Without gravity, entities would float in air after leaving a platform
- The grace period prevents jitter but doesn't block gravity

See: `MovableTrait.cpp:updateSimulation()` - `isOnStableSurface` check

### Query Methods

| Method | Returns true if... |
|--------|---------------------|
| `isGrounded()` | Grounded on anything (Ground, Boundary, or Entity) |
| `isGroundedOnTerrain()` | Grounded on Ground only |
| `isGroundedOnBoundary()` | Grounded on Boundary only |
| `isGroundedOnEntity()` | Grounded on Entity only |
| `isGroundedOn(MovableTrait*)` | Grounded on specific entity |
| `groundedSource()` | Returns the GroundedSource enum |

### Grace Period

Grounded state uses a grace period (`GroundedGracePeriod = 15 frames`) to prevent jitter when contact is intermittent. The grace period only decrements when Y velocity is significant (`> 0.001`).

See: `MovableTrait.cpp:updateGroundedState()`

### Setting Grounded State

Callers must specify the source when setting grounded:

```cpp
// In Scene.physics.cpp (static collisions)
movable->setGrounded(GroundedSource::Ground);
movable->setGrounded(GroundedSource::Boundary);
movable->setGrounded(GroundedSource::Entity, collidedEntityPtr);

// In ConstraintSolver.cpp (dynamic collisions)
bodyA->setGrounded(GroundedSource::Entity, bodyB);
```

See: `Scene.physics.cpp:applyCollisionResponse()`, `ConstraintSolver.cpp:solveVelocityConstraints()`

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

The physics system uses collision primitives from `Base/Math/Space3D`:

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

## Particle Physics & Modifiers

Particles (used by ParticlesEmitter) integrate with the scene modifier system.

**Particle simulation** (`Particle.cpp:updateSimulation()`):
1. Query scene modifiers for forces
2. Apply gravity from environment properties
3. Apply drag based on atmospheric density

**Modifier integration**:
```cpp
scene.forEachModifiers([this, &worldCoordinates, &particleProperties] (const auto & modifier) {
    const auto force = modifier.getForceAppliedTo(worldCoordinates, m_size * 0.5F);
    m_linearVelocity += force * particleProperties.inverseMass() * WorldPhysicsUpdateCycleDurationS<float>;
});
```

Key points:
- Particles pass `m_size * 0.5F` as radius (half diameter = bounding radius)
- If radius > 0, modifier creates a Sphere for influence testing
- If radius == 0, modifier uses point-based influence
- Force is integrated with inverse mass and timestep

See: `Particle.cpp:updateSimulation()`, `@src/Scenes/AGENTS.md` → "Modifier System"

## Detailed Documentation

For complete physics system architecture:
- [`../../docs/physics-system.md`](../../docs/physics-system.md) - Detailed 4-entity architecture

Related systems:
- [`../../docs/coordinate-system.md`](../../docs/coordinate-system.md) - Y-UP convention (CRITICAL)
- [`../Scenes/AGENTS.md`](../Scenes/AGENTS.md) - Nodes with MovableTrait for physics, Modifier system
- [`../../dependencies/emeraude-base/src/AGENTS.md`](../../dependencies/emeraude-base/src/AGENTS.md) - Math (Vector, Matrix, collision primitives including Capsule)
