---
id: celestial-clock-real-date-and-place
title: The celestial clock at a real date and place — UTC, longitude, the engine's birthplace as the default position
status: in-progress
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

The planet model (`TimeModel::Planet`) keeps a latitude, days since the vernal equinox and a local solar time, with
generic Keplerian astronomy (emeraude-base `Math/Astronomy.hpp`); the calendar (`TimeModel::Calendar`, below) is the
real sky.

## What remains

**Done 2026-10-26** (lot S3a; scenes doc 02 § CelestialClock "The calendar", base caution points
§ `Math/SolarSystem.hpp`): the calendar time model (a UT Julian day, the longitude, the sidereal time, the Sun and the
Moon from the ephemerides with their parallax, the precessed star frame), `setCalendarDate` / `setNow` / `setLongitude`,
the birthplace in `Identification::Birthplace*`. Owner decisions 2026-10-26: the real date before the planets; the Moon
from Schlyter's perturbations (measured up to 3.5′ from PyEphem — a truncated ELP-2000/82 if ever needed).

- **An approximate position of the machine** — owner decision 2026-10-26: the system's TIME ZONE → its reference city
  (the IANA `zone.tab`: offline, no permission, a few hundred km; Linux/macOS from the TZ / `/etc/localtime` link,
  Windows from its zone id through the CLDR `windowsZones` mapping). Without one, the birthplace.
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
