---
id: half-res-unprojection-texel-centre-rtao-ssr
title: Unproject the fetched texel's centre in the half-res RTAO and SSR passes
status: open
priority: unranked
scope: Graphics/Effects/Lighting
opened: 2026-09-13
tags: [ray-tracing, screen-space, g-buffer]
---

# Unproject the fetched texel's centre in the half-res RTAO and SSR passes

## Why
A half-res pass that reads ONE full-res depth texel with `texelFetch(ivec2(vUV * fullSize))` but
unprojects `vUV` (the half-res pixel centre) lands half a full-res pixel away from the point whose
depth it holds — off the surface by that offset times the depth slope, a bias along the view ray
that grows with the grazing angle. Fixed in `RTR.cpp` on 2026-09-13 (NDC from
`(fullResCoord + 0.5) / fullResSize`); the same pattern is still in `RTAO.cpp` (two sites) and
`SSR.cpp` (normals / trace reads).

## What remains
- Apply the same one-line change at each site, then measure each effect on a grazing floor before
  and after (the bias is a constant per frame, so a temporal series will NOT show it — compare
  images, or the ray origin's distance to the plane in a debug channel).

## ⚠️ Traps
- SSR's second site reads the TRACE texture at half-res, not the depth: check what it unprojects
  before "fixing" it.

## References
- `docs/reflection-pipeline.md` § 3.2, *Unprojection at the TEXEL centre*.
