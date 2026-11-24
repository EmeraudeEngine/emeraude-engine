---
id: reduced-passes-name-their-gbuffer-texel
title: Every reduced pass names its G-buffer texel through the shared helper (RTAO, SSR, RTGI, RTR, RT contact shadows, GI denoiser)
status: open
priority: high
scope: Graphics/Effects/Lighting, Graphics/GIDenoiser
opened: 2026-09-13
tags: [defect, ray-tracing, screen-space, g-buffer, resolution]
---

# Every reduced pass names its G-buffer texel through the shared helper

## Why

A pass rendered at half resolution that reads the full-resolution G-buffer at its pixel's centre reads the corner four
texels share: the texel returned follows the rounding of the interpolated UV, and a position unprojected at `vUV` with
that depth is half a texel off its surface. It drew SSGI's and SSAO's screen lattice at 1280 × 720; both were fixed on
2026-11-24 with `Graphics/Effects/Shared/GBufferTexelGLSL.hpp` (graphics doc 13-12 § 45: the mechanism, the rule, the
measurements). Owner decision that day: SSGI and SSAO then, this item for the rest.

This item absorbs `half-res-unprojection-texel-centre-rtao-ssr` (opened 2026-09-13): the unprojection half of the same
defect in RTAO and SSR, fixed in `RTR.cpp` alone that day (`docs/reflection-pipeline.md` § 3.2).

Priority `high` by the default rule (a wrong result). None of the sites below has been MEASURED to draw a lattice.

## What remains

Each site adopts the helper — the reduction stated once (`reducedSize()` for the targets, `sourceGLSL()` for the
shader), the centre read with `texelFetch` at `emGBufferTexel()`, every unprojection from `emGBufferTexelUV()` — and
is measured before and after, one effect at a time:

| Site | Today | Notes |
|---|---|---|
| `RTAO.cpp` (the trace, and the second shader's fetch) | `ivec2(vUV * size)` + unprojection at `vUV` | pick and unprojection; its size comes from `traceSize(…, pixelDoubling)` |
| `SSR.cpp` trace | `ivec2(vUV * mip0Size)`, `reconstructPosition(vUV, …)` | pick and unprojection |
| `SSR.cpp` resolve | `texture(depthTex, vUV)`, `texture(normalTex, vUV)`, unprojection at `vUV`; the trace texture by `texelFetch(ivec2(vUV * size))` | ⚠️ consistent with itself as it stands: check what it unprojects before changing it |
| `RTGI.cpp` trace | `texture(depthTex, vUV)`, `texture(normalTex, vUV)` | pick; check the unprojection |
| `RTR.cpp` trace | `ivec2(vUV * fullResSize)`, unprojection at the fetched texel (2026-09-13) | the pick alone |
| `RTContactShadows.cpp` | `ivec2(vUV * vec2(depthSize))` | pick; check the unprojection |
| `GIDenoiser.cpp` (every stage) | `texture(depthTex, vUV)`, `texture(normalTex, vUV)`, neighbours at `vUV + offset` | shared by SSGI, RTGI and RTR; the lattice of SSGI did not need it |

- The trace effects size their targets with `IndirectPostProcessEffect::traceSize(fullSize, pixelDoubling)`: the same
  rule as `GBufferTexelGLSL::reducedSize(fullSize, pixelDoubling ? 2 : 1)`. State it once when they adopt the helper.
- A scene for the ray-traced lane: `jungle-ruins` option 1 cannot show it (no main directional light, its four
  ray-traced effects fall back to the screen-space ones). The lattice needs sloped, plain surfaces and a window whose
  half size has an odd factor (1280 × 720).
- RTAO and SSR: the bias is a constant per frame, so a temporal series will NOT show it — compare images on a grazing
  floor, or the ray origin's distance to the plane in a debug channel.

## ⚠️ Traps

- The fixed first texel of the block is an owner decision for a DIFFUSE term (doc § 45): RTR and SSR are sharp —
  weigh the half-texel offset it implies before adopting it there, and ask.
- The measurement's traps (regular content read as a lattice, frozen noise with the temporal chain off): doc § 45.

## References

- Graphics doc 13-12 § 45; `docs/reflection-pipeline.md` § 3.2, *Unprojection at the TEXEL centre*.
- Engine item `ssgi-five-pixel-column-grid` (SSGI and SSAO, awaiting the peers).
