## GroundedSource System

MovableTrait tracks not just WHETHER an entity is grounded, but WHAT it's grounded on. This enables differentiated physics behavior.

### GroundedSource Enum (`MovableTrait.hpp:GroundedSource`)

| Source | Description |
|--------|-------------|
| `None` | Not grounded |
| `Ground` | On terrain (a ground triangle manifold) |
| `Boundary` | On the world box's floor (the boundary pass) |
| `Entity` | On a StaticEntity, a kinematic node or another Node (any manifold whose normal is within ~45° of gravity) |

⚠️ Since the physics overhaul P2 (2026-10-01) **gravity always applies** and friction is the solver's (Coulomb, per
contact): the former "stable surface" switch-off of gravity, the downward-velocity clamp and the per-tick friction
multiplier are gone (`docs/physics-overhaul.md` § 2 D). The grounded state is set by the physics step from the
manifolds and still decays over the 15-frame grace period (the walkers read it until the P4 character controller).

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
// In Scene.physics.cpp (the physics step, section 6: from the solved manifolds)
movable->setGrounded(GroundedSource::Ground);                  // a ground triangle under it
movable->setGrounded(GroundedSource::Entity, otherMovable);    // a static (nullptr), kinematic or dynamic support
// ... and in the boundary pass (section 7), through applyCollisionResponse():
movable->setGrounded(GroundedSource::Boundary);
```

See: `Scene.physics.cpp:resolveCollisions()` (sections 6 and 7), `applyCollisionResponse()`
