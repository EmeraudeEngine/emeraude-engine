# Physics System

Context for developing the Emeraude Engine physics system.

## Module Overview

The Emeraude Engine physics system implements a 4-entity type architecture with differentiated collision handling to balance realism, performance, and game design.

## Physics-Specific Rules

### CRITICAL Coordinate Convention
- **Y-DOWN mandatory** in all physics calculations
- Gravity: `+9.81` on Y axis (pulls downward)
- Jump impulse: negative Y value (pushes upward)
- Forward thrust: negative Z value

### Entity Types (4 distinct types)
1. **Boundaries**: Game constraints (invisible walls)
2. **Ground**: Hybrid physical surfaces (stability + realism)
3. **StaticEntity**: Static objects with defined mass
4. **Nodes**: Full dynamic entities

### Physics Execution Order
1. Force integration → 2. Broad phase → 3. Narrow phase → 4. Inter-entity resolution → 5. Ground collision → 6. Boundary collision → 7. Ground resolution

## Development Commands

```bash
# Physics-specific tests
ctest -R Physics
./test --filter="*Physics*"
```

## Important Files

- `Manager.cpp/.hpp` - Main physics system manager
- `ConstraintSolver.cpp/.hpp` - Impulse-based constraint resolution
- `ContactManifold.cpp/.hpp` - Collision data structure
- `Collider.cpp/.hpp` - Collision detection
- `@docs/physics-system.md` - Detailed architecture
- `@docs/coordinate-system.md` - Y-down convention (CRITICAL)

## Development Patterns

### Adding a New Collision Type
1. Define the method in `Collider`
2. Create appropriate manifolds
3. Test with all 4 entity types
4. Verify Y-down coordinate consistency

### Modifying the Solver
1. Maintain 8 velocity iterations, 3 position iterations
2. Apply impulses according to entity type
3. Respect entity/ground separation
4. Preserve boundary separation (no manifolds)

## Critical Points

- **NEVER** convert Y coordinates
- Calculate `penetrationDepth` BEFORE hard clipping (ground)
- Mass matters for StaticEntity (no infinite mass)
- Two separate solver calls (entities then ground)
- **Scenes integration**: Scene graph Nodes inherit MovableTrait for physics
- **Spatial octree**: Scene owns Octree for physics broad-phase

## Detailed Documentation

For complete physics system architecture:
- @docs/physics-system.md - Detailed 4-entity architecture

Related systems:
- @docs/coordinate-system.md - Y-down convention (CRITICAL)
- @src/Scenes/AGENTS.md - Nodes with MovableTrait for physics
- @src/Libs/AGENTS.md - Math (Vector, Matrix, collision detection)
