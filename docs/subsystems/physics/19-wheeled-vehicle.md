## Wheeled vehicle (physics overhaul bonus phase, design 2026-10-02)

Owner decisions 15 (`docs/physics-overhaul.md`): sphere-cast wheels, slip-curve tyres, a complete drive train, the wheels
solved in the solver. The model follows Jolt's `VehicleConstraint` / `WheeledVehicleController` (J. Rouwé, MIT) and
Bullet's `btRaycastVehicle` (zlib) as references — no code taken. Work item: `docs/todo/physics-wheeled-vehicle.md`.

### What a vehicle is
- A DYNAMIC rigid chassis (a Node with its collision model, mass and P3 inertia): it pitches, rolls, skids and overturns.
  Not a kinematic character.
- Wheels attached to it (chassis local): an attachment point (the top of the suspension travel), a suspension direction
  (down, −Y by default), a radius, the suspension's minimum / maximum length, its natural frequency (Hz) and damping
  ratio, the steering (a maximum angle; steered about the suspension axis), the brake and hand-brake torques, the wheel's
  inertia about its axle and its angular damping, its longitudinal and lateral friction curves.
- A drive train: an engine (maximum torque, a normalized torque curve over the RPM range, minimum / maximum RPM, inertia,
  angular damping), a gearbox (forward and reverse ratios, automatic shifting: up / down RPM, switch time, clutch
  strength), differentials (a left and a right wheel, a ratio, the left / right split, a limited-slip ratio, the share
  of the engine torque).
- Inputs: forward (−1…1, the throttle; negative: reverse), right (−1…1, the steering), brake (0…1), hand brake (0…1).

### One physics step
1. **Wheel casts** (the scene, before the pairs): each wheel sweeps a sphere of its radius from its attachment along the
   suspension direction over its maximum length, against the ground triangles, the statics (meshes included) and the
   other bodies — never its own chassis. A hit gives the contact point, the normal, the ground body and the suspension
   length; no hit: the wheel hangs at its maximum length.
2. **Drive train** (before the solver): the engine RPM from the driven wheels' spin through the differentials and the
   gear (clutch engaged), the automatic gearbox's decision, the engine torque from its curve × the throttle, split to the
   driven wheels by the differentials; the brake torques.
3. **Solver** (`SoftStepSolver`, with the contacts, every sub-step): for each wheel on the ground —
   - the SUSPENSION: a soft constraint along the contact normal (Box2D's soft-step formulation with the wheel's
     frequency and damping ratio), pushing only (a non-negative impulse), towards the rest length; a hard stop when
     compressed past the minimum length (the chassis' own contacts catch the rest);
   - the LONGITUDINAL friction: the contact point's velocity along the wheel's forward direction (in the contact plane)
     driven to the wheel's surface speed ω·r; its impulse spins the wheel back (I·Δω = −r·λ), bounded by
     μ_long(slip ratio) × the suspension impulse;
   - the LATERAL friction: the contact point's side velocity driven to 0, bounded by μ_lat(slip angle) × the suspension
     impulse; both bounded together by the friction circle;
   - the wheel's spin integrates its drive torque, its brake (an impulse towards ω = 0, at most brake torque × h) and its
     angular damping. A dynamic ground body takes the reactions.
4. **After the solver**: the wheels' spin and rotation angle, the engine RPM, the visual wheels (the component moves the
   nodes attached to it: suspension offset, steering, spin).

Slip ratio = (ω·r − v_long) / max(|v_long|, ε); slip angle = atan2(v_lat, |v_long|) (degrees in the curves). A curve
is a piecewise-linear function (base `Math::PiecewiseLinear`).

### Determinism
The wheels are solved in the vehicle's creation order, its wheels in their declaration order; the casts keep the
earliest hit with the static world's ties broken as in step 4b.

### Use (2026-10-02)
- The chassis: a movable, collidable entity with its collision model, its mass and inertia set on the COMPONENT
  (`bodyPhysicalProperties()`), as every bench body.
- `Physics::VehicleSettings` (`src/Physics/Vehicle.hpp`): one `WheelSettings` per wheel (chassis-local attachment,
  radius, steering, brakes, …), `DifferentialSettings` naming the driven wheels by index, then
  `useDefaultCurvesAndRatios()` (Jolt's default engine curve, gear ratios and tyre curves) unless set by hand;
  `isValid()` refuses a wheel of zero radius, an inverted suspension range, a differential naming an absent wheel, a
  non-finite value.
- `Scenes::Component::Vehicle` (`src/Scenes/Component/Vehicle.hpp`) on the chassis: `controller().setup(settings)`;
  the inputs through `controller().setInput(forward, right, brake, handBrake)` (clamped, a non-finite one refused) or
  an input SCRIPT (`setInputScript()`: `ScriptedInput{fromCycle, forward, right, brake, handBrake}`, by physics cycle —
  a deterministic driver; it overrides the inputs every cycle).
- The visual wheels: a child node of the chassis per wheel, `attachWheelNode(index, node)`; the component sets its
  frame every cycle (attachment + suspension direction × length, the steering about the chassis' up, the spin about the
  axle, chassis-local X when the forward is −Z). Make it `setCollidable(false)`: the wheels are the vehicle's casts.
- A chassis is woken by a throttle or by a steering its wheels have not reached (`VehicleController::wantsToMove()`):
  a parked car steered shows its wheels turned, then sleeps again.

### Console / MCP
- `getNodePhysics(name)` gains a `vehicle` object: `engineRPM`, `gear`, `clutch` and per wheel `contact`,
  `suspensionLength`, `angularVelocity`, `steerAngle`, `slipRatio`, `slipAngle`, the suspension / longitudinal /
  lateral impulses and the contact point.
- `setVehicleInput(name, forward, right, brake, handBrake)`.

### Measured (`collision-debug` ROW 9, z +62 to +85; Linux RTX 3070 Ti, 2026-10-02)
A 1500 kg box chassis (1.8 × 0.5 × 4 m), four 0.35 m wheels at (±0.8, −0.1, ±1.4), rear drive, front steering 30°,
rear hand brake, Jolt's defaults otherwise. 3 launches bit-identical on the 48 stations; the 45 older stations
bit-identical to the reference of decision 14 (the cars do not disturb them).

| Station | Inputs | Result |
|---|---|---|
| `BenchCarIdle` | none | settles on its suspension at y 0.8207 (0.371 m of suspension, 12.9 cm of sag), drifts 0.09 mm, then SLEEPS |
| `BenchCarStraight` | full throttle from cycle 60, full brake from cycle 300 | 2.1 / 6.5 / 10.8 / 14.9 m/s after 1 / 2 / 3 / 4 s, squatting 1.4°; stops in 2.30 s over 15.9 m (≈ 6.5 m/s², 0.66 g) diving 2.3°; at rest straight (0.026° tilt), asleep |
| `BenchCarTurn` | half throttle, half steering to the RIGHT from cycle 60 | a steady circle to the right: 9.28 m/s, yaw rate −0.860 rad/s (R = v / ω = 10.8 m), the body rolled 7.5° OUTWARDS; the mirror of the left turn measured before the sign fix (+0.860, the same speed and roll) |

The visuals were checked on screenshots: the wheels on the ground at the idle sag, the spokes left at their spin
angles after the straight run, the front wheels of the parked car steered (`setVehicleInput(…, 0, 1, 0, 0)`: both at
−0.524 rad, under the chassis' edges). Demos without vehicles: unchanged (8 demos, 0 NaN, 0 VUID, clean exits).

### ⚠️ Traps and limits
- **The suspension is solved like a contact's separation**: `prepareWheels()` keeps an ADJUSTED length (the cast
  length minus the anchors' current offset along the normal) and every sub-step measures it against the moved anchors.
  Without the adjustment the spring saw the full anchor offset (0.95 m) as a compression and launched the chassis.
- **The gearbox shifts on the GROUND speed, the clutch closed**: shifting on the wheels' spin made it cycle 1-2-1 under
  wheelspin. The bench car spins its rear wheels in first gear at full throttle (Jolt's 500 N·m through 2.66 × 3.42
  exceeds the rear tyres' grip): it stays in first until 16 m/s. Realistic for the gearing; no traction control.
- **A chassis on its wheels is not `grounded`** (`groundedSource` None): the wheels hold it, not its own contacts. A
  game reads the wheels' `contact` (the console's `vehicle.wheels[].contact`).
- **Steering sign**: `WheelState::steerAngle` is right-handed about the suspension's up, so POSITIVE turns LEFT (the
  forward −Z towards −X) and a RIGHT input gives a negative angle (`VehicleController::targetSteerAngle()`). The first
  version mapped `right` straight to the angle: `right = +0.5` turned the bench car LEFT.
- **A wheel node is oriented first, placed last**: `CartesianFrame::rotate(angle, axis, false)` (parent space) also turns
  the frame's POSITION about the parent's origin; placed before the steering, a steered wheel swung 0.7 m sideways
  about the chassis centre.
- **The 100 m scene boundary is a wall**: the bench's first right-turn circle (from z 80) met it at z ≈ 100 and the car
  rolled over. A station's whole path must stay inside the boundary.
- The wheels do not cast against their own chassis, and a non-collidable wheel node is never a solid.
