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

**Done 2026-10-26** (WaterWorld lot): the machine's approximate place from its time zone
(`PlatformSpecific::approximateLocation()`, platformspecific doc 20; `CelestialClock::observerPlace()`; console
`useSystemLocation`), the observation windows (`findObservationWindows()`, console too), the GoTo in WaterWorld (N: aim,
frame, track; projet-alpha builtin doc 16-14).

- **A GoTo for every demo** — a console command (then the MCP tool) that aims the ACTIVE camera at a named body and frames
  it, tracking it while on: WaterWorld's N does it from the demo (aim through the player, focal length from the disc,
  re-aim every 16 ms); the engine has no camera-to-body command yet.
- **The real rate re-synchronised to the system clock**: the clock advances by logic cycles × the time scale (exact while
  the logic loop keeps its rate); a long stall would drift it from the wall clock. A `followSystemClock` option (re-anchor
  on the system time at the real rate) if a demo must stay on the present for hours.
- **The Windows time zone path** (`SystemLocation.windows.cpp`) is not compiled on Linux: the Windows peer's validation.

## ⚠️ Traps

- Seconds since the epoch stay in DOUBLE (base caution point § Math/Astronomy).
- Longitude sign: east positive (the birthplace is 5.57° EAST).
- A time zone is not a longitude: civil time differs from solar time by up to ~2 h (Spain, western China); the
  position, not the time zone offset, places the sun.

## References

- Engine graphics doc 42 (the physical sky, § "The moons"), scenes doc 02 (`CelestialClock`); base caution points
  § Math/Astronomy.
- J. Meeus, *Astronomical Algorithms*, 2nd ed., 1998 (sidereal time, the Moon's position).
