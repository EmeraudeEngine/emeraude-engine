---
id: physics-wheeled-vehicle
title: A wheeled vehicle — a dynamic chassis on cast wheels (bonus phase)
status: in-progress
priority: medium
scope: Physics, Scenes (a vehicle component)
opened: 2026-10-02
tags: [physics, vehicle, physics-overhaul]
---

# A wheeled vehicle — a dynamic chassis on cast wheels (bonus phase)

## Why

Owner, 2026-10-02: after the kinematic character, "on fera le véhicule en bonus". A vehicle must pitch, roll, skid and
overturn, so it is NOT a kinematic character: a DYNAMIC rigid chassis (P3 inertia), each wheel a ray / shape cast to
the ground (P1 casts, the ground triangles of P2) that gives a suspension force (spring + damper) and a tyre force
(longitudinal and lateral friction, engine and brake torque).

A bonus phase after the physics overhaul, started 2026-10-02 (P5 done). Owner decisions 15 (`docs/physics-overhaul.md`):
sphere-cast wheels, slip-curve tyres, a complete drive train, the wheels in the solver. Design:
`docs/subsystems/physics/19-wheeled-vehicle.md`.

## What remains

- [x] V1 base: `Math::PiecewiseLinear` (torque and tyre curves), 4 tests (Release + ASan/UBSan).
- [x] V2 engine: `Physics::VehicleSettings` / `VehicleController` (wheels, engine, gearbox, clutch, differentials).
- [x] V3 solver: the wheel constraints in `SoftStepSolver` (suspension, longitudinal + lateral friction, wheel spin).
- [x] V4 scene: `Component::Vehicle`, the wheels' sphere casts in the physics step, the wheel visuals, the console.
- [x] `collision-debug` ROW 9: idle, straight + brake, turn (measured: `docs/subsystems/physics/19-wheeled-vehicle.md`).
- [x] The peers' validation of `9360d863` (+ `1ab2bbb9`): macOS and Windows accepted (19-wheeled-vehicle.md § Validated).
- [x] Driving one in projet-alpha: citadel's plain, E to board, a chase camera (owner, 2026-10-02).
- [ ] The peers' validation of the brake fix (the locked wheel, the 1 mm/s slip floor): the cars' new values.
- [ ] Stations still to add (owner: after): a ramp, a bump, an overturn, a car meeting a dynamic body.

## References

- Bullet `btRaycastVehicle` (zlib); Jolt `VehicleConstraint` / `WheeledVehicleController` (MIT); PhysX Vehicle SDK.
