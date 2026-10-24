---
id: dynamic-sky-sun-light-from-transmittance
title: Dynamic sky — the sun's directional light takes its colour and illuminance from the atmosphere's transmittance
status: open
priority: unranked
scope: Scenes/Component (CelestialClock, DirectionalLight), Graphics (atmosphere transmittance LUT readback or CPU twin)
opened: 2026-10-24
tags: [sky, atmosphere, photometry, lighting, design]
---

# Dynamic sky — the sun's directional light takes its colour and illuminance from the atmosphere's transmittance

## Why

`CelestialClock` sets the sun's light with an empirical model (`src/Scenes/Component/CelestialClock.cpp`, `apply()`): an
air-mass Beer-Lambert extinction (Kasten & Young 1989) and an exponential fit of the colour
temperature between `zenithTemperature` and `horizonTemperature`. A physical sky computes the real
transmittance of its atmosphere toward the sun. If the light keeps its own model, the sun disc
drawn in the sky and the light on the ground disagree. On an alien sky the fit means nothing at
all, since its 2000/5800 K anchors are Earth's.

## What remains

- [ ] With a dynamic sky, the light's colour and illuminance = the star's top-of-atmosphere
      illuminance × the transmittance toward the star, read from the same model as the sky.
      Without one (a picture sky), the clock keeps its current model. The moons' lights take the same change.
- [ ] Choose how the CPU gets the value. Either a CPU twin of the transmittance integral (cheap: a
      few dozen samples per cycle, one per star), or an asynchronous readback of the LUT. The CPU
      twin is deterministic and has no frame latency; the readback is guaranteed to match the GPU.
      Escalate to the owner.
- [ ] Below the horizon, the light still switches off (current behaviour). The penumbra at
      sunrise (the disc half cut) follows the transmittance instead of a hard switch.
- [ ] Measure: the light's lux and colour against the drawn disc at 60°, 10°, 2° and 0°.

## ⚠️ Traps

- ⚠️ The clock writes a sun light that shadows. A colour that changes every cycle must not trigger
  a shadow-map rebuild or a pipeline change: only the light UBO changes.

## References

- the physical sky, lot 1 (graphics doc 42, `42-physical-sky-atmosphere.md`); the clock: `Scenes::Component::CelestialClock` (scenes doc 02).
- `src/Scenes/Component/CelestialClock.cpp`, `src/Graphics/CelestialBody.hpp`.
