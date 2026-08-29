---
id: volume-thickness-texture
title: KHR_materials_volume thicknessTexture is read as a constant factor — the map is ignored
status: open
priority: low
scope: Scenes/Loaders/GLTFLoader + Graphics/Material/StandardResource
opened: 2026-08-29
tags: [gltf, volume, khr-extension]
---

# `KHR_materials_volume`'s `thicknessTexture` is ignored

## Why

`thicknessFactor` is read and now does real work — it is the LENGTH of the refraction ray whose
exit point the screen-space refraction projects. Its **texture** (G channel, multiplying the factor)
is logged and dropped:

```
Material 'glassCover': KHR_materials_volume thicknessTexture is not supported yet (the factor is applied), ignored.
```

A varying thickness is what makes a moulded glass object refract unevenly — thick at the base, thin
at the rim. With the factor alone the whole surface bends light by the same amount.

## What remains

- [ ] Give it a `ComponentType` and read the **G** channel, the way
  `ComponentType::IridescenceThickness` was added (2026-08-29) — that one is the template to follow,
  including the DATA (never sRGB) resolution and the independent-of-the-other-map handling.
- [ ] Feed it into the grab-pass ray length in place of the UBO scalar, in **all three** sites:
  the standard branch, the three dispersion rays, and the low-quality branch.

## ⚠️ Traps

- ⚠️ `SubsurfaceThickness` already exists and is a **different quantity** — do not reuse it.
- ⚠️ The thickness is a MESH-space length: whatever replaces the scalar must still be multiplied by
  `svModelScale`, like the current one.

## References

- `Scenes/Loaders/GLTFLoader.cpp` (volume block), `Graphics/Material/StandardResource.cpp`
  (`generateGrabPassTransmissionFragmentShader()`).
- Asset: `projet-alpha.data/data-stores/glTF/IridescentDishWithOlives.glb` (`glassCover`).
