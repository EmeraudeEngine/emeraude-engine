---
id: ocean-sheltered-water
title: An ocean is as rough in a basin enclosed by land as in the open sea
status: in-progress
priority: unranked
scope: Graphics/Geometry, Saphir/Generator, Scenes
opened: 2026-11-24
tags: [water, ocean, terrain, waves]
---

# An ocean is as rough in a basin enclosed by land as in the open sea

## Why

Owner (2026-11-24, on `water-world`'s archipelago): the islands are wide, and in their middle hollows go under the sea
level. The ocean's plane shows there as a small lake — as rough as the open sea. "Il n'y a finalement aucune force
physique qui peut faire bouger ce niveau d'eau conscrit dans une île. […] Comme une espèce de map qui indique à quel
endroit la mer bouge et à d'autres où ça reste calme ?"

The ocean (`Renderable::OceanResource`) is one infinite plane displaced by the same FFT cascades everywhere: nothing
tells it where the land shelters the water.

## Owner decisions (2026-11-24, AskUserQuestion)

1. **The engine makes the map, from the relief.** When it loads, it reads the ground's heights, marks what is under
   the sea level and spreads from the map's edge: a basin the spread does not reach is enclosed, hence calm. A new
   contract: the scene presents its ground to its sea. (Not taken: an image the application provides; both.)
2. **Calm water keeps small ripples**: the swell goes, the smallest waves stay, much attenuated — a pond under the same
   wind. How much stays is tuned on screen with him. (Not taken: a perfect mirror.)
3. **A basin joined to the open sea by a narrow passage stays calm**: the swell does not enter a passage narrower
   than a threshold — 30 m to start with, a parameter. (Not taken: rough as soon as joined; a gradual shelter
   everywhere, which would also lower the waves along every coast and change the beaches he approved.)
4. **One source for where the surface stands**: the ocean's shader helper becomes the only description of the
   displaced surface, used by the ocean AND by the underwater view's waterline — the map enters it once. This closes
   the debt recorded in `underwater-view` (the moving variant of `WaterVolume` restated the ocean's mesh).

## Defaults stated to the owner

- The map's computation is a pure grid algorithm: in emeraude-base, with its unit tests (yesterday's precedent,
  `VertexFactory::buildNestedLattice()`).
- One texture of one channel, 1024 × 1024 (1 MB), over the scene's boundary.

## Progress (2026-11-24, Linux, RTX 3070 Ti — NOT committed, the other OS not run)

Built as decided, documented in graphics doc 44 (`docs/subsystems/graphics/44-the-oceans-sheltered-water.md`): base
`VertexFactory::buildShelterMap()` with its ten unit tests, the contract `SeaLevelInterface::adaptToGround()` called by
the scene, `OceanResource` building the map and its surface uploading it (binding 5), every read of the cascades
weighed through `ocCascadeWeight()`, and the underwater view compiling the ocean's own `ocSurfaceRiseAt()`.

Measured on `water-world`: 15 381 m² of sheltered water found in 54 ms; a sheltered basin a hundred times stiller than
the open sea (temporal standard deviation of a pixel 0.17 against 15.9); the open sea, the waterline and the sun's
shafts unchanged; 0 VUID.

## What remains

- **The look, with the owner on screen**: how much of the small waves a sheltered water keeps
  (`OceanSurfaceParameters::shelteredWaveShares`, first values 0 / 0.04 / 0.35 for the 512 m, 64 m and 8 m cascades);
  the 30 m passage and the 25 m fade. ⚠️ Seen from afar a sheltered water is a mirror: the cascade that keeps a
  share is faded by the pixel's footprint like everywhere, its ripples show at close range only. No command changes
  the shares on a running scene: one is to be proposed if the tuning needs it.
- The shore foam still outlines a sheltered pond in white (it follows the water column): his to judge.
- The map's cost in the ocean's own stages (one more fetch a read of the cascades): not measured.
- AMD. **Verified on the three OS (2026-11-24, the peers, on base 7824170 / engine 94632582 / projet-alpha 7c172234).**
  macOS (Apple M6, MoltenVK, AppleClang 21; the screen-space lane alone) and Windows (RTX 3500 Ada, MSVC 14.44 /WX; both
  lanes): the cascade in Release and Debug, 0 error, 0 warning; the base suite as expected (2867 and 2864 ran, 2860
  passed on each); `water-world` with the validation layer loaded, 0 VUID, no shader that fails to compile. The log's
  shelter line gives the SAME areas on the three OS — 15 381.2 m² of 5.90067e+06 m² — in 39 to 44 ms on the M6 and
  166 ms on Windows. The five poses show what Linux shows: the veil and the surface from below, Snell's window, a
  basin that does not move (mean difference between two frames a second apart: 0.26 to 0.34 on the pond, 4 to 16 on
  the open sea), a straight waterline in the pond. ⚠️ Windows caught no frame cut by the waterline in its twelve (each
  was all air or all water): the cut along the mesh is seen on Linux and macOS only. ⚠️ On the M6 the water volume
  joins the chain about 10 s after the scene is up (the sea answers its water once it is ready for instantiation).
  NOT run: AMD (no session on the AMD laptop that day). `gate.py feature` ran on Linux the same day (the Debug build, the base suite under ASan /
  UBSan and TSan, 0 report).
