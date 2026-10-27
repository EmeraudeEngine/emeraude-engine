---
id: ssao-ground-truth-horizon-based
title: Replace the hemisphere SSAO with a horizon-based ground-truth AO (GTAO), view-robust at grazing angles
status: open
priority: unranked
scope: Graphics/Effects/Lighting/SSAO
opened: 2026-10-27
tags: [ssao, gtao, screen-space, ambient-occlusion]
---

# Replace the hemisphere SSAO with a horizon-based ground-truth AO (GTAO), view-robust at grazing angles

## Why

Owner decision 2026-10-27, after the SSAO sea fix (`docs/caution-points.md` § "SSAO darkened the open sea"): the current SSAO is a normal-oriented hemisphere
of point samples tested against the depth buffer (Crytek-style, `SSAO.cpp`). At grazing angles it counts the relief in
FRONT of a sample (wave crests, terrain bumps) as occluders: the open sea took ~11 % of darkening where RTAO measures
none — answered for water by `setAOResponse(0)`, not for terrain or any bumpy surface seen from afar. Ground-truth
ambient occlusion integrates the visible horizon angles per slice with the cosine weighting of the true AO integral and
a thickness heuristic; it is the screen-space state of the art (UE, HDRP, Call of Duty).

## What remains

- Implement GTAO (Jimenez, Wu, Pesce, Jarabo, "Practical Real-Time Strategies for Accurate Indirect Occlusion",
  SIGGRAPH 2016 course; XeGTAO, Intel, MIT licence — compatible with LGPLv3, cite at the point of use): slices from the
  shared interleaved gradient noise, the horizon search in view space through `ViewReconstructionGLSL`, the
  depth-precision bias already in SSAO (`depthStepMetres()`), the multi-bounce approximation, optional bent normal
  (item `specular-occlusion-from-the-bent-normal`).
- Decide with the owner: replace SSAO or add GTAO as a second screen-space AO effect in the `AmbientOcclusion` slot.
- Proof: same frames, SSAO / GTAO / RTAO / off, on `water-world` (open sea, with `setAOResponse(1)` for the test),
  `relief`, `sponza`; cost on the three OS; 0 `VUID-`.

## References

- J. Jimenez, X. Wu, A. Pesce, A. Jarabo, "Practical Real-Time Strategies for Accurate Indirect Occlusion", SIGGRAPH 2016.
- Intel XeGTAO, https://github.com/GameTechDev/XeGTAO (MIT).
- `docs/caution-points.md` § "SSAO darkened the open sea".
