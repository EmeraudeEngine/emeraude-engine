---
id: celestial-clock
title: A celestial clock component — latitude, date and time place the sun, the moons and the celestial sphere
status: open
priority: unranked
scope: Scenes/Component (new CelestialClock, SunCourse), Scenes (Toolkit::generateSunCourse), Console (environment adapters)
opened: 2026-10-24
tags: [sky, time-of-day, astronomy, owner-decision, design]
---

# A celestial clock component — latitude, date and time place the sun, the moons and the celestial sphere

> **Owner decision (2026-10-24): option B**, a real celestial clock, rather than deriving the moon
> and the stars from `SunCourse`'s great circle. It must also work for **extraterrestrial skies**.

## Why

`SunCourse` moves the sun on a great circle at a constant speed, with a night as long as the day.
That cannot place a moon or a sky of stars consistently: the stars would turn once per SOLAR day,
the moon would have no rising time of its own, and there are no seasons and no latitude (no short
summer nights, no polar sky).

## What remains

- [ ] **The state**: a latitude, a date and a time of day, a time scale (1 = real time, 720 = a day
      in 2 minutes), pause and jump. Keep the same rule as `SunCourse`: an INTEGER cycle count
      recomputed from a stored reference, never a float accumulated cycle after cycle.
- [ ] **The model, planet-generic**: the planet's sidereal day, its axial tilt, its orbital period
      (the year) and the latitude give the sun's direction and the local sidereal time. Each moon is
      a simple orbit (period, inclination, phase). Earth's values are a PRESET, with the Moon's main
      terms (Meeus, *Astronomical Algorithms*) if the owner wants more than a circular orbit.
      Escalate the precision level: a generic Kepler model for every planet vs Meeus terms for
      Earth only.
- [ ] **The outputs**, every logic cycle: a direction toward each body (`CelestialBody`), the
      celestial sphere's rotation (pole + sidereal angle) for the stars, and the elevation
      getters the sky, the lights and the game read.
- [ ] **`SunCourse`'s place**: it becomes a CONSUMER (the clock gives the direction, `SunCourse`
      keeps the light's photometry), or a special case of the clock (tilt 0, equator). Decide
      with the owner; `Toolkit::generateSunCourse()` and every scene using it must keep working.
- [ ] **Console**: set and read the date, the time and the scale (the environment adapters,
      `src/Scenes/Component/EnvironmentConsoleAdapters.cpp`), so a test can jump to dusk.
- [ ] Unit tests: the sun at noon on the equinox at latitude φ reaches 90° − φ, the polar day and
      night, a sidereal day 3 min 56 s shorter than a solar day on Earth, the phases of the Moon.

## ⚠️ Traps

- ⚠️ **The engine's frame**: Y up, and `SunCourse`'s layout (it rises at +X and culminates toward
  +Z in the northern hemisphere). Keep the same mapping, or every scene's sun moves.
- ⚠️ **Float precision over a long date**: a Julian date in float loses seconds. Keep the time as
  integer cycles plus a reference epoch, and compute the angles in double before converting.

## References

- J. Meeus, *Astronomical Algorithms*, 2nd ed., 1998.
- `src/Scenes/Component/SunCourse.hpp`, `src/Scenes/Toolkit.cpp` (`generateSunCourse`).
- `dynamic-sky-atmosphere`, `dynamic-sky-stars-and-milky-way`, `dynamic-sky-moon`.
