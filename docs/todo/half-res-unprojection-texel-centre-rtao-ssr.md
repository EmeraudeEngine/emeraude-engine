---
id: half-res-unprojection-texel-centre-rtao-ssr
title: Unproject the fetched texel's centre in the half-res RTAO and SSR passes
status: open
priority: high
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
`(fullResCoord + 0.5) / fullResSize`, `src/Graphics/Effects/Lighting/RTR.cpp:651`); the same pattern
is still in:
- `src/Graphics/Effects/Lighting/RTAO.cpp:130` (the depth fetch at `:109`) — ONE site: the derived
  shader (`:243-257`) only forwards the depth, it unprojects nothing;
- `src/Graphics/Effects/Lighting/SSR.cpp:283` (`reconstructPosition(vUV, …)`, the depth fetch at
  `:273`) in the trace pass.

## What remains
- Apply the same one-line change at each site, then measure each effect on a grazing floor before
  and after (the bias is a constant per frame, so a temporal series will NOT show it — compare
  images, or the ray origin's distance to the plane in a debug channel).

## ⚠️ Traps
- SSR's resolve reads the trace texture with `texelFetch` (`SSR.cpp:577`) but its depth and normal
  with `texture(…, vUV)` (`:587`, `:595`) and unprojects `vUV` (`:623`): consistent as it stands, not
  this pattern — check what it unprojects before "fixing" it.

## References
- `docs/reflection-pipeline.md` § 3.2, *Unprojection at the TEXEL centre*.
