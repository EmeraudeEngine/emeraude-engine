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
   active one — or by a MOVING kinematic one (a platform, a character, faster than 1 mm/s) — wakes; the rest of its
   island wakes at step 1c of the next step. A body ↔ TRIANGLE MESH pair gives one manifold per triangle met
   (one-sided, internal edges corrected — `18-triangle-mesh-statics.md`).
1c. **Islands wake as a whole** (P5, 2026-10-02): a body woken since the last step (a contact, a push, a force, a move)
   wakes every body that fell asleep with the same island key.
1b. **Ground recovery** (2026-10-02): a dynamic body whose centre is UNDER the ground — the height on the rendered
   triangle vertically under its centre (`NarrowPhase::heightOverGround()`) is negative: it crossed the surface within
   one step — is put back ON it before any contact: the bottom of its world box onto that surface, its approaching
   velocity bounced off the triangle's normal with the pair's restitution. The ground is the top of a solid.
3. **Ground**: `GroundLevelInterface::visitTriangles()` under every active body — its RENDERED triangles, one manifold
   per triangle (keys: creation number, `GroundKey`, triangle feature id + 1), through
   `NarrowPhase::generateGround()`: ONE-SIDED — a manifold whose normal would push the body down is dropped.
4. **Solve**: manifolds sorted by key, `Physics::SoftStepSolver::step()` (4 sub-steps: gravity, warm start, soft solve,
   integrate, relax; restitution 4 passes; impulses cached by feature id). Materials: friction = √(μA μB),
   restitution = max(eA, eB); the ground's default material is μ 1, e 0.
4b. **Continuous collision** (P5, 2026-10-02): a dynamic body that moved more than half its core size in the step
   (`NarrowPhase::coreRadius()`: a sphere's or a capsule's radius, a box's smallest half extent, 0 for a point) sweeps
   ITSELF along its motion (`NarrowPhase::sweepCore()`: a sphere, a capsule, a point's ray, a BOX by its faces, edges
   and corners — base `castBox()`, GJK, decision 14) against the ground triangles (one-sided: a triangle met from under
   it stops nothing), the static, kinematic and sleeping bodies, and the OTHER DYNAMIC BODIES at their end-of-step pose
   (translated; decision 14, Box2D v3's "bullet" method for every fast body — found through a one-axis sweep and prune
   along X, a total order). ONLY A CROSSING is stopped — the body's centre ending the step on the far side of the surface
   met: it is put back 5 mm short of the contact and its approaching velocity bounces off the hit normal with the pair's
   restitution — against a dynamic body the RELATIVE velocity, an impulse shared by the two masses (momentum kept) — so
   the next step's contact adds nothing. A body ending on the near side (sliding along a wall, or sinking less than its
   centre — up to its radius into a thin wall) is the contacts' business.
   ⚠️ **The sweep REPORTS the impact itself** (2026-10-07): mass × approach speed / step above 0.05 m/s, for the swept
   body and for a dynamic body it met — the contacts' formula and threshold, step 6. Since the next step's contact sees a
   body LEAVING, nothing else reports it: until then a fast body met its first obstacles silently and only "hit" once
   slow enough for a plain contact (projet-alpha's canon shell, 1000 m/s, exploded at the end of its bounces). Motion is
   unchanged by this: only `NodeCollision` is added.
5. **Write back** the dynamic bodies (velocities, `moveFromPhysics()`, `rotateFromPhysics()` with a WORLD axis), then
   the impacts (an approach above 0.05 m/s) are COLLECTED and the grounded state set from the manifolds (a contact
   within ~45° of gravity), then the **world boundaries**: the former clip + bounce, after the solver.
8. **Islands and sleep** (P5, decision 13): a union-find over the step's contacts links the awake dynamic bodies (the
   static world and the kinematic bodies do not link), rebuilt EVERY step (Jolt's `IslandBuilder`, Bullet's
   `btSimulationIslandManager`). A body is slow below 5 cm/s and 0.05 rad/s; an island whose bodies have ALL been slow
   for 30 steps (0.5 s) sleeps as a whole — velocities zeroed, `pauseSimulation(true)`, each body keyed by the island's
   lowest creation number (`MovableTrait::sleepIsland()`). On ANY support: a box on a box sleeps (before, a body slept
   only on the ground or the world floor, alone, `MovableTrait::checkSimulationInertia()` — removed). A sleeping body is
   solid and still: the solver treats it as infinite mass.
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
