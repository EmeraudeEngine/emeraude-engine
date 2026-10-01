## Critical: the box model's world AABB is the BROAD-PHASE envelope only

> [!CRITICAL]
> **`BoxCollisionModel::getAABB(worldFrame)` must handle rotation and scale, not just translation** — and since P3
> (2026-10-02) it is no longer what the contacts use.
>
> - `getAABB(worldFrame)`: the world axis-aligned envelope — `OrientedCuboid` transforms all 8 corners through the full
>   model matrix, then `getAxisAlignedBox()` rebuilds the box. The physics octree, the boundary clip (exact for a
>   plane: the extreme of a rotated box along X is its envelope's), the push modifiers and the editor's picking use it.
> - `toWorldBox(worldFrame)`: the ORIENTED world box (`Space3D::OrientedBox::fromCuboid()`), what `Physics::NarrowPhase`
>   collides. Before P3 the narrow phase collided the envelope: a box resting flat tilted 5°, a box dropped on its edge
>   stayed on it, a rotated slab was a step.
>
> **Code references:**
> - `BoxCollisionModel.hpp:getAABB()` / `toWorldBox()`
> - `Base/Math/OrientedCuboid.hpp:getAxisAlignedBox()` - Rebuilds AABB from transformed corners
> - `Scenes/AbstractEntity.debug.cpp` - the collision-shape overlay draws the LOCAL box in the entity's space (it turns
>   with it, like the collider); before P3 it drew the world envelope through the inverse entity matrix
