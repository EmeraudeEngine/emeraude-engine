---
id: ssgi-horizon-samples-unprojected-off-their-texel
title: SSGI's horizon search unprojects each sample at its UV with the depth of the nearest texel — a supposed bias, not shown to harm
status: parked
priority: unranked
scope: src/Graphics/Effects/Lighting/SSGI.cpp (the horizon pass, the two sample reads)
opened: 2026-11-24
tags: [ssgi, horizon-search, gbuffer, bias]
---

# SSGI's horizon search unprojects each sample at its UV with the depth of the nearest texel

> **Parked (owner, 2026-11-25: "on garde une note").** Not a defect anybody saw: an imprecision found by reading the
> code. The one attempt changed the picture, was not shown to be more right, added a grain far away, and was measured
> on a scene that does not load whole. Kept as a note for whoever touches the horizon search again.

## Why

The centre of a reduced pass names ONE G-buffer texel and unprojects from its centre since 2026-11-24 (graphics doc
13-12 § 45). The horizon search's SAMPLES were left as they were:

```glsl
vec3 delta = reconstructPosition(sampleUV0, texture(depthTex, sampleUV0).r) - P;
```

`depthTex` has a NEAREST sampler (`GrabPass.cpp`): the depth is the one of the texel that holds `sampleUV0`, the
position is unprojected at `sampleUV0` itself — up to half a texel beside the point that depth belongs to. On a sloped
surface the sample stands above or below the surface it was read from; the search keeps the HIGHEST horizon, so the
error would not average out and the surface would occlude itself — a SUPPOSITION, not shown (below). XeGTAO snaps its samples to texel centres for this reason.

It is a uniform bias, not a lattice (the 10 px and 5 px values do not move with the probe below).

## What is measured

A temporary probe (2026-11-24, Linux, RTX 3070 Ti, window-less 1280 × 720, screen-space lane, exposure pinned at
f/11 – 1/250 s – ISO 100, validation layer loaded, 0 VUID, no shader that fails to compile; removed after the run):
each sample unprojected at the centre of the texel it fetches (`texelFetch` + `emGBufferTexelUV()`). Three captures a
state, the state's mean image compared; the floor is a second launch of the SAME binary.

| scene | frame mean | per pixel, mean \|difference\| | floor (two launches of one binary) |
|---|---|---|---|
| `jungle-ruins` option 1 | 38.34 → 39.91 (+1.57) | 1.93 (p99 8.2) | 0.014 |
| `sponza` | 25.55 → 25.87 (+0.32) | 0.39 (p99 2.4) | 0.067 |

- The picture gets BRIGHTER almost everywhere it moves: 59.5 % of `jungle-ruins`' pixels gain more than one level,
  4.3 % lose more than one (`sponza`: 12.8 % against 0.2 %). The ground and the slopes of `jungle-ruins` gain about a
  tenth (rows 180-540: 21.3 → 23.4 and 22.0 → 24.6); its sky does not move.
- With the IndirectDiffuse slot off the two binaries give the same picture (mean \|difference\| 0.000 and 0.010): the
  change is SSGI's alone.

- **The snap was tried as a fix and REMOVED** (2026-11-25, owner, on a before / after sheet: "ce n'est pas à
  garder"). Same numbers as the probe on the three scenes, and one thing the means did not show: the FAR relief of
  `jungle-ruins` turns salt-and-pepper — bright and dark pixels mixed where the picture was smooth. Reading, not
  proven: far away the search's step is under one texel, so a snapped sample lands on the centre's own texel or on a
  neighbour with the noise, and the horizon jumps from one pixel to the next (XeGTAO's own warning, below).
- ⚠️ **`jungle-ruins` is not a reference** (owner, same day: "cette scène n'est pas chargée correctement […] je ne la
  trouve pas très fiable pour corriger un effet visuel"): its USD stage loads with its terrain and tree textures
  missing (the log's `not found next to the stage` lines). Every number of this item taken there waits for a scene
  that loads whole.
- ⚠️ **A flat ground did NOT tell the two apart**: `light-and-shadow-debug` (a flat paved ground seen at a grazing
  angle) gives the same picture before and after (mean \|difference\| 0.010, under the 0.018 between two captures of
  one state). SSGI moves that frame by 0.6 level only (its sky term is small there): the test is not sensitive, and
  the self-occlusion of a slope is NOT shown by it. What moves is detailed geometry — relief, vegetation, masonry.

⚠️ That the snapped read is the RIGHT one is a reasoning (the depth and the position then belong to one point), not a
measurement: no reference picture was compared.

## What remains

- A scene that loads whole and where SSGI's sky term weighs (not `jungle-ruins`, not `light-and-shadow-debug`),
  chosen with the owner, before any other attempt.
- Two forms not tried: snap only the samples farther than one texel from the centre; or keep the sample's UV and
  interpolate the depth between its four texels. The owner decides which, on a picture.
- A proof that it is right, not only different: a flat tilted plane under a uniform sky must show no occlusion from
  itself (the Visibility view of SSGI, or the frame against the IndirectDiffuse slot off).
- The look changes (a scene gains up to a tenth where SSGI darkened it): the owner sees it before it is kept; the
  measurements of § 45 and the peers' follow.
- NOT looked at: the trace pass's march compares the ray's depth with `texture(depthTex, sampleUV)` — a depth
  comparison, not an unprojection; `SSAO`'s own samples; `GIDenoiser`.

## ⚠️ Traps

- XeGTAO's own comment on its snap: the direction math is more correct, and the samples are pushed off their slice —
  another, smaller error. Look at the picture, not only at the mean.

## References

- `docs/subsystems/graphics/13-12-post-processing-effects/45-the-g-buffer-texel-of-a-reduced-pass.md`.
- Jimenez et al., "Practical Real-Time Strategies for Accurate Indirect Occlusion" (SIGGRAPH 2016); XeGTAO (Intel,
  MIT), `XeGTAO.hlsli`, the sample offsets snapped to pixel centres.
- Related: `reduced-passes-name-their-gbuffer-texel`.
