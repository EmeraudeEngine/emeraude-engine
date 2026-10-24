---
id: dynamic-sky-moon
title: Dynamic sky — moons drawn with their phase, and the moonlight as the night's directional light
status: open
priority: unranked
scope: Graphics/Renderable (DynamicSkyResource), Graphics (CelestialBody), Scenes/Component (directional light, celestial clock)
opened: 2026-10-24
tags: [sky, moon, night, lighting, photometry, design]
---

# Dynamic sky — moons drawn with their phase, and the moonlight as the night's directional light

## Why

A night under the full moon is not a night without it: about 0.25 lx (`CelestialBody.hpp`
references), enough to cast shadows. A dynamic sky must show the moon where the clock puts it,
with the phase its angle to the sun gives, and light the scene with it.

> **Done 2026-10-24 with the celestial clock** (owner: "with the light"): every moon's direction, illuminated
> fraction and LIGHT — a directional light per moon, no shadow map, full-moon illuminance (0.3 lx above the
> atmosphere) × a Lambert-sphere phase function × the extinction, on while the moon is up and the sun down
> (`Scenes::Component::CelestialClock`, scenes doc 02). What remains is the DISC and the IBL.

## What remains

- [ ] **The disc**: a texture (albedo, optional normal map), lit by the SUN's direction, so the
      phase and the terminator come out of the geometry. No phase table. Add earthshine (the
      dark side faintly lit by the planet) as an artistic gain.
- [ ] The Moon's opposition surge if the Lambert sphere reads too bright at quarter (~0.32 against ~0.1 measured
      on the Moon). Its light already goes through the dynamic sky's transmittance (lot 2, 2026-10-25: graphics
      doc 42 § "The light").
- [ ] The moon in the IBL bake: included, unlike the sun (it is an extended, faint source, and it
      is what makes a moonlit ambient blue-grey).

## ⚠️ Traps

- ⚠️ Two shadowing directional lights double the shadow cost at dusk: the clock's moon lights carry NO shadow map
  (`Toolkit::generateCelestialClock()`); a shadowed moon would be an option to add.
- ⚠️ `sun-unlit-without-shadow-mapping`: a CSM directional light adds no light at all when shadow
  mapping is off. The moon light inherits that defect until it is fixed.

## References

- the physical sky, lot 1 (graphics doc 42, `42-physical-sky-atmosphere.md`), `dynamic-sky-stars-and-milky-way`; the clock: `Scenes::Component::CelestialClock` (scenes doc 02).
- `src/Graphics/CelestialBody.hpp`, `src/Scenes/Scene.lighting.cpp` (`applyBackgroundLightingNow`).
