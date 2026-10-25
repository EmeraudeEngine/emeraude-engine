---
id: celestial-clock-real-date-and-place
title: The celestial clock at a real date and place — UTC, longitude, the engine's birthplace as the default position
status: open
priority: unranked
scope: Scenes/Component (CelestialClock), emeraude-base Math/Astronomy, Identification (engine constants), PlatformSpecific (an approximate position)
opened: 2026-10-25
tags: [sky, astronomy, clock, time, geolocation, design]
---

# The celestial clock at a real date and place

## Why

The owner's plan (2026-10-25), after the stars lot: a sky generated for a time of day AND a position on the planet,
able to show the real sky of the moment — the sun, the Moon in its real phase, the planets, the stars — wherever the
application runs. The first consumer is projet-alpha's WaterWorld (its item `waterworld-dynamic-sky`).

Today `CelestialClock` takes a latitude, days since the vernal equinox and a LOCAL SOLAR time: no longitude, no
calendar, and the Moon's default orbit is "full at the equinox" (an arbitrary phase). Its astronomy is generic
Keplerian (emeraude-base `Math/Astronomy.hpp`: the Sun within ~1°, the Moon within a few degrees of Meeus).

## What remains

- **A real instant**: the clock set from a UTC time (the system clock), converted to the model's epoch (the vernal
  equinox of the year, J2000-based), the local solar time from the LONGITUDE (+ the equation of time if the model's day
  is the mean one), the local sidereal time for the stars from Greenwich's.
- **The Moon's real phase and position** at that instant (its elements at J2000, mean elongation): a full moon on the
  calendar's full moon. Measure the model against a reference ephemeris (Meeus, or JPL Horizons) for a few dates.
- **The engine's birthplace as identity constants** (owner, 2026-10-25): Liège, Belgium, latitude
  50.639311006498865°, longitude 5.568172430592806° — where this system was born; the default position when none is
  known. Their home is to decide (`EmEn::Identification` holds the engine's identity constants today).
- **An approximate position of the machine** — an owner decision between:
  - the coordinates of the system's TIME ZONE (the IANA `zone.tab` gives each zone's reference city: offline, no
    permission, accurate to a few hundred km);
  - the operating system's location service (Windows.Devices.Geolocation, CoreLocation, GeoClue): precise, but a
    permission prompt and three platform implementations;
  - an IP geolocation service: the network, a third party, privacy.
  Without any, the birthplace.
- **Observation windows and GoTo** (owner, 2026-10-25, for WaterWorld's Mars): from the clock's ephemeris, the dates
  and hours a body is above the horizon at the observer's place with the sun down (below −12°: a dark sky), with its
  apparent size, the best first; and a GoTo that aims a camera at a named body and sets the focal length that frames
  it. Console commands first (the MCP tools follow), a demo binds keys to them.
- **Time running at the real rate** (`timeScale` 1, the clock re-synchronised to the system clock rather than
  accumulated — the no-accumulation rule) and an acceleration a consumer can switch (WaterWorld: ×10 while a key is
  held).

## ⚠️ Traps

- Seconds since the epoch stay in DOUBLE (base caution point § Math/Astronomy).
- Longitude sign: east positive (the birthplace is 5.57° EAST).
- A time zone is not a longitude: civil time differs from solar time by up to ~2 h (Spain, western China); the
  position, not the time zone offset, places the sun.

## References

- Engine graphics doc 42 (the physical sky, § "The moons"), scenes doc 02 (`CelestialClock`); base caution points
  § Math/Astronomy.
- J. Meeus, *Astronomical Algorithms*, 2nd ed., 1998 (sidereal time, the Moon's position).
