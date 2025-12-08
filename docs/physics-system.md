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
   → Query GroundResource for ground height
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