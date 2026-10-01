## Critical: Collision Normal Convention

> [!CRITICAL]
> **Since the physics overhaul P2 (2026-10-01) the step uses the base CONTACT MANIFOLDS, whose normal points FROM A TO
> B** (`Base::Math::Space3D::ContactManifold`, emeraude-base `docs/subsystems/source-tree/17-math-space3d-contacts.md`),
> the convention `Physics::SoftStepSolver` expects (relative velocity = vB − vA). No negation anywhere any more.
>
> The OVERLAP tests keep the opposite convention: the base `Space3D::isColliding(A, B, mtv)` answers an MTV that pushes
> A OUT of B (from B towards A). The physics step no longer calls it; the collision models' own MTV tests
> (`isCollidingWith()`) were removed in P3.a (2026-10-02) and the base MTV overloads are to be retired (owner, P2
> decision (i)); the boolean ones stay for lights, the editor and the octree.
>
> The former bug pattern (normals not negated before the old `ConstraintSolver`, fixed Mar 2026) is history: that
> solver and `CollisionDetection.cpp` were removed on 2026-10-01.
