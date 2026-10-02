## Kinematic character controller (physics overhaul P4, 2026-10-02)

Owner decisions: `docs/physics-overhaul.md` § 1.9. Code: `Physics/CharacterController.hpp/.cpp` (the algorithm, behind
`Physics::CharacterWorldInterface`), `Scenes/Component/CharacterController.hpp/.cpp` (the component),
`Scene::resolveCollisions()` step 0 (the scene's world queries, `SceneCharacterWorld`).

### Use

```cpp
node->componentBuilder< Component::CharacterController >("Character").setup([] (Component::CharacterController & character) {
	character.controller().setSize(0.3F, 1.8F);                 // radius, height feet to head
	character.controller().setStepHeight(0.35F);
}).build();

character->controller().setWantedVelocity(direction * walkSpeed); // each tick, from the input or the AI
character->controller().jump(5.0F);                               // a launch speed, only when grounded
```

- The node is MOVED, not simulated: its `MovableTrait` velocity, forces and gravity are not used. Its feet are its origin.
- The entity takes a capsule collision model (`controller().localCapsule()`) and is collidable whatever its mass. It is a
  KINEMATIC body for the solver: infinite mass, the velocity of its motion — dynamic bodies meet it as a moving solid.
- The capsule model follows the controller's size: set at the link, rebuilt by the physics step when `setSize()` changed
  it later (a crouch).
- Events, notified after the physics step (outside its lock): `Landed` (data: the fall speed, float m/s — fall damage),
  `LeftGround`, `HitWall`. `MovableTrait::isGrounded()` / `groundedSource()` are kept up to date for existing code.
- Console / MCP: `setCharacterVelocity(name, x, y, z)`, `characterJump(name, speed)`; `getNodePhysics()` adds a
  `character` object (grounded, velocity, groundNormal, supportKey).

### One step

1. Out of what overlaps the capsule INFLATED BY THE SKIN (1 cm): a platform rising into it, a solid put on it — and back
   to a skin width from what it touches. Dynamic bodies never push a character.
2. Velocities: on the ground the wanted horizontal velocity is taken at once and follows the ground plane (a walkable
   slope keeps the horizontal speed), plus the support's velocity (a moving platform); in the air it is blended by the
   air control and gravity applies; a jump is a launch speed.
3. Collide and slide (4 iterations at most): the capsule is swept, stops a skin before the first obstacle and slides —
   along a walkable surface; a wall or a slope too steep only blocks HORIZONTALLY (never a ramp to climb); a ceiling
   stops a rise; a dynamic body hit is pushed with `pushForce × dt` (N·s).
4. Step-up against a low obstacle: up by the step height (headroom-tested), forward, down onto it. The capsule's rounded
   bottom usually lands on the step's EDGE (a leaning normal): a 5 mm vertical probe just past the edge accepts it when
   the step's top is walkable, and a step just climbed keeps the character grounded — the next steps climb on until its
   axis is over the step (the stair walk of PhysX's and Jolt's controllers).
5. Ground probe: a sweep down by the snap distance (0.3 m) when it was grounded, a skin otherwise. A walkable hit grounds
   it and snaps it down (stairs and slopes down stay grounded); a steep hit is accepted when it is a step's edge with a
   walkable top, else a half-radius probe looks for the floor under the character (a walker standing against a 50°
   ramp stays grounded).

### Flying (2026-10-02)

The entity's free fly mode (`MovableTrait::enableFreeFlyMode()`, the flag a free-flying body already had) makes its
character FLY: the scene sets `CharacterController::setFlying()` from it before each step. In flight: no gravity, no
jump, no step-up, no ground probe; the WHOLE wanted velocity (up and down included) is reached with a response of
10 / s (about 0.1 s to start or stop); the capsule still depenetrates, collides and slides. Flying keeps the velocity
it had; back to walking, the flight's velocity splits into the horizontal velocity and a vertical speed (a fall). The
movement mode of Unreal's `CharacterMovementComponent` (`MOVE_Flying`). Measured on citadel (projet-alpha's Player,
10 m/s): W 9.97 m/s level, Space +10 m/s, Ctrl −10 m/s, stopped 0.1 s after the release, held against the barbican
tower with W down, landed 1 s after V off from 6 m. Before (P4 as first pushed), the controller ignored the flag: the
Player's former fly FORCE (500) became a wanted speed of 500 m/s horizontal, with gravity — "very fast, and it falls".

### Measured (`collision-debug`, ROW 6 and beside, 3 launches bit-identical)

| Station | Result |
|---|---|
| `BenchWalkFlat`, 1 m/s | 30 m in 30 s at y 0.010 (the skin), Y amplitude 0 |
| `BenchWalkRamp`, a 30° ramp | climbed to its top (y 3.10) and down the other side |
| `BenchWalkSteep`, a 50° ramp | blocked at its foot, grounded, still |
| `BenchWalkStairs`, S8's 0.29 m steps | climbed to the top (y 2.33) at 1 m/s, walked off its end |
| `BenchWalkPlatform`, on S9's platform | carried 4 m and back |
| `BenchWalkOnBox`, on a static 1 m box | stands, Y amplitude 0 |
| `BenchWalkPush` into a dynamic box | pushed it 8 m, then walked past it |
| `BenchStandSlope`, still on a 30° ramp | does not move (0 creep over 30 s) |

- citadel's west flight (34 steps of 0.29 m, the former `physics-step-up-pass` case): projet-alpha's Player walked up
  from y 0 to 9.65 m in 9 s at 1.4 m/s, grounded all the way (keyboard path, `keyDown(87)`).

### Traps met

- A character met TWICE by the physics step collided with its own capsule (0.91 m deep, normal +Y) and three
  depenetration passes put it 2.3 m under the one-sided ground: the physics octree held entities in several sectors
  (`OctreeSector::expand()`, fixed — `docs/subsystems/scenes/24-octree-storage-and-traversal-scenes-octreesector-hpp.md`).
  The step skips the character's own INDEX: it is sound only on a list free of duplicates (checked in Debug).

- A character standing still on a SLOPE crept downhill (1.3 cm/s on a terrain slope): the depenetration pushed it out
  along the slope's normal, the ground probe snapped it straight down — a little downhill every step. Out of a walkable
  surface the correction is now VERTICAL (the height that clears the same distance from the plane).

- A character starting a step closer than the skin to the ground met the ground at fraction 0 in every sweep — even
  along it — and never moved: the depenetration restores the skin (step 1).
- macOS: the component had every virtual inline, so the application held its own hidden typeinfo and the engine's
  `dynamic_pointer_cast` to it answered nullptr — no character on macOS. It has an out-of-line destructor (its key
  function); `docs/caution-points.md`, item `rtti-key-function-audit`.
- A step's edge under the rounded bottom looked like a wall to the ground probe; the half-radius fallback probe, still
  behind the step, found the floor 0.2 m lower and snapped the walker back down: edges with a walkable top are support.
