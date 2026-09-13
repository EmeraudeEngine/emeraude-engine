---
id: blended-lit-sprite-writes-nan
title: A blended, lit sprite writes NaN into the scene colour buffer on its transparent texels
status: open
priority: unranked
scope: Saphir / Graphics/Renderable/SpriteResource
opened: 2026-09-13
tags: [sprite, blending, nan, ssr]
---

# A blended, lit sprite writes NaN into the scene colour buffer on its transparent texels

## Why
Owner-captured on `light-and-shadow-debug` (2026-09-13): 7x7 black squares on every glossy surface
of the screen-space lane, at fixed positions and a constant period. Bisected live to SSR, then to
the animated `pinup` sprite (`BlendingMode: Normal`, `Lit: true`): SSR rays hitting the sprite's
quad read a NON-FINITE scene colour there. The SSR resolve now rejects such a colour (the guard that
removed the symptom), but the producer is untouched: somewhere in the lit path of a BLENDED quad, a
fully transparent texel yields a NaN that the blender writes as `dst · 1 + NaN · 0 = NaN`. Any other
reader of the colour buffer (bloom, DoF, motion blur, the TAA history) is exposed to the same NaN.

## What remains
- Probe first: paint every non-finite pixel of the TAA input in a flat colour (boolean probe,
  `docs/temporal-stability-measurement.md` § 4 of projet-alpha) and watch the sprite — the texels
  and the frame draw themselves.
- Then read the generated fragment shader of the blended lit sprite (`ShowSourceCode` +
  `--clear-shader-cache`): candidates are a division by the texture alpha, a degenerate specular term
  (`NdotV = 0` with `4 · NdotL · NdotV` in a denominator), or a normalisation of a zero vector.
- Fix at the producer, then consider a `discard` of fully transparent texels in the blended path
  (they contribute nothing and cost a full lighting evaluation).

## ⚠️ Traps
- The sprite's default is UNLIT + alpha test; `Lit: true` + `BlendingMode: Normal` is the
  combination that reaches the lighting pass with alpha-0 texels. Test that combination, not the
  default.
- A blended material REPLACES normals and material properties over its whole quad (Aug 2026 policy):
  the transparent texels also overwrite the G-buffer of what is behind them.

## References
- `docs/caution-points.md` § "7x7 black squares … under an animated BLENDED sprite".
- `src/Graphics/Effects/Lighting/SSR.cpp`, the non-finite guard in the resolve.
