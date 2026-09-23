---
id: specular-antialiasing-at-the-source
title: Specular antialiasing in the material, so a far normal-mapped surface does not alias before the TAA
status: open
priority: unranked
scope: Graphics/Material/StandardResource, Saphir (lighting code), texture mip generation
opened: 2026-09-23
tags: [antialiasing, specular, normal-map, taa]
---

# Specular antialiasing in the material, so a far normal-mapped surface does not alias before the TAA

## Why

Owner decision 2026-09-23 (TAA scope question): the TAA fix first, the SOURCE-side antialiasing as its own item.
The `relief` far ground shimmered because the TAA rejected its history (fixed: `src/Graphics/AGENTS.md` § The TAA
resolve); what the TAA then accumulates is still a normal-mapped surface whose sub-pixel normal variation aliases
the specular lobe. Without TAA (the jitter off) that aliasing is spatial and stable; with TAA it is the input the
accumulation has to average. Filtering it at the source lowers what the TAA must hide, and helps every path
without TAA (FXAA, window-less captures, reflections).

## What remains

- Measure first: the far band of `relief` with the TAA OFF, specular term alone vs diffuse alone (the part of the
  residual that is specular), with `temporalCapture` + `tools/temporal-analysis.py`.
- Candidates, from the state of the art:
  - Tokuyoshi & Kaplanyan, "Improved Geometric Specular Antialiasing" (I3D 2019): roughness² += min(2·σ²·(|∂n/∂x|² +
    |∂n/∂y|²), κ), σ² = 0.25, κ = 0.18 — screen-space derivatives of the shading normal, deferred-friendly (HDRP ships
    it as Geometric Specular AA);
  - Toksvig, "Mipmapping Normal Maps" (JGT 2005): the length of the mip-averaged normal widens the roughness, baked
    per mip level;
  - LEAN / LEADR mapping (Olano & Baker 2010; Dupuy et al. 2013) for anisotropic filtering of slope moments.
- Decide with the owner where it lives: the lighting code (per pixel, every material) or the mip generation of
  the normal and roughness textures (baked, zero runtime cost).

## References

- The research summary of 2026-09-23 (TAA session): HDRP, UE4/TSR, FSR2, Decima, Playdead, SMAA T2x and the
  Yang/Liu/Salvi survey; the specular AA papers above.
