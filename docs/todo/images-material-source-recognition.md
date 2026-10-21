---
id: images-material-source-recognition
title: Recognise a conventionally named Images file as a material source (T13)
status: open
priority: unranked
scope: Resources (Images store), Graphics/Material, tools/texture-bake
opened: 2026-10-22
tags: [textures, naming]
---

# Recognise a conventionally named Images file as a material source (T13)

## Why

Owner decision T13 (graphics § 40): in `Images` the convention is lax, and an image named `<Name>.<Purpose>.<ext>` with a
vocabulary token is RECOGNISED as a material source: a material can then be built from a texture-set name alone, and
the bake tool finds its sources without a recipe. The parser exists (`TextureName::parse(…, TextureStore::Sources)`)
but nothing uses it on `Images` yet; projet-alpha's sources are not renamed either (`source-images-pbr-naming-pass`).

## What remains

- A material (JSON or code) declared by a texture-set name whose components come from the `Images` files of that name.
- `EmeraudeTextureBake` baking a set from those files without a material definition.

## References

- `docs/subsystems/graphics/40-gpu-textures-store.md` (T13), base `src/PixelFactory/TextureName.hpp`.
