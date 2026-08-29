---
id: transmission-composited-as-addition
title: Transmissive glass renders milky white — the transmission is ADDED to the diffuse instead of replacing it
status: open
priority: high
scope: Saphir/LightGenerator + Graphics/Material
opened: 2026-08-29
tags: [measured, gltf, transmission, ior]
---

# Transmissive glass renders milky white

## Why

Reported by the owner on `IridescentDishWithOlives.glb`: *"c'est l'effet du verre qui va pas du
tout"*. The glass dish and cover render as **opaque milky white** with the olives washed out behind
a white haze, where the Khronos reference shows clear glass with the olives plainly visible.

## What is actually wrong — four separate things, in order of impact

### 1. The transmission is ADDED, never mixed (the main cause)

`LightGenerator.PBR.cpp`, direct-light pass:

```glsl
const vec3 transmitted = albedo * transBackAbsorption * transNdotLBack * transmissionFactor;
fragmentColor.rgb += transmitted * radiance;      /* ADDED */
```

The diffuse BRDF is computed and kept **in full**, and a transmission term is added on top. glTF
specifies the opposite: light that passes *through* a surface cannot also scatter back off it, so

```
diffuse = mix(diffuse_brdf, transmitted, transmissionFactor)
```

With `transmissionFactor = 1` and a white base colour — exactly this asset — the result is a full
white diffuse **plus** transmitted light. That is the milky white, and it is energy-non-conserving.

### 2. That "transmitted" term is back-lit translucency, not see-through transmission

`albedo * absorption * max(dot(-N, L), 0) * transmissionFactor` is the wax/leaf/lampshade
approximation: light entering from *behind* relative to a light source. It is not "what is behind
the object, refracted". The see-through path is a different one — the grab pass below — so the two
concepts share a single `transmissionFactor` while modelling different physics.

### 3. The grab-pass refraction offset is calibrated against a thickness that used to be constant

`StandardResource.cpp`, high-quality grab-pass path (gated on bindless **and** high quality):

```glsl
float gpEta = 1.0 / RefractionIOR;
vec3 gpRefractDir = refract(gpViewDir, reflectionNormal, gpEta);
vec2 gpOffset = gpRefractDir.xy * ThicknessFactor * 0.05;   /* world thickness -> screen UV ?! */
```

⚠️ The IOR **does** bend the ray here — that part works. But the offset multiplies a **world-space
thickness** by a constant to obtain a **screen-space UV** offset, which is dimensionally
meaningless. It only ever looked right because `ThicknessFactor` was hard-coded to the engine
default **1.0**; since `KHR_materials_volume` was wired (2026-08-28) real assets supply their real
thickness — 0.01 and 0.1 here — and the offset collapses to 0.05 % of the screen.

⚠️⚠️ **Do NOT "fix" this by reverting the volume wiring.** Measured A/B on this very asset: forcing
`thicknessFactor` back to 1.0 makes the cover **MORE opaque**, not less — the olives disappear
entirely instead of showing through a haze. The calibration is wrong in both directions; the volume
wiring is correct and merely exposed it. A screen-space offset needs the object's apparent size and
the view, not a bare world thickness times a magic 0.05.

### 4. `iridescenceThicknessTexture` is not read, and this asset lives on it

Both iridescent materials declare `iridescenceThicknessMinimum: 500`, `Maximum: 550` **and** a
thickness texture. The whole visible pattern is that texture modulating a 50 nm band. Without it the
thickness is the constant maximum (spec-correct fallback) and the film shows **one uniform colour**
instead of the reference's varying rainbow. Needs its own `ComponentType`, as the two specular maps
did.

## What remains

- [ ] Replace the addition with the spec's mix, and decide what `transmissionFactor` means when the
  same scalar drives both the back-lit term and the grab pass.
- [ ] Re-derive the grab-pass offset so it is dimensionally sound (screen-space, view-dependent),
  instead of a world thickness times 0.05.
- [ ] Read `iridescenceThicknessTexture` (new `ComponentType`, G channel, `mix(min, max, tex.g)` —
  the shader's `mix(min, max, 1.0)` already has the 1.0 waiting to become that channel).
- [ ] Confirm whether the grab-pass path even ran here: it is gated on `bindlessTexturesEnabled()
  && highQualityEnabled()` and nothing in the log says which tier was chosen. **Measure the tier
  before attributing anything else to the composition.**

## ⚠️ Traps

- **`refract()` is NOT absent from the engine.** A first pass grepped only `src/Saphir/` and
  concluded the IOR never bends anything — wrong: the grab-pass code lives in
  `Graphics/Material/StandardResource.cpp`. Grep the material layer too before concluding a
  rendering feature is missing.
- **A rendering regression is not always the last change.** The volume wiring was the obvious
  suspect here and the A/B exonerated it in one build. Do the A/B.

## References

- Asset: `projet-alpha.data/data-stores/glTF/IridescentDishWithOlives.glb` (`glassDish`,
  `glassCover`), and the Khronos sample-assets page for the reference image.
- `Saphir/LightGenerator.PBR.cpp` (transmission composition),
  `Graphics/Material/StandardResource.cpp::generateGrabPassTransmissionFragmentShader()`.
