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
