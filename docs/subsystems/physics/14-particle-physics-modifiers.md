## Particle Physics & Modifiers

Particles (used by ParticlesEmitter) integrate with the scene modifier system.

**Particle simulation** (`Particle.cpp:updateSimulation()`):
1. Query scene modifiers for forces
2. Apply gravity from environment properties
3. Apply drag based on atmospheric density

**Modifier integration**:
```cpp
scene.forEachModifiers([this, &worldCoordinates, &particleProperties] (const auto & modifier) {
    const auto force = modifier.getForceAppliedTo(worldCoordinates, m_size * 0.5F);
    m_linearVelocity += force * particleProperties.inverseMass() * WorldPhysicsUpdateCycleDurationS<float>;
});
```

Key points:
- Particles pass `m_size * 0.5F` as radius (half diameter = bounding radius)
- If radius > 0, modifier creates a Sphere for influence testing
- If radius == 0, modifier uses point-based influence
- Force is integrated with inverse mass and timestep

See: `Particle.cpp:updateSimulation()`, `@src/Scenes/AGENTS.md` → "Modifier System"
