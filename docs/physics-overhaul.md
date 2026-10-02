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
6. **P2 design (owner, 2026-10-01, the recommendations)**:
   (a) ONE pipeline: ground, static entities and movables all become manifolds for one solver; the world boundaries
   keep their hard clip, after the solver. (b) In P2 the collision models map to the base primitives as they are:
   AABB → an axis-aligned `OrientedBox` (turning it with the entity is P3), sphere → `Sphere`, capsule → `Capsule`,
   point → a zero-radius sphere. (c) The ground answers its real TRIANGLES under a box (a new `GroundLevelInterface`
   query), contacts against them are exact, and the P4 walker reuses the same casts. (d) A stable CREATION NUMBER
   (uint64) per entity, given by the scene, sorts the pairs and keys the persistent manifolds. (e) The solver is Box2D
   v3's soft step: sub-steps, warm starting, restitution once from the pre-solve speed, soft contacts replacing the
   Baumgarte bias and the position pass. (f) Materials combine as Box2D: friction = geometric mean, restitution = max.
   (g) The walkers may regress on the branch between P2 and P4 — no compatibility workaround. (h) `DynTopCube` moves
   its properties to the component in P2. (i) The old `Collisions/` MTV overlap tests are RETIRED once the collision
   models use the contacts; only their boolean defects are fixed. ⚠️ REVISED by the owner on 2026-10-02: the
   emeraude-base MTV overloads are KEPT ("they may serve in other cases"); their known defects stay open in base
   `collision-pair-test-defects`. Only the engine's model-level wrappers (`isCollidingWith()`, removed in P3.a) and the
   dead `MovableTrait` API stay removed (owner, same day).
8. **P3 design (owner, 2026-10-02, the recommendations)**: (a) `AABBCollisionModel` becomes ORIENTED — its local box
   follows the entity's rotation (`OrientedBox::fromCuboid(localBox, frame)`), the octree keeps the world AABB — and
   is renamed `BoxCollisionModel` / `CollisionModelType::Box`; no second box model. (b) The centre of mass is the
   shape's CENTROID (Box2D v3 `localCenter`): the solver integrates the centre of mass and the write-back moves the
   origin to `COM − R · offset`. (c) The inertia tensor is DERIVED from the shape and the mass (`RigidBody::solid*Inertia`)
   unless the author set one explicitly (`BodyPhysicalProperties` remembers it); a point model does not rotate; the
   JSON stays code-only (`InertiaKey` is not read). (d) Rotation is ON by default for every dynamic body; actors and
   the player keep it off (their final state: the P4 controller never takes its rotation from the physics).
   (e) The angular drag keeps today's feel at the reference rate, integrated exactly: `ω *= (1 − c)^(dt · rate)`.
9. **P4 design (owner, 2026-10-02, the recommendations)**: (1) a `CharacterController` COMPONENT on a `Node`, the
   node made kinematic (the P2 kinematic path: its velocity from its motion, a solid for the others, carried by what
   it stands on); (2) a capsule, feet at the node origin; (3) it moves in the physics step, BEFORE the solver, on the
   logic thread; (4) it pushes dynamic bodies with a bounded force and is not pushed back (except by what carries
   it); (5) the jump is a launch velocity; (6) per-character parameters on the component (step height, walkable
   slope, ground snap distance, air control, push force), no JSON key yet; (7) order: the engine controller and its
   `collision-debug` station (flat, 30° and 50° slopes, 0.29 m steps, moving platform, standing on a box, no
   vertical jitter), then projet-alpha's Player, Paladin, Fox, Drone, then the teleport step-up removed; first
   acceptance: citadel's stairs. A WHEELED VEHICLE (a dynamic chassis, wheels by casts: suspension and tyre friction)
   is a BONUS phase after the overhaul (owner: "on fera le véhicule en bonus").
10. **Order revised (owner, 2026-10-02)**: P5 BEFORE P4 ("on inverse P4 et P5"), starting with the CONTINUOUS collision
   of fast bodies — the owner's own idea: use the segment from a body's old to its new position to find what it would
   cross. A swept cast (P1's casts) of the body's round core against the static world, the body put back at the time
   of impact; the ground recovery (step 1b) stays as the safety net.
11. **Order (owner, 2026-10-02, after P5's continuous collision)**: P4 now (the kinematic character), then the two
   remaining P5 items (`physics-triangle-mesh-static-shapes`, `physics-simulation-islands`).
12. **P4 migration (owner, 2026-10-02, the recommendations)**: (1) the physical Player takes the CharacterController
   (a capsule of its size, its speed as the wanted velocity); the ethereal one is unchanged; (2) its jump becomes a
   launch speed giving TODAY's height, √(2 g h), Shift keeping its ×2 on the height — the owner tunes the feel by
   playing; (3) fall damage from the Landed event's fall speed, the thresholds converted to keep today's damage;
   (4) Paladin and Fox: walk / run speeds instead of forces (Paladin 1.5 / 4.5 m/s, Fox 1.2 / 5 m/s, to tune on the
   animations), their animation from the controller's real velocity; (5) the Drone stays a dynamic body (a flying
   machine); (6) order: the Player on citadel's stairs, the Paladin, the Fox, the dead teleport step-up removed, the
   walking demos checked.
13. **P5's last two items (owner, 2026-10-02, the recommendations; "Ok pour les derniers points")**: (1) ISLANDS are
   rebuilt every step by a union-find over the step's contacts (Jolt's `IslandBuilder`, Bullet's
   `btSimulationIslandManager`), not Box2D v3's persistent islands; an island sleeps when all its bodies have been slow
   (< 5 cm/s) for 0.5 s, on any support; it wakes as a whole when an active body touches it, a push or a force reaches
   it; the static world and the kinematic bodies do not link islands. (2) Static TRIANGLE MESHES are ONE-SIDED by
   default (the front face collides: glTF winding, normals out — like the ground, PhysX and Jolt by default), with a
   two-sided option for open geometry (a thin panel, a `doubleSided` material). (3) The mesh is an EXPLICIT
   `TriangleMeshCollisionModel` built from a geometry (the visual's or a simplified one), per entity, with a Toolkit
   option — existing scenes do not change; a glTF / USD loader option comes after. Order: islands, then meshes; the
   BVH in emeraude-base (a binary SAH tree built once, Wald 2007); internal edges by edge flags computed at build time
   (Jolt's `MeshShape` "active edges").
14. **The end of continuous collision (owner, 2026-10-02, the recommendations)**: (1) DYNAMIC ↔ dynamic: every fast
   body is swept against the other dynamic bodies at their end-of-step pose too (no "bullet" flag, as decision 10);
   (2) a BOX sweeps its own shape: GJK + conservative advancement in emeraude-base (`castBox()`), not its inscribed
   sphere.
15. **The wheeled vehicle (owner, 2026-10-02; bonus phase)**: (1) a wheel finds the ground by a SPHERE CAST of its radius
   along its suspension (not a ray: it neither drops into a slot nor hits a kerb edge); (2) the tyre: SLIP CURVES
   (longitudinal friction from the slip ratio, lateral from the slip angle, simple tunable curves, combined in a friction
   circle — Jolt's model), not Pacejka; (3) a COMPLETE drive train from the start (an engine torque curve, an automatic
   gearbox with ratios and a clutch, differentials — Jolt's `WheeledVehicleController`); (4) the wheels act IN THE SOLVER
   (each wheel on the ground a soft constraint solved with the contacts: the suspension a soft spring along the contact
   normal, the friction bounded by the load), not as forces before it. Design: `subsystems/physics/19-wheeled-vehicle.md`.
7. **Branch**: every change of the overhaul goes to the `physics_overhaul` branch of EACH repository (projet-alpha,
   emeraude-engine, emeraude-base), created on 2026-10-01 from `main` / `develop` / `develop`.

## 1b. P2 progress

- **P2.a (infrastructure, 2026-10-01)**: `AbstractEntity::creationNumber()` — 0 for the root, then 1, 2, … in creation
  order, drawn from `Scene::allocateEntityNumber()` in the constructor (deterministic: entities are built in a fixed
  order; the octree insertion order is not). `GroundLevelInterface::visitTriangles(region, visitor)` with a
  `GroundTriangleVisitor` (no allocation): `BasicGroundResource` and `TerrainResource` (full-resolution grid, not the
  CDLOD levels) answer their rendered triangles through `Scenes/GroundTriangles.hpp` and the base
  `Grid::forEachTriangleInRegion()`; feature id = (cellZ × cells + cellX) × 2 + half.

- **P2.b/c (the pipeline and the solver, 2026-10-01)**: `Physics::NarrowPhase` (model → base primitive, speculative
  margin 2 cm), `Physics::SoftStepSolver` (Box2D v3 soft step, 4 sub-steps, 30 Hz / ζ 10 soft contacts, pushout ≤ 3 m/s,
  restitution from the pre-solve speed above 1 m/s in 4 passes, Coulomb disc friction, impulses cached by feature id),
  `Scene::resolveCollisions()` rewritten (pipeline: `docs/subsystems/physics/02-physics-specific-rules.md`), the old
  `ConstraintSolver` / `ContactManifold` / `ContactPoint` / `CollisionDetection` removed, `Node::rotateFromPhysics()`
  takes a world axis. Bench, Linux, 5 launches × 1800 cycles:

  | Station | P0 baseline | P2 |
  |---|---|---|
  | balls e 0.5 / 1.0 | 0.494 / 0.998 (static path) | 0.491 / 0.998 (solver) |
  | 5-box stack | never settles, 0.16-0.33 m over 5 s | settles: Y 4.515 m at the top, residual 1.5 mm/s and 0.3° (envelope, P3) |
  | box on an edge | stays at 45° | 44.4° (the envelope stays flat-bottomed: P3) |
  | box dropped flat | flat | tilts 5.2° (sequential solve, nothing restores it: P3) |
  | spinner (ω world Y) | turns about its LOCAL axis | upward Y and backward Y constant — world axis ✔ |
  | twin `BenchTipCube` | limit cycle on its base | falls off, rests on its side |
  | platform (kinematic) | box not carried | carried 10 → 14 m and back ✔ |
  | run to run | Linux identical | Linux identical (0 differing samples) |

  Measured on the way: (1) a symmetric sweep (relax pass backward) broke the solve / relax coherence (the stack
  toppled) — reverted; (2) `Quaternion::toAngleAxis()` answered 0 for a small rotation (`2 acos(w)` in float): the
  bodies never turned by less than ~1e-3 rad/s while the solver's ω grew — fixed in emeraude-base with `2 atan2(|v|,
  w)` (base caution points § Math); (3) `onCollision()` fired for every resting contact (1001 per cycle in
  balls-of-steel) — an impact now needs 0.05 m/s; (4) "pure virtual method called" at shutdown, 2 of 16
  balls-of-steel runs: projet-alpha's `Stage::unloadActiveAct()` erased the act (its actors observe the nodes) while
  the logic thread could still tick the scene — the scene is now disabled first (0 of 8 since).
  Demos checked (no NaN, no error, clean exit): balls-of-steel, physics-debug, lighten-marbles, game-logic, collision,
  citadel, animation-debug. The walkers' feel is not checked (keyboard; may regress until P4, owner).
- **P2 on macOS M2 (peer, 5 recorded launches)**: clean build (clang, 0 warning), base 2272 / 0 failed; **0 differing
  samples on 16 of 17 stations** — the stack is now bit-identical on the M2 (it diverged in P0 / P1). The 17th,
  `DynTopCube`, was the same trajectory SHIFTED IN TIME (released at cycle 40 / 50 / 52 depending on the launch): its
  shape was not overridden, so it stayed out of the physics octree until its geometry loaded, and its properties were
  on the entity. Decision (h) applied (component properties, overridden shapes): on Linux it now falls from cycle 0,
  3 launches identical, and rests exactly like its twin (3.211, 1.137, −4.997 vs 33.212, 1.137, −59.997, both 81.5°).
  Every other station matches the Linux values; balls-of-steel: 3 clean shutdowns of 3.
- **P2 acceptance MET on macOS M2** (peer, 2026-10-02; base `0238d8a`, engine `b8bbeb29`, alpha `b49e2cde`): 0 warning;
  5 recorded runs, run 1 compared with runs 2-5: 1800 common cycles, **0 differing samples, max gap 0.0 on all 17
  stations**, no non-finite value. `DynTopCube` at cycle 60 y 4.2303, vy −0.587; at rest (3.211, 1.137, −4.997), 81.5°
  — the Linux values to the printed digits.
- **P2 acceptance MET on Windows** (peer, 2026-10-02; same commits): MSVC 0 warning (no C4459), base 2272 / 0 failed /
  3 skipped (live HTTPS); recorded bench on the RTX 3060 Laptop AND the AMD iGPU: **0 differing samples on all 17
  stations, 5 runs each**, and NVIDIA run 1 = AMD run 1 over all 1800 cycles (before the `DynTopCube` fix it was the
  only station to differ, from cycle 62, on both GPUs). Values = Linux: stack tops 0.501 / 1.500 / 2.503 / 3.508 /
  4.515, tail amplitude of the top box (0.0033, 0.0078, 0.0056) m; box flat 5.24°, edge 44.45°; e 0.4915 / 0.9978;
  spinner axes constant; platform carries the box 9.999 → 14.019; `DynTopCube` 81.54°. balls-of-steel 5 / 5 clean
  shutdowns.
- **Run-to-run determinism: CLOSED (2026-10-02, three OS)** — item `physics-run-to-run-determinism` deleted. The
  P0 / P1 divergences came from the ORDER of the contact pairs (octree traversal) fed to a sequential solver; P2 sorts
  the bodies by creation number and the pairs / manifolds by key. Two lessons that stay: compare runs CYCLE BY CYCLE
  (§ 6, a final state hides a time shift and invents differences), and a body whose collision shape waits for an
  asynchronous geometry load joins the simulation late — a scene that must reproduce overrides its shapes and sets
  the properties on the component (`DynTopCube`, decision 6h).

- **P3 (rotation, 2026-10-02)**: P3.a — `AABBCollisionModel` → `BoxCollisionModel` / `CollisionModelType::Box`, the
  narrow phase collides `toWorldBox()` (oriented), the models' MTV tests and their four `.cpp` removed, the debug overlay
  draws the local box. P3.b — centre of mass = shape centroid, inertia derived from the shape unless explicit
  (`std::optional` tensor), rotation on by default (off for projet-alpha's `AbstractLiving`), exact angular drag
  (`docs/subsystems/physics/16-rigid-body-rotation.md`). Bench, Linux, 3 recorded launches, **0 differing samples on
  all 17 stations**:

  | Station | P2 | P3 |
  |---|---|---|
  | box dropped flat | tilts 5.24° | flat (0.00°), y 0.500 |
  | box on an edge | stays at 44.4° | falls onto a face (0.00°), y 0.500 |
  | 5-box stack | top 4.515, tail (3.3, 7.8, 5.6) mm | top 4.494, tail 0.47 mm in X only, 0° (≈ 1.5 mm soft penetration per contact) |
  | `DynTopCube` / twin | rest at 81.5° (envelope) | rest on a face (90.00°, y 1.000) |
  | slope (30° slab) | an axis-aligned step | the ball rolls down, then rolls without slipping on the ground (\|v\| = \|ω\| r within 2 %); the box tumbles down onto a face; both rest |
  | balls e 0.5 / 1.0 | 0.4915 / 0.9978 | unchanged |
  | spinner | world-axis ✔ | unchanged (2 rad/s, upward Y constant) |
  | platform | carries 10 → 14 | carries 10.000 → 14.018 |

  The two tipping cubes no longer rest at the same place relative to their base (x +2.822 / −0.025 vs +2.866 / −0.010
  in z): tumbling over an edge amplifies the rounding of their different world coordinates — chaos, not a defect (each
  is bit-identical run to run).
- **P3 accepted on macOS M2** (peer, 2026-10-02; base `3fcc3db`, engine `cc3cdaa8`, alpha `1bf2e020`): 0 warning,
  base 2272 / 0 failed / 3 skipped, 0 differing samples on all 17 stations in 5 runs, 7 demos clean. Every station
  equals the Linux values EXCEPT the two cubes that tumble before resting: `DynTopCube` x 2.809 (Linux 2.822),
  `BenchTipCube` (32.811, −59.947) vs (32.866, −60.010) — each deterministic on its platform. Likely the FMA
  contraction of clang on arm64 (`-ffp-contract=on` by default) amplified by the tumble; not verified, and not a goal
  (decision 3: same machine, same binary).
- **P3 accepted on Windows** (peer, 2026-10-02; same commits): MSVC 0 warning (no C4459 / C4305), base 2272 / 0 / 3
  skipped; 0 differing samples on all 17 stations, 5 runs on the RTX 3060 AND the AMD iGPU, NVIDIA run 1 = AMD run 1;
  every value = Linux (the tipping cubes too: x86 both). 7 demos: clean exits, 0 NaN. Seen once in 3 launches of
  lighten-marbles, NOT physics: 4 × "[RenderableInstance] Descriptor set contract violation: the sealed pipeline layout
  declares the 'PerLight' set, but the renderable instance cannot provide it" on the terrain right after its creation
  (a load-time race, rendering side; reported to the owner).
- **Defect found by the owner (2026-10-02): balls tunnel through the GROUND** (balls-of-steel, a P2 regression).
  Measured with the new console `getGroundLevel(x, z)`: 82 of 400 sampled balls have their centre under the terrain
  after 25 s (down to the lower boundary, 167 m below), speeds up to 66 m/s. Cause: the ground contact exists only
  within the fixed 2 cm speculative margin; a ball moving more than its radius per step (> 30 m/s for r 0.5 m) gets
  its centre under the triangle, the sphere ↔ triangle normal then points DOWN and pushes it through. Before P2 the
  ground was a height test (anything under it went up). Fix options to the owner (one-sided solid ground,
  velocity-scaled speculative margin, both); the owner took both.
  **Fixed (2026-10-02)** by the one-sided, solid-below ground (`NarrowPhase::generateGround()`,
  `subsystems/physics/02-physics-specific-rules.md` step 3). Bench stations `BenchFastBall` / `BenchFastBox`
  (released 95 m up, 43 m/s at the ground) went to the lower boundary (y −99.5) before, and rest at y 0.500 after:
  they sink once (centre ≈ −0.02 for one step), then come back up. balls-of-steel: 0 ball under the terrain at 5, 12
  and 25 s (82 of 400 before). The 14 stations that never cross the surface are bit-identical to P3; `BenchBoxFlat`,
  `DynTopCube` and `BenchTipCube` change from their first ground contact (the one-sided filter dropped manifolds that
  pushed them DOWN) and stay within 1 cm of P3 for the flat box, on a face for the tipping cubes.
  ⚠️ The velocity-scaled speculative margin (B) was tried and NOT kept: with this solver it MAGNIFIES the
  sequential-impulse impact artefact — the whole stop happens in one sub-step, the first corner's impulse tilts the box,
  the other corners (under the centre of mass) get a tangential velocity, friction answers, and the box leaves
  sideways and in yaw: `BenchBoxFlat` slid 0.21 m, `BenchFastBox` 3.5 m, the e = 1 ball lost 1 % (e 0.9906). Face
  contacts from the low points, merged per plane, did not cure it either (a flat box over a grid also had its
  corners ON the cells' diagonals, claimed by two triangles — kept: one triangle per low point). The margin is P5's
  (`physics-continuous-collision`), with the solver's impact behaviour.
- **Peers on `a5c27ef3` (2026-10-02)**: bench accepted on macOS and Windows (19 / 19 stations, NVIDIA = AMD), but 1-2
  balls of balls-of-steel per launch still went under a hilly terrain, some 100 m deep. Two defects of that first fix,
  found with a per-body trace: (1) "under the ground" was decided per TRIANGLE — near a ridge the centre was under one
  plane and over its neighbour's; (2) the low points were taken along the steep face's NORMAL, up to a radius sideways,
  out of every visited triangle's footprint — "underground, 8 triangles, 0 manifold". Then the solver's 3 m/s pushout
  recovered a ball entering a hillside at 26 m/s slower than it sank. Replaced by the RECOVERY of step 1b (decided once
  per body on the triangle vertically under its centre, the body put back on the surface, its velocity bounced) — the
  former height test, for the bodies that crossed only. `getGroundLevel()` also answers the triangles' exact
  `surface` (the bilinear `position` is metres off on a steep 1 m cell: it reported false "under the ground").
- **P5 continuous collision (2026-10-02, order revised, decision 10)**: step 4b (`subsystems/physics/02-…`). Bench
  `BenchBulletBall` (0.1 m ball) and `BenchBulletBox` (0.2 m box) shot at 80 m/s (1.33 m per step) at a 0.2 m wall:
  they stop at x = −0.205 (the wall face −0.1, their half size 0.1, the 5 mm slop) and bounce at 40 m/s (e 0.5);
  `BenchFastBall` no longer sinks (lowest centre 0.443 vs −0.022), `BenchFastBox` no longer slides (−21.999, −75.000).
  Trap met and fixed: a ball the sweep stopped every step against a triangle the contacts ignored (met from under it)
  stayed FROZEN while gravity kept adding to its velocity (110 m/s after 25 s, then it flew off) — the sweep is now
  one-sided like the contacts, and it bounces the velocity itself. A second freeze, in animation-debug: a walker
  sliding along a wall a few mm off was stopped every step by a TANGENTIAL hit that bounced nothing while its own
  controller kept accelerating it (369 m/s) — the sweep now stops only a CROSSING (the centre ending past the surface
  met), as Box2D v3 and Bullet use their continuous pass; `BenchFastBall` then sinks 12 cm for one step (lowest centre
  0.383) and the contacts take it back. Energy check (balls-of-steel, the sum of ½v² + g·y
  over 1000 balls): 361 k → −64 k → −375 k → −421 k → −422 k J/kg at 10, 20, 35, 50, 65 s, kinetic 32 at 65 s — no
  pump. 0 ball under the exact surface at 5, 12 and 30 s on two launches. 23 stations, 3 runs bit-identical; the 14
  stations that never touch the change are bit-identical to P3.
- **P5 continuous collision ACCEPTED on the three OS** (2026-10-02; engine `7b4835bb`, alpha `51fd9b11`): macOS M2 and
  Windows NVIDIA + AMD, 0 differing samples on all recorded stations, 5 runs, NVIDIA r1 = AMD r1; every value = Linux
  (the tumbling cubes: the arm64 values); balls-of-steel 0 ball under the exact surface, the energy only decreasing
  (plateau ≈ −408 k J/kg on both); 7 demos clean. macOS: 0 GPU device loss in 35 launches, 20 with the validation
  layers (0 VUID) — the 2 losses seen on `a5c27ef3` / `d7795acf` did not come back.
- **P4 (2026-10-02)**: the engine's kinematic character (`subsystems/physics/17-kinematic-character.md`, engine
  `40d347e7`) — ACCEPTED on Windows (31 stations at 0, NVIDIA = AMD, every walker = Linux). macOS found that NO walker
  was a character there: the component had every virtual inline, the application held its own hidden typeinfo and the
  engine's `dynamic_pointer_cast` to it answered nullptr (libc++ compares type_info by address) — fixed by an
  out-of-line destructor (its key function), `caution-points.md`, item `rtti-key-function-audit`. Then the actors
  (projet-alpha `docs/subsystems/actor/06-6-walking-actors.md`): the physical Player, the Paladin and the Fox on the
  controller, the Drone dynamic, the jump a launch speed (the former heights), the landing into `onCollision()`, the
  dead `MovableTrait::setStepHeight()` removed. Measured: citadel's west flight climbed 0 → 9.65 m in 9 s at 1.4 m/s
  by the keyboard path (console `keyDown` / `keyUp`, new: a held key survives the per-frame hardware copy); the
  animation-debug paladins and fox walk to the player and stop (they left at 50-80 m/s as dynamic bodies); default,
  basic-scenery, beams, terrain, liminal, citadel, collision, game-logic, animation-debug and the physics demos: clean
  exits, 0 NaN, no new error, every character grounded. One defect found on the way (terrain): a still character crept
  downhill 1.3 cm/s — the walkable depenetration is now vertical (`BenchStandSlope`).
- **P5 ISLANDS (2026-10-02, decision 13)**: step 8 of the physics step (`subsystems/physics/02-…`): a union-find over the
  step's contacts, rebuilt every step; an island sleeps when all its bodies have been slow (< 5 cm/s, < 0.05 rad/s) for
  30 steps, on any support; a woken body wakes its island at step 1c; a MOVING kinematic body (a platform, a character)
  wakes a sleeping body it touches. `MovableTrait::checkSimulationInertia()` (a body alone, on the ground or the world
  floor only) is gone. Measured, the logic thread's CPU per cycle (median of 5 launches, 15 s after 20 s): balls-of-steel
  7.025 → 6.994 ms, lighten-marbles 0.614 → 0.614 ms — no gain there: their bodies never rest (balls roll on, marbles
  live and die; 15-30 of 400 sampled asleep); game-logic 155 of 161 asleep after 25 s (0.703 ms). The bench: the stack
  now SLEEPS (it crept 1 mm/s before), 1 mm higher; a creeping ball on the slope sleeps 2 cm short; the pushed box
  wakes one step after the walker's push and ends 45 cm further; 3 launches bit-identical.
- **P5 STATIC TRIANGLE MESHES (2026-10-02, decision 13)**: base `Space3D::TriangleMesh` (13 tests, ASan/UBSan green),
  engine `Physics::TriangleMeshCollisionModel` + `Toolkit::generateTriangleMeshInstance()`, ROW 7 of the bench
  (`subsystems/physics/18-triangle-mesh-statics.md`): a box crosses 288 coplanar triangles with y exactly constant, a
  mesh ramp = a box ramp to the cycle, mesh stairs climbed, one-sided and two-sided panels as expected, an 80 m/s ball
  stopped by a zero-thickness mesh panel. Found on the way: `ground-kicks-fast-rolling-ball` (the flat ground).
- **P5 ACCEPTED on macOS M2** (2026-10-02, base `daea2d6`, engine `d4f38cfe`, alpha `1493e92f`): 2282 base tests (13/13
  mesh); bench 40 stations, run 2 = run 1 = a third run, 0 differing; ROW 7 = Linux (MeshSlider y 1.0000 all along,
  mesh ramp max y 3.0966, stairs 1.5100, UpBall apex 4.2463, UpBallTwo 2.4990, the bullet stopped at x 74.495); the old
  stations changed where bodies now sleep; 9 demos clean, a validated citadel run 0 VUID, 0 device loss (0 in 77).
  Logic thread (Apple `sample`, busy outside `sleep_for`): balls-of-steel 6.68 ms per cycle (Linux 7.0), lighten-marbles
  2.84 ms (Linux 0.61, `/proc` CPU ticks) — ~0.9 ms in the terrain's `visitTriangles` (ground contacts and the
  continuous sweep), ~0.8 ms in `checkEntityLocationInOctrees` (`OctreeSector::erase`). Not compared with a pre-P5 build
  on macOS: then measured (3 launches each, the same `sample` method): P5 2.999 ms, pre-P5 (`b5dc8297`) 2.949 ms, +1.7 %
  inside the ~3 % spread — NOT a P5 change. The macOS / Linux gap (~3 vs 0.61 ms, while balls-of-steel agrees) stays a
  platform observation; the methods differ (wall-clock samples outside `sleep_for`, which count a blocked or
  descheduled thread, vs CPU time) — `thread_info(THREAD_BASIC_INFO)` would give the Linux measure.
- **P5 continuous collision completed (2026-10-02, decision 14)**: dynamic ↔ dynamic sweeps and exact box sweeps (step
  4b, `subsystems/physics/02-…`; base `ConvexDistance.hpp` + `castBox()`, 15 tests, ASan/UBSan green). Bench ROW 8 and
  S17: two balls at ±60 m/s stop where they meet (1.0 m apart; without the dynamic sweep they interpenetrated 0.91 m),
  a ball at 80 m/s pushes a 10 kg box straight (without: it climbed it, the box turned and left sideways), a box turned
  45° stops with its EDGE at the wall (centre x −0.2469 for −0.246; its inscribed sphere let the edge 4 cm in). The
  other 40 stations: identical to the P5 reference but `BenchFastBox` (1.4 mm) and `BenchBulletBox` (0.5 mm). Cost: the
  plain dynamic loop added 5 % of balls-of-steel's logic thread (7.35 vs 6.99 ms), the sweep and prune 1.8 % (7.12 ms,
  inside the ~3 % spread; medians of 5 launches). The bench: 45 stations, 3 launches bit-identical.
- **Decision 14 ACCEPTED on macOS M2 and Windows** (2026-10-02, base `a4eb68f`, engine `6154b9a4`, alpha `b50628d4`):
  0 warning (MSVC: 0 C4xxx, 0 LNK), 2297 base tests + 3 skipped (15/15 `MathSpace3DConvexDistance`); bench 2 runs × 45
  stations at 0 differing (Windows: NVIDIA = AMD on every cycle); the values = Linux (BulletPairA / B end at x −1.275 /
  −0.268, 1.007 m apart, at rest; HitBox x 43.4986 z 54.0026, straight; BulletAtBox behind it at 41.923; TurnedBox max
  x −0.2469 at cycle 67); against the P5 runs only `BenchFastBox` (1.39 mm) and `BenchBulletBox` (0.50 mm) moved; 9
  demos clean (macOS: a validated citadel 0 VUID, 0 device loss in 90 launches; Windows: citadel's known teardown VUIDs,
  terrain's known slow shutdown). Continuous collision is accepted on the three OS.
- **The wheeled vehicle (bonus phase, decisions 15; 2026-10-02, Linux)**: base `Math::PiecewiseLinear`, engine
  `Physics::VehicleController` (drive train), the wheels in `SoftStepSolver`, `Component::Vehicle`, step 1d (the sphere
  casts), the console (`getNodePhysics().vehicle`, `setVehicleInput`); bench ROW 9 (idle, straight + brake, a right
  turn), 48 stations × 3 launches bit-identical, the 45 older unchanged. Design, measurements and traps:
  `subsystems/physics/19-wheeled-vehicle.md`; item `physics-wheeled-vehicle`.
- **The vehicle ACCEPTED on macOS M2 and Windows** (2026-10-02, base `1e3728f`, engine `9360d863`, alpha `89595b75`;
  Windows needed `1ab2bbb9`, the `VehicleSettings` export — MSVC LNK2019, invisible on ELF / Mach-O): 48 stations × 2
  runs at 0 differing, the 45 older bit-identical, the cars within 0.3 % across OS (probably the math library, not proven), 0 VUID. The
  Windows peer also found `BenchSpinner`'s phase varying run to run since P2 (positions-only `--compare` cannot see it):
  projet-alpha items `physics-bench-spinner-phase`, `physics-bench-compare-full-state`.
- **Driven in citadel (2026-10-02)**: a parked car crept down a 6° slope with its hand brake pulled — the brake was
  solved outside the friction, and the slip floor (0.5 m/s) hid a creep. Fixed on Jolt's model (a locked wheel, its
  surplus brake impulse on the ground; a 1 mm/s floor): `subsystems/physics/19-wheeled-vehicle.md` § Traps.
- **P5 ACCEPTED on Windows** (2026-10-02, RTX 3060 + AMD iGPU, same commits): 2282 base tests (13/13 mesh); bench 2 runs
  per GPU, 40/40 at 0 differing, NVIDIA = AMD on every cycle; ROW 7 = Linux (MeshSlider y 1.0000 all along, ramp 3.0966,
  stairs 1.5100, UpBall apex 4.2463); 19 of the 32 old stations changed, the islands' sleep (PushedBox 0.45 m, the rest
  ≤ 3.4 cm); 9 demos clean with validation, except citadel's known teardown VUIDs and terrain's known slow shutdown. ⚠️
  `--first-cycle 1` starts when the console comes up (cycle 7 macOS, 15 Windows): an event before it is not recorded.
  P5 is accepted on the three OS.
- Two build breaks of `8a890f9f` / `abfe615` found by the peers, fixed: a `switch` without `TriangleMesh` (clang
  `-Wswitch`, engine `d4f38cfe`) and a local named `far` (windef.h's empty macro, MSVC C3329, base `daea2d6`).
- **P4 follow-up ACCEPTED on macOS M2** (2026-10-02, engine `b5dc8297`, alpha `302a5d8c`): `nm -m` — the
  component's typeinfo DEFINED in the framework only, the app imports it; bench 2 runs × 32 stations at 0 differing,
  the 23 non-walker stations bit-identical to the previous run, the walkers = Linux (WalkFlat 29.98 m, ramp max y 3.097,
  steep blocked at 1.8 m, stairs max y 2.330, StandSlope still); citadel's paladins at y ≥ 0.0100 over 3600 cycles; the
  flying Player 10.00 m/s, ±10 m/s vertical, lands at V off; 9 demos clean, a validated citadel run 0 VUID; 0 device
  loss (0 in 61 launches since the two).
- **P4 follow-up ACCEPTED on Windows** (2026-10-02, RTX 3060 + AMD iGPU): bench 2 runs × 32 stations at 0 differing on
  each GPU, NVIDIA = AMD; BenchStandSlope still (y range 2.4e-7); the paladins at y ≥ 0.0100; the flying Player
  10.000 m/s, ±10 m/s, lands at V off; citadel's west flight climbed grounded at 1.4 m/s (top y 10.01); 18 demo
  launches clean. Seen there, not physics: citadel's teardown VUIDs (item `texture-destroyed-while-upload-in-flight`),
  terrain's 60 s shutdown during its loads (`shutdown-hangs-after-act-removal`), the PerLight race twice on NVIDIA
  (`lighten-marbles-perlight-descriptor-race`).
- **P4, the owner's play test (2026-10-02, citadel)**: (1) the three paladins vanished — a character met twice by the
  physics step (the octree's `expand()` kept a splitting sector's elements AND filed them in the children: ~100 statics
  and a paladin 2 to 7 times per cycle) collided with its own capsule and sank under the ground. Fixed in the octree
  (one element, one sector in every operation, the owner's choice; `docs/caution-points.md`): 0 duplicates, 0 falls in
  4 runs, the bench bit-identical. (2) V no longer flew — the controller ignored the free fly flag; it now has a
  FLYING mode (the owner's choice, `subsystems/physics/17-kinematic-character.md` § Flying). The Shift jump keeps its
  former 2.69 m (owner).

- **P2 implementation decisions (owner, 2026-10-01)**: (1) a COLLIDABLE dynamic body is integrated by the scene's
  physics step (gravity and position inside the sub-steps); a non-collidable one (`setCollidable(false)`) keeps
  integrating itself in `MovableTrait::updateSimulation()`; `addForce()` and the drag are unchanged for the actors.
  (2) The solver integrates the orientation in WORLD space (`RigidBody::integrateOrientation()`): the
  `rotateFromPhysics()` local-axis defect is fixed in P2, not P3 (P3 keeps the oriented boxes and the shape-derived
  inertia). (3) An animated non-movable node is a KINEMATIC body: infinite mass, a velocity derived from its motion
  between two cycles, so what rests on it is carried.

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

### C. Solver (the item `physics-solver-restitution-and-position-correction` held the detail; closed by P2)
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
| P2 solver | engine | `physics-unified-contact-pipeline` — CLOSED 2026-10-02, accepted on the three OS (absorbed `physics-solver-restitution-and-position-correction`, closed 2026-10-01; closed `physics-run-to-run-determinism` on 2026-10-02; expected to close `physics-no-rest-on-generated-terrain`, probably `physics-nan-linear-velocities`) |
| P3 rotation | engine | `physics-oriented-box-collision-model`, `rotational-physics` — both CLOSED 2026-10-02 (§ 1b) |
| P4 walking | engine, then projet-alpha | `kinematic-character-controller` (superseded `physics-step-up-pass`, closed 2026-10-02: citadel's stairs climbed), projet-alpha `actors-kinematic-character-migration` |
| P5 | engine (+ base for the mesh and GJK) | `physics-continuous-collision`, `physics-triangle-mesh-static-shapes`, `physics-simulation-islands` — all CLOSED 2026-10-02 (§ 1b) |
| Bonus: vehicle | engine (+ base for the curves) | `physics-wheeled-vehicle` — in progress (§ 1b) |

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
| `BenchPlatform`, `BenchPlatformBox` | massless non-movable node (kinematic): waits 1.5 s, 10 → 14 m in X in 1.8 s, waits 0.9 s, back in 1.8 s; a box resting on it | the box rides the platform |
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
| `getGroundLevel(x, z)` | `{"position": [x, level, z], "normal": [...]}`: the ground height there (`getLevelAt()`, the bilinear height field — not exactly the triangles' surface), to count bodies under the ground |

- The logic thread never allocates (start() reserves nodes × cycles) and never writes the file (save() does, on the
  console thread, outside the lock). An idle recorder costs one atomic load per cycle (`m_armed`).
- A name absent when the recording begins is recorded with `"found": false`.
- `tools/physics-bench.py` uses it by default (`--first-cycle 60`, i.e. from 1 s); `--poll` keeps the P0 method.
- **First measurement (Linux, RTX 3070 Ti, 5 launches × 1800 cycles × 17 stations)**: the 5 runs are BIT-IDENTICAL on
  every cycle of every station (0 differing samples, `DynTopCube`, the stack and the twin included). The isolated
  differences seen by polling were the one-cycle labelling offset — proven. Linux reproduces exactly; the divergence
  on Windows and macOS is now measurable to the cycle (next: the peers' recorded runs).
- **macOS M2 (peer, 5 recorded launches, 1800 common cycles each)**: 13 stations bit-identical 5/5 — the twin
  `BenchTipCube` INCLUDED (so on the M2 the single-pair twin is bit-repeatable). `DynTopCube` gives 5 distinct
  trajectories that already differ at the FIRST recorded cycle 60 (x 0.85-2.87, ωz from -0.1 to -28.5): its split
  happens in cycles 0-59, the loading window — consistent with its entity-level properties re-derived at a
  machine-dependent moment. The stack: runs 1, 3, 5 bit-identical; run 2 already differs at cycle 60; run 4 is
  IDENTICAL to run 1 over cycles 60-71 and splits at cycle 72 (the top box E first, then B, A) — long after the loads,
  which fits the pair-ORDER half of the hypothesis (a different octree insertion order changing the Gauss-Seidel order
  once several contacts interact), not the property half.
- **Windows, RTX 3060 and AMD iGPU (peer, 5 + 5 recorded launches)**: the twin `BenchTipCube` bit-identical 5/5 on BOTH
  GPUs, with the same final state on NVIDIA and AMD (31.955, 4.405); 12 other stations bit-identical. The stack and
  `DynTopCube` already differ at cycle 60, in a few DISCRETE states (StackA's height at cycle 60 takes 0.1870 or
  0.2019; `DynTopCube` repeats on AMD in 4 of 5 runs, at the Linux 6.855): the first fork lies in cycles 0-59, where the
  loads complete. Two NVIDIA runs share `DynTopCube`'s state at cycle 60 yet end apart: a second fork later.
- **Reading across the three OS**: one contact pair repeats exactly everywhere; several interacting pairs fork in
  discrete states — the signature of an ORDER (of insertion, hence of solving), not of floating-point noise. P2's sort
  of the manifolds by stable ids is the targeted fix; then the bench must read 0 differing samples on every station on
  the three OS. Dating the first fork would need a recording armed before the scene starts: NOT designed — owner
  (2026-10-01) agreed it is not needed before P2; design it only if P2 does not reach 0 differing samples.

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
