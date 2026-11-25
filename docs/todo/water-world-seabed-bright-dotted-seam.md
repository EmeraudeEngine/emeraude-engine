---
id: water-world-seabed-bright-dotted-seam
title: A bright dotted line across the seabed of water-world — a skirt of the detail window lit twice
status: open
priority: high
scope: Saphir/Generator (MeshShadingSurfaceHelper), Vulkan/GraphicsPipeline (the light passes' depth state)
opened: 2026-11-25
tags: [terrain, defect, water-world]
---

# A bright dotted line across the seabed of water-world — a skirt of the detail window lit twice

## Why

Found by the Windows peer on 2026-11-25 while verifying the sea's light, then seen on Linux at the same place: a thin
line of single bright pixels, dotted, drawn as a V on the sand of the calm basin. It looks like a crack between two
parts of the terrain with something lighter showing through.

Facts:
- projet-alpha `water-world`, window-less 1280×720, exposure pinned f/11 – 1/250 s – ISO 100, the player at
  `setPosition(-401, -2.55, -189)`, `lookAt(-401, -3.4, -180)`: the line runs from about (1000, 357) down to
  (1170, 392), then up to the right edge near (1280, 378).
- Windows (RTX 3500 Ada): on the ray-traced lane and on the screen-space lane, the same pixels; with the sea's light
  on or off (`setSeaLight`). Linux (RTX 3070 Ti, ray-traced lane, the sea's light off): the same line.
- So it is neither a lane's effect nor the sea's light. It predates the caustics lot (not bisected).

## Located (2026-11-25, Linux, RTX 3070 Ti)

It is the border of the terrain's **mesh-shading detail window** (`Geometry::HeightfieldDetailSurfaceResource` over
`CDLODTerrainResource`, engine item mesh-shading-surface-on-heightfield): a probe build that never enables the window
(`CDLODTerrainResource::enableDetailWindow()`) draws the same pose without the line. Facts:
- `water-world`'s terrain: cells of 1.25 m, patches of 64 quads, detail distance 384 m, the window 2 × 2 quarters of
  40 m around the camera; the line is far inside the level-0 range, so the CDLOD's own level transitions are not in it.
- The line is fixed in the world while the camera moves by a few metres (the window is snapped to quarters), and its
  dots are REGULARLY spaced, closer with the distance: about one a cell.
- At the border a tile has one quad (the subdivision is 1 past the material's 5 m handover) and no skirt (skirts
  hang only toward a coarser tile, owner decision 2026-09-28).

Two probes that did NOT remove it (both reverted):
- reading the base height exactly at a lattice point in `msBaseHeight()` (a division rounded just under an integer
  would have interpolated instead of reading the corner);
- `invariant` on the clip position of every vertex stage and of the mesh stage (⚠️ whether the probe shaders were
  really the ones drawn was not proven: no shader failed, but the run's log names no recompiled pipeline).

## The cause (2026-11-25, a RenderDoc capture at the pose, then a probe)

**A skirt of the detail window is lit twice by the sun's additive pass.** Pixel history of three pixels of the line
(scene target, 1920 × 1080): the ambient pass of the mesh-shaded window writes one fragment and a second one FAILS
the depth test (0.9886910 against 0.9886830); the directional light pass of the same draw then writes TWO fragments,
both pass, and their light adds up (0.43262 then 0.87500 in red, where a pixel six rows below gets one fragment and
0.32666). A probe build without any skirt (`skirtEdges = 0`) draws the pose without the line.

Why both pass: a light pass tests `LESS_OR_EQUAL` under a slope-scaled depth bias of −1
(`Vulkan::GraphicsPipeline`, the additive passes), so its fragments land a little nearer than what the ambient pass
stored — 4 × 10⁻⁵ here; the skirt's top row, which the ambient pass rejected by 8 × 10⁻⁶, is well inside that
margin. A skirt hangs where a tile meets a COARSER neighbour (the ring where the relief's subdivision changes with
the distance, hence a line fixed in the world and one dot a cell). It is the bright one-pixel line of 2026-09-28
again (item mesh-shading-surface-on-heightfield: skirts then hung on every edge): reduced to the coarser-neighbour
edges, not removed.

Neither the CDLOD's border nor a hole: the first reading of this item ("a crack with something showing through")
was wrong. The two earlier probes (the base height read at a lattice point, `invariant` clip positions) could not
have changed it.

## What remains

The fix, to decide with the owner:
- a skirt drawn by the ambient pass only (its fragments discarded in a light pass): it still plugs the crack, lit by
  the ambient alone;
- or no fragment lit twice in an additive pass, whatever the geometry (a stencil mark a light pass sets and tests);
- or the skirt's top lowered under the edge by more than the light passes' margin.
Then the proof at the pose (the pixel history: one fragment in the light pass), on the ray-traced and screen-space
lanes, and Windows. Not concerned: macOS (no mesh shading, no window).
