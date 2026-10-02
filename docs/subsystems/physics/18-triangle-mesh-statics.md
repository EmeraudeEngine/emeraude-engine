## Static triangle meshes (physics overhaul P5, 2026-10-02)

A building, a staircase or a ramp collides as ITS TRIANGLES instead of a box (owner decisions 13, `docs/physics-overhaul.md`).

### Use
- Explicit, per entity: `Physics::TriangleMeshCollisionModel` (`src/Physics/TriangleMeshCollisionModel.hpp`) on an
  entity (`setCollisionModel()`). `TriangleMeshCollisionModel::fromShape(shape, twoSided, activeEdgeCosine)` builds it
  from a `VertexFactory::Shape` (nullptr when no triangle is valid). Existing scenes do not change: nothing converts a
  visual to it; a glTF / USD loader option comes later.
- `Toolkit::generateTriangleMeshInstance< StaticEntity >(name, shape, material, properties, twoSided)`: the instance of
  `generateRenderableInstance(shape)`, then the mesh model from the same shape, and `setCollidable(true)`.
- ONE-SIDED by default: only the front face collides (the winding normal); `fromShape()` reverses a triangle whose
  winding opposes its vertices' normals, so the front face is the one the author's normals point out of. Two-sided on
  request (a thin panel, an open surface).
- The triangles live in the entity's local space; its frame's scaling applies (a mirroring frame keeps the front face
  pointing out). The mesh is an emeraude-base `Space3D::TriangleMesh` (SAH hierarchy, active edges — base
  `docs/subsystems/source-tree/20-math-space3d-triangle-mesh.md`), shared between entities through a `shared_ptr`.
- Never solved as a body (no mass, no inertia): a MOVABLE entity carrying it is kinematic (`isSolvedBody()`).

### Where the step meets it (`Scenes/Scene.physics.cpp`)
- Pairs (step 2): a body ↔ mesh pair gives ONE MANIFOLD PER TRIANGLE met in the body's box (+ the speculative margin),
  each from `NarrowPhase::generateMeshTriangle()` — the base body ↔ triangle contacts, dropped when they push a body
  out of a one-sided triangle's back, then the internal-edge correction — with the triangle index + 1 as the sub key
  (warm starting). Two meshes never meet.
- The kinematic character: its sweep keeps the earliest front-face hit (`NarrowPhase::acceptMeshHit()`, the hit normal
  corrected on an inactive edge), its depenetration uses `capsuleMeshTriangleContacts()`.
- The continuous pass (step 4b): a mesh obstacle is swept triangle by triangle, front-face hits only.
- Its world box is thickened by 1 mm on each side (`BoundsPadding`): a flat mesh's box would be refused by
  `AACuboid::isValid()` (zero thickness).

### Measured (`collision-debug` ROW 7, z +26 to +38; Linux, 3 launches bit-identical, the 32 older stations unchanged)

| Station | Setup | Result |
|---|---|---|
| `BenchMeshSlider` / `BenchBoxSlider` | a 1 m box at 4 m/s across a floor of 288 coplanar triangles / across a box solid | the same stop (x −19.925 / −19.926); on the mesh y exactly 1.0 all along and ω ≤ 0.0017 rad/s, on the box y range 0.18 mm and ω ≤ 0.0026: the shared edges are not felt |
| `BenchWalkMeshRamp` | a walker at 1 m/s up a 30° mesh ramp | the trajectory of `BenchWalkRamp` on its box ramp (max y 3.097, the same x at every cycle, 20 m apart) |
| `BenchWalkMeshStairs` | a walker up six 0.25 m steps that are ONE mesh | climbs to 1.51 m |
| `BenchMeshBall` | a ball dropped from 5 m on a mesh slab | rests on its top (y 1.0) |
| `BenchMeshUpBall` | thrown up at 8 m/s under a ONE-sided panel facing up (y 3) | goes through it (apex 4.246, theory 4.26), lands on it (y 3.5) |
| `BenchMeshUpBallTwo` | the same under a TWO-sided panel | bounces off the underside (apex 2.499 = 3 − r), lands on the ground |
| `BenchMeshBullet` | a ball at 80 m/s (1.33 m per step) against a vertical two-sided mesh panel with no thickness | stopped at x 74.495 (the panel at 75 minus the radius), bounced |

### Traps
- Rolling back on the ground afterwards, `BenchMeshBullet` is kicked upward three times on the FLAT ground (not the
  mesh): item `ground-kicks-fast-rolling-ball`.
- A station's lane must be its own: a walker going on past its ramp climbed the stairs placed on the same line.
