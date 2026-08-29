---
id: transmission-grab-pass-screen-offset
title: The grab-pass refraction offset multiplies a world thickness to get a screen UV
status: open
priority: medium
scope: Graphics/Material/StandardResource
opened: 2026-08-29
tags: [measured, gltf, transmission, ior]
---

# The grab-pass refraction offset is dimensionally unsound

## Why

`StandardResource.cpp`, `generateGrabPassTransmissionFragmentShader()`, high-quality path:

```glsl
float gpEta = 1.0 / RefractionIOR;
vec3 gpRefractDir = refract(gpViewDir, reflectionNormal, gpEta);
vec2 gpOffset = gpRefractDir.xy * ThicknessFactor * 0.05;   /* world thickness -> screen UV ?! */
```

The IOR **does** bend the ray — that half works. But the offset multiplies a **world-space
thickness** by a magic constant to obtain a **screen-space UV** offset, which has no dimensional
meaning. It only ever looked plausible because `ThicknessFactor` was hard-coded to the engine
default `1.0`. Since `KHR_materials_volume` was wired (2026-08-28) real assets supply their real
thickness — `0.01` and `0.1` on `IridescentDishWithOlives.glb` — and the offset collapses to 0.05 %
of the screen, i.e. no visible refraction at all.

A screen-space offset needs the object's **apparent size** and the view, not a bare world thickness
times 0.05. The Khronos reference derives a refraction *ray* in world space, transforms its exit
point by the view-projection, and takes the screen delta — `getVolumeTransmissionRay()` +
`getIBLVolumeRefraction()` in the glTF Sample Viewer.

## What remains

- [ ] Re-derive the offset in screen space from the projected refraction ray.
- [ ] Keep the dispersion variant (three channel offsets) in lockstep with whatever replaces it.

## ⚠️ Traps

- ⚠️⚠️ **Do NOT "fix" this by reverting the volume wiring.** Measured A/B on
  `IridescentDishWithOlives.glb`: forcing `thicknessFactor` back to `1.0` makes the cover **MORE**
  opaque, not less — the olives disappear entirely instead of showing through. The calibration is
  wrong in both directions; the volume wiring is correct and merely exposed it.
- ⚠️ **`refract()` is not absent from the engine.** A first pass grepped only `src/Saphir/` and
  concluded the IOR never bends anything. The grab-pass code lives in the material layer. Grep
  `Graphics/Material/` too before declaring a rendering feature missing.

## References

- `Graphics/Material/StandardResource.cpp::generateGrabPassTransmissionFragmentShader()`.
- Asset: `projet-alpha.data/data-stores/glTF/IridescentDishWithOlives.glb` (`glassDish`, `glassCover`).
