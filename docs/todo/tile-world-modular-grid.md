---
id: tile-world-modular-grid
title: TileWorld — a modular grid of square tiles with a slope-bounded corner relief, merged into static meshes
status: in-progress
priority: unranked
scope: engine (new module TileWorld)
opened: 2026-10-28
tags: [procedural, geometry, physics, terrain]
---

# TileWorld — a modular grid of square tiles with a slope-bounded corner relief, merged into static meshes

## Why

Owner, 2026-10-28: a "Lego-like" city builder, improvable later — square tiles that assemble on a grid, each tile a
C++ class describing its possibilities through an interface. The GENERIC part lives in the engine (owner decision:
engine + projet-alpha); the concrete city tiles and the `city` demo live in projet-alpha (item
`city-demo-tile-builder` there). The engine module knows nothing of "a city": tiles, connections, zones, relief.

## Owner decisions (2026-10-28)

- **An OPTION of the engine, never an obligation**: one more way to build a scene, beside the grounds, the
  terrain, the loaders and the hand-built geometry. Nothing else in the engine depends on it, nothing requires it;
  a scene that does not use it pays nothing for it.
- **Tile size 16 m** (256 m²); a 0.5 m sub-grid inside a tile for zones and props (1 m first; 0.5 m since the
  owner chose a 7 m carriageway between two 4.5 m sidewalks — the kerbs fall on half metres).
- **The tiles are NOT the ground**: they carry their own geometry (render + collision); a scene may have no
  `GroundLevel` at all.
- **Relief = heights at the tile corners** ((N+1) × (N+1), shared by neighbours: continuous by construction), each
  tile type bounding its slope; a road slopes only along its own axis (OpenTTD's rule).
- **The rules are solved GLOBALLY** (owner, 2026-10-28, replacing the per-tile foundations first proposed): cyclic
  projections onto the convex sets `|h_a − h_b| ≤ c` (Gubin, Polyak, Raik 1967) — the roads stay continuous, the
  terrain bends around them. Per-tile foundations (a raised tile and its retaining walls) come back later for the
  built lots only.
- **Non-convergence** within the sweep bound: the relief amplitude is halved and the solve rerun, at most 4 times,
  then the relief is flat — logged with the numbers; the world always builds.
- **The city stands on a plinth**: a vertical skirt from the tiles' outer edge down to a base, and a low barrier.
- **Connections by a 4-bit mask per side** (autotiling): the generator states "road here", the topology and the
  rotation are deduced (16 cases).
- **A WYSIWYG editor later** (owner, 2026-10-28, out of this scope): the API must not preclude it — a cell edit
  rebuilds its region only (`Builder::buildRegion()`), a corner height can be set by hand (`Relief::setHeight()`),
  a tile is identified by a stable name (`TileInterface::name()`, for a saved layout).

## What remains (step 1 of 3: the tile system)

- `TileInterface`: per-side connection masks (one bit per network, road first), slope rule, zones on the 1 m
  sub-grid (kind + rectangle + facing), `build()` emitting the tile's geometry in its own frame.
- `Layout`: the N × M grid (tile + rotation), validated (neighbouring sides must agree; refused and logged otherwise).
- `Relief`: seeded corner heights (the base Perlin noise, `PortableRandom`-shuffled), then the tiles' constraints
  solved globally, deterministically and in bounded time; the halve-and-retry fallback (`Relief::solve()`).
- `Builder`: merges per region of 8 × 8 tiles into static, solid triangle meshes (the `labyrinth` pattern), plus
  the plinth.
- The public headers are submitted to the owner BEFORE the implementation.
- Steps 2 (road details: kerbs, markings, camber) and 3 (props, buildings) follow, each on the owner's go.

## References

- OpenTTD, corner heights and road foundations: https://docs.openttd.org/source/d1/d01/landscape_8h.html ,
  https://newgrf-specs.tt-wiki.net/wiki/NML:List_of_tile_slopes
- Autotiling bitmasks: https://www.redblobgames.com/articles/autotile/claude/
- Street grades (local streets usually capped at 10-12 %, 6 % desired): municipal standards, e.g.
  https://reno.gov/home/showpublisheddocument/729/635219272179470000
- Later generators: Y. Parish, P. Müller, "Procedural Modeling of Cities", SIGGRAPH 2001; M. Gumin, Wave Function
  Collapse (2016).
