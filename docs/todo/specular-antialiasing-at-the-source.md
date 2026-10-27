---
id: specular-antialiasing-at-the-source
title: Specular antialiasing in the material, so a far normal-mapped surface does not alias before the TAA
status: in-progress
priority: medium
scope: Graphics/Material/StandardResource, Saphir (lighting code), texture mip generation
opened: 2026-09-23
tags: [antialiasing, specular, normal-map, taa]
---

# Specular antialiasing in the material, so a far normal-mapped surface does not alias before the TAA

## Why

Owner decision 2026-09-23 (TAA scope question): the TAA fix first, the SOURCE-side antialiasing as its own item.
The `relief` far ground shimmered because the TAA rejected its history (fixed: `docs/subsystems/graphics/13-12-post-processing-effects/06-the-taa-resolve-the-history-is-rejected-by-depth-never-by-co.md`); what the TAA then accumulates is still a normal-mapped surface whose sub-pixel normal variation aliases
the specular lobe. Without TAA (the jitter off) that aliasing is spatial and stable; with TAA it is the input the
accumulation has to average. Filtering it at the source lowers what the TAA must hide, and helps every path
without TAA (FXAA, window-less captures, reflections).

## Already in the engine

Tokuyoshi & Kaplanyan's geometric specular AA (σ² 0.25, κ 0.18), ONE function since 2026-10-27:
`LightGenerator::specularAntialiasedRoughness()`. Two callers:
- the roughness WRITTEN TO THE G-BUFFER (`Saphir/Generator/SceneRendering.cpp`, since `30a66b88f`, 2026-09-14), read by
  RTR and SSR;
- the DIRECT lights' BRDF (`LightGenerator.PBR.cpp`, 2026-10-27, owner decision "per pixel, in the lighting code"):
  every lobe of the base layer reads `pbrFilteredRoughness`. It fixed the `water-world` sun glints that burst under the
  motion blur (`docs/caution-points.md` § the direct specular had no geometric specular antialiasing).

## What remains

- The IBL / ambient pass (`LightGenerator.cpp`, the split-sum `iblEnvBRDF` lookup and the reflection's roughness) still
  reads the raw roughness: route it through the same function (the ambient pass already declares the perturbed `N`).
- Clear coat and sheen keep their own roughness, unfiltered: decide whether they take the same kernel.
- Measure: the far band of `relief` with the TAA OFF, specular term alone vs diffuse alone (the part of the residual
  that is specular), with `temporalCapture` + `tools/temporal-analysis.py` — before / after the IBL step.
- Other candidates from the state of the art, if the per-pixel kernel is not enough:
  - Toksvig, "Mipmapping Normal Maps" (JGT 2005): the length of the mip-averaged normal widens the roughness, baked
    per mip level;
  - LEAN / LEADR mapping (Olano & Baker 2010; Dupuy et al. 2013) for anisotropic filtering of slope moments.

## References

- The research summary of 2026-09-23 (TAA session): HDRP, UE4/TSR, FSR2, Decima, Playdead, SMAA T2x and the
  Yang/Liu/Salvi survey; the specular AA papers above.
