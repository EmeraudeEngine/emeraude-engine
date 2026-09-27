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
