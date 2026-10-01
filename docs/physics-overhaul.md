# Physics overhaul (opened 2026-10-01)

The owner ordered a complete review of the physics: collisions, rotation by inertia, a "decent physical model", and
a special state for a body that walks on the ground or on another object, so that walking does not behave like a raw
collision bounce. This file holds the analysis, the owner decisions and the phase plan. The open work is one item per
idea in the `docs/todo/` of the repository that must change (ids in § 4).

## 1. Owner decisions (2026-10-01)

1. **In-house solver**, rebuilt on the Catto / Box2D v3 model. Integrating Jolt Physics (MIT) was rejected: the engine
   keeps its own physics. Jolt, Box2D (MIT) and Bullet (zlib) may still be READ as references and cited.
2. **Walking = a kinematic character** (collide and slide), not a dynamic body with locked rotation. The ETHEREAL
   player (free fly, no collision response) keeps behaving as it does today; choosing between the two is a projet-alpha
   option, not an engine mode of the controller.
3. **Floating point**: no `-ffast-math` for now; the owner may want to enable it one day. Consequences: the physics must
   not depend on bit-exact arithmetic to be CORRECT (no equality tests on floats, no reliance on a specific rounding),
   and the reproducibility target is the same machine + the same binary + the same inputs. Bit-exact results across
   the three OS are not a goal.
4. **Order**: P0 → P1 → P2 → P3 → P4, then P5 (§ 4). Solid contacts first, the walker on top of them.
5. **P1 design (owner, 2026-10-01)**: (a) contacts live in a new header-only `Math/Space3D/Contacts/` folder of
   emeraude-base — a manifold type (one normal, up to 4 points, each with its depth and a stable feature id) and one
   function per pair; `isColliding()` stays the cheap overlap test. (b) A new `Space3D::OrientedBox` primitive
   (centre, 3 axes, half extents); `Math/OrientedCuboid` stays for its current uses. (c) The manifold normal points
   FROM A TO B (Box2D, Bullet, Jolt, and the engine solver); `isColliding()` keeps its MTV ("push A out of B").
   (d) Order: box ↔ box, sphere ↔ box, capsule ↔ box, sphere / capsule ↔ triangle, the sphere / capsule pairs; then
   the casts, the defects, the inertia helpers — each with its unit tests (Release + ASan/UBSan).
6. **Branch**: every change of the overhaul goes to the `physics_overhaul` branch of EACH repository (projet-alpha,
   emeraude-engine, emeraude-base), created on 2026-10-01 from `main` / `develop` / `develop`.

## 2. Analysis of the physics as it stood on 2026-10-01

Read from the code (engine `develop` at `5c2b2c40`, after the triad engine pass). Nothing below was
measured unless stated; P0 measures it.

### A. Two collision paths
- Movable ↔ movable contacts go through the impulse solver (`Physics/ConstraintSolver.cpp`), with torque and friction.
- Movable ↔ ground, boundary and `StaticEntity` contacts do NOT: `Scene::resolveCollisions()` phase 1 sums position
  corrections, then `applyCollisionResponse()` (`Scenes/Scene.physics.cpp`) reflects the linear velocity along one
  dominant normal. No torque (a box can never tip over on the floor; rotation can only come from a movable ↔ movable
  contact), no Coulomb friction.
- Grounded friction is a per-tick multiplier, `v.xz *= 1 − stickiness` (`MovableTrait::updateSimulation()`): at 0.7 a
  body loses 70 % of its horizontal speed per tick, and the result depends on the tick rate.

### B. Narrow phase
- Box ↔ box runs on the WORLD AABB of each box (`AABBCollisionModel::collideWithAABB()` → `toWorldAABB()`): a rotated
  box collides as its axis-aligned envelope. Every pair yields ONE contact point, no manifold. A box resting on one
  point rocks and tips at random — consistent with `physics-run-to-run-determinism` (resting x from 2.66 to 18.87 m).
- emeraude-base has a 15-axis SAT between two oriented boxes (`Math/OrientedCuboid.hpp` `isIntersecting()`), unused by
  the engine and covered by one extents-only test.
- Capsule ↔ ground does nothing: the `Capsule` case of `Scene::accumulateGroundCorrection()` is empty.
- Base pair tests return one MTV, no contact point, no feature id. Capsule ↔ AABB and capsule ↔ triangle use a fixed
  4-iteration alternating projection (approximate), and their deep cases are wrong (`CapsuleCuboid.hpp:166-215`,
  `CapsuleTriangle.hpp:215-219`). Tri ↔ tri seems to return its MTV in the opposite direction to every other pair
  (`SAT.hpp:~221-230`, `SamePrimitive.hpp:96`) — to be confirmed by a unit test.
- No sweep, no time of impact, and the ray tests return neither a distance nor a normal (`Space3D/Intersections/`).
  `Line` is infinite: `LineSphere` / `LineCuboid` can answer a hit behind the origin.

### C. Solver (`physics-solver-restitution-and-position-correction` has the detail)
- Restitution re-applied on each of the 8 velocity iterations; position correction applied 3× with a stale depth, on
  top of the Baumgarte bias; no warm starting (manifolds rebuilt every tick, accumulated impulses lost); `onCollision`
  up to 8× per contact per tick; Gauss-Seidel order taken from the octree traversal.

### D. Integration and rotation
- Gravity is skipped while grounded on terrain or a boundary, and the downward velocity is zeroed; "grounded" lasts
  15 ticks (250 ms) after the contact is lost.
- Grounded on an entity, gravity stays on with no clamp: the body re-penetrates and is pushed out every tick (the
  micro-bounce on top of objects). Standing on a MOVABLE body never grounds (`ConstraintSolver.cpp`).
- The inertia tensor defaults to the identity, whatever the mass and shape; nothing derives it from the shape.
- Angular drag is `ω *= 1 − drag` per tick (tick-rate dependent).
- `Node::rotateFromPhysics()` applies a WORLD axis in `TransformSpace::Local` (`Scenes/Node.hpp:930`), while the
  solver computes `ω` in world space: an already rotated body spins around the wrong axis. PROVEN by the bench
  (`BenchSpinner`, § 6).

### E. Terrain
- The ground correction is applied along the slope normal with the VERTICAL depth as length
  (`accumulateGroundCorrection()`), which pushes the body sideways downhill every tick. Hypothesis for
  `physics-no-rest-on-generated-terrain`, not measured.
- An AABB tests its 4 bottom corners only; a sphere tests the height under its centre only.

### F. Walking
- Player: feet-anchored axis-aligned box, walking writes the velocity axis by axis (`setMinimalVelocity()`), the jump is
  a force ramped over 16 ticks, the step-up teleports the whole rise in one tick (`physics-step-up-pass`).
- Paladin and Fox walk with `addForce()`; the Paladin's bounciness is 0.8.
- No slope limit (only a hard-coded 0.7 normal threshold), no ground snapping, no platform carry, no walking state.

## 3. State of the art the plan follows

- **Solver**: sequential impulses with accumulated impulses, warm starting and persistent contacts — E. Catto,
  "Iterative Dynamics with Temporal Coherence" (GDC 2005); "soft step" with sub-stepping and relaxation — E. Catto,
  "Solver2D" (2024) and Box2D v3 `b2SolverStage` (MIT); restitution from the pre-solve normal velocity with a speed
  threshold (Box2D `b2ContactSolver`). Sub-stepping rationale: M. Macklin et al., "Small Steps in Physics Simulation"
  (SCA 2019).
- **Contacts**: SAT + reference-face clipping, 1-4 points with feature ids for warm starting — D. Gregorius, "The
  Separating Axis Test between Convex Polyhedra" (GDC 2013) and "Robust Contact Creation for Physics Simulations"
  (GDC 2015); C. Ericson, *Real-Time Collision Detection* (2005), ch. 4-5 for closest points and OBB tests.
- **Walking**: kinematic capsule moved by collide and slide with shape sweeps — K. Fauerby, "Improved Collision
  Detection and Response" (2003); the PhysX Character Controller guide; Jolt `CharacterVirtual` (MIT); Godot
  `move_and_slide()`. Common features: grounded / airborne states, slope limit, step-up and step-down by sweep,
  ground snapping, inheriting the support's velocity, pushing dynamic bodies with a bounded force.

## 4. Phases and items

Each phase is measured on projet-alpha's `collision-debug` stations (P0), then validated on macOS and Windows.

| Phase | Repository | Items (`docs/todo/<id>.md`) |
|---|---|---|
| P0 bench | projet-alpha | `physics-collision-debug-bench` |
| P1 foundation | emeraude-base | `contact-manifold-generation`, `shape-casts-with-hit-normal`, `collision-pair-test-defects`, `rigid-body-math-helpers` |
| P2 solver | engine | `physics-unified-contact-pipeline` (absorbs `physics-solver-restitution-and-position-correction`; expected to close `physics-run-to-run-determinism`, `physics-no-rest-on-generated-terrain`, probably `physics-nan-linear-velocities`) |
| P3 rotation | engine | `physics-oriented-box-collision-model`, `rotational-physics` |
| P4 walking | engine, then projet-alpha | `kinematic-character-controller` (supersedes `physics-step-up-pass`), projet-alpha `actors-kinematic-character-migration` |
| P5 later | engine | `physics-continuous-collision`, `physics-triangle-mesh-static-shapes`, `physics-simulation-islands` |

## 5. What must survive the overhaul

- The 4 entity kinds stay as AUTHORING concepts (boundary, ground, static entity, node). P2 changes how their contacts
  are solved (one pipeline, a static = inverse mass 0), not what an author declares. Boundaries keep their hard clip
  as a final safety, outside the solver.
- The octree broad phase and its pairing contract (`src/Scenes/AGENTS.md` § Octree storage and traversal) stay.
- The actor-facing API (`addForce`, `setLinearVelocity`, `isGrounded()`, `NodeCollision` with the impact force,
  `setStepHeight()`, free fly) is migrated, not broken silently: every removal is listed in the item that removes it.

## 6. Bench (P0) and the Linux baseline

### The stations

projet-alpha's `collision-debug` (`src/Builtin/CollisionDebug.cpp`, `buildPhysicsBench()`), sampled by projet-alpha
`tools/physics-bench.py` through `Core.SceneManagerService.getNodePhysics()` (extended for the bench with the world
orientation, the angular velocity, `simulationPaused` and `sceneCycle`). Samples are labelled by physics cycle; the
client reads ~7 samples per second per station (15 stations), enough for a bounce apex (≈ 3 cm), coarse for jitter.

| Station | Setup | Expected |
|---|---|---|
| `BenchBallHalf`, `BenchBallFull` | 0.5 m ball, bottom 10 m above the ground, no drag, e = 0.5 / 1.0 | first apex e² · 10 m: 2.5 m / 10 m |
| `BenchBoxFlat` | 1 m box (10 kg, cube inertia) dropped flat from 5 m | rests flat, upward (0, 1, 0) |
| `BenchBoxEdge` | the same, rolled 45° about Z | falls onto a face and rests flat |
| `BenchStackA`…`E` | 5 boxes released 1 cm apart | settles, Y amplitude < 1 mm over 5 s |
| `BenchSpinner` | free fly, pre-rotated 30° yaw + 30° pitch, ω = (0, 2, 0) WORLD | upward Y and backward Y constant |
| `BenchSlopeBall`, `BenchSlopeBox` | above a 6 × 0.5 × 6 static slab rolled 30° | roll / slide down the slope |
| `BenchStepA`…`H` | 8 steps, 0.29 m rise, 0.35 m tread | the walker climbs (P4) |
| `BenchPlatform`, `BenchPlatformBox` | massless non-movable node animated 10 → 18 → 10 m in X (6 s), a box dropped on it | the box rides the platform |
| `DynTopCube` | the pre-bench tipping test (unchanged) | rests on a face; same place every launch |
| `BenchTipBase`, `BenchTipCube` | `DynTopCube`'s twin, properties on the component, shapes overridden | the same place on every launch and every machine |

⚠️ Bench traps (all measured on 2026-10-01):
- The toolkit appends a process-wide counter to every name (`String::incrementalLabel()`): `BenchBallHalf` is
  `BenchBallHalf11`. No base name ends with a digit; the script maps names through `listEntities()`.
- A World-space rotation also turns the POSITION about the world origin: the stations rotate in Local space.
- **Physical properties set on the ENTITY are overwritten**: the entity re-derives them from its components on every
  component update, including asynchronously when the geometry finishes loading. Set them on the component
  (`docs/caution-points.md` § Physics). `DynTopCube` still sets them on the entity: its declared bounciness,
  stickiness and inertia are therefore NOT the ones simulated (kept as is for comparison with the earlier readings).

### The per-cycle recorder (2026-10-01)

`Scenes::PhysicsRecorder` (`src/Scenes/PhysicsRecorder.hpp`), owned by each `Scene`, sampled by `Scene::processLogics()`
right after `resolveCollisions()`: the end-of-cycle state of chosen root nodes on EVERY cycle of `[firstCycle,
firstCycle + count)`. Console / MCP (`Core.SceneManagerService`):

| Command | Does |
|---|---|
| `recordNodePhysics("A,B,C", firstCycle, cycleCount)` | arms a recording (1-64 nodes, 1-36000 cycles); the names are ONE quoted argument (the console splits unquoted commas) |
| `getPhysicsRecordingStatus()` | `{"state": Idle / Waiting / Recording / Complete, firstCycle, cycleCount, recordedCycles, nodes}` |
| `stopPhysicsRecording()` | stops; what is recorded stays savable |
| `savePhysicsRecording()` | writes `captures/physics-recording-<unix s>.json` (the `getNodePhysics()` keys per sample, by node) and releases it |

- The logic thread never allocates (start() reserves nodes × cycles) and never writes the file (save() does, on the
  console thread, outside the lock). An idle recorder costs one atomic load per cycle (`m_armed`).
- A name absent when the recording begins is recorded with `"found": false`.
- `tools/physics-bench.py` uses it by default (`--first-cycle 60`, i.e. from 1 s); `--poll` keeps the P0 method.
- **First measurement (Linux, RTX 3070 Ti, 5 launches × 1800 cycles × 17 stations)**: the 5 runs are BIT-IDENTICAL on
  every cycle of every station (0 differing samples, `DynTopCube`, the stack and the twin included). The isolated
  differences seen by polling were the one-cycle labelling offset — proven. Linux reproduces exactly; the divergence
  on Windows and macOS is now measurable to the cycle (next: the peers' recorded runs).

### Baseline — Linux, RTX 3070 Ti, engine `5c2b2c40` + the bench, 5 launches × 30 s of physics

| Station | Measured (5 runs) | Verdict |
|---|---|---|
| Balls vs ground | e_eff 0.493-0.494 for e = 0.5; 0.997-0.998 for e = 1.0; 0 NaN | ✅ ground restitution is right |
| `BenchBoxFlat` | rests at Y 0.500, upward 0° | ✅ |
| `BenchBoxEdge` | rests ON ITS EDGE, Y 0.707, upward 45°, ω 0 — 5/5 | ❌ a ground contact makes no torque (§ 2 A) |
| Stack | never settles: B…E keep a downward velocity of 0.2-1.6 m/s while "resting", Y amplitude 0.16-0.33 m over the last 5 s | ❌ movable ↔ movable solver (§ 2 C) |
| `BenchSpinner` | upward vector fixed at (0.250, 0.866, 0.433); backward Y swings −0.500 … +0.500 | ❌ PROVEN: it spins about its LOCAL up axis — `Node::rotateFromPhysics()` applies the world axis in local space |
| Slope | ball and box rest at Y 4.217, ω 0, on the slab's axis-aligned envelope | ❌ rotated boxes collide as their world AABB (§ 2 B) |
| Platform | the box is not carried: it ends on the ground at X 7.88, tilted 79° (platform 10 … 18 m) | ❌ an animated body has no velocity for the contacts (P2 / P4) |
| `DynTopCube` | X 6.855, upward 140.9°, 5/5 identical | the same value as the two earlier Linux runs |

### Baseline — macOS, Apple M2 (MoltenVK), the same commits, 5 launches × 30 s (peer, 2026-10-01)

Every station gives the Linux verdict within sampling noise (e_eff 0.494-0.496 and 0.997-0.998; the edge box on its
edge 5/5; the stack never settles, tail Y amplitude 0.17-0.32 m, different on every run; the spinner about its local
axis; the slope bodies at Y 4.217; the platform box not carried, X 7.881, tilted 78.7°), 0 NaN — EXCEPT `DynTopCube`,
which lands in a different place on every launch: X 12.597 / 14.966 / 4.714 / 10.862 / 5.530, upward 62.9° / 23.5° /
5.4° / 62.7° / 54.6°, Y 1.09-1.40 (not resting on a face at 30 s). Linux gives X 6.855 and 140.9° on 5/5.

### Baseline — Windows, RTX 3060 Laptop and the AMD iGPU (forced), the same commits, 5 + 5 launches (peer, 2026-10-01)

Build clean under MSVC /WX. Every station gives the Linux verdict on both GPUs (e_eff 0.493-0.496 and 0.997-0.998; the
edge box on its edge; the stack never settles, tail Y amplitude 0.15-0.39 m, StackA sometimes ending at Y 0.2; the
spinner about its local axis; the slope bodies stopped dead at Y 4.217; the platform box not carried), 0 NaN.
`DynTopCube`: on the AMD iGPU X 6.855, 140.91° on 5/5 — the Linux value; on the RTX 3060 X 10.217 / 18.230 / 18.072 /
2.663 / 18.230 (runs 2 and 5 bit-identical), so a few discrete outcomes rather than noise.

**Hypothesis (not measured yet)**: `DynTopCube` sets its body properties on the ENTITY and its AABB model is NOT marked
overridden, so the asynchronous end of its geometry load re-derives both (mass from volume × density, bounciness and
stickiness 0.5, the identity inertia; a merged shape) at a physics cycle that depends on the machine's loading speed.
Test: the twin station `BenchTipBase` / `BenchTipCube` (row 3, Z = -60), declared the bench way (component properties,
overridden shapes). If the twin repeats on every machine while `DynTopCube` does not, the hypothesis holds.

### ⚠️ Comparing runs: by physics cycle, never by final state

A body that never comes to rest (the stack, `BenchTipCube`) is sampled at a different cycle at the end of each run, so
its final states differ while the simulations are identical. Compared AT THE SAME CYCLES (`tools/physics-bench.py
--compare <dir>`), the 5 Linux runs are identical on every station (`DynTopCube`, the twin, the stack), except 1-2
isolated samples per run that are equal again on the next common cycle. Each isolated gap is ONE CYCLE of the body's
motion (the platform: 0.0427 m against 2.67 m/s ÷ 60 = 0.044 m): `getNodePhysics()` reads the position and `sceneCycle`
from the console thread while the logic thread ticks, so a sample can be labelled one cycle off. A per-cycle recorder on
the logic thread would remove it (item `physics-collision-debug-bench`). Linux reproduces cycle for cycle. The earlier reading "the stack
differs run to run on Linux" (final states) was wrong and is withdrawn; the macOS stack reading is the same kind and
proves nothing. `DynTopCube` on the RTX 3060 / M2 lands metres apart and RESTS (Y ≈ 1.1-1.4 m): a real divergence.

**The twin** (Linux, 5 runs): `BenchTipCube` does NOT behave like `DynTopCube` — it stays on top of its base, tilted
57-62°, and never rests (ω ≈ 1.47 rad/s and 0.2-0.4 m of motion over the last 5 s of 30 s): a LIMIT CYCLE, the solver
feeding energy into a body tipping on its axis-aligned envelope. `DynTopCube` falls off and rests at X 6.855: the
difference is the re-derived properties (§ Bench traps). Whether the twin repeats on the RTX 3060 / M2 is the test.

### The peers compared by cycle (2026-10-01, after the twin and `--compare`)

⚠️ Method limit (Windows peer): the client samples each station about every 9 cycles with a phase that differs per
launch, so two runs share few cycles (often 0-45 of ~200 samples); a "no divergence" on few common cycles proves
little. A per-cycle recorder on the logic thread would remove both this limit and the one-cycle labelling offset.

| Machine | `DynTopCube` | Stack (`BenchStack*`) | Twin (`BenchTipCube`) |
|---|---|---|---|
| Linux, RTX 3070 Ti | identical | identical (39-82 common cycles per pair) | identical |
| Windows, AMD iGPU (P0 runs) | identical (134 common cycles) | DIVERGES from cycle ~117-132 | (not in the P0 runs) |
| Windows, RTX 3060 | DIVERGES on 3/4 pairs (from cycle 408-1769, 4-12 m apart) | DIVERGES on every pair (from cycle 68-1783) | no divergence in the few common cycles (7-44); final angle 63-76°, but the twin never rests, so final states cannot decide: NOT proven bit-repeatable |
| macOS, M2 | DIVERGES on 4/4 (from cycle 239-932) | DIVERGES on 3/4, once from the first contact (cycle ~10) | isolated samples only, the same final position 5/5 |

**Working hypothesis (not proven)**: the ORDER in which asynchronous events complete — the geometry loads — changes
(1) `DynTopCube`'s re-derived body properties (§ Bench traps) and (2) the order the entities enter the physics octree,
hence the order of the contact pairs the Gauss-Seidel solver visits. The twin is a single pair: the order cannot
matter to it, and it is the most stable one — much closer than `DynTopCube`, not proven bit-repeatable on the RTX 3060
(only the per-cycle recorder can settle it). The stack is many pairs: it diverges wherever the load order varies. Linux
(the fastest machine) would complete the loads in the same order every time. P2's sort of the manifolds by stable ids
(`physics-unified-contact-pipeline`) addresses (2); declaring the body properties on the component addresses (1).

Raw runs: kept outside the repository (one JSON per run, ~1.6 MB); re-run with the command in the script's header.
