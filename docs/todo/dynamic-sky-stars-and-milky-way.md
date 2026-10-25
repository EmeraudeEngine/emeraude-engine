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
- [ ] **The planets** (owner, 2026-10-25: "je veux Mars" — zoom from WaterWorld onto Mars at night and see its
      details). Each planet is a sky body like the moons (graphics doc 42 § "The moons": analytic disc, surface map,
      the star's light, the atmosphere's transmittance): its direction from heliocentric Keplerian orbits (the
      planet's minus the observer's planet), its apparent diameter from the distance (Mars 4″–25″, Jupiter ~47″,
      Venus up to 66″ in crescent), its phase angle and phase law, its brightness from its geometric albedo. At a
      normal focal length a planet is far below a pixel: drawn as a POINT with the stars' footprint (energy kept, TAA
      stable), the same shader resolving into the textured disc as the focal length grows (~18 m for a 100-px Mars).
      Maps: projet-alpha's data store `Astronomy/<Body>{2K,4K,8K}` (Mars recoloured from Viking, owner decision
      2026-10-25). Saturn's rings: the Voyager PPS optical-depth profile. Optional: atmospheric seeing (~1″).
      Owner, 2026-10-25: EVERY planet visible in the sky first; options to enable the surface textures come later; the
      8K maps are kept for a space game (bodies seen up close), useless from the ground. Design points: the memory
      (~30 maps: ~330 MB of VRAM in 2K, ~5 GB in 8K — a resolution per body or by apparent size) and the cost per sky
      pixel (today each moon is evaluated on every sky pixel, 4 at most: a cheap proximity test before the disc).
- [ ] **Artistic controls** (default neutral): a star brightness gain, a limiting magnitude (how
      many stars), a Milky Way gain and tint, a twinkle amount, and a **night lift**. The night
      lift is a floor on the night sky's luminance so a game night stays readable (see Traps).
- [ ] **The night sky's own glow** (owner, 2026-10-25: the night lift should be PHYSICAL first, artistic on top). A
      moonless zenith is ~21.5–22 mag/arcsec² (V), ~2e-4 cd/m², and the sun's scattering gives none of it below −18°:
      it is airglow (the upper atmosphere's chemiluminescence, the green 557.7 nm oxygen line first), zodiacal light
      and the integrated starlight (Leinert et al., "The 1997 reference of diffuse night sky brightness", A&AS 127,
      1998). Model: the airglow as a thin emitting layer, brighter toward the horizon by the van Rhijn path factor and
      seen through the transmittance; its zenith luminance and colour are PLANET DATA (Earth's measured; another
      planet's its own); the starlight comes from the catalogue and the Milky Way below. The sun's own twilight is
      measured (graphics doc 42 § "The night", 2026-10-25): 7.6e-6 cd/m² at −18°, far under the ~2e-4 floor.
- [ ] **Realistic or artistic, by parameters only** (owner, 2026-10-25): every term above has a gain and a tint,
      neutral by default (the physical sky), exposed in the manifest and the console like the sky's grading. A
      realistic sky is the defaults; an artistic one changes numbers, never code.
- [ ] **Twinkle (scintillation)**: a small temporal noise on the stars, stronger near the horizon
      (longer air path). It must be stable under TAA (not a per-frame white noise).
- [ ] **Licence check** of the chosen catalogue and Milky Way picture, recorded in
      `docs/third-party-notices.md`.

## ⚠️ Traps

- ⚠️ **A physical night is close to black**: a moonless sky is ~2e-4 nits and the auto-exposure
  multiplier stops at `maxExposure` 100 (`ToneMapping.hpp:122`). The artistic night lift, a raised
  `maxExposure`, or both are an owner choice. Measure before choosing.
- ⚠️ **A star is under a pixel**: drawn as a raw point, it flickers with TAA jitter and disappears
  under the upscaler. Spread its energy over a fixed footprint (a pixel-sized Gaussian) and keep
  the total energy.
- ⚠️ The stars must not enter the IBL bake at full weight: thousands of points are noise in the
  prefiltered mips. Their total contribution is negligible anyway (~1e-3 lx).

## References

- Yale Bright Star Catalogue, 5th revised edition (Hoffleit & Warren, 1991).
- the physical sky, lot 1 (graphics doc 42, `42-physical-sky-atmosphere.md`; the moons: § "The moons"); the clock: `Scenes::Component::CelestialClock` (scenes doc 02).
