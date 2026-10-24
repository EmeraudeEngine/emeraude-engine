---
id: dynamic-sky-stars-and-milky-way
title: Dynamic sky — a night sky of catalogue stars and a Milky Way, physical with artistic controls
status: open
priority: unranked
scope: Graphics/Renderable (DynamicSkyResource), Saphir (background program), Resources (star catalogue, Milky Way cubemap)
opened: 2026-10-24
tags: [sky, stars, night, photometry, owner-decision, design]
---

# Dynamic sky — a night sky of catalogue stars and a Milky Way, physical with artistic controls

> **Owner decision (2026-10-24):** BOTH sources, a star catalogue for the sharp stars and a
> cubemap for the diffuse Milky Way, and **an artistic possibility** on top of the physics.

## Why

A dynamic sky must be starry at night. Done physically, the stars are additive and ALWAYS drawn:
they vanish by day because the sky is ~1e6 times brighter, then reappear one by one at dusk with no
scripted fade. This is the correct behaviour, and it costs nothing.

## What remains

- [ ] **Catalogue stars**: e.g. the Yale Bright Star Catalogue (~9 100 stars down to magnitude
      6.5), as right ascension / declination, visual magnitude and B-V colour index. Magnitude →
      illuminance (`E = 10^(-0.4·(m + 14.18))` lux), B-V → colour temperature → the engine's
      chromaticity. Drawn as points or small sprites, sized for the pixel, and attenuated by the
      atmosphere's transmittance (they dim and redden near the horizon).
- [ ] **The Milky Way**: an HDR cubemap in equatorial coordinates, in the same frame as the
      catalogue so both rotate together.
- [ ] **The rotation**: the celestial sphere turns around the pole at the sidereal rate, from the
      clock's local sidereal angle and celestial pole (`CelestialClock::siderealAngle()`, `celestialPole()`, done 2026-10-24).
- [ ] **Alien skies**: the catalogue and the cubemap are DATA. A generated star field (seeded,
      with a magnitude distribution) for a fictional sky, and the pole's direction from the
      planet's axis.
- [ ] **Artistic controls** (default neutral): a star brightness gain, a limiting magnitude (how
      many stars), a Milky Way gain and tint, a twinkle amount, and a **night lift**. The night
      lift is a floor on the night sky's luminance so a game night stays readable (see Traps).
- [ ] **Twinkle (scintillation)**: a small temporal noise on the stars, stronger near the horizon
      (longer air path). It must be stable under TAA (not a per-frame white noise).
- [ ] **Licence check** of the chosen catalogue and Milky Way picture, recorded in
      `docs/third-party-notices.md`.

## ⚠️ Traps

- ⚠️ **A physical night is close to black**: a moonless sky is ~1e-3 nits and the auto-exposure
  multiplier stops at `maxExposure` 100 (`ToneMapping.hpp:122`). The artistic night lift, a raised
  `maxExposure`, or both are an owner choice. Measure before choosing.
- ⚠️ **A star is under a pixel**: drawn as a raw point, it flickers with TAA jitter and disappears
  under the upscaler. Spread its energy over a fixed footprint (a pixel-sized Gaussian) and keep
  the total energy.
- ⚠️ The stars must not enter the IBL bake at full weight: thousands of points are noise in the
  prefiltered mips. Their total contribution is negligible anyway (~1e-3 lx).

## References

- Yale Bright Star Catalogue, 5th revised edition (Hoffleit & Warren, 1991).
- the physical sky, lot 1 (graphics doc 42, `42-physical-sky-atmosphere.md`), `dynamic-sky-moon`; the clock: `Scenes::Component::CelestialClock` (scenes doc 02).
