---
id: dynamic-sky-moon
title: Dynamic sky — moons drawn with their phase, and the moonlight as the night's directional light
status: blocked
priority: unranked
scope: Graphics/Renderable (DynamicSkyResource), Graphics (CelestialBody), Scenes/Component (directional light, celestial clock)
opened: 2026-10-24
blocked-by: [dynamic-sky-atmosphere, celestial-clock]
tags: [sky, moon, night, lighting, photometry, design]
---

# Dynamic sky — moons drawn with their phase, and the moonlight as the night's directional light

## Why

A night under the full moon is not a night without it: about 0.25 lx (`CelestialBody.hpp`
references), enough to cast shadows. A dynamic sky must show the moon where the clock puts it,
with the phase its angle to the sun gives, and light the scene with it.

## What remains

- [ ] **The disc**: a texture (albedo, optional normal map), lit by the SUN's direction, so the
      phase and the terminator come out of the geometry. No phase table. Add earthshine (the
      dark side faintly lit by the planet) as an artistic gain.
- [ ] **The illuminance**: from the phase angle (a Lambert-sphere phase function, or the
      opposition-surge fit for the Moon), × the transmittance toward the moon.
- [ ] **The light**: a second directional light, on while the moon is above the horizon AND the
      sun is below it, with or without shadows (an option). The sun → moon handover around dusk
      must not pop: both lights can exist at once, their weights following their illuminance.
- [ ] **Several moons** for alien skies: the contract is a list of bodies; the clock places each
      one from its orbit.
- [ ] The moon in the IBL bake: included, unlike the sun (it is an extended, faint source, and it
      is what makes a moonlit ambient blue-grey).

## ⚠️ Traps

- ⚠️ Two shadowing directional lights double the shadow cost at dusk. A moon light without shadows
  by default is the likely answer: ask the owner.
- ⚠️ `sun-unlit-without-shadow-mapping`: a CSM directional light adds no light at all when shadow
  mapping is off. The moon light inherits that defect until it is fixed.

## References

- `dynamic-sky-atmosphere`, `celestial-clock`, `dynamic-sky-stars-and-milky-way`.
- `src/Graphics/CelestialBody.hpp`, `src/Scenes/Scene.lighting.cpp` (`applyBackgroundLightingNow`).
