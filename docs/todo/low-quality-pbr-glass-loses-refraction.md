---
id: low-quality-pbr-glass-loses-refraction
title: A PBR glass generated at low quality loses its refraction (and transmission) in the ambient pass
status: open
priority: high
scope: src/Saphir/LightGenerator.cpp (ambient pass)
opened: 2026-10-13
tags: [saphir, lighting, glass, refraction, quality-tier, defect]
---

# A PBR glass generated at low quality loses its refraction (and transmission) in the ambient pass

## Why

Found by the board audit (2026-10-13) while closing `ambient-pass-dead-branches`. In the ambient-pass chain of
`src/Saphir/LightGenerator.cpp`, the high-quality legs come first (reflection + refraction, reflection + transmission,
reflection alone, each gated by `generator.highQualityEnabled()`), then the low-quality fallbacks:
`else if ( m_useReflection )` (l.1193) and `else if ( m_useRefraction )` (l.1249). A material with BOTH reflection and
refraction (a glass) generated without `HighQualityEnabled` therefore takes the reflection-only fallback: no refracted
leg at all, and the transmission / Beer absorption of the refraction fallback (l.1255-1262) never runs. The glass renders
as an opaque reflector.

**Latent today:** `SceneRendering` enables `HighQualityEnabled` unconditionally (`src/Saphir/Generator/SceneRendering.hpp:91`,
the `@todo` at l.75), so no program is generated at low quality yet. It becomes visible the day item
`quality-tier-distance-driver` drives the flag by distance.

## What remains

- [ ] Give the low-quality chain a reflection + refraction leg (the Fresnel mix without the per-fragment
      `reflectionNormal` / `reflectionI`, the same approximation as the reflection fallback), with the refraction
      fallback's transmission / Beer absorption.
- [ ] Prove it: a glass material generated at both qualities (force the flag off for the test), the generated GLSL read,
      a pinned-exposure capture, 0 VUID.

## References

- `docs/caution-points.md` § Shader/GLSL Pitfalls, *The ambient-pass chain has ONE lighting model*.
- Item `quality-tier-distance-driver` (the switch that will expose it).
