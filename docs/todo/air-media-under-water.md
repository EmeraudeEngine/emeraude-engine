---
id: air-media-under-water
title: The air's media treat the part of a view path that lies under water as air
status: open
priority: high
scope: Graphics/Effects/Atmosphere
opened: 2026-11-24
tags: [water, ocean, participating-medium, fog, aerial-perspective, clouds]
---

# The air's media treat the part of a view path that lies under water as air

## Why

Found on rereading how the underwater view was integrated (2026-11-23): `Effects::Atmosphere::WaterVolume` composites
last, over whatever the air's effects did before it, and none of those reads the sea. With the eye under water:

- `AtmosphericFog` and `VolumetricScattering` (slot `Fog`) integrate the scene's `AtmosphericMedium` from the eye to
  the depth — through the water, as if it were air;
- `AerialPerspective` adds the physical sky's haze over that same distance;
- `VolumetricClouds` is composited at the depth of the surface behind it, the sea's underside included.

What is right: under water, the air begins at the sea surface. A seabed seen from under water has no air on its path;
what is seen through Snell's window has the air beyond the surface, not the water before it.

Owner decision (2026-11-24): left as it is in the underwater view's lot (`underwater-view`, decision 9); the frame's
state is published for when it is done (`FrameContext::seaImmersion`, graphics doc 13-12 § 44, part 2).

Priority `high` by the default rule (a wrong result), never ranked by the owner: no demo shows it today (below).

## What is measured

- ⚠️ Trap of the measurement: a physical sky follows the REAL clock — the first run, at night, gave black captures
  and a difference of exactly zero. Set the time first: `CelestialClock.setTimeOfDay(WaterWorldSun0,
  WaterWorldSunClock, 0.5)` (the entity's address is the node's, `WaterWorldSun0`, not the name given to the
  toolkit).
- `water-world`, default options (2026-11-24, Linux, RTX 3070 Ti, ray-traced lane): `PostProcess.getStatus` lists
  no occupant in `AerialPerspective`, `Clouds` nor `Fog` — none of the four runs there.
- `water-world` with its physical sky (`--demo-options 1000,0,1`, the clock set to midday: the sun at 18.7° on that
  date; same machine, pinned exposure, four underwater poses, three captures a state): the aerial perspective is in
  the chain. On against off, a channel's mean moves by −0.3 to +0.9 level of 255 — under the 1.5 to 7.6 between two
  captures of ONE state. Nothing to see: its haze over the few tens of metres a view reaches under water is small,
  and the water's own extinction then dims it.
- The slot `Fog` (a local medium, far denser than the sky's haze) and the clouds are NOT measured: no demo has one
  together with a sea that declares its water. That is where the wrong result would show.

## What remains

To decide with the owner before any code — the two options he was shown:

- **Cut them under water**: on a `Submerged` frame the four effects are not executed. Simple, costs nothing; the haze
  of what is seen through Snell's window goes too.
- **Exact per pixel**: each one counts only the part of the path beyond the surface. Right everywhere; four shaders to
  change and the surface's depth to give them (the depth from before the translucent pass tells the sea's underside
  from what lies behind it).

An `Astride` frame needs the per-pixel answer either way: only `WaterVolume` has it today.

A proof needs a scene that has both: an air medium (or a physical sky, or a cloud) and a sea that declares its water.

## References

- Graphics doc 13-12 § 44 (`44-watervolume-the-underwater-view.md`), § 12 (`12-atmosphericfog.md`).
- Engine item `underwater-view`.
