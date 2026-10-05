## Critical Points

- **NEVER** convert Y coordinates
- Calculate `penetrationDepth` BEFORE hard clipping (ground)
- Mass matters for StaticEntity (no infinite mass)
- **4 correction passes** in priority order (Boundaries → Ground → StaticEntity → Node↔Node)
- **Scenes integration**: Scene graph Nodes inherit MovableTrait for physics
- **Spatial octree**: Scene owns Octree for physics broad-phase
- **Drag is integrated exactly** (`Physics::getDragVelocityFactor()`): never go back to an explicit `dv = k v² dt`, it
  diverges for light, fast bodies (`docs/caution-points.md`, triad 11)
- **Guard a speed or a depth before dividing by it**: base `Vector / s` follows IEEE 754 since 2026-10-05 (an exact 0
  gives ±inf or NaN; before, NaN for every `|s| <= epsilon`); guard `> FLT_MIN` and multiply by `1 / s`
- **Property setters refuse non-finite values** (NaN, ±inf) with a warning; the JSON reads Mass, Surface,
  DragCoefficient, AngularDragCoefficient, Bounciness, Stickiness (the inertia tensor is code-only)
