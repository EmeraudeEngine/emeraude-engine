---
id: ambient-diffuse-not-weighted-by-metalness
title: The ambient diffuse of a metal is not weighted by (1 - metalness)
status: open
priority: unranked
scope: Saphir/LightGenerator (the ambient pass, plain branch and the reflection / refraction mixes)
opened: 2026-10-03
tags: [pbr, ambient, ibl, metalness, owner-decision]
---

# The ambient diffuse of a metal is not weighted by (1 - metalness)

## Why

The direct passes weight their diffuse by `(1 - metalness)` (`kD`, `LightGenerator::diffuseWeightShaderExpression()`):
a metal has no diffuse lobe, its base colour is the specular F0. The ambient pass does not. Its two diffuse legs,
the scalar `albedo / π × ambient` and the IBL `albedo × irradiance × environment luminance`, are added at full weight
for every material (read in a dumped ambient shader, 2026-10-03). A metal therefore takes the ambient of a Lambertian
surface of its base colour, on top of its specular IBL.

The transmission weight was added to the same plain-branch legs on 2026-10-03 (`docs/caution-points.md` § *a thin
grab-pass glass scaled its transmission twice*). The metalness weight was left out on purpose: it changes the look of
every metal in every scene. That is the owner's decision.

## What remains

- Owner decision: weight the ambient diffuse by `(1 - metalness)` (the PBR-correct form) or keep the current look.
- If yes: apply `diffuseWeightShaderExpression()` to every ambient diffuse leg, including the reflection / refraction
  `mix()` forms. Then compare per pixel at a pinned exposure on a metal-heavy scene (Sponza's metals, glTF
  `MetalRoughSpheres`) and on the RT lane (RTGI owns the indirect diffuse there).

## References

- Related, not the same: `transmission-ignores-metalness` (a metal's TRANSMISSION, the reflection + transmission branch).
