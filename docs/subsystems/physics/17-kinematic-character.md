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

### Traps met

- A character starting a step closer than the skin to the ground met the ground at fraction 0 in every sweep — even
  along it — and never moved: the depenetration restores the skin (step 1).
- A step's edge under the rounded bottom looked like a wall to the ground probe; the half-radius fallback probe, still
  behind the step, found the floor 0.2 m lower and snapped the walker back down: edges with a walkable top are support.
