---
id: sponza-terrain-receives-no-indirect-diffuse
title: Sponza's surrounding terrain receives NO indirect diffuse, in either lane
status: open
priority: unranked
scope: Graphics (the albedo G-buffer / material properties of that asset's terrain), Scenes (Sponza)
opened: 2026-09-14
tags: [rendering, lighting, gbuffer, measurement, sponza]
---

# Sponza's terrain receives no indirect diffuse

## Why

Measured on `sponza` (2026-09-14, exposure pinned f/11 · 1/250 s · ISO 100, 2880×1620, pose
`setPosition(-9, 0, -2.5)` + `lookAt(-9, 1.2, 6)`, the grass apron filling the bottom third of the
frame):

| Comparison | Grass band (y 1140-1560) | Building (y 0-1000) |
|---|---|---|
| SSGI sky term ON vs OFF | mean abs delta **0.21**/255 | 16.8 |
| RTGI alone vs raster-only (`setLightingMode("None")`) | mean abs delta **0.21**/255 | 36.5 |

The whole indirect-diffuse concept — the raster's IBL leg, SSGI's sky and bounce, RTGI's sky and
bounce — moves that surface by two tenths of a grey level while the building next to it moves by
tens. It is lit by the direct sun and by nothing else, under a 36 910-nit sky it sees in full.

⚠️ This is how it was found: that grass was used as the OPEN-SKY reference of the sky-visibility
acceptance table and returned a suspiciously perfect "ratio 1.0000". An A/B on a surface where the
term is zero on both sides always does.

## What remains

1. Attribute. The two combines multiply by `albedo.rgb * albedo.a` and by `(1 - emissiveMask)` read
   from the G-buffers, so a zero albedo lane, a saturated emissive nibble, or a material that never
   writes those attachments all produce exactly this. Read the attachments at that pixel
   (`post-processor-effect-debug`, or a RenderDoc capture).
2. ⚠️ The raster leg is ALSO ~zero there, which no G-buffer lane explains — a material with no IBL
   branch at all (legacy path without bindless) would. Check what material that terrain carries in
   `Sponza.ktx2.glb`; the demo declares no `enableBasicGround()`, so it is asset geometry.
3. Then decide whether it is the asset's material or an engine path that must change.

## References

- `src/Graphics/AGENTS.md` § "The screen-space sky visibility" (the corrected acceptance table).
- `docs/caution-points.md` § "the screen-space lane lit enclosed spaces with the UNOCCLUDED sky".
