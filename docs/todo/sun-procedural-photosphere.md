---
id: sun-procedural-photosphere
title: The sun's disc — a procedural photosphere (granulation, sunspots) over the limb darkening
status: open
priority: unranked
scope: Graphics/Material (SkyAtmosphereResource, the star's disc)
opened: 2026-10-25
tags: [sky, sun, design]
---

# The sun's disc — a procedural photosphere

## Why

The owner chose (2026-10-25) a PROCEDURAL photosphere for the Sun: no colour map of the real photosphere exists (SDO
HMI continuum data are Carrington synoptic maps on a sine-latitude grid, nearly uniform but for the sunspots). Today the
star's disc is its mean radiance shaped by a linear limb darkening (graphics doc 42 § "The star's disc"): uniform at a
long lens, where a filtered telescope shows granulation and spots.

## What remains

- Granulation from measurements: cells ~1 000 km (~1.4″ from Earth), lifetimes ~5–10 min, a contrast of a few % in
  the visible; animated with the clock's time (never accumulated).
- Sunspots: umbra / penumbra (~0.2–0.3 / ~0.8 of the photosphere), with a count following a solar-cycle parameter;
  faculae brighter near the limb.
- The disc's mean radiance unchanged (the light stays the star's illuminance): the pattern redistributes, it adds
  nothing.
- Visible only through a long lens with the star disc not capped (`StarDiscCeiling`) — a camera "solar filter"
  (a neutral density) to look at it, an exposure decision for the owner.

## References

- Graphics doc 42 § "The star's disc"; Stix, *The Sun: An Introduction* (2004) for the granulation and spot figures.
