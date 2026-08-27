---
id: gltf-conformance-loader-gaps
title: glTF conformance — the loader gaps the bench located (re-measured 2026-08-27)
status: open
priority: high
scope: Scenes/Loaders/GLTF
opened: 2026-08-25
blocked-by: [hardcoded-near-plane]
tags: [gltf, material, measured]
---

# glTF conformance — the loader gaps the bench located

## Why

Bench re-run on **2026-08-27** against `glTF-Sample-Assets` @ `2bac6f8` (19 models, vendored in
`dependencies/`), 44 captures, **zero VUID and zero `VK_ERROR`** with the Khronos validation layer
active: **7 PASS / 10 FAIL / 2 undecided** — the same headline as the 2026-08-25 run, but one gap
is closed and two failures are now attributed to a cause that is not this loader.

Since the 2026-08-25 run, `c77313f7` closed the **sampler wrap mode** gap. Measured on the
`TextureTransformTest` scale quad, where the UV deliberately leaves `[0,1]`: the area outside is a
uniform grey (std **5.3** and **8.0** out of 255) with **0.00 %** arrow-ink pixels, against std
57.3 and 11.19 % ink inside the sampled square. CLAMP_TO_EDGE is honoured; a REPEAT would put a
second arrow there. Only `MetalRoughSpheres` and `TextureTransformTest` declare a non-default wrap
in the whole bench, so that is the full extent of the change.

**PASS (7)** — each with the number that decides it:

| model | measurement |
|---|---|
| `NormalTangentTest` | geometry 98.7° vs normal-mapped 93.2°, max row delta 11.5°, no sign flip |
| `AlphaBlendModeTest` | MASK cutoffs **0.260 / 0.504 / 0.748** vs 0.25 / 0.50 / 0.75; OPAQUE control flat (spread 0.017); BLEND ramp ×3.50 monotone |
| `OrientationTest` | 6/6 arrows on their same-colour target, quaternion **and** matrix encodings |
| `EmissiveStrengthTest` | linear-luminance ratios 2.21 / 2.28 / 1.97 / 1.59 against a declared 1-2-4-8-16 doubling |
| `MetalRoughSpheres` | clean mirror-to-diffuse progression (also the exposure control) |
| `WaterBottle` | matches the reference, chirality correct |
| `BoomBox` | correct, but only 14.1 % of frame height (near-plane clamp) |

**FAIL (10)**: NormalTangentMirrorTest, TextureTransformTest (rotation only, see below),
VertexColorTest, SpecularTest, ClearCoatTest, TransmissionTest, TransmissionRoughnessTest,
Iridescence ×2, AnisotropyStrengthTest.

**Undecided (2)**: `MetalRoughSpheresNoTextures` (5.5 % of frame height, blocked by
`hardcoded-near-plane`), `SheenCloth` (the viewer's daylight sky cannot isolate a sheen rim —
Khronos shoots it on black).

## What remains

- [ ] **`KHR_materials_specular` + `KHR_materials_ior`: declared, NEVER READ.** fastgltf mask at
  `GLTFLoader.cpp:482`, but no access to `glTFMaterial.specular` — the only other occurrence is a
  comment at `:1194`. `setSpecularComponent/Factor/Color` exist on `Material::StandardResource` and
  are called NOWHERE else in the cascade. `ior`: zero occurrences. Wiring, not implementation.
  Measured symptom: the 35 spheres all sit between **3.65 and 3.83 out of 255** (total spread 0.18)
  with **zero exactly-black pixels** — drawn, then crushed, not absent.
- [ ] **Iridescence**: `GLTFLoader.cpp:1119-1125` reads only `iridescenceFactor`;
  `iridescenceIor` and the film thickness (min/max/texture) are never parsed — and those are the
  two axes both test models sweep. The test CANNOT pass as it stands.
- [ ] **Clearcoat / transmission / sheen**: only the scalar factors are read, no texture (nor coat
  normal map). ClearCoatTest shows **no separation at all** between its `Base layer`, `Coated` and
  `Coating Only` columns, so the factor→shading wiring must be verified too — but see
  `material-basecolor-factor-collapse.md` first: part of what that model shows is not this loader.
- [ ] **`KHR_texture_transform` rotation**: offset ✓ and scale ✓ and clamp ✓ — **rotation ✗** is
  now the SOLE remaining cause of this model's failure. The loader logs it and drops it
  (`GLTFLoader.cpp:997`); the asset declares 0.39270 rad and the arrow lands on the yellow
  "not applied" marker instead of the green one.
- [ ] Supplied tangents ignored (`NormalTangentMirrorTest`). Measured: the `Geometry` and `Normal`
  columns hold a level horizon (means 91.3° and 94.4°, spreads 12.8° and 17.1° over five
  roughnesses), while `V Mirror` and `U Mirror` scatter over **263.8°** and **274.1°**. The
  reflection is rotated, not merely flipped.
- [ ] `COLOR_0` not multiplied: zero occurrences in the loader, and the capture shows the README's
  literal failure signature — a cyan X on the red check, magenta on the green, yellow on the blue.
- [ ] Anisotropy produces no elongation. Measured as the aspect ratio of the top 5 % brightest
  pixels: reference **3.65** at anisotropy 1.0 against **1.62** at 0.0 (ratio **2.26**); ours
  **2.67** against **3.29** (ratio **0.81**, inside the noise and pointing the wrong way).

## ⚠️ Traps of this bench (they cost real time)

The bench harness and its traps now live with the tool, in
[`tools/gltf-conformance-bench/README.md`](../../tools/gltf-conformance-bench/README.md).
Two that this run added, because each one nearly produced a wrong verdict:

- **A reference screenshot shot in a dark studio is not a criterion.** `EmissiveStrengthTest` looks
  like a failure next to its reference — no cube glows — purely because the viewer meters a
  daylight sky at sunny-sixteen. Judged on the RATIOS between cubes it passes cleanly.
- **Luminance variation along a test's axis is not proof the axis works.** The anisotropy grid
  varies 14.3 % down its anisotropy axis at roughness 0 and 2.8 % at roughness 1 — which is exactly
  what a sky gradient over spheres at different heights produces, with no anisotropy at all. The
  2026-08-25 run's "16.2 % where the spec mandates zero" was that confound. Measure the highlight
  SHAPE.

## References

- Captures: `~/.local/share/LNIsle/projet-alpha/captures/bench-gltf-20260827/` (44 PNG +
  `bench-report.json`).
- The owner's gallery verdict on the pre-merge bench run
  (`captures/bench-gltf-20260812/galerie-banc-gltf.html`) was the D5 gate of the material merge;
  it is still pending.
