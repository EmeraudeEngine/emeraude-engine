---
id: texture-purpose-vocabulary-beyond-ambientcg
title: The Purpose tokens beyond ambientCG's set
status: open
priority: unranked
scope: Graphics (base PixelFactory/TextureName), graphics § 40
opened: 2026-10-22
tags: [textures, naming, pbr]
---

# The Purpose tokens beyond ambientCG's set

## Why

The `GPUTextures` names (graphics § 40, T3, T12) take their tokens from ambientCG's nine words. The components ambientCG
does not name — clear coat, sheen, transmission, specular colour, iridescence, anisotropy, subsurface… — have no token
yet, so a material using them cannot have a `GPUTextures` entry for them, and projet-alpha's source renaming
(`source-images-pbr-naming-pass`, its legacy `-specular` maps) waits on it. An owner question (§ 40 § Open questions).

## What remains

- List the tokens with the owner, in ambientCG's style.
- Extend `TexturePurpose`, the parser's channel rules and its tests (emeraude-base), the bake tool's grouping.

## References

- `docs/subsystems/graphics/40-gpu-textures-store.md` (T3, T12, § Open questions).
