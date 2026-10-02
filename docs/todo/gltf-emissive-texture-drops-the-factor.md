---
id: gltf-emissive-texture-drops-the-factor
title: A glTF emissive texture drops the material's emissiveFactor
status: open
priority: unranked
scope: Scenes/Loaders/GLTFLoader (the emissive), Graphics/Material/StandardResource (setAutoIlluminationComponent)
opened: 2026-10-03
tags: [gltf, emissive, materials, photometry]
---

# A glTF emissive texture drops the material's emissiveFactor

## Why

glTF 2.0 defines the emitted colour as `emissiveFactor × emissiveTexture` (× `KHR_materials_emissive_strength`).
`GLTFLoader.cpp` (the Emissive block, ~2846) calls `setAutoIlluminationComponent(emissiveTex, emissiveStrength)` when a
texture is present and uses `emissiveColor` (the factor) only when there is none. A textured emissive therefore
glows in the texture's raw colour, whatever its factor.

Found by reading, on Khronos' CarConcept (2026-10-03): its `Dashboard` material has factor (1, 0.259, 0) with
`Dash_E.png` (grey) and strength 3. The dashboard should glow orange; it glows in the texture's grey, at
3 × 2000 nits (the loader's `EmissiveLuminanceAnchor`). Materials with factor (1, 1, 1) — CarConcept's rims, mirrors,
brakes, hardware over `Khronos_C.png` — are unaffected.

## What remains

- Multiply the texture by the factor (a material-side colour × texture product, or a factor uniform on the
  texture component), then verify on CarConcept's dashboard and on `EmissiveStrengthTest` /
  `EmissiveTest`-style samples in the `+ModelViewer`.

## References

- glTF 2.0 § 3.9.2 (material.emissiveFactor, emissiveTexture); KHR_materials_emissive_strength.
