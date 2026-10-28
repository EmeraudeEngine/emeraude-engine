# TileWorld — square tiles on a grid, over a slope-bounded relief

`src/TileWorld/` (2026-10-28). An **OPTIONAL** way to build a scene (owner: "an option of the engine, not a
mandatory way of doing things"): nothing else in the engine depends on it, a scene that does not use it pays nothing.
The first consumer is projet-alpha's `city` demo (the concrete tiles live there, not here: the engine knows tiles,
connections, zones and relief — never "a road" or "a city").

## The pieces

| Header | Role |
|---|---|
| `Types.hpp` | `Side` (N = −Z, E = +X, S = +Z, W = −X), `Rotation` (quarter turns, clockwise seen from above), `NetworkMask` (one bit per network, named by the CONSUMER), `SlopeRule`, `SubRect` / `Zone` (an area on the 32 × 32 sub-grid, 0.5 m, a consumer `kind`, a facing side), `DefaultTileSize` 16 m. |
| `TileInterface.hpp` | A tile KIND: `name()`, `connections(side)`, `slopeRule()`, `zones()`, `build(context, sink)`. One shared instance per kind (no per-cell state; `build()` must not mutate it). `TileContext` gives the corner heights IN THE TILE'S OWN FRAME, the cell centre and the rotation; `worldPlanar()` turns an own-frame point into world X / Z (world-aligned texture coordinates, continuous across tiles whatever their rotations). `SurfaceSink` hands one `Shape` per surface slot. |
| `Autotile.hpp` | `autotile(sides)` → `{Topology, Rotation}`: the 4-bit autotile (16 cases, 6 shapes: isolated, dead end, straight, corner, T, cross), canonical orientations stated, `static_assert`-checked. |
| `Layout.hpp` | The N × M grid (≤ 512 per side), `place()`, `validate()` (every cell holds a tile; two sides sharing an edge carry the same mask; the border connects to nothing — every mismatch logged). Centred on the origin, a column along +X, a row along +Z. |
| `Relief.hpp` | The heights at the tile corners, (N+1) × (N+1), shared by the neighbours: the surface is continuous by construction. `generate()` (two octaves of the base Perlin noise, `PortableRandom`-shuffled: the same relief on the three OS), `flatten()`, `constrain()`, `solve()`, `setHeight()` (an editor's tool). |
| `Builder.hpp` | Surface slots (`declareSurface()`: name, material, contact), the plinth, zone overlays; `buildGeometry()` / `buildRegion()` (pure, no scene), `instantiate()` (one solid static entity per region and slot, colliding as its triangles). |

## Owner decisions (2026-10-28)

- Tile 16 m (256 m²), a 0.5 m sub-grid for the zones and the props (1 m first; 0.5 m once the owner chose a 7 m
  carriageway between two 4.5 m sidewalks: the kerbs fall on half metres).
- The tiles are NOT a ground: they carry their geometry; a scene may have no `GroundLevel`. The world stands on a
  **plinth**: a vertical skirt from the border tiles down to a base `depth` below the lowest corner, a low barrier
  (`barrierHeight`, `barrierThickness`) on the edge, a base facing down under every region.
- The slope rules are solved **globally**: each rule is a set of `|h_a − h_b| ≤ c` on the corners (every edge ≤
  `maxGrade × tileSize`; with an axis, the two edges across it ≤ `crossGrade × tileSize` — a straight road slopes
  along itself only, OpenTTD's rule). Cyclic projections onto these convex sets (L. G. Gubin, B. T. Polyak, E. V.
  Raik, 1967) — the flat relief meets every rule, so the sets intersect and the sweep converges; each projection
  moves both corners by half the excess (the mean height is kept); a fixed order (deterministic); at most 4096
  sweeps. The roads stay continuous, the terrain bends around them. Per-tile FOUNDATIONS (a raised tile, retaining
  walls) come back later for the built lots only.
- **Non-convergence**: `solve()` halves the amplitude and starts again, at most 4 times, then flattens — logged with
  the numbers; the world always builds.
- **A WYSIWYG editor later** (out of this scope): the API keeps it possible — a cell edit rebuilds its region only
  (`buildRegion()`), a corner height can be set by hand, a tile has a stable name (a saved layout).

## How it builds

- Regions of 8 × 8 cells (128 m): each region's tiles write in their own frame, the builder rotates and places them
  (exact quarter-turn matrices: no trigonometric error) and merges them per slot into one shape (`labyrinth`'s
  pattern: few, large static meshes). The entity sits at the region centre.
- Zone overlays (`setZoneOverlay(kind, slot)`): every zone of that kind gets sub-cell quads `ZoneOverlayLift` (3 cm)
  above the bilinear surface of its tile — a debug view, an editor's highlight.
- Winding: counter-clockwise around the normal (the base's convention); `addQuad()` picks the order from the facing.

## Measured (`city`, Linux, 3070 Ti, 2026-10-28, validation layers on)

32 × 32 tiles, seed 1, amplitude 12 m: 298 road tiles, 17 blocks; the relief converged in **76 sweeps** (one
attempt), heights −4.38 to +7.26 m; 64 static meshes over 16 regions (4 slots); 467 711 triangles in the view lists
at the spawn; **60 FPS**, 0 VUID, 0 `[Error]`.

## ⚠️ Traps

- A NEW engine top-level directory must be added to `EMERAUDE_DIRECTORIES` in `cmake/PrepareEngineSourceFiles.cmake`:
  `src/TileWorld` was silently not compiled until it was (no error — nothing used it yet).
- `Physics::BodyPhysicalProperties` is not copyable (an observable): a surface slot keeps its two contact scalars and
  the builder makes the properties at `instantiate()`.
- A tile's surface is the BILINEAR patch of its corners; the vertex normals are its analytic normals — smooth inside a
  tile, not across tiles whose slopes differ (a light crease at the edge).
