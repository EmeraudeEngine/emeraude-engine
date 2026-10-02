---
id: physics-wheeled-vehicle
title: A wheeled vehicle — a dynamic chassis on cast wheels (bonus phase)
status: in-progress
priority: unranked
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
- [ ] The peers' validation (macOS, Windows NVIDIA + AMD): build, bench 48 stations at 0 differing, the car values.
- [ ] Stations still to add: a ramp, a bump, an overturn (and a car meeting a dynamic body).
- [ ] Driving one in projet-alpha (the player at the wheel, a dedicated demo or not): owner's choice, not asked yet.

## References

- Bullet `btRaycastVehicle` (zlib); Jolt `VehicleConstraint` / `WheeledVehicleController` (MIT); PhysX Vehicle SDK.
