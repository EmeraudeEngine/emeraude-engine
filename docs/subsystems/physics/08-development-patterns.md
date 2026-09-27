## Development Patterns

### Adding a New Collision Type
1. Define the method in `Collider`
2. Create appropriate manifolds
3. Test with all 4 entity types
4. Verify Y-UP coordinate consistency (gravity is -Y; a ground contact normal points DOWN)

### Modifying the Solver
1. Maintain 8 velocity iterations, 3 position iterations
2. Apply impulses according to entity type
3. Respect entity/ground separation
4. Preserve boundary separation (no manifolds)
