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
