# ADR-007: Physics Four-Entity Architecture

## Status
**Accepted** - Implemented as core physics system design

## Context

Physics engines must balance competing concerns:
- **Realism**: Accurate Newtonian physics simulation
- **Performance**: Efficient simulation for real-time applications  
- **Game Design**: Artificial constraints for gameplay purposes
- **Stability**: Reliable behavior under all conditions

**Traditional Approaches:**
1. **Single entity type**: All objects use same physics rules (rigid bodies)
2. **Static vs Dynamic**: Two categories (immovable vs movable)
3. **Kinematic objects**: Movable but not affected by forces
4. **Trigger volumes**: Non-physical areas for game logic

**Problems with Traditional Approaches:**
- **Mixed semantics**: Same collision rules apply to fundamentally different object types
- **Performance waste**: Full physics on objects that never need it
- **Game design conflicts**: Realistic physics vs gameplay requirements
- **Complex special cases**: Many "if immovable then..." conditions

**Specific Challenges:**
- **Boundaries**: Invisible walls for player containment (not realistic)
- **Ground/Terrain**: Must be stable (no tunneling) but still physically realistic
- **Static objects**: Should affect physics but never move themselves  
- **Dynamic objects**: Full realistic physics simulation required

## Decision

**Emeraude Engine implements a four-entity architecture where each entity type has clear, non-overlapping collision semantics optimized for its purpose.**

**Entity Types:**
1. **Boundaries**: Game design constraints (invisible walls)
2. **Ground**: Hybrid approach (stability + realism)  
3. **StaticEntity**: Performance optimization (immovable with physics properties)
4. **Nodes**: Full realism (complete Newtonian simulation)

**Core Principle:**
> "Each entity type represents an explicit design decision between realism, performance, and game design needs."

## Architecture Details

**Entity Type 1: Boundaries (World Cube Limits)**
- **Purpose**: Define playable area, prevent entities from escaping world
- **Collision method**: Hard clipping + manual velocity handling
- **Physics approach**: Fake physics for better game feel
- **NO contact manifolds**: Never routed through physics solver

```cpp
// Implementation
if (entity.x > boundary.maxX) {
    entity.setX(boundary.maxX);  // Hard clip
    entity.velocity.x = -entity.velocity.x * dampening;  // Manual inversion
}
```

**Entity Type 2: Ground (Scene Area Resource)**
- **Purpose**: Terrain/floor surface with guaranteed stability
- **Collision method**: Hard clipping + contact manifolds
- **Physics approach**: Hybrid (stability for tunneling prevention + realism for physics response)
- **Critical**: Calculate penetration depth BEFORE hard clipping

```cpp
// Implementation  
float penetrationDepth = entity.y - groundY;  // BEFORE clip!
if (penetrationDepth > 0.0F) {
    entity.setY(groundY);  // Hard clip for stability
    createManifold(entity, ground, penetrationDepth);  // Physics for response
}
```

**Entity Type 3: StaticEntity (Kinematic Deflector)**
- **Purpose**: Static world objects that affect physics but never move
- **Collision method**: Contact manifolds with defined mass
- **Physics approach**: Simplified realistic (impulses only applied to other entity)
- **Mass matters**: StaticEntity mass affects collision response calculation

```cpp
// Implementation
StaticEntity building;
building.setMass(50000.0f);  // 50 tons
// Collision with Node: only Node receives impulse, StaticEntity never moves
```

**Entity Type 4: Nodes (Dynamic Entities)**
- **Purpose**: All dynamic game objects requiring full physics
- **Collision method**: Full bilateral contact manifolds 
- **Physics approach**: Complete Newtonian simulation
- **All physics features**: Forces, impulses, velocity integration, position integration

## Collision Interaction Matrix

| Collider 1 | Collider 2 | Method | Physics Response |
|------------|------------|---------|------------------|
| **Boundary** | Any | Hard clip + velocity inversion | NO solver |
| **Ground** | Node | Hard clip + manifolds | Node receives impulse |
| **StaticEntity** | Node | Manifolds only | Node receives impulse |  
| **Node** | Node | Bilateral manifolds | Both receive impulses |

## Execution Order (Critical)

```
1. Integrate forces (Nodes only)
   → Apply gravity, user forces, drag
   → Update velocities: v += (F/m) * dt
   → Update positions: p += v * dt

2. Broad phase collision detection
   → Octree spatial queries for potentially colliding pairs

3. Narrow phase collision detection  
   → Precise intersection tests, generate manifolds

4. Solve inter-entity collisions (FIRST solver call)
   → Node↔Node and Node↔StaticEntity manifolds
   → 8 velocity iterations, 3 position iterations

5. Ground collision check
   → Query SceneAreaResource for height
   → Calculate penetration BEFORE clipping (critical!)
   → Apply hard clip, create ground manifolds

6. Boundary collision check  
   → Check world cube limits
   → Apply hard clip + manual velocity inversion
   → NO manifolds (skip solver)

7. Solve ground collisions (SECOND solver call)
   → Process ground manifolds from step 5
   → Apply realistic bounce/friction response
```

## Consequences

### Positive
- **Clear semantics**: Each entity type has well-defined collision behavior
- **Performance optimization**: StaticEntity skips expensive physics integration
- **Stability guarantee**: Ground hard clipping prevents tunneling
- **Game design support**: Boundaries provide non-realistic but useful constraints
- **No leakage**: Implementation details don't affect other entity types
- **Explicit tradeoffs**: Each type represents conscious design decision

### Negative
- **Complexity**: Four different collision code paths to maintain
- **Learning curve**: Developers must understand which entity type to use when
- **Code duplication**: Some collision detection logic repeated across types
- **Debug complexity**: Different entity types behave differently in edge cases

### Neutral
- **Flexibility vs consistency**: Trade-off between optimized behavior and uniform interface
- **Realism vs practicality**: Explicit choice points between realistic and practical physics

## Implementation Guidelines

**Entity Type Selection:**
```cpp
// Use Boundaries for:
Invisible walls, world limits, player containment areas

// Use Ground for:  
Terrain, floors, any surface requiring absolute stability

// Use StaticEntity for:
Buildings, rocks, walls, any immovable object with defined mass  

// Use Node for:
Players, projectiles, vehicles, any object that should move
```

**Y-Down Convention Integration:**
```cpp
// Gravity (positive Y = downward)
const float GRAVITY = +9.81f;

// Jump impulse (negative Y = upward)  
node.applyImpulse(Vector3(0.0f, -jumpForce, 0.0f));

// Ground normal (pointing upward)
Vector3 groundNormal(0.0f, -1.0f, 0.0f);
```

**Critical Implementation Notes:**
- Ground penetration depth MUST be calculated before hard clipping
- Boundary collisions bypass physics solver completely
- StaticEntity mass affects Node collision response
- Two separate solver calls (inter-entity, then ground)

## Design Rationale

**Why Not Traditional Static/Dynamic Split?**
- Ground needs stability (hard clipping) + realism (physics response)
- Boundaries need fake physics for game design (velocity inversion)  
- StaticEntity needs optimization (no integration) + physics participation

**Why Not Single Physics Model?**
- Performance: StaticEntity saves computational cost
- Stability: Ground hard clipping prevents tunneling
- Game Design: Boundaries provide necessary non-realistic constraints

**Why Four Types Specifically?**
- Boundaries: Pure game design constraint
- Ground: Hybrid stability + realism
- StaticEntity: Performance optimization
- Nodes: Pure realism
- Covers all common use cases without overlap

## Related ADRs
- ADR-001: Y-Down Coordinate System (gravity and forces use +Y downward)
- ADR-006: Component Composition Over Inheritance (Node uses MovableTrait for physics)
- ADR-008: Double Buffering Thread Safety (physics writes, render reads separate states)

## References
- `docs/physics-system.md` - Complete physics architecture and collision semantics
- `src/Physics/AGENTS.md` - Physics system implementation context
- `docs/coordinate-system.md` - Y-down convention for physics calculations