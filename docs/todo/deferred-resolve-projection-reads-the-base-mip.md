---
id: deferred-resolve-projection-reads-the-base-mip
title: The deferred resolve samples a light's projection at its base mip level — a fine picture seen from far may alias where the forward pass did not
status: open
priority: unranked
scope: src/Graphics/DeferredLightResolve.cpp (the projection term of the fragment shader)
opened: 2026-11-23
tags: [deferred-lights, projection, aliasing]
---

# The deferred resolve samples a light's projection at its base mip level

## Why

The forward light pass samples a light's colour projection with `texture()`: the mip level comes from the screen
derivatives. The deferred resolve (2026-11-22, graphics doc 38 § *A light that projects a texture*) samples it inside
its tile loop, which has no screen derivative to give: it reads the BASE level (`textureLod(…, 0.0)`). A picture with
fine detail (a grid, a text, a foliage gobo) lit from far, or at a grazing angle, is then under-sampled: it may alias
and shimmer where the forward pass filtered it.

⚠️ Not observed: the two pictures measured are smooth (a dipped-beam pattern, a flashlight's disc) and the resolve
matched the forward pass within the noise on three OS. This item is the measurement, then the cure if it shows.

## What remains

- A scene with a FINE projected picture seen from far and at a grazing angle; A/B forward against resolve in one
  launch (`Core.RendererService.setDeferredPunctualLights(0 | 1)`), a pinned exposure, a `temporalCapture` while the
  camera moves (aliasing shows in motion).
- If it aliases: give the resolve a level — `textureGrad()` with the projection coordinates' gradients rebuilt from the
  pixel's position and normal, or an analytic level from the texel's footprint at the pixel's distance — and measure
  its cost in the tile loop.

## ⚠️ Traps

- Derivatives of the RECONSTRUCTED position are wrong across a depth edge: a level taken from them blurs the picture
  along every silhouette.
- The traced lanes read the base level too (`emTracedLightProjection()`): a ray has no screen derivative, and that is
  the usual practice — not part of this item.

## References

- `docs/subsystems/graphics/38-deferred-punctual-lights.md` § *A light that projects a texture* (the term's row and
  the last ⚠️).
