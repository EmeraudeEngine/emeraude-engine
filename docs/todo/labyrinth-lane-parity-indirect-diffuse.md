---
id: labyrinth-lane-parity-indirect-diffuse
title: In labyrinth the screen-space lane's indirect diffuse is 15–20 times the traced lane's
status: open
priority: high
scope: Graphics/Effects/Lighting (SSGI, RTGI), lane parity
opened: 2026-10-25
tags: [gi, parity, defect]
---

# In labyrinth the screen-space lane's indirect diffuse is 15–20 times the traced lane's

## Why

Met while measuring the depth-reconstruction port (`post-process-depth-linearisation-wrong`), 2026-10-25, Linux, RTX
3070 Ti, 1920×1080, validation ON, 0 VUID. `labyrinth`, pose `Act.setPosition(-6, 0, 67.3)` +
`Act.lookAt(-6, 3.9, 66)`, exposure pinned f/2.8 · 1/30 s · ISO 400 (`Camera.setExposure(Head, Eyes, 2.8, 0.0333,
400)`), settings copies differing by `LightingLane` only. The contribution of the `IndirectDiffuse` slot = the mean
luminance (linearised: sRGB, then the inverse of the Narkowicz ACES fit) with the slot on minus with it disabled:

| Binary | ScreenSpace lane (SSGI) | RayTracing lane (RTGI) |
|---|---|---|
| before the depth port (`522e9d47`) | 0.106 | 0.0079 |
| after it | 0.127 | 0.0065 |

The frame without GI, reflections and AO is the same on both lanes (0.026–0.027): the direct light agrees, the
indirect does not. SSGI turns the lamp-lit walls bright; RTGI adds almost nothing. It predates the depth port (which
moved SSGI by +20 %, its rays now covering their true metres).

The two lanes are meant to be two costs of ONE lighting (graphics doc 13-12 § 18, § 20). Sponza's documented gap is
3.7× at its worst pose, explained by the off-screen occluder blind spot; 15–20× is beyond it.

## What remains

- Find the cause. Candidates, none checked: (1) the SS sky visibility cannot see an off-screen ceiling (the
  documented blind spot) — does labyrinth have one above this pose, and what sky lights it; (2) RTGI's hit shading
  ignores an EMISSIVE hit (the lamp panels) where SSGI samples their radiance on screen; (3) related to
  `rtgi-bounce-hits-miss-flat-ambient` (RTGI's hits carry no flat ambient). First split SSGI's two halves (the sky
  term off: `IndirectDiffuse/ScreenSpace/SkyVisibilityEnabled` false at launch) and look at RTGI's hit radiance on a
  panel.
- Measure the parity again at this pose after the fix, and on sponza's four poses (§ 18).

## References

- The protocol: graphics doc 13-12 § 41 (two binaries, one launch per lane, each slot disabled in turn).
- `src/Graphics/Effects/Lighting/SSGI.cpp`, `RTGI.cpp`; graphics doc 13-12 § 18, § 20.
