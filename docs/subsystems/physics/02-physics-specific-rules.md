## Physics-Specific Rules

### CRITICAL Coordinate Convention
- **Y-UP mandatory** in all physics calculations
- Gravity: a **VECTOR**, not a signed scalar — `EnvironmentPhysicalProperties::DownDirection{0,-1,0}` scaled by the surface gravity magnitude (`Physics::Gravity::Earth = 9.807`). Read the direction from the environment properties, never hard-code a Y sign
- Jump impulse: POSITIVE Y value (pushes upward)
- Forward thrust: negative Z value

### Entity Types (4 distinct types)
1. **Boundaries**: Game constraints (invisible walls)
2. **Ground**: Hybrid physical surfaces (stability + realism)
3. **StaticEntity**: Static objects with defined mass
4. **Nodes**: Full dynamic entities (via MovableTrait)

### The physics step (physics overhaul P2, 2026-10-01 — `Scene.physics.cpp:resolveCollisions()`)

⚠️ The former two phases (phase 1: each Node pushed out of boundaries / ground / statics and its velocity reflected,
with no torque and a per-tick friction multiplier; phase 2: Node ↔ Node through the old `ConstraintSolver`) are GONE.
Their defects and the measurements that replaced them: `docs/physics-overhaul.md` § 2 and § 6.

ONE pipeline, once per logic cycle, under the physics octree lock:
1. **Bodies**, every physics-octree element once, sorted by `AbstractEntity::creationNumber()`; index 0 is the static
   world (ground, static entities). A movable Node is DYNAMIC (asleep when paused); a non-movable Node is KINEMATIC
   (infinite mass, the velocity of its own motion between two cycles — what rests on it is carried).
2. **Pairs** from `OctreeSector::forEachSector()`'s pairing contract (owned × owned, owned × inherited, each pair once,
   NO dedup set), in canonical order (A = the lower creation number), AABB pre-filter, then
   `Physics::NarrowPhase::generate()` → a base `Space3D::ContactManifold` (normal A → B). A sleeping body touched by an
   active one wakes.
3. **Ground**: `GroundLevelInterface::visitTriangles()` under every active body — its RENDERED triangles, one manifold
   per triangle (keys: creation number, `GroundKey`, triangle feature id + 1), through
   `NarrowPhase::generateGround()`: the ground is ONE-SIDED and SOLID BELOW (2026-10-02). A body whose centre is over
   a triangle's plane gets the generators' contact, dropped if its normal would push the body down; a body whose
   centre is UNDER the plane (it crossed the surface in one step) gets its low points pushed back up along the face
   normal, each low point given to one triangle only.
4. **Solve**: manifolds sorted by key, `Physics::SoftStepSolver::step()` (4 sub-steps: gravity, warm start, soft solve,
   integrate, relax; restitution 4 passes; impulses cached by feature id). Materials: friction = √(μA μB),
   restitution = max(eA, eB); the ground's default material is μ 1, e 0.
5. **Write back** the dynamic bodies (velocities, `moveFromPhysics()`, `rotateFromPhysics()` with a WORLD axis), then
   the impacts (an approach above 0.05 m/s) are COLLECTED and the grounded state set from the manifolds (a contact
   within ~45° of gravity), then the **world boundaries**: the former clip + bounce, after the solver.
6. After the lock is released, `Scene::processLogics()` relocates the moved entities in the octrees, then EMITS the
   impacts (`MovableTrait::onCollision()` → `NodeCollision`) in manifold order. ⚠️ Never emit them inside the step: it
   holds `m_physicsOctreeAccess` (a plain `std::mutex`), and a handler that creates or removes an entity would take it
   again (fixed 2026-10-02; no handler did yet — they all defer).

A collidable body's gravity and motion happen in the step; `MovableTrait::updateSimulation(env, integratedByScene)`
only applies the drag to it (a body without a collision model still integrates itself).

**Determinism**: Linux 5 launches bit-identical on every cycle of every bench station (2026-10-01); the other OS:
`docs/physics-overhaul.md` § 6.

**Known P2 limits** (P3: oriented boxes): an AABB model collides as its WORLD envelope; a box with rotation physics
that tilts gets no restoring torque (a box dropped flat rests tilted 5°, one dropped on an edge stays on it).
