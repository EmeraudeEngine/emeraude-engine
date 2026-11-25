---
id: water-world-seabed-bright-dotted-seam
title: A bright dotted line across the seabed of water-world — a seam of the terrain
status: open
priority: high
scope: Graphics (terrain geometry or its shading — not located)
opened: 2026-11-25
tags: [terrain, defect, water-world]
---

# A bright dotted line across the seabed of water-world — a seam of the terrain

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

## What remains

- Locate it: which geometry meets there (two terrain tiles, two levels of detail, a T-junction), and what shows
  through (the sky's colour, the water volume's, a back face). A RenderDoc capture at the pose answers both.
- Not looked at: macOS, other poses, other terrains (`water-world` is the only scene checked).
