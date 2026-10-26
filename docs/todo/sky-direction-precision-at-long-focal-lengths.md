---
id: sky-direction-precision-at-long-focal-lengths
title: The physical sky draws a planet's disc 1.2" off at a 20-m focal length — the sky direction's precision
status: open
priority: high
scope: Graphics/Material (SkyAtmosphereResource), Saphir (the sky's direction), Graphics/Renderable (the skybox)
opened: 2026-10-26
tags: [sky, planets, precision, defect]
---

# The physical sky draws a planet's disc 1.2" off at a 20-m focal length

## Why

Measured 2026-10-26 (graphics doc 42 § "The planets' discs"): Mars on 2020-10-13 23:00 UTC from Liège, the camera aimed
at its direction at 20 m of focal length (0.069° of field, 0.23" per pixel): the disc's centroid sits **5.4 px (1.2")
above** the projected position, 0.8 px to the left; the map's pattern correlates at 0.977 once that offset is fitted
(longitude exact). At 600 mm the same angle is 0.16 px (invisible), so only the telescope zoom shows it — the owner's
goal in WaterWorld is precisely to zoom onto Mars.

**Re-measured 2026-10-26** (after the planets' own light direction and the depth-of-field fix): sky-lab, FIXED camera
(`Act.lookAt`), clock frozen, Mars 2020 at 20 m — still **3.5–3.9 px (≈ 0.85″) above**, correlation 0.969 once fitted;
WaterWorld, the PLAYER's camera re-aimed every 16 ms (`Player::aimAt()`) at 78 m — the disc **centred within 2 px**
(0.18″). A geodesic sky of 1° triangles instead of the cube changed nothing measurable (it was reverted): the
interpolation hypothesis is not supported. First suspects now: the fixed camera's aim path (`Act.lookAt`, the fixed
camera's frame), then the reference's own geometry (the measurement script's camera model).

## What remains

- **Find the cause** (not established): the hypothesis is the sky material's direction, a vertex attribute (the skybox
  cube's position) interpolated over triangles that span thousands of screen widths at that focal length — the
  rasterizer's and the clip's precision. Test it: aim the same camera at a star (the star pass projects in its vertex
  stage, no interpolation) and at Mars's disc; a star centred while the disc drifts accuses the sky's direction.
- **The fix, if confirmed**: reconstruct the direction per pixel from `gl_FragCoord` and the view (the inverse
  projection's tangents and the view rotation), as the post-process effects do (`ViewReconstructionGLSL`), for every
  view type the sky is drawn into (main view, cubemap probes); or tessellate the sky cube finer. Measure the residual
  with the same protocol (the reference projection in the session's `marsfit.py` recipe, doc 42).

## References

- Engine graphics doc 42 § "The planets' discs" (the measurement), § "The planets".
