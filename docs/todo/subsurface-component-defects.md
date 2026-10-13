---
id: subsurface-component-defects
title: The Subsurface material component — missing 1/π, NaN at radius 0, dead intensity with a texture, and stale docs
status: open
priority: high
scope: Saphir/LightGenerator (PBR light pass, ambient pass), Graphics/Material/StandardResource, docs
opened: 2026-09-25
tags: [material, subsurface, photometry]
---

# The Subsurface material component — missing 1/π, NaN at radius 0, dead intensity with a texture, and stale docs

## Why

Found by the leaf-translucency design audit (2026-09-25), each confirmed by an adversarial code check. The
component (`StandardResource.hpp:934-952`; light pass `src/Saphir/LightGenerator.PBR.cpp:1058-1080` wrap diffuse,
`:1125-1147` back-lit transmittance; lines re-checked 2026-10-13) is the engine's only back-lit term; its users are Porcelain and the Liminal
gems (their look changes when it is repaired).
- no 1/π in the back-lit transmittance (`LightGenerator.PBR.cpp:1135`, `:1145`) — not photometric (the wrapped
  diffuse itself does divide by π, `:1064`, `:1078`);
- the wrap reaches `N·L = -w`;
- the specular is not multiplied by `N·L` (`:1066-1067` clear-coat path, `:1080`; the non-SSS paths do, `:1109`,
  `:1121`);
- `smoothstep(0.0, sssWrap, …)` / `smoothstep(sssWrap, 1.0, …)` are undefined at intensity 0 or 1 — no clamp
  (`:1063`, `:1077`);
- NaN at radius 0 with a thickness map (`exp(-thickness / sssRadius)`, `:1135`);
- with a TEXTURE the intensity is the texel `.r` alone: the UBO intensity (`setSubsurfaceIntensity`) is dead;
- the ambient SSS term (`src/Saphir/LightGenerator.cpp:1320-1338`) is a unitless constant and is NOT multiplied by
  `IBLDiffuseWeight`, so it stays active under both lanes;
- stale docs: `docs/subsystems/saphir/13-pbr-advanced-material-features.md:38` claims a 0.99 clamp that does not
  exist and cites stale lines; `docs/subsystems/graphics/20-15b-level-of-detail.md:65` says the term "needs a
  thickness" (false: a no-thickness branch exists, `exp(-0.5/radius)`).

## What remains

Fix each point, re-measure Porcelain and the Liminal gems at a pinned exposure (before/after), fix the docs.
Owner decision (2026-09-25): a prerequisite of `foliage-diffuse-transmission`, which gets its OWN component — the
Subsurface component stays the skin/wax model.
