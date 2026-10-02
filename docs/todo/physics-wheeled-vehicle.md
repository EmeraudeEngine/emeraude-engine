---
id: physics-wheeled-vehicle
title: A wheeled vehicle — a dynamic chassis on cast wheels (bonus phase)
status: parked
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

PARKED: a bonus phase after the physics overhaul (P5 included), by the owner's order.

## What remains

- [ ] Design with the owner: raycast vs shape-cast wheels, the tyre model (a simple friction ellipse or Pacejka), the
  drive train (engine curve, gears, differential), an arcade (kinematic) variant or not.
- [ ] A `collision-debug` (or a new demo) station: drive straight, turn, brake, a ramp, a bump, an overturn.

## References

- Bullet `btRaycastVehicle` (zlib); Jolt `VehicleConstraint` / `WheeledVehicleController` (MIT); PhysX Vehicle SDK.
