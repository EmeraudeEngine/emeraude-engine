---
id: iridescence-missing-from-transmissive-ambient
title: A TRANSMISSIVE iridescent surface gets no iridescence in the ambient pass
status: open
priority: medium
scope: Saphir/LightGenerator
opened: 2026-08-29
tags: [measured, gltf, iridescence, transmission]
---

# Iridescence is absent from the ambient pass of a transmissive material

## Why

`generateAmbientFragmentShader()` applies the iridescent Fresnel in **one** branch only — the
reflection-without-transmission one (`m_useIridescence` inside the IBL block). A material that is
both **iridescent and transmissive** takes the combined reflection+transmission branch instead,
which computes a plain dielectric Fresnel and never calls `evalIridescence()`.

The result is the same class of defect that was just fixed for the film thickness: **the surface is
iridescent in the light passes and not in the ambient one**. Measured on
`IridescentDishWithOlives.glb` — 26 generated light-pass shaders carry `iridescenceThickness`,
**zero** ambient shaders do. `glassCover` is exactly such a material (transmission 1, ior 1.5,
iridescence with a thickness map).

Since the ambient pass is where the environment reflection lives, that is precisely where a thin
film is most visible — and it is the half the Khronos reference shows most strongly.

## What remains

- [ ] Decide how the iridescent Fresnel composes with the reflection/transmission Fresnel split in
  the combined branch. The two currently compute their own `fresnelDielectric` / `fresnelIBL`
  independently; iridescence replaces F, so it belongs at that split, not on top of it.
- [ ] Whatever the shape, both branches must end up calling the same helper, the way
  `iridescenceThicknessExpression()` now serves both passes.

## ⚠️ Traps

- ⚠️ The check is the generated GLSL, not the picture: count the shaders that mention
  `evalIridescence` and confirm the ambient ones are among them. A thin film on a dark background
  is easy to talk yourself into seeing.
- ⚠️ Do not "fix" this by forcing the iridescence branch to win: a transmissive surface still needs
  its transmission composed, and that composition is itself recent
  (`src/Saphir/AGENTS.md` § "Transmission belongs to the AMBIENT pass").

## References

- `Saphir/LightGenerator.cpp::generateAmbientFragmentShader()` — the `m_useIridescence` block and
  the combined reflection+transmission branch above it.
