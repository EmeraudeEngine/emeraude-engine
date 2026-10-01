## Important Files

- `CollisionModelInterface.hpp` - Abstract interface for all collision models
- `PointCollisionModel.hpp/.cpp` - Zero-volume point collision
- `SphereCollisionModel.hpp/.cpp` - Sphere collision primitive
- `AABBCollisionModel.hpp/.cpp` - Axis-aligned bounding box
- `CapsuleCollisionModel.hpp/.cpp` - Swept sphere (capsule) collision
- `NarrowPhase.hpp/.cpp` - Collision model → base primitive → base contact manifold (speculative margin)
- `SoftStepSolver.hpp/.cpp` - The contact solver of the physics step (Box2D v3 soft step, persistent impulses)
- `../Scenes/Scene.physics.cpp` - The physics step itself (bodies, pairs, ground, solve, write-back, boundaries)
- `MovableTrait.hpp/.cpp` - Movement physics trait with GroundedSource tracking
- `Particle.hpp/.cpp` - Physics particle with velocity, lifetime, and modifier integration
- `@docs/physics-system.md` - Detailed architecture
- `@docs/coordinate-system.md` - Y-UP convention (CRITICAL)
