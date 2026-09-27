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
