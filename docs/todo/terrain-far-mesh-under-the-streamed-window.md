---
id: terrain-far-mesh-under-the-streamed-window
title: A coarse mesh of the whole terrain around the streamed window, so its edge is never a picture
status: open
priority: unranked
scope: Graphics/Renderable
opened: 2026-09-22
tags: [terrain, streaming, lod, cdlod]
---

# A coarse mesh of the whole terrain around the streamed window

## Why

`TerrainResource` draws ONE window of the grid (4096 m of side by default) out of a terrain that
is 16 km wide on the `terrain` demo. Whatever the slide margin, the edge of that window is at most
2048 m from the camera, and on a hilly terrain seen from any height the horizon is farther: **the
edge of the streamed window is a picture, not a corner case** — the owner saw "the limit of the
terrain in video memory" while flying (2026-09-22). The margin and the parallel generation make it
arrive later and go away faster; nothing in the current design removes it.

The remedy every large-terrain renderer uses is a NESTED coverage: a coarse mesh of the WHOLE
terrain (32-64 m cells → 262 144 to 65 536 vertices, ~15 MB at today's 56 B/vertex, or a few MB
packed) drawn around the fine window, with a hole under it — the clipmap rings of Losasso & Hoppe
2004, the outer quadtree levels of Strugar's CDLOD 2010.

Owner: "C'est une bonne idée" (2026-09-22).

## What remains

- [ ] Decide where it lives: a second geometry inside `TerrainResource` (a static
  `AdaptiveVertexGridResource` or `VertexGridResource` of the full grid at a coarse step, whose
  sectors overlapping the fine window are culled from the draw list — the hole moves with the
  window), or the outer levels of the CDLOD quadtree if option B lands first. ⚠️ The two surfaces
  must never be drawn on the same ground: the coarse one under the fine one z-fights and shows
  through on every slope.
- [ ] The seam between the coarse ring and the fine window: the fine window's outer sectors are at
  their coarsest level (step 128 on `terrain`) and the ring should match that step at the border,
  or stitch to it.
- [ ] The ray-tracing proxy already covers the whole grid at a budgeted step
  (`TerrainBLASMaxTriangles`): the ring changes nothing there.

## ⚠️ Traps

- The full grid at 1 m is 268 M points; the ring must be SUBSAMPLED from `m_localData` (every 32nd
  or 64th point), never extracted at full resolution and decimated.
- The fine window's normals are per-vertex at 1 m and read by every level (see
  `docs/caution-points.md` § Ray Tracing, the dark streaks): the ring's normals must be computed at
  ITS step or it inherits the same aliasing at a larger scale.

## References

- `src/Graphics/Renderable/TerrainResource.{hpp,cpp}` — the window, `updateVisibility()`.
- F. Strugar, *Continuous Distance-Dependent Level of Detail for Rendering Heightmaps*, 2010 — https://github.com/fstrugar/CDLOD
- F. Losasso, H. Hoppe, *Geometry Clipmaps: Terrain Rendering Using Nested Regular Grids*, SIGGRAPH 2004.
