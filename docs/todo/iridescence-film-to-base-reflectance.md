---
id: iridescence-film-to-base-reflectance
title: Iridescence uses the base's reflectance against AIR as the film-to-base term
status: open
priority: unranked
scope: Saphir/LightGenerator (PBR direct light and ambient/IBL copies of evalIridescence)
opened: 2026-09-22
tags: [pbr, iridescence, gltf-conformance, cross-platform-bench]
---

# Iridescence uses the base's reflectance against AIR as the film-to-base term

## Why

Found by the macOS peer session on the glTF conformance bench (2026-09-22, ScreenSpace lane, all 343 + 343
spheres drawn). The failure is in the formula, not in a missing object:
- `IridescenceDielectricSpheres`: the base-IOR 1.0 layer shows no film at all. Along the film-IOR 2.0 edge,
  every thickness from 100 to 500 nm reads (216,215,213) to (216,216,214), saturation 0.013. That is where
  the Khronos reference is most colourful, so the colour trend is inverted.
- `IridescenceMetallicSpheres`: the black-base layer (F0 = 0) reads (38,41,48) at every thickness, where the
  reference shows gold, teal and magenta.

Cause, confirmed in the source: `vec3 R23 = baseF0;` in `evalIridescence()`, in BOTH copies
(`LightGenerator.PBR.cpp:197` direct light, `LightGenerator.cpp:716` ambient). R23 must be the FILM→BASE
interface reflectance. `baseF0` is the base against air: F0 = 0 gives R23 = 0 and F = R12, achromatic. It is
also wrong when the film IOR equals the base IOR (R23 must then be 0). There are no interface phase shifts
either.

## What remains

- [ ] Belcour & Barla 2017 ("A Practical Extension to Microfacet Theory for the Modeling of Varying
      Iridescence"), as in the Khronos glTF Sample Viewer: base IOR from F0, film→base Fresnel at the refracted
      angle, the π phase shifts. One implementation shared by both copies.
- [ ] Re-judge both grids. The conformance item lists them as PASS on "the film sweep reads", and that
      verdict missed this.

## References

- Khronos glTF Sample Viewer, `iridescence.glsl`; `KHR_materials_iridescence` specification.
