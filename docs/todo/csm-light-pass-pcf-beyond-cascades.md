---
id: csm-light-pass-pcf-beyond-cascades
title: The CSM light pass runs its PCF loop on fragments beyond the last cascade — the far imposters pay it for nothing
status: open
priority: unranked
scope: Saphir/LightGenerator.ShadowMap (CSM sampling), Scenes (lit lists)
opened: 2026-09-25
tags: [shadows, performance, vegetation]
---

# The CSM light pass runs its PCF loop on fragments beyond the last cascade — the far imposters pay it for nothing

## Why

By reading (2026-09-25, the terrain hang diagnosis): the DirectionalLightPassCSM runs a `(2·PCFSamples + 1)²` tap
loop per fragment (81 taps with `PCFSamples` = 4, 289 with 8 — the Windows peer's setting; doubled in the 10 %
cascade blend band). Its only early exit is a light-space depth outside [0, 1]. On `terrain` the cascades cover
~2000 m (4 cascades, `cascadeScale` 5) while the lit imposters are drawn to 6 km (`TreeDrawDistance`), so every far
imposter fragment runs the full loop and its result is unshadowed anyway.

## What remains

- Exit before the loop when the fragment is beyond the last cascade's far distance (visibility 1, or a fade), and
  measure the sun pass on `terrain` at the startup pose with `PCFSamples` 4 and 8.
- Check the same in the point/spot light passes.
