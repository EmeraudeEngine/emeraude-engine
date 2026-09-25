---
id: subsurface-component-defects
title: The Subsurface material component — missing 1/π, NaN at radius 0, dead intensity with a texture, and stale docs
status: open
priority: unranked
scope: Saphir/LightGenerator (PBR light pass, ambient pass), Graphics/Material/StandardResource, docs
opened: 2026-09-25
tags: [material, subsurface, photometry]
---

# The Subsurface material component — missing 1/π, NaN at radius 0, dead intensity with a texture, and stale docs

## Why

Found by the leaf-translucency design audit (2026-09-25), each confirmed by an adversarial code check. The
component (`StandardResource.hpp` ~926-944, light pass `LightGenerator.PBR.cpp` ~938-962) is the engine's only
back-lit term; its users are Porcelain and the Liminal gems (their look changes when it is repaired).
- no 1/π in the wrapped diffuse (~949, ~959) — not photometric;
- the wrap reaches `N·L = -w`;
- the specular is not multiplied by `N·L`;
- `smoothstep` is undefined at intensity 0 or 1;
- NaN at radius 0 with a thickness map;
- with a TEXTURE the intensity is the texel `.r` alone: the UBO intensity (`setSubsurfaceIntensity`) is dead;
- the ambient SSS term (`LightGenerator.cpp` ~1320-1338) is a unitless constant and is NOT multiplied by
  `IBLDiffuseWeight`, so it stays active under both lanes;
- stale docs: `Saphir/AGENTS.md` ~482 claims a 0.99 clamp that does not exist and cites stale lines;
  `Graphics/AGENTS.md` ~5029-5031 says the term "needs a thickness" (false: a no-thickness branch exists,
  `exp(-0.5/radius)`) and names a non-existent item.

## What remains

Fix each point, re-measure Porcelain and the Liminal gems at a pinned exposure (before/after), fix the docs.
Owner decision (2026-09-25): a prerequisite of `foliage-diffuse-transmission`, which gets its OWN component — the
Subsurface component stays the skin/wax model.
