---
id: sun-unlit-without-shadow-mapping
title: With shadow mapping disabled, a CASCADED (CSM) directional light adds no light at all
status: open
priority: unranked
scope: Saphir (LightGenerator light-pass selection), Scenes (LightSet), Graphics/Renderer
opened: 2026-09-28
tags: [lighting, shadows, directional-light, csm, settings, measured]
---

# With shadow mapping disabled, a CASCADED (CSM) directional light adds no light at all

## Why

Observed while bisecting the `terrain` ground blobs (2026-09-28, docs/caution-points.md § Two-sided normals): with
`Core/Graphics/ShadowMapping/Enabled = false` in a settings copy, `terrain --demo-options 100000,25,0,0,1` renders a
FLAT ground — no modelling of the relief at all, the sky blown out — and the near-ground crop mean falls from 98 to
41/255 (auto exposure). A light without a shadow map should still light: the frame looks as if the sun contributed
nothing. Not investigated: this item only records the observation.

## Confirmed at a pinned exposure (2026-09-28)

Owner request: "une confirmation à exposition fixe". Protocol: the spawn pose, the exposure PINNED at sunny-16
(`Camera.setExposure(Head, Eyes, 16, 0.01, 100)`, auto off), `bypassSceneEffects(true)` so only the raster lighting
remains, then the SAME instance with the sun on and off (`DirectionalLight.setEnabled`) — the A/B that needs no
second launch. Mean luminance of the lower half of the frame (/255), validation ON, 0 VUID everywhere:

| Run | shadow mapping | sun on | sun off |
|---|---|---|---|
| `terrain` (CSM, 4 cascades — default) | on | 60.5 | 10.0 |
| `terrain` (CSM) | **off** | **10.0** | **10.0** |
| `terrain` option 6 = 1 (classic 4096 px map) | off | 66.8 | 10.1 |
| `water-world` (classic map, sky-manifest sun, no `SunCourse`) | on | 74.0 | 19.7 |
| `water-world` | off | 75.2 | 19.9 |

- Confirmed: on `terrain`, with shadow mapping disabled, switching the sun off changes NOTHING — it contributes no
  light at all.
- It is the CASCADED path: the same demo with the classic map lights normally without shadows, and so does
  `water-world`. `SunCourse` is not involved (it drives both `terrain` variants).

## What remains

1. Find what a CSM directional light's lit pass does when `Core/Graphics/ShadowMapping/Enabled` is false: which
   `RenderPassType` it takes (`DirectionalLightPassCSM` vs `DirectionalLightPass`), what its shadow sampler is bound
   to, and what `shadowFactor` it computes. ⚠️ `DummyShadowTexture` exists as 2D and cubemap only, not as a 2D ARRAY
   (what a CSM sampler reads), and the runs showed 0 VUID — so a mismatched dummy binding is not established.
2. Fix it in the engine, then re-run the table above: the second row must light like the third.
## ⚠️ Traps

- A shadow A/B is meaningless until this is settled: switching the shadows off removed the blobs only because every
  pixel lost the sun.

## References

- Captures of 2026-09-28: `1790549338`/`1790549343` (terrain, shadows on, sun on/off), `1790549359`/`1790549364`
  (terrain, shadows off, sun on/off), all at the pinned sunny-16 exposure.
