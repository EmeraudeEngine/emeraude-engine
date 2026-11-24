---
id: vehicle-rest-pose-differs-between-arm64-and-x86-64
title: A wheeled vehicle dropped on the ground comes to rest 16 mm lower on macOS (arm64) than on Linux and Windows (x86-64)
status: open
priority: high
scope: src/Physics (Vehicle, the solver, the sleep of a body), src/Scenes/Scene.physics.cpp
opened: 2026-11-22
tags: [physics, vehicle, determinism, macos, arm64]
---

# A wheeled vehicle comes to rest 16 mm lower on macOS (arm64) than on Linux and Windows (x86-64)

## Why

The same scene, the same build of the engine (`53b0ecc6`), the same vehicle (a downstream testbed's car: 1888 kg, four
wheels of 0.384 m, the default suspension, its hand brake held, dropped from a few decimetres at the scene's start),
read with `getNodePhysics()` once it sleeps:

| Machine | Rest pose of the chassis' centre |
|---|---|
| Linux, x86-64, GCC (RTX 3070 Ti) | (27.7749, 0.1542, −8.0292) |
| Windows, x86-64, MSVC (RTX 3500 Ada) | the same to four decimals |
| macOS, arm64, AppleClang (Mac mini M6) | (27.7815, 0.1384, −8.0260) |

macOS rests **15.8 mm lower**, 6.6 mm further in X and 3.2 mm in Z. It is not a pose read before the body had settled
(macOS peer, 2026-11-22, an instance at 60 logic cycles a second): at scene cycle 6 the chassis is at
(27.79907, 0.36580, −8.00013), falling at 0.90 m/s; at cycle 67 at (27.7817764, 0.138348281, −8.02627563) with
0.006 m/s left; at cycle 122 at (27.7815113, 0.138350442, −8.02598095), asleep (`simulationPaused` true) — and
identical to the last digit over the 60 reads that follow, to cycle 3240. The same pose was read in another run of the
same day.

⚠️ The poses above were read BEFORE the vehicle's hold aid (engine `ef3e5a2b`, 2026-11-22): with it the vehicle
settles with its service brakes applied and every machine rests a little elsewhere — Linux and Windows at
(27.7760, 0.1535, −8.0289), the forward pitched −3.03°; macOS at (27.7823, 0.1403, −8.0228), pitched −2.64°. The gap
is then 13.2 mm in height and 0.39° in pitch: compare poses of ONE engine version only.

The owner ranked it a defect (2026-11-22): one vehicle, one ground, two rest heights. A suspension has ONE
equilibrium; 16 mm is far beyond a rounding difference, so on one side at least the body sleeps away from it, or the
two machines do not resolve the same contacts.

## What remains

- Record the SAME trajectory on Linux and Windows: the chassis' pose and speed at every scene cycle from the scene's
  start to the sleep (the macOS reads above are the reference), and each wheel's suspension length, load and contact at
  rest on the three machines. Find the first cycle where they part, and which quantity parts first.
- Then the cause. Candidates, none tested: floating-point contraction (arm64 fuses a multiply-add by default, x86-64
  does not without FMA: `-ffp-contract`), the standard library's math functions, an iteration over an unordered
  container that orders the contacts or the wheels differently per standard library, the sleep of the body freezing
  the suspension at a different phase of its settling, the tyres' static friction holding the landing's last slide
  (the pose differs in X and Z too).
- State the contract in `docs/physics-system.md` once known: what the engine promises between two machines (the
  same rest pose within a tolerance at least), next to the per-cycle determinism it already verifies on one machine
  (`physics-bench.py --compare`).

## ⚠️ Traps

- Read a pose only once the body sleeps, and count CYCLES: before 2026-11-22 a macOS window-less process ran at a
  fifth of its logic rate past its first 30 s (`docs/caution-points.md` § *macOS: App Nap demoted a window-less process
  after 30 s*), and a wait in wall seconds read an unsettled body.
- The physics bench compares two runs BY CYCLE on ONE machine: it does not see this.

## References

- `docs/subsystems/physics/19-wheeled-vehicle.md` (the wheel model, the suspension), `docs/physics-system.md`.
- `src/Physics/Vehicle.hpp`, `src/Scenes/Scene.physics.cpp`.
