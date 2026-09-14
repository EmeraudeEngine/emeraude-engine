---
id: specular-colour-factor-above-one-is-clamped
title: specularColorFactor > 1 is clamped away by Color<float>, and SpecularTest's last row tests it
status: open
priority: medium
scope: Graphics/Material
opened: 2026-09-14
blocked-by: []
tags: [gltf, material, measured, owner-decision]
---

# `specularColorFactor > 1` is clamped away, and one conformance row tests exactly that

## Why

`KHR_materials_specular` explicitly allows `specularColorFactor` above 1.0, so that a material's
IOR does not cap its specular response; `SpecularTest`'s seventh row (`color factor > 1.0`) exists
to check it, and its README says **"the last sphere on this row should look like a mirror ball"**.

The loader builds that factor as a `Base::PixelFactory::Color< float >`
(`GLTFLoader.cpp:1268`), and `Color`'s constructor clamps **every component to [0,1]**
(`emeraude-base/src/PixelFactory/Color.hpp:78`, `Math::clampToUnit`). Everything above 1 is gone
before the material ever sees it.

## Measured, 2026-09-14

The seven rows, under the bright reflected environment the test needs (mean sRGB per sphere, five
samples across each row):

| row | left → right |
|---|---|
| specular factor | 0.00 → 8.13, monotone |
| specular texture | 0.00 → 8.00, monotone (matches the row above to **0.06/255**) |
| white color factor | 0.00 → 8.48 |
| white color texture | 0.00 → 8.35 |
| yellow color factor | 0.00 → 5.29 (darker than white — a yellow tint absorbs) |
| yellow color texture | 0.00 → 5.37 |
| **color factor > 1.0** | 0.00 → **9.33, 9.35, 9.05, 8.87 — flat, and never a mirror** |

Six rows of seven pass cleanly. The seventh is the clamp.

## What remains — OWNER DECISION FIRST

The fix is **not** to unclamp `Color`: it is a foundation type whose contract is a colour in
[0, 1], used everywhere, and widening it would change far more than this. The choice is:

- [ ] carry the specular colour factor as a `Math::Vector< 3, float >` from the loader to
      `setSpecularComponent()` (a multiplier is not a colour), **or**
- [ ] keep `Color` and carry the excess separately (a scalar gain alongside the tint), **or**
- [ ] declare the clamp intentional and record `SpecularTest`'s last row as a knowing deviation.

⚠️ Whichever is chosen, check the shader end too: the spec clamps the *result*
(`F0 = min(dielectricF0 · specularColor · specularFactor, 1)`), which
`LightGenerator.PBR.cpp:589-590` already does — so the energy conservation is already in place and
only the transport of the authored value is missing.

## References

- `src/Scenes/Loaders/GLTFLoader.cpp:1260-1275`; `emeraude-base/src/PixelFactory/Color.hpp:78`.
- Capture: `~/.local/share/LNIsle/projet-alpha/captures/bench-gltf-20260914-final/SpecularTest_front.png`.
