## Important Files

- `CollisionModelInterface.hpp` - Abstract interface for all collision models
- `PointCollisionModel.hpp/.cpp` - Zero-volume point collision
- `SphereCollisionModel.hpp/.cpp` - Sphere collision primitive
- `AABBCollisionModel.hpp/.cpp` - Axis-aligned bounding box
- `CapsuleCollisionModel.hpp/.cpp` - Swept sphere (capsule) collision
- `CollisionDetection.cpp` - Collision detection algorithms
- `ConstraintSolver.hpp/.cpp` - Sequential Impulse solver for collision resolution
- `ContactManifold.hpp/.cpp` - Collision contact data structure
- `MovableTrait.hpp/.cpp` - Movement physics trait with GroundedSource tracking
- `Particle.hpp/.cpp` - Physics particle with velocity, lifetime, and modifier integration
- `@docs/physics-system.md` - Detailed architecture
- `@docs/coordinate-system.md` - Y-UP convention (CRITICAL)
