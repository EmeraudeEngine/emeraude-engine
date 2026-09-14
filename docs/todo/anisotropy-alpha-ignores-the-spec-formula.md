---
id: anisotropy-alpha-ignores-the-spec-formula
title: Anisotropy still acts at roughness 1.0, where KHR_materials_anisotropy says it must not
status: open
priority: medium
scope: Saphir/LightGenerator
opened: 2026-09-14
blocked-by: []
tags: [gltf, brdf, material, measured, owner-decision]
---

# Anisotropy still acts at roughness 1.0, where the extension says it must not

## Why

`AnisotropyStrengthTest`'s README states the criterion outright:

> The effects should be most apparent in the column with zero material roughness, and **anisotropy
> should not offer any effect when material roughness is 1.0**.

The engine's directional roughness (`src/Saphir/LightGenerator.PBR.cpp:646` and `:663`, the
per-pixel and uniform branches) is:

```glsl
const float at = max(alphaRoughness * (1.0 + anisoValue), 0.001);
const float ab = max(alphaRoughness * (1.0 - anisoValue), 0.001);
```

At roughness 1.0, `alphaRoughness` is 1.0 and this gives `at = 2.0`, `ab = 0.0` — maximally
anisotropic, and `at` outside the valid range of a GGX alpha. The glTF sample viewer instead
widens ONE axis toward the isotropic ceiling, which collapses to isotropy exactly when the
material is already fully rough.

## Measured, 2026-09-14 (first time this test was measurable at all)

The lobe elongation (major/minor axis of the highlight at half maximum), in a dark environment
where the only specular source is the viewer's key light — the bench poses this now:

| | roughness 0.17 | 0.33 | 0.50 | 1.00 |
|---|---|---|---|---|
| anisotropy 1.00 | 9.26 | 5.92 | 4.10 | **2.93** |
| anisotropy 0.50 | 2.05 | 1.82 | 1.83 | 1.39 |
| anisotropy 0.00 | 1.11 | 1.68 | 1.65 | **1.59** |

The axis **works** — monotone over six steps, which no previous bench run could show. But the last
column is the finding: at roughness 1.0 the lobe still stretches **×1.84** from anisotropy 0 to 1,
where the extension mandates ×1.

⚠️ **This became visible only because the environment changed.** In the viewer's daylight sky a
sphere's brightest pixels are an image of the sky, not a BRDF lobe, and five successive metrics
read noise. Nothing about the shading changed between those runs.

## What remains — OWNER DECISION FIRST

- [ ] **Decide the formula.** Do not copy one from memory: read
      `KHR_materials_anisotropy`'s implementation notes and the glTF Sample Viewer's
      `material_info.glsl` side by side, then choose. The candidate is
      `at = mix(alphaRoughness, 1.0, anisotropy²)`, `ab = alphaRoughness`, which satisfies the
      "no effect at roughness 1.0" clause by construction.
- [ ] Whatever is chosen, it changes the look of **every** anisotropic material, so it is a
      rendering-semantics decision, not a bug fix to be applied silently.
- [ ] Re-measure with the table above: the roughness-1.0 column must come out flat, and the
      low-roughness columns must stay monotone.

## References

- `src/Saphir/LightGenerator.PBR.cpp:646`, `:663`.
- Capture: `~/.local/share/LNIsle/projet-alpha/captures/bench-gltf-20260914-final/AnisotropyStrengthTest_front.png`.
- The bench poses the environment itself now: `ENVIRONMENTS` in `tools/gltf-conformance-bench/bench.py`.
