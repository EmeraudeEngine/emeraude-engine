---
id: ssgi-five-pixel-column-grid
title: SSGI draws a regular 5-pixel column grid over geometry at 1280x720
status: open
priority: high
scope: src/Graphics/Effects (SSGIEffect, the IndirectDiffuse screen-space lane)
opened: 2026-10-18
tags: [defect, post-process, ssgi, resolution]
---

# SSGI draws a regular 5-pixel column grid over geometry at 1280x720

## Why

Found by Window-PA (2026-10-18, JungleRuins option 1 — the USD split validation): a dense regular grid of thin
lines over all the geometry, not over the sky. Measured on its 1280x720 capture: vertical lines every 5 px in SCREEN
space (136 peaks, median spacing 5), barely lighter than their neighbours ((113,136,166) vs (111,134,165)); the
horizontal ones irregular (mostly geometry edges). Bisected slot by slot (`PostProcess.disable()`, the
column-to-column luma-step standard deviation over rows 400-700): 6.77 with everything on, 0.09 as soon as
`IndirectDiffuse` is off. `getStatus()` shows `IndirectDiffuse: SSGIEffect <- FALLBACK` (RTGI selected, "no main
directional light" in JungleRuins option 1), so the SCREEN-SPACE lane draws it.

On Linux (RTX 3070 Ti, 1920x1080, the same scene and the same SSGI fallback): no grid — step standard deviation
0.90 with SSGI, 1.02 without, no periodicity at 5 px (captures 1792355645 / 1792355660). So it depends on the machine
or the resolution: the Windows laptop runs an RTX 3500 Ada at 1280x720.

**RESOLUTION-dependent (Window-PA, the same day):** on the same GPU and driver at 1920x1055, SSGI on gives a step
standard deviation of 0.11 with no periodicity (autocorrelation 0.99-1.00 at lags 3-8), IndirectDiffuse off 0.06;
at 1280x720, 6.82 with the autocorrelation broken to 0.37-0.41 at lags 4-7. 1280x720 is projet-alpha's DEFAULT window
size. Suspects: a tile or thread-group size, a half-resolution buffer whose size does not divide evenly at 1280 (640
against an 8- or 16-wide tile), a sample pattern keyed on the pixel coordinate.

## What remains

- Confirm cross-platform: SSGI on Linux at 1280x720 (a resize crashes on Linux — engine item on the resolution change;
  launch at that size instead, the owner's screen permitting).
- Read SSGIEffect for a per-column or per-tile pattern tied to the resolution (a half-resolution pass, a tile size, a
  jitter or noise texture period) — 5 px at 1280 wide.
- Verify: the grid gone on Windows (step standard deviation near the SSGI-off value), Linux unchanged.

## References

- `PostProcess.getStatus()`, `PostProcess.disable(slot)` (engine `docs/ai-runtime-control.md`).
- Window-PA's captures usd-grid2-s*.png and usd-grid-ssgi-ab.png (its scratchpad), Linux 1792355645 / 1792355660.
