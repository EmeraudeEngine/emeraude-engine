---
id: rt-alpha-test-candidate-texture-lod
title: Sample the alpha-test opacity at an explicit LOD in the shared RT candidate confirmation
status: open
priority: unranked
scope: Graphics/Effects/Shared
opened: 2026-09-13
tags: [ray-tracing, alpha-test, aliasing]
---

# Sample the alpha-test opacity at an explicit LOD in the shared RT candidate confirmation

## Why
`EMEN_RT_CONFIRM_ALPHA_TESTED_CANDIDATE` (RTAlphaTestGLSL.hpp) confirms a candidate hit by sampling
the opacity — or the albedo alpha — with `texture()` inside `rayQueryProceedEXT()` loops: divergent
control flow, discontinuous UVs, undefined implicit derivatives, finest mip in practice. The RTR
trace fixed the same defect for its hit SHADING on 2026-09-13 (ray-cone LOD, Ray Tracing Gems
ch. 20); the cutout edges of the palm leaves reflected in `light-and-shadow-debug` still come
from an unfiltered opacity test, and so do every alpha-tested shadow ray and RTGI/RTAO candidate.

## What remains
- Carry a ray-cone width into the macro (it needs the ray's spread and distance — the callers have
  them) and sample with `textureLod()`; the cutoff comparison on a filtered alpha is the usual
  alpha-to-coverage trade-off, measure the leaf silhouettes before/after.

## ⚠️ Traps
- The macro is shared by RTR, RTGI, RTAO, RTContactShadows and the shadow rays: one change, five
  consumers, measure at least two.

## References
- `docs/reflection-pipeline.md` § 3.2 (the RTR LOD fix and its measurement).
