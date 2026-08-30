---
id: gbuffer-blended-normals-matprops-policy
title: Blended materials still REPLACE the normals and material-properties lanes over their whole quad
status: open
priority: unranked
scope: Saphir/Generator/SceneRendering
opened: 2026-08-30
tags: [g-buffer, transparency, reflections]
---

# Blended materials still REPLACE the normals and material-properties lanes over their whole quad

## Why

The albedo attachment of a blended material is now alpha-blended by `weight · opacity` (Aug 2026).
Normals and material properties keep the REPLACE policy — their alpha lanes are packed data (normals:
roughness + 2·metalness; matprops: nibbles), so `SRC_ALPHA` cannot be an opacity there — which the
flat-water fix relies on. Consequence: a decal quad still publishes ITS roughness/metalness and
reflectivity nibble over its transparent texels. With RTR/SSR on, a "metal" decal quad is a
reflective rectangle on a matte wall.

## What remains

1. Measure with RTR on Sponza once the reflections are back (the "fourmillant" report may include it).
2. Options: a separate opacity source for the blend factor (dual-source blending), or `discard` for
   blended fragments under an opacity threshold in the passes that write the G-buffer (needs the
   colour write to stay blended — a per-attachment discard does not exist), or a dedicated decal path.

## References

- `src/Saphir/Generator/SceneRendering.cpp` § onGraphicsPipelineConfiguration.
- `docs/caution-points.md` § "big dark squares under Sponza's decals".
