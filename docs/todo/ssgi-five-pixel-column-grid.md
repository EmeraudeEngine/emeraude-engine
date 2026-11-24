---
id: ssgi-five-pixel-column-grid
title: SSGI draws a regular 5-pixel column grid over geometry at 1280x720
status: in-progress
priority: high
scope: src/Graphics/Effects (SSGI and SSAO, the screen-space lane)
opened: 2026-10-18
tags: [defect, post-process, ssgi, ssao, resolution]
---

# SSGI draws a regular 5-pixel column grid over geometry at 1280x720

## Why

Found by the Windows peer (2026-10-18, `jungle-ruins` option 1): a lattice of thin lines over all the geometry at
1280 × 720 — projet-alpha's default window — and not at 1920 × 1080. Reproduced on Linux on 2026-11-24 with the peer's
metric: the same grid on both machines (the std of the column profile's first difference over rows 400-700, auto
exposure: 0.856 on Windows and 0.969 on Linux with SSGI on, 0.105 and 0.124 with it off; periods 10.07 and 5.02 px).

## Done (2026-11-24, Linux only)

The cause, the rule and the measurements are in graphics doc 13-12 § 45
(`docs/subsystems/graphics/13-12-post-processing-effects/45-the-g-buffer-texel-of-a-reduced-pass.md`) and in
`docs/caution-points.md`: a half-resolution pass read the G-buffer on a texel corner. Owner decisions: one shared
helper (`Effects/Shared/GBufferTexelGLSL.hpp`), applied to SSGI and SSAO, the fixed first texel of the block.
On Linux the lattice is gone (far slope, column profile folded on 10 px: 1.573 → 0.009; SSAO alone: ×6.3 → ×1.0).

## What remains

- **The peers, after the push**: macOS (the screen-space lane) and Windows at 1280 × 720 on `jungle-ruins` option 1 —
  the std of the first difference back near its SSGI-off value, no 10 px / 5 px period on the far slope, 0 VUID, no
  shader that fails to compile (the two effects' GLSL is compiled at runtime: MoltenVK and the Windows drivers are
  its first other compilers). AMD when the laptop has a session.
- Then delete this item, and the line of `docs/subsystems/scenes-loaders/03-implemented-loaders/02-usdloader.md`
  that names it.

## ⚠️ Traps

- The per-pixel horizontal step does not see the grid; the column profile does. A spectrum over the whole band takes
  the scene's stepped pyramid for a residue (doc § 45, the two traps).

## References

- Engine item `reduced-passes-name-their-gbuffer-texel` (the other reduced passes).
