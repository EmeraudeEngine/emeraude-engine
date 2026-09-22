---
id: gltf-conformance-loader-gaps
title: glTF conformance — 20/20 on the bench (re-judged cell by cell 2026-09-15), and what that does not cover
status: open
priority: high
scope: Scenes/Loaders/GLTF
opened: 2026-08-25
blocked-by: []
tags: [gltf, material, measured]
---

# glTF conformance — the loader gaps that remain

> ⚠️ **Asset location** (owner decision, 2026-08-31): `glTF-Sample-Assets` is a **submodule of
> projet-alpha** (`projet-alpha/dependencies/glTF-Sample-Assets`), not of the engine. Test data
> belongs to the testbed. ⚠️ It is a **FULL clone**, ~3.2 GB (1.4 GB of working tree, 148 models
> with every variant, plus 1.8 GB of git objects). This file said "partial clone (`blob:none`,
> 201 MB against 1.7 GB upstream) — do not re-clone it plainly" until 2026-09-18; **measured that
> day, no partial-clone filter is configured at all**, in the submodule or in its real git dir, so
> there is no filter to preserve. The bench searches three layouts for it since 2026-09-14 (the
> engine checkout here is a **symlink to a sibling directory**, so no path arithmetic on the
> resolved script path can find the consumer's submodule).

## The count, re-judged CELL BY CELL 2026-09-15 — 20 / 20

**49 captures, ZERO VUID, zero shared-UBO failure, zero entity-name collision**, emeraude-base
**2049/2049**. Every model re-derived from its own README, and — for the first time — every model
passed through the structural control below before a single pixel was read.

| | 2026-08-27 | first pass | second pass | third pass | **cell by cell** |
|---|---|---|---|---|---|
| PASS | 7 | 14 | 16 | 18 | **20 / 20** |
| FAIL | 10 | 1 | 4 | 2 | **0** |

On the nineteen Khronos models alone (the twentieth is our own volume probe): **19 / 19**, against
7 / 19 in August.

### ⚠️⚠️ The structural control, which is new and which every future run starts with

The engine builds one static entity per mesh-bearing NODE, so **anything below that count is
geometry that never reached the scene** — and no per-pixel criterion can see it. `bench.py` now
reports `entities / meshNodes` for every model and flags a deficit. Measured on this run:

    every model came out at meshNodes + 2 (the viewer's own two entities). No deficit anywhere.

It exists because `TransmissionTest` was marked PASS for weeks with **two of its twelve spheres
absent**: its three nodes named `BlueTransWithMask` collided in a registry keyed by name, and the
reading criterion only asked whether the four declared hues appeared *somewhere in the frame*.

### The numbers each verdict rests on

| model | measurement |
|---|---|
| `AlphaBlendModeTest` | MASK cutoffs spaced **0.237 / 0.245** against a declared 0.25; BLEND ramp ×38.7; OPAQUE flat |
| `AnisotropyStrengthTest` | roughness 1.0 column **flat (×0.85)** as the spec demands, roughness 0.17 monotone **5.84 → 1.11** |
| `BoomBox` | matches the reference, crisp |
| `ClearCoatTest` | all six rows. ⚠️ `Partial coating` settled by the coat CONTRIBUTION (Coated − Base): **bimodal, 62.9 % of pixels at +1.21 (uncoated) against 37.1 % at +11.09 (coated)**, where the two uniform-coat rows put **89.5 %** in a single mode. `Coat normal map` corrugates since the coat reflects along its own normal |
| `EmissiveStrengthTest` | linear-luminance ratios **2.42 / 2.21 / 1.81 / 1.48** against a declared doubling |
| `IridescenceDielectricSpheres` | ⚠️ **FAIL since 2026-09-22** (macOS bench): the grid is complete, but `R23 = baseF0` kills the film on the base-IOR 1.0 layer and inverts the colour trend — item `iridescence-film-to-base-reflectance`. "The film sweep reads" was a whole-frame verdict. |
| `IridescenceMetallicSpheres` | ⚠️ **FAIL since 2026-09-22**, same cause: the black-base layer (F0 = 0) is achromatic at every thickness |
| `MetalRoughSpheres` | the exposure/IBL control; matches the reference |
| `MetalRoughSpheresNoTextures` | metallic axis **10.8 → 119.1 → 204.9** smooth, **60.9 → 115.1 → 190.2** rough, both monotone |
| `NormalTangentTest` | normal-mapped highlight within 17° of geometry, same quadrant, no Y flip |
| `NormalTangentMirrorTest` | the four columns within **3.8°** |
| `OrientationTest` | **6/6** arrows on their same-colour target, quaternions AND matrices |
| `SheenCloth` | both maps read, tiling at the authored frequency (hue dispersion **8.51° → 2.57°**), rim/body **0.79** against the reference's 0.64. ⚠️ Judged on RATIOS: it renders brighter than the Khronos shot, which is the viewer's lighting against their studio, not the sheen |
| `SpecularTest` | **7/7 rows monotone**, first sphere exactly **0.00**, each texture row matching its factor row to **0.06 / 255** |
| `TextureTransformTest` | offset ✓ rotation ✓ scale ✓ clamp ✓ — three arrows on their green ✓, and **bit-exact** through the indexed-table rewrite |
| `TransmissionRoughnessTest` | the Air row does NOT blur (**×1.34**) while Sapphire / Glass / Water fall **×4.35 / ×4.55 / ×3.12** |
| `TransmissionTest` | **the 3×4 grid is complete** (24 entities / 22 nodes) and the four declared hues read at 0-10° / 60° / 120° / 200° |
| `VertexColorTest` | pure R/G/B check marks, zero complementary ink |
| `VolumeAbsorptionProbe` (ours) | centre G/R **0.993 / 0.993 / 14.87** |
| `WaterBottle` | matches the reference, chirality correct |

⚠️ **Four verdicts rest on a visual match rather than a number** — `BoomBox`, `WaterBottle`,
`MetalRoughSpheres` and the two iridescence grids' film sweep. They are showcase assets whose
READMEs state no numeric criterion. Do not quote them as measured.

⚠️ **Reservation on `TransmissionRoughnessTest`**: the Diamond row (IOR 2.42) decays only ×1.37
because at that IOR the reflection dominates and a sharpness metric cannot separate a sharp
reflection from a sharp transmission. Unresolvable with this metric, not a defect.

### ⚠️⚠️ What 20 / 20 does NOT mean

100 % of what this bench drives: **nineteen Khronos models chosen because each isolates one
defect**, plus one probe of ours. Not a percentage of glTF 2.0 — the corpus holds 148 models, and
the gaps the bench does not exercise at all are unchanged and
listed in [`src/Scenes/Loaders/AGENTS.md`](../../src/Scenes/Loaders/AGENTS.md) § *Known gaps*: no
multi-UV, four skin influences, no morph targets, `TRIANGLES` only, no rigid-node animation, no GPU
instancing — plus `KHR_materials_transmission`'s texture, the last extension reading only its
scalar factor.

## What the second pass fixed

- **The shared-UBO ceiling** — 197 of 344 materials per iridescence grid had no seat and their
  spheres were absent. `bankSize()` now caps the device limit instead of ignoring it, and
  `addElement()` grows a bank instead of failing. Item deleted (done); the knowledge and the
  concurrency constraint are in [`docs/caution-points.md`](../caution-points.md)
  § *A shared-UBO bank that fills up…*.
- **Clearcoat's three maps** — read, with the roughness map on **GREEN** as the extension requires.
  `Simple coating`, the row declaring no texture, is **bit-exact**: the control this needed.
- **The bench poses a per-test environment** (`ENVIRONMENTS` in `bench.py`) and restores the
  session's values. That is what made `SpecularTest`, `AnisotropyStrengthTest` and `SheenCloth`
  judgeable at all.

⚠️⚠️ **A correction to the previous revision of this file**: it claimed `Core.SettingsService` had
**no `set`**, and built a whole "missing capability" section on it. False — `set(key, value)` has
been there since the console was unified. The claim came from a `help` dump piped through
`grep | head`, where `set` sorts one line past `save` and fell off the end. **`<path>.lsfunc()`
enumerates a level; never conclude a capability is missing from a truncated pipe.**

### ⚠️⚠️ The capture's ENVIRONMENT was not deterministic — FIXED 2026-09-18

Re-running the compressed-variant bench unchanged, two rows of 34 moved hugely while the other 32
reproduced to the fourth decimal: `RiggedSimple` by **+28.39**/255 of mean, `CarConcept` by −1.78.

Neither was a codec difference, and the way to tell is worth keeping: comparing each capture against
its own counterpart in the previous run showed the row that moved was the **plain** variant for one
model and the **Draco** variant for the other — a different side each time, which no codec can
produce. In both, the subject was pixel-identical and only the BACKGROUND differed. The viewer's
environment cubemap streams, and the scene logs its choice before the texture is resident.

**Two changes, and only the second cures it:**

1. The comparison is cropped to the subject's projected **AABB** (`subject_box()`). ⚠️ This does
   **not** fix the race — it was tried first and made it worse, `RiggedSimple` going 28.43 → 38.40,
   because a tight box around a slender model is still mostly background and removing the identical
   sky merely concentrates the difference. It is kept because the mean now describes the subject
   instead of being diluted over two megapixels of sky.
2. `DRACO_AB_ENVIRONMENT` poses the bit-exact black backdrop on every A/B row that does not declare
   an environment of its own. **Nothing left to stream, no race.**

⚠️ The crop had its own defect, caught by checking the box DIMENSIONS rather than the means:
`MorphPrimitivesTest` is flat, so seen face-on its AABB projects to a LINE — box `(223, 360, 1057,
360)`, zero height, and a relative margin of zero gave an empty box and a row that silently dropped
out of the table. Hence `MIN_SUBJECT_BOX_MARGIN_PIXELS`. **Read the boxes, not only the numbers: a
row that vanishes looks like nothing at all.**

> [!CAUTION]
> **Figures from before 2026-09-18 are NOT comparable with the ones after.** The mean is taken over
> the subject's box and the environment reflection is gone from the comparison; every value roughly
> doubled for that reason alone.

**Measured after both changes**, two consecutive full runs: 23 of 33 shared rows identical to the
fourth decimal, largest drift **0.062**/255 — against 28.39 before. 34 rows compared, 0 entity
mismatch, 0 load error, 0 degenerate box, 0 VUID.

## What remains

Nothing on the bench itself. What is left is everything the bench does not exercise, and one
half-fix taken deliberately:

- [ ] **`KHR_materials_transmission`'s texture** — the last extension reading only its scalar
      factor. No model in the bench fails on it.
- [ ] **A clear coat WITHOUT a normal map still samples the environment at the BASE roughness.**
      Wrong for the same reason the normal was — a coat is smoother than what it covers — and scoped
      out of the 2026-09-14 fix on purpose, to keep the rows without a coat normal map bit-exact as
      the control. Fixing it moves `Simple coating` and `Roughness variations`, so it needs its own
      before/after.
- [ ] `KHR_texture_transform`'s per-`TextureInfo` **`texCoord` override** — the multi-UV gap
      (`GLTFLoader.cpp:1000`). Walls into
      [`vertex-attribute-presence-belongs-to-geometry.md`](vertex-attribute-presence-belongs-to-geometry.md).
- [ ] ⚠️ **The node-HIERARCHY import path is not covered by the entity-name fix.**
      `SceneDataConsumer` also builds children with `createChild(nodeDesc.name)` (the `+ModelViewer`
      file path, `AssetRoot`); whether that registry tolerates a duplicate is UNVERIFIED. Measure it
      the same way — entity count against node count — before assuming either answer.
- [ ] ⚠️ **The FBX specular's PBR branch is unexercised.** `FBXLoader` reads
      `pbr.specular_factor`/`specular_color` gated on `ufbx_material::shader_type`; the only FBX
      assets here are the Paladin's, which are legacy Phong. The gate is proven CLOSED where it must
      be and untested where it opens.
- [ ] ⚠️ **`ModelViewer` misses the extents on `TextureTransformTest`** — *"published no extents in
      time, using the fallback framing"*, on every attempt. The capture stays usable but its framing
      is not the computed one, and the warning appears only in the engine log.

## Closed since the previous revision of this file (2026-08-28 → 2026-08-29)

- `KHR_materials_iridescence`'s **thickness texture** (`cb4ad679`) — `ComponentType::IridescenceThickness`.
- `KHR_materials_volume`'s **thickness texture** and **one ambient Fresnel** (`809e0f78`): the
  ambient pass had four branches disagreeing on the Fresnel term, two of them hard-coding F0 = 0.04
  and ignoring `KHR_materials_ior`; iridescence reached zero ambient shaders and now reaches two.
  ⚠️ Both items' own files were deleted with the work, as the rule requires — this list exists so
  the next reader does not re-open them.

## ⚠️ Traps of this bench (they cost real time)

- ⚠️⚠️ **A criterion that reads the WHOLE frame hides a per-cell failure.** `TransmissionTest` was
  marked PASS on "the four declared hues are present" — and they were, because each hue appeared
  somewhere. Two of its twelve spheres were **absent from the scene entirely**, spotted by the owner
  looking at the picture, weeks after the verdict. Count the CELLS, not the palette: a grid test must
  assert that every cell is drawn AND distinct, and the cheapest form of that assertion is the
  engine's own entity count against the asset's mesh-bearing node count
  (`Core.SceneManagerService.getSceneInfo()`).
- ⚠️⚠️ **A projected sample that misses its target reads the BACKDROP, and a backdrop is a plausible
  colour.** Probing those two spheres by projecting their node positions returned "saturation 4.9 and
  4.4", which reads as *desaturated* — while the truth was *not drawn at all*. Two different defects
  behind one number. The picture settled it in one look; the probe never would have. Same trap as the
  iridescence grids, paid twice.

The harness and its older traps live with the tool, in
[`tools/gltf-conformance-bench/README.md`](../../tools/gltf-conformance-bench/README.md). What the
2026-09-14 run added:

- ⚠️⚠️ **A missing sphere shows you the sphere BEHIND it.** The first probe of the iridescence
  grids sampled every sphere centre and found the seat-less half *saturated and correlated with its
  declared film thickness* — the opposite of "not drawn", and wrong. In a 7×7×7 grid almost every
  cell has another cell behind it. Only the **isolated** spheres (nothing of the lattice in front
  of or behind, computed from the projected discs) answer the question, and they answered it
  cleanly. **Before reading a pixel, prove that nothing else can be under it.**
- ⚠️⚠️ **The brightest pixels of a sphere in an outdoor environment are an IMAGE OF THE SKY, not a
  BRDF lobe.** The highlight-elongation metric for `AnisotropyStrengthTest` — the shape metric this
  file has been asking for — returned pure noise (1.23 … 6.82 with no structure, and the
  anisotropy-0 row no more isotropic than the rest). That is the **fifth** confounded metric for
  this test. The metric is not the problem: no shape metric can work while the reflected
  environment is a landscape. Pose the test in a dark environment with a distinct source first.
- ⚠️ **`ModelViewer` can miss the extents and fall back.** `TextureTransformTest` (a `.gltf` with
  external textures) logged *"The imported content published no extents in time, using the fallback
  framing"* on **both** attempts — reproducible, not a fluke. The capture was usable here, but the
  framing was not the computed one. A capture whose framing silently differs is a measurement
  hazard; the warning is in the engine log and nowhere in the report.
- ⚠️ **The plan's `coverage~38.9 %` is stale arithmetic.** The real subtended height is ~83 % at
  `DISTANCE_FACTOR = 5.142`, which is what every capture shows. Cosmetic, but do not use that
  number to conclude anything about framing.

## References

- Captures: `~/.local/share/LNIsle/projet-alpha/captures/bench-gltf-20260914/` (48 PNG).
- Previous runs kept for pixel A/B: `bench-gltf-20260827/`, `bench-gltf-20260828/`.
- The owner's gallery verdict on the pre-merge bench run
  (`captures/bench-gltf-20260812/galerie-banc-gltf.html`) was the D5 gate of the material merge;
  it is still pending.
