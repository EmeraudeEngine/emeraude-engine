---
id: iridescence-thickness-texture
title: iridescenceThicknessTexture is not read — the film shows one uniform colour
status: open
priority: medium
scope: Scenes/Loaders/GLTFLoader + Graphics/Material/StandardResource
opened: 2026-08-29
tags: [gltf, iridescence, khr-extension]
---

# `iridescenceThicknessTexture` is not read

## Why

`KHR_materials_iridescence` sweeps the film thickness between
`iridescenceThicknessMinimum` and `iridescenceThicknessMaximum`, and the **G channel** of
`iridescenceThicknessTexture` is what places each texel inside that band. Without the texture the
thickness is the constant maximum — the spec-correct fallback — so the film renders **one uniform
colour** where the reference shows a varying rainbow.

`IridescentDishWithOlives.glb` lives on it: both iridescent materials declare
`iridescenceThicknessMinimum: 500`, `Maximum: 550` **and** a thickness texture. The whole visible
pattern is that texture modulating a 50 nm band.

## What remains

- [ ] New `ComponentType` for the volume/iridescence thickness map, the way the two
  `KHR_materials_specular` maps were added.
- [ ] Read the **G** channel and emit `mix(min, max, tex.g)` — the generated shader already carries
  `mix(min, max, 1.0)`, and that `1.0` is exactly the slot waiting for the channel.
- [ ] While a thickness `ComponentType` exists, wire `KHR_materials_volume`'s `thicknessTexture`
  too: the loader currently warns and ignores it (`GLTFLoader.cpp`, volume block). ⚠️ `SubsurfaceThickness`
  is a DIFFERENT quantity — do not reuse it.

## References

- `Scenes/Loaders/GLTFLoader.cpp` (iridescence block), `Graphics/Material/StandardResource.cpp`.
- Asset: `projet-alpha.data/data-stores/glTF/IridescentDishWithOlives.glb`.
