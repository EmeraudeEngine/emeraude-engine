---
id: dynamic-sky-moonlit-sky-glow
title: Dynamic sky — the atmosphere scatters the moons' light (the moonlit sky)
status: open
priority: unranked
scope: Graphics (SkyAtmosphere sky-view pass, AtmosphereGLSL), Graphics/Material (SkyAtmosphereResource), the sky's IBL
opened: 2026-10-25
tags: [sky, moon, night, atmosphere, ibl, photometry]
---

# Dynamic sky — the atmosphere scatters the moons' light (the moonlit sky)

## Why

The sky-view pass integrates the scattering of the STAR only. Under a full moon the real sky is not black: the air
scatters the moonlight like daylight, ~1e-6 of it (0.25 lx against ~1e5 lx): a dark blue sky, the daylight sky lit
by the moon (Jensen et al., *A Physically-Based Night Sky Model*, SIGGRAPH 2001, model it so), slightly redder since
the Moon's albedo rises toward the red. The engine draws the moon's disc and
lights the scene with it (graphics doc 42 § "The moons"), but the vault around it stays at the sun's own twilight floor,
and the sky's IBL — what makes a moonlit ambient blue-grey — gets nothing from the moon (its disc is a light face, left
out of the capture on purpose).

## What remains

- The sky-view table holds radiance PER LUX of the star, for the star's direction. A moon needs its own term: its
  illuminance × the same per-lux scattering seen from its direction. Options to weigh (owner): a second sky-view
  table per moon (one more dispatch per frame), or the moon's term read from the star's table at the moon's relative
  azimuth (the table is symmetric about the star's vertical plane only if the light's zenith angle matches — it does
  not: a lookup in a table parametrised by the light's zenith is needed).
- The aerial perspective's in-scattering under the moon (the same question for the froxel volume).
- Measure against a reference: the zenith luminance under a full moon at 45° against the daylight zenith scaled by the
  moon's illuminance over the sun's.

## ⚠️ Traps

- The moon's illuminance is ~1e-6 of the sun's: the tables are RGBA32F for that reason (graphics doc 42 § "The model":
  half floats flushed the night to zero).
- The moonlit sky must not double count the moon's disc: the disc stays a light face, out of the IBL.

## References

- Graphics doc 42 § "The moons", § "The environment", § "The night"; `SkyAtmosphere::recordFrame()`.
- H. W. Jensen, F. Durand, J. Dorsey, M. M. Stark, P. Shirley, S. Premože, SIGGRAPH 2001.
