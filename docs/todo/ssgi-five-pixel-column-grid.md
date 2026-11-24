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

## Re-checked on both OS (2026-11-24, engine `develop` 74beadf5) — STILL THERE, on Linux too: it is the scene and the size, not the machine

Window-less at 1280 × 720, the screen-space lane selected (`IndirectDiffuse: SSGIEffect`), `IndirectDiffuse` on, off,
on again, a capture 5 s after each switch. The metric, rebuilt by the Windows peer and run identically on Linux: luma
of the 8-bit values over rows 400-700; P the column profile, D its first difference; **A = std(D)**; the spectrum of D
at a period over its median; the autocorrelation of D at lags 3 to 8.

| scene | machine | A, SSGI on / off / on | D's spectrum at 5 px, on / off | autocorrelation of D, on |
|---|---|---|---|---|
| `jungle-ruins --demo-options 1` | Windows, RTX 3500 Ada | **0.856 / 0.105 / 0.866** | ×38.3 / ×1.5 | −0.41 −0.15 −0.05 −0.15 −0.39 −0.31 |
| `jungle-ruins --demo-options 1` | Linux, RTX 3070 Ti | **0.969 / 0.124 / 0.958** | ×37.6 / ×1.5 | −0.38 −0.10 −0.02 −0.10 −0.36 −0.36 |
| `global-illumination` | Windows / Linux | 1.42 / 1.48 / 1.48 — 1.77 / 1.88 / 1.82 | ×1.4 / ×1.5 — ×1.4 / ×1.4 | flat (0.00) |
| `sponza` | Windows / Linux | 4.96 / 4.09 / 4.83 — 5.57 / 4.97 / 5.39 | ×2.2 / ×2.0 — ×1.9 / ×2.2 | no period |

- **The two machines give the same grid on `jungle-ruins` option 1**: the step is 8 times its SSGI-off value, the
  strongest periods are 10.07 px and 5.02 px (the fundamental and its harmonic), on both. The Windows captures also
  show lines across the whole width at irregular heights (y ≈ 225, 315, 365, 405, 455, 495, 545, 585, 635, 685).
- **It does not show on `global-illumination` nor on `sponza`, on either machine.** `jungle-ruins` option 1 renders
  grey-white and untextured-looking; whether the other scenes have no grid or hide it under their textures is not
  known.
- ⚠️ A first Linux check that day, on those two scenes only, was read as "it takes the Windows machine": wrong, and
  corrected here by the peer's remark. The 2026-10-18 Linux check found no grid at 1920 × 1080, where Windows had
  none either.
- The per-pixel step (the standard deviation of the horizontal step over the band: 1.5 to 1.8 on `jungle-ruins`,
  on as off) does not see the grid; A does.

## What remains

- ~~Confirm cross-platform: SSGI on Linux at 1280x720~~ — done on 2026-11-24, above: it reproduces (a window-less
  instance IS 1280 × 720; the live-resize crash this line feared no longer reproduces, `docs/caution-points.md`).
- Read SSGIEffect for a per-column or per-tile pattern tied to the resolution (a half-resolution pass, a tile size, a
  jitter or noise texture period) — 5 px at 1280 wide.
- Verify: the grid gone on Windows (step standard deviation near the SSGI-off value), Linux unchanged.

## References

- `PostProcess.getStatus()`, `PostProcess.disable(slot)` (engine `docs/ai-runtime-control.md`).
- Window-PA's captures usd-grid2-s*.png and usd-grid-ssgi-ab.png (its scratchpad), Linux 1792355645 / 1792355660.
