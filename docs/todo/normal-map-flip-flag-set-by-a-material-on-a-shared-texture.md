---
id: normal-map-flip-flag-set-by-a-material-on-a-shared-texture
title: A material's "FlipNormalMapY" is set on the shared texture resource — two materials can disagree, and a late flag is ignored
status: open
priority: high
scope: src/Graphics/Material/Component/Texture.cpp, src/Graphics/TextureResource (Abstract, Texture2D)
opened: 2026-10-12
tags: [defect, materials, normal-map, shared-resource]
---

# A material's "FlipNormalMapY" is set on the shared texture resource

## Why

Found by reading while fixing `normal-map-flip-mutates-shared-image` (2026-10-12). The pixel flip now works on a copy,
so the store IMAGE is safe, but the FLAG is not per material: `Material::Component::Texture` reads `"FlipNormalMapY"`
from the material JSON and calls `m_textureResource->enableFlipNormalMapY(flip)` on the texture RESOURCE it found in
the store by name (`Texture.cpp` ~119-123). Texture resources are shared by name, so:

- two materials naming the same texture with different flags: the last one parsed wins for both;
- a flag set after the texture was already uploaded (another material loaded it first) changes nothing on the GPU,
  silently.

projet-alpha's `normal-map-debug` is not affected: it builds one texture resource per flip state
(`…/flippedGreen/texture`).

## What remains

- Owner decision: make the flip part of the texture's IDENTITY (a material that asks for it gets a distinct texture
  resource, e.g. the name suffixed, as normal-map-debug does by hand), or refuse a material whose flag disagrees with an
  already-created texture (an error naming both).
- Proof: two materials on one normal map, one flipped, in one scene; each shows its own convention.

## References

- `docs/coordinate-system.md` (the normal-map convention, the copy-at-upload rule).
