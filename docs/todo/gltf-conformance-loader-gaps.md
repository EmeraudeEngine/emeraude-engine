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
Khronos shoots it on black). ⚠️ `MetalRoughSpheresNoTextures` was undecided for TWO independent
reasons and only one of them was known: the framing, and the fact that its 98 spheres were one
sphere. The second is fixed (see the 2026-08-28 section); the framing still stands.

> [!CAUTION]
> **2026-08-28 — FIVE OF THOSE VERDICTS ARE VOID, and one of the undecided with them.** The root
> cause found that day is not in this loader's material handling at all: the loaders keyed every
> resource on the asset's **name**, which neither glTF nor FBX makes unique, so a grid of meshes
> sharing a name collapsed onto ONE renderable — one geometry, one material. Fixed in
> `Scenes::Loaders::buildResourceKey()`; see
> [`docs/caution-points.md`](../caution-points.md) § *Fixed: an asset NAME was used as a resource
> identity* (the item file that tracked it is gone — the work is done).
>
> | verdict | duplicate names it was measured through | re-judged 2026-08-28 |
> |---|---|---|
> | `ClearCoatTest` FAIL | `ClearCoatSampleMesh` ×18 | **the collapse WAS the whole failure** — see below |
> | `TransmissionTest` FAIL | `Sphere` ×12 + 3 different materials named `BlueTransWithMask` | **the collapse WAS the whole failure** |
> | `MetalRoughSpheresNoTextures` undecided | `Sphere` ×98 | **now renders its grid** — see below |
> | `TransmissionRoughnessTest` FAIL | `RoughnessSamples` ×6, images `RoughnessGrid` ×2 | **still FAIL**, cause re-attributed to `ior` |
> | `SpecularTest` FAIL | `OneSample` ×20, `FiveSamples` ×3 | **still FAIL, capture BIT-IDENTICAL** — the collapse was invisible here |
>
> ⚠️ **The other verdicts stand.** `AnisotropyStrengthTest` and both iridescence models have
> **unnamed** meshes and materials, so the index fallback already protected them: their failures
> are genuine extension gaps. `NormalTangentMirrorTest`, `TextureTransformTest`, `VertexColorTest`
> and all seven PASS models carry unique names and were never affected.

## Re-run 2026-08-28 — the resource-key fix, measured (44 captures, ZERO VUID)

Captures: `~/.local/share/LNIsle/projet-alpha/captures/bench-gltf-20260828/`. Same script, same
framing as the 2026-08-27 run, whose captures **survive on disk** — so this pass is a true pixel
A/B, not a re-description.

**⚠️⚠️ THE METHOD THAT DECIDED IT — a full-frame pixel diff against the previous run, partitioned
by whether the asset has duplicate names.** This is what a loader-wide change must be verified
with, and it is far stronger than re-reading individual captures:

| group | captures | changed pixels | max delta |
|---|---|---|---|
| models with **unique** names | 32 | 0.000 % … 1.322 % | **≤ 2 / 255** |
| models with **duplicate** names | 12 | 0.000 % … **15.137 %** | **243 / 255** |

The ≤2 LSB on the unique-name group is ordinary temporal dither: **the fix is a bit-exact no-op
wherever names are unique**, which is the control this change needed. Change happens if and only
if an asset has duplicate names AND the aliased materials actually differ.

**Per-model numbers:**

- `ClearCoatTest`: std across the 18 cell colours **R 2.32 → 46.79**, G 0.96 → 11.19,
  B 1.04 → 28.47; the red channel's range over the grid goes from 159.8‥168.9 (i.e. one colour,
  eighteen times) to 5.5‥167.4. Row 1 now measures a linear **1.000 : 0.033 : 0.022** against a
  declared 1.000 : 0.040 : 0.020, rows 2-6 **0.15 : 0.21 : 1.000** against 0.119 : 0.203 : 1.000,
  and the whole `Coating Only` column its declared **black** (mean sRGB 5.5‥15.4, the residual
  being the sky's specular on the coat — physically required). **The three columns separate.**
- `TransmissionTest`: hues of the saturated spheres
  **[16,16,16,16,17,17,17,17,17,17] → [16,16,17,61,61,61,123,127,128,194]** — one hue became the
  four declared ones (6° red, 62° yellow, 124° green, 210° blue).
- `MetalRoughSpheresNoTextures`: with a 7×7 lattice **fitted to the image** (guessed coordinates
  produce garbage at this framing — see the trap below), cell-mean std **6.43 → 35.22** (×5.5),
  spread 23.98 → 163.84, and the **metallic axis is now strictly monotone over its seven steps**
  (116.3 → 127.2 → 140.9 → 155.8 → 169.9 → 177.4 → 189.2). Before, all 49 sampled cells read
  180 ± 1. The roughness axis peaks at column 5 and falls — physically expected (a smooth metal
  mirrors the dark forest, a rough one scatters the bright sky, then the lobe widens past it), not
  a defect. ⚠️ Still only **5.5 % of frame height** (`hardcoded-near-plane`), so per-cell
  photometry stays coarse; the axis monotonicity is what carries the verdict.
- `SpecularTest`: **capture bit-identical, 0.000 % of pixels changed.** Its 24 materials all
  declare `baseColorFactor [0,0,0,1]`, `metallicFactor 0`, `roughnessFactor 0` and differ ONLY in
  `KHR_materials_specular` — which is never read. So the aliasing had nothing to alias, and the
  35 cell means still span 2.70‥4.57 / 255 (std 0.52) with zero exactly-black pixels. **FAIL
  confirmed, and now correctly attributed to the unread extension rather than to the key.**
- `TransmissionRoughnessTest`: the 6 spheres are now 6 distinct renderables (0.87 % of pixels
  changed), and the failure is isolated: over the declared IOR range 1.00 → 2.42 the five rows
  move by **1.46 / 255** — noise — while the roughness axis moves by **15.42**. `ior` does
  nothing, exactly as the code says (zero occurrences). **FAIL, cause pinned.**

⚠️ **A trap this re-run added.** Sampling a small grid on **guessed** pixel coordinates produced a
*plausible but wrong* measurement — a per-cell std of 51.97 on the BEFORE capture of
`MetalRoughSpheresNoTextures`, which is impossible for 98 identical spheres and was the sampling
window drifting into the sky. Fit the lattice to the image (high-pass, then a brute-forced
pitch/offset over the row and column profiles) before reporting any per-cell number.

**Consumers re-verified at runtime** (`animation-debug`, zero VUID): the Fox renders with its
texture and shadow through the renamed `glTF:Fox/Mesh/fox1-0`, and both Paladins render through
the FBX path whose keys changed identically.

## What remains

- [x] **`KHR_materials_specular` + `KHR_materials_ior`: FACTORS WIRED 2026-08-28.** The GPU side
  was already complete and spec-exact (`LightGenerator.PBR.cpp:589-590`); only the loader's read
  was missing, and the identity defaults hid it. `SpecularTest`'s four factor rows go from flat
  (spread ≤ 0.17 / 255) to monotone (2.36‥3.35), `specularFactor 0` renders exactly 0, the three
  texture rows stay bit-identical (built-in control), and the controls `MetalRoughSpheres` /
  `WaterBottle` move by 1 and 0. `TransmissionRoughnessTest`'s IOR axis: **1.46 → 4.24 / 255,
  monotone** (diamond darkest, air brightest). Zero VUID. Details and traps in
  [`docs/caution-points.md`](../caution-points.md) § *An IDENTITY default makes an unwired feature
  indistinguishable from a disabled one*.
  - [ ] **Remaining: the two specular TEXTURES** — `specularTexture` (**A** channel, scales the
    factor) and `specularColorTexture` (**RGB**, tints F0). They are logged and ignored today.
    Blocking design point: the material has a single `ComponentType::Specular` slot and glTF needs
    two, so a new `ComponentType` must be added (plus its `to_cstring`/`to_ComponentType` entries),
    two texture overloads on `StandardResource`, and a conditional in
    `declareSurfaceKHRSpecular()` following the established
    `componentIt != cend() ? variableName() : MaterialUB(...)` pattern used by Transmission.
    These are `SpecularTest`'s remaining 3 rows out of 7 (15 spheres of 35).
  - [ ] **`FBXLoader` reads neither, on purpose.** ufbx's `pbr.specular_factor`/`specular_color`
    mean the dielectric specular weight on an OpenPBR/Standard-Surface material but the **Phong**
    specular on a legacy `FbxSurfacePhong`, and the engine's legacy specular is a glossiness path.
    The semantics must be settled before any code is copied across — see
    `project_specular_normalisation_glossiness` reasoning in the specular/glossiness work.
  Measured symptom: the 35 spheres all sit between **3.65 and 3.83 out of 255** (total spread 0.18)
  with **zero exactly-black pixels** — drawn, then crushed, not absent.
- [ ] **Iridescence**: `GLTFLoader.cpp:1119-1125` reads only `iridescenceFactor`;
  `iridescenceIor` and the film thickness (min/max/texture) are never parsed — and those are the
  two axes both test models sweep. The test CANNOT pass as it stands.
- [ ] **Clearcoat / transmission / sheen**: only the scalar factors are read, no texture (nor coat
  normal map). ⚠️ **The scalar factor IS wired and now measurable** (2026-08-28): the `Coated`
  column is brighter than `Base layer` on all six rows, with the top-5 % highlight up **+6.7 to
  +17.3** out of 255. The 2026-08-27 claim of "**no separation at all**" was the resource-key
  collapse, not the shading — those three columns were literally the same renderable. What remains
  is therefore narrower than it looked: the clearcoat **textures** (factor/roughness texture and
  the coat normal map), which is why the `Partial coating`, `Base normal map`, `Shared normal map`
  and `Coat normal map` rows still cannot be conformant. **`ClearCoatTest` stays FAIL on that
  residual**, and the base-colour half of its failure is closed.
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
- [ ] **Full conformance re-judgement against each model's README is NOT done.** The 2026-08-28
  pass closed the resource-key collapse and re-attributed the five affected verdicts; it did not
  re-read all nineteen READMEs and re-derive PASS/FAIL from scratch. The headline count above is
  still the 2026-08-27 one. Do not quote it as current.
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
