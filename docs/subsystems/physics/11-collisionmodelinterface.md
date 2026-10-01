## CollisionModelInterface

The collision system uses a unified `CollisionModelInterface` for all collision primitives. This stateless design injects world positions at test time, enabling sharing between identical entities. Since the physics overhaul the models only DESCRIBE a shape: the contacts come from `Physics::NarrowPhase` (base contact manifolds); the models' former MTV tests (`isCollidingWith()`, `collideWith*()`, `CollisionDetectionResults`) were removed on 2026-10-02 (P3.a), nothing called them since P2.

### Collision Model Types

| Model | Class | Narrow-phase shape | Use Case |
|-------|-------|--------------------|----------|
| Point | `PointCollisionModel` | a sphere of the speculative margin; never rotates | Raycasting endpoints, triggers |
| Sphere | `SphereCollisionModel` | `Space3D::Sphere` at the origin (frame scaling ignored) | Simple entities, particles, projectiles |
| Box | `BoxCollisionModel` | `Space3D::OrientedBox` that TURNS with the entity (`toWorldBox()`), frame scaling applied | Boxes, props, static solids, characters |
| Capsule | `CapsuleCollisionModel` | `Space3D::Capsule` turned by the frame (scaling ignored) | Characters, elongated objects |

`BoxCollisionModel` was `AABBCollisionModel` until 2026-10-02 (P3, owner decision 8a): the narrow phase collided the
world ENVELOPE of the 8 rotated corners, so a 45° box was 1.41× wider than itself. The octree still uses that envelope
(`getAABB(frame)`), the contacts use the oriented box.

### Key Interface Methods

| Method | Purpose |
|--------|---------|
| `modelType()` | Returns the enum the narrow phase dispatches on |
| `getAABB()` | Returns local or world-space bounding box (broad phase, boundaries, editor picking) |
| `getRadius()` | Returns maximum bounding radius for the shape |
| `centerOfMassOffset(scaling)` | The centroid of the shape from the entity's origin, in its axes: the point the physics turns the body about (P3, decision 8b) |
| `solidInertia(mass, scaling)` | The inertia tensor of the shape as a uniform solid about that centroid (nothing for a point or an unusable shape) |
| `overrideShapeParameters()` | Manually set shape dimensions (marks as overridden) |
| `areShapeParametersOverridden()` | Check if manually configured |
| `mergeShapeParameters()` | Expand shape to encompass new bounds |
| `resetShapeParameters()` | Reset to empty state before recalculation |

The body's inverse inertia combines the two: `Physics::localInverseInertia(properties, model, scaling)` takes the
EXPLICIT tensor of `BodyPhysicalProperties` when the author set one, else `solidInertia()`; `worldInverseInertia()`
turns it into world space (`Physics/BodyInertia.hpp`). Topic: `16-rigid-body-rotation.md`.

### getRadius() Implementation

Returns the bounding sphere radius for each model type:
- **Point**: `0.0F`
- **Sphere**: `m_radius`
- **Box**: `max(width, height, depth) * 0.5F` (⚠️ not the half diagonal, and it ignores an off-centre box: a loose
  bound for the push modifiers and the editor, never used by the physics)
- **Capsule**: `halfAxisLength + radius`

See: `CollisionModelInterface.hpp:getRadius()`

### Convenient Constructors

```cpp
// Box with separate half-extents
BoxCollisionModel(halfWidth, halfHeight, halfDepth, parametersOverridden = false)

// Vertical capsule from radius and total height
CapsuleCollisionModel(radius, height, parametersOverridden = false)
```

### Auto-Box Creation

When an entity has visual components but no collision model:
1. `AbstractEntity::updateEntityProperties()` iterates all components
2. Merges component bounding boxes into a single local box
3. Creates `BoxCollisionModel` automatically if none exists

**CRITICAL**: If `areShapeParametersOverridden()` returns true, auto-merge is skipped.

See: `AbstractEntity.cpp:updateEntityProperties()`
