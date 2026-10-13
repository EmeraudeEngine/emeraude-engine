---
id: path-ghost-in-indirect-diffuse-history
title: A path's old shape survives in the indirect-diffuse temporal history after a point-count change
status: open
priority: high
scope: Graphics/Effects/Lighting (IndirectDiffuse temporal history, both lanes: SSGI and RTGI), Graphics/Renderer (markInvalidTemporalHistory)
opened: 2026-10-13
tags: [graphics, temporal, paths, gi, defect]
---

# A path's old shape survives in the indirect-diffuse temporal history after a point-count change

## Why

The TAA ghost of a path whose point count changes is fixed (engine 15905e01: the old shape's screen region is cleared
to 1 in the reactive mask, graphics doc 35 § The point-count change). What remains is the INDIRECT DIFFUSE effect's own
temporal history, which never reads the reactive mask: the ribbon's bounce light stays on the ground in the old
shape's footprint for 30+ frames. A wrong image for about a second: a defect.

Measured:
- Linux (RTX 3070 Ti, RT lane, 3840x1990), 2026-10-13: a band of ~650 pixels just under the old zigzag, identical
  before and after the TAA fix.
- macOS peer (M6, ScreenSpace lane only, 2560x1440, exposure pinned, threshold 16/255 against the steady frame),
  2026-10-14, zigzag → 2-point line, 30 frames:

  | Run | Old ribbon (frames 0 / 10 / 29) | Ground around it (frames 0 / 10 / 29) |
  |---|---|---|
  | all effects on | 1485 / 1303 / 1091 | 2709 / 2458 / 1797 |
  | IndirectDiffuse off | 0 / 0 / 0 | 260 / 228 / 121 |
  | TAA only | 0 / 0 / 0 | 0 |
  | everything off | 0 | 0 |

  Red glows in the zigzag's shape on the ground, a darker band, speckles over the old footprint — all gone with
  IndirectDiffuse off. So it is NOT ray-tracing specific: the SSGI history on the ScreenSpace lane, the RTGI history on
  the RT lane (Linux attribution by the same A/B still to do). A small residue (260 → 121 px) goes with SSR / AO /
  contact shadows off, not isolated further.

## What remains

- Linux, RT lane: the same A/B (`PostProcess.disable("IndirectDiffuse")`) to confirm the RTGI attribution; then
  isolate the small SSR / AO / contact-shadow residue.
- A design decision for the owner: let the indirect-diffuse (and, if the residue matters, SSR / AO) history rejection
  read the reactive mask — or the invalidated regions themselves (`SceneInstanceTransforms::temporalHistoryInvalidations()`)
  — versus a per-effect disocclusion / radiance test. Reuse the region invalidation; no second mechanism.

## ⚠️ Traps

- The demo's Catmull-Rom loop ROTATES and pollutes a whole-frame difference mask: count inside the old zigzag's area
  (Linux 3840x1990: x 419-909, y 672-733; M6 2560x1440: x 557-1209, y 896-988).
- `temporalCapture` blocks the main thread: send `setPoints` first, then start the capture. Pin the exposure.
- Not this item: the NEW line's rim converges over ~30 frames under TAA alone (macOS: 1590 → 582 px at threshold 16,
  1 px at 32) — the TAA accumulating the anti-aliasing of a shape that just appeared, on both sides of it.
- Related, distinct: `gi-history-keeps-a-switched-off-emitter-lit` (a radiance DROP on unchanged geometry, same
  history) — a fix of one may cover the other; decide them together.

## References

- engine `docs/subsystems/graphics/35-paths-polylines-and-curves.md` § The point-count change, `33-beams-lasers-and-electric-arcs.md` § The reactive mask.
- AMD FidelityFX Super Resolution 2, "Reactive mask"; NVIDIA ReBLUR history reset on disocclusion (NRD documentation).
