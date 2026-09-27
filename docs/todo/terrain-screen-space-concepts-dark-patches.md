---
id: terrain-screen-space-concepts-dark-patches
title: Large stair-edged dark patches on a close-range TerrainResource, made by the screen-space lighting concepts
status: open
priority: unranked
scope: Graphics/Renderable/TerrainResource, Saphir (heightfield G-buffer outputs), Graphics/Effects/Lighting (AO, IndirectDiffuse, ContactShadows)
opened: 2026-09-27
tags: [terrain, heightfield, cdlod, ambient-occlusion, indirect-diffuse, contact-shadows, measured]
---

# Large stair-edged dark patches on a close-range TerrainResource, made by the screen-space lighting concepts

## Why

Found when projet-alpha's `water-world` island moved from a `BasicGroundResource` to a `TerrainResource`
(2026-09-27, owner: "ouvre l'item moteur pour les taches sombres"). At the spawn, the near ground (a few metres
from the eye) shows LARGE dark regions — hundreds of pixels wide, textured (not pure black), with STAIR-STEPPED
edges — that read like pits with walls. They are shading, not geometry: with the concepts below disabled the
same pixels are a smooth, evenly lit slope.

Repro: `projet-alpha --load-demo water-world` (RTX 3070 Ti, 2880×1620, validation ON, 0 VUID), then
`Core.SceneManagerService.targetActiveScene()`, `Act.setPosition(-0.106367, 12.466258, -1.115876)`,
`Act.lookAt(8.564221, 9.053319, 2.513552)`, screenshot ~4 s later. Material `Grounds/Sand001` (albedo + normal +
height), diamond-square `factor` 75 on 1 m cells (very spiky: metres of relief per cell).

Measured at that replayed pose, one launch each, mean luminance of a crop inside the darkest patch
(x 835-1123, y 1008-1440 of the 2880×1620 frame); two identical runs differ by 0.06/255 over the whole frame:

| Run | Patch mean (/255) |
|---|---|
| reference (everything on) | 27.2 |
| `Core/Graphics/ShadowMapping/Enabled = false` | 27.6 |
| material without its `Height` (no POM) | ≈ the reference (whole frame 1.7/255 apart) |
| `PostProcess.disable(IndirectDiffuse)` | 32.4 |
| `PostProcess.disable(ContactShadows)` | 37.2 |
| `PostProcess.disable(AmbientOcclusion)` | 77.3 — the region keeps its exact outline, now saturated ORANGE |
| the three disabled | **113.1 — the patches are gone** |

- Not the shadow map, not the POM.
- Not ONE concept: each of the three darkens the SAME region, with the same outline. A common screen-space
  input is wrong on those pixels, and every concept that reads it misbehaves.
- The former `BasicGroundResource` (same generator, seed and pose, `desert001`) shows no such region; from 25 m
  up the `TerrainResource` renders clean.

## What remains

1. Name the wrong input. Hypothesis (NOT verified): the G-buffer the heightfield writes — its per-pixel
   normal (rebuilt from the normal clipmap, `FragmentShader` `HeightfieldFrameOverrides`) or its depth / view
   position — disagrees with what the colour pass shades, so AO, GI and contact shadows see a wrong surface. Dump
   the normal and depth MRTs at this pose and compare with a `BasicGroundResource` of the same heights.
2. The stair steps: check the resolution of the AO / GI buffers and their upsampling (half-resolution buffers
   upsampled against a mismatched depth would draw exactly those edges).
3. Check whether it is the same defect as `terrain-ground-black-blobs-indirect-diffuse` (heightmapped ground,
   screen-space concepts, 2026-09-25): different signature there (small pure-black NaN-like blobs, IndirectDiffuse
   alone removes them), but the same kind of ground and the same concepts.

## ⚠️ Traps

- Compare only at the REPLAYED pose (`setPosition` + `lookAt` through the console for every run): the spawn
  itself is not the same frame as the replay.
- `PostProcess.disable(...)` persists across lane switches; relaunch between runs rather than re-enabling.

## References

- projet-alpha `docs/subsystems/builtin/16-14-waterworld-scene.md` § The ground.
- Captures: session of 2026-09-27 (projet-alpha captures `1790540152` reference, `1790540658` three concepts off,
  `1790540701` AO off, `1790540433` the `BasicGroundResource` baseline).
