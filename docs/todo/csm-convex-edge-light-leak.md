---
id: csm-convex-edge-light-leak
title: Cascaded shadows leak a bright line along the convex edges of a box in shadow (absent with a classic map)
status: open
priority: high
scope: Saphir/LightGenerator.ShadowMap (CSM sampling, bias / normal offset), Scenes (cascade setup)
opened: 2026-10-28
tags: [shadows, csm, defect]
---

# Cascaded shadows leak a bright line along the convex edges of a box in shadow (absent with a classic map)

## Why

Reported by the Windows peer (MSVC 19.44, RTX 3500 Ada, 2026-10-28) on a downstream demo built of box towers under a
physical sky, the sun shadowed through 4 cascades of 2048 px (λ 0.8, covered depth 400 m, view distance 3 km): a
bright cream line, 2-4 px wide and near white (225-255), runs exactly along a tower's vertical CONVEX corner and along
its base edge on the ground, while the wall around it is in shadow (RGB ~17, 24, 40). It is identical on the
ray-traced and the screen-space lighting lanes, and ABSENT at the same pose with a classic shadow map (120 m
half-extent) — so it belongs to the cascaded path: a depth bias or a normal offset that pushes the lookup of an edge
fragment out of the caster's own shadow. REPRODUCED on macOS (M6, the same day, the same pose): a line ~6 px wide at
2560 × 1440, luma 197-233 against 10-80 on both sides, on 239 of 460 rows of the corner, continuing along the wall's
bottom edge (peak 242); the single-map capture at the same pose has none (max luma 29 / 13 in those columns) — the sun
had moved ~20 min between the two captures. Not yet reproduced on Linux (reproduce with the clock paused).

## What remains

- Reproduce on Linux at a fixed sun (the demo's clock paused) and measure the line (pixels, width) at a pinned
  exposure; then the same pose with a classic map as the control.
- Read the CSM sampling's bias and normal offset per cascade (constant vs scaled by the cascade's texel size and the
  slope) and the edge case of a fragment whose normal is perpendicular to the light at a convex corner.
- Fix in the engine, never in the scene; record the before / after numbers here, then in the shadow docs.

## References

- Related: `csm-light-pass-pcf-beyond-cascades` (the CSM light pass), `rtcontactshadows-backlit-self-hit`.
- Normal-offset shadows: D. Holbert, "Saying Goodbye to Shadow Acne" (GDC 2011 poster); "Normal Offset Shadows",
  D. Holbert (2011).
