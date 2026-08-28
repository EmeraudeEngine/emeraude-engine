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
  - [x] **The two specular TEXTURES — DONE the same day.** `ComponentType::SpecularColor` added
    (glTF declares two maps for one extension, the material had one slot); `specularTexture` →
    `ComponentType::Specular`, **A channel**; `specularColorTexture` → `ComponentType::SpecularColor`,
    RGB, sRGB-decoded. Measured: the three texture rows go from flat (0.10 / 0.10 / 0.25) to
    monotone (3.12 / 3.60 / 2.41), the four factor rows **bit-identical** (control), the two paths
    agreeing to within **0.33 / 255** (ratio 1.05‥1.11). Controls `MetalRoughSpheres` delta 1,
    `WaterBottle` delta 0, zero VUID.
  - [ ] `KHR_texture_transform` on either specular map is dropped with a warning — no UV transform
    slot in the material UBO for those component types. No conformance asset needs it.
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
- [x] **`KHR_texture_transform` rotation — DONE 2026-08-28. `TextureTransformTest` PASSES**:
  offset ✓ scale ✓ clamp ✓ **rotation ✓**. The value was already parsed and thrown away with a
  warning; it now reaches the material UBO as **(cos, sin)** in a second block of 6 vec4 (offsets
  80-103, neutral (1,0,0,0)), the trig resolved ONCE per material on the CPU rather than per
  fragment. The shader composes `mat2(cos, -sin, sin, cos) * (uv * scale) + offset`.
  ⚠️ Two sign/order traps, both specified by the extension and both able to produce a
  plausible-but-wrong result: the composition is `translation * rotation * scale` (rotating the
  offset lands the texture elsewhere), and the extension's matrix is `[cos, sin ; -sin, cos]` —
  the minus on the BOTTOM-LEFT, a clockwise UV rotation, the opposite of the usual maths
  convention — which in GLSL's column-major `mat2` is written `mat2(cos, -sin, sin, cos)`.
  **Measured on the test's own markers**: the `Rotation` quad's arrow moved off the yellow
  "not applied" marker onto the **green ✓**, and the `All` quad (offset + rotation + scale) now
  lands on its green ✓ too. The `Scale` quad, which declares no rotation, is untouched.
  **Control**: the rotation is the ONLY thing in the whole bench that declares one, and every
  other capture is **bit-exact** (`AlphaBlendModeTest`, `BoomBox`, `WaterBottle` exactly 0 px;
  `MetalRoughSpheres` delta 1 = temporal dither) — the neutral `mat2(1,0,0,1)` is the identity.
  Zero VUID. Material UBO grows 80 → 104 floats (320 → 416 bytes).
  ⚠️ Still missing from this extension: the per-`TextureInfo` **`texCoord` override**, which is
  the multi-UV gap, and any transform on the two **specular** maps (no UV slot for those
  component types).
- [ ] ⚠️ **A measurement trap this lot re-paid.** Marker boxes placed on GUESSED pixel
  coordinates read 85.9 % ink both before and after — the box was saturated and could not
  discriminate. Locate the changed region first (`np.where` on the diff mask, then group the
  columns into bands) and crop THAT; the two rotated quads showed up as two clean column bands
  with the untouched `Scale` quad between them, which is what identified them.
- [x] **Supplied tangents — READ 2026-08-28** (`NormalTangentMirrorTest`). The accessor was not
  read at all, and the bitangent handedness did not exist anywhere in the cascade:
  `ShapeVertex::biNormal()` was a bare `cross(normal, tangent)`, so mirrored UV islands got a
  flipped bitangent. `ShapeVertex` gained a signed handedness member (neutral +1 ⇒ bit-exact no-op
  for every other loader and every generated shape), `setTangent(Vector<4>)` stops dropping W, and
  the loader skips its own tangent computation when the asset authored them.
  Measured (highlight angle per column, five roughnesses): the two mirrored columns go from a
  circular spread of **107.7°** and **102.9°** (deviating −115.3° / −110.8° from `Geometry`) to
  **1.8°** and **1.3°** at −7.8° / −12.1°, the same family as the already-passing `Normal` column
  (−6.3°). Controls: `Geometry` and `Normal` **identical to the decimal**, `NormalTangentTest`
  **bit-identical on all three views**, `MetalRoughSpheres` delta 1. `BoomBox` reads visibly
  crisper; `WaterBottle` and `SheenCloth` differ in micro-detail only. Zero VUID.
  ⚠️ Side effects to know: it is **all-or-nothing per mesh** (a mixed mesh recomputes everything and
  logs it), and `sizeof(ShapeVertex)` 80 → 84 forced the **native format version to 2** with no v1
  read path — `FileFormatNative` writes vertices as a raw blob, so a size change with an unchanged
  version is silent corruption. New base tests: `test_VertexFactoryShapeVertex.cpp` (6 cases,
  including a `sizeof` pin) + a v1-rejection case; base suite 2029 → **2036**.
- [x] **`COLOR_0` — READ 2026-08-28. `VertexColorTest` PASSES.** The README's literal failure
  signature is gone: the test tiles went Red 39.8 % red + **58.1 % cyan** → **97.4 % red, zero
  cyan**; Green 37.6 % + **59.9 % magenta** → **97.4 % green, zero magenta**; Blue **60.5 % yellow**
  + 39.5 % → **100 % blue, zero yellow**, while the reference tiles stayed 100 % pure. Every other
  capture in the lot is **bit-exact** (0 px, delta 0) — `COLOR_0` is the only thing that changed.
  Sponza's 136 meshes load with **zero VUID**, which is what proves the material-variant split
  correct on its 17 shared materials.
  ⚠️ Design consequence, deliberate: a `…-vc` material variant, because `UseVertexColors` changes
  the shader contract while the attribute belongs to the geometry. The architecture is upside down
  and is recorded as
  [`vertex-attribute-presence-belongs-to-geometry.md`](vertex-attribute-presence-belongs-to-geometry.md);
  `TEXCOORD_1+` will hit the same wall.
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
