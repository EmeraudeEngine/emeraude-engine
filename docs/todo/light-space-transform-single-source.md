---
id: light-space-transform-single-source
title: One source of truth for the light-space transform — it is open-coded at four sites
status: open
priority: high
scope: Scenes/Component/DirectionalLight, Graphics/ViewMatrices2DUBO, Graphics/Toolkit
opened: 2026-09-08
tags: [shadow-map, architecture, contract]
---

# One source of truth for the light-space transform

> This is what remains of the CSM rework after the 2026-09-08 verification. **Everything else that
> item listed is done and pushed** — see [`../shadow-mapping.md`](../shadow-mapping.md) § *Cascaded
> Shadow Maps* for the four root causes (`c7eef938`, `8c6417f0`, `882e5f29`, `864e6582`), the
> rotation-invariant fit and texel snapping, inter-cascade blending (B3), the shader-derived
> per-cascade bias, and the publish contract (A1-A5). Do not re-open any of those.

## Why

The recipe that builds a directional light's light-space frame is **open-coded at four sites**, and
they do not agree. Nothing forces them to, so a change to one silently desynchronises the others —
the shadow map is then rasterised with one transform and sampled with another, which is the exact
shape of the family of defects that cost this subsystem months.

The four sites:

- `DirectionalLight::updateLightSpaceMatrix()`
- `DirectionalLight::move()`
- `ViewMatrices2DUBO::updateOrthographicViewProperties()`
- `DirectionalLight::createOnHardware()` — open-codes the recipe a **second** time with a
  *different* coverage fallback: `m_coverageSize > 0.0F ? m_coverageSize : getDistanceOrFar() * 0.5F`

## What remains

- [ ] Extract the frame recipe into **one** function that all four sites call — coverage fallback
      included, so there is a single answer to "how wide is this light's box".
- [ ] Make the divergence unrepresentable rather than merely fixed: the callers must not be able to
      rebuild the transform by hand.

## ⚠️ Traps

- **Latent, non-square shadow maps**: `updateLightSpaceMatrix()` hard-codes
  `-m_coverageSize, m_coverageSize` while `ViewMatrices2DUBO` computes
  `halfSide = (far * 0.5F) * getAspectRatio()`. The two agree **iff the map is square**, which every
  map is today. A non-square shadow map would silently misregister — fold this into the single
  recipe rather than leaving it to be discovered.
- **Dormant, `getUniformBlockCSM()`**: the GLSL array is declared with `cascadeCount` while the C++
  offsets assume **4** matrices hard-coded. Every call uses the default `= 4`, so it is harmless
  today; passing 2 or 3 silently shifts the rest of the block.
- **`Toolkit::generateDirectionalLight` selects CSM purely by ARGUMENT COUNT** — `(name, colour,
  lux, 2048, 4, 0.5F)` is CSM, `(name, colour, lux, 2048, 140.0F)` is the classic map. The two are
  easy to confuse and the failure is silent. Worth settling in the same pass.
- **TODO(perf) left by `882e5f29`**: CSM now walks **every** caster (the light-entity distance cull
  was the bug). The right filter is the union of the per-cascade frustums — a separate concern, do
  not smuggle it in here.

## References

- [`../shadow-mapping.md`](../shadow-mapping.md) § Cascaded Shadow Maps — the full verified record.
- [`volumetric-light-single-scattering.md`](volumetric-light-single-scattering.md) — first external
  consumer of the shadow maps; the "outside the map = lit" convention must live in ONE place.
