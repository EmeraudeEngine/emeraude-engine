---
id: gltf-conformance-loader-gaps
title: glTF conformance — the loader gaps that remain (re-judged 2026-09-14)
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
> belongs to the testbed. It is a **partial clone** (`blob:none`, 201 MB against 1.7 GB upstream)
> — do not re-clone it plainly. The bench searches three layouts for it since 2026-09-14 (the
> engine checkout here is a **symlink to a sibling directory**, so no path arithmetic on the
> resolved script path can find the consumer's submodule).

## The count, re-judged 2026-09-14 (second pass, after three fixes the first pass found)

Full re-run against the current cascade: **49 captures, ZERO VUID, zero `VK_ERROR`**, zero
shared-UBO failures. Every model re-read against its own README. base suite **2049/2049**.

| | 2026-08-27 | 2026-09-14 (first pass) | **2026-09-14 (final)** |
|---|---|---|---|
| PASS | 7 | 14 | **16** |
| FAIL | 10 | 1 | **4** |
| blocked by the instrument | 2 | 5 | **0** |

⚠️ **The FAIL count went UP from the first pass to the final one, and that is the good outcome**:
the five "blocked" models became judgeable, and four of the judged models fail for a reason now
pinned in code. A blocked model hides a defect; a failing one names it.

**PASS (16)** — each with the number that decides it:

| model | measurement |
|---|---|
| `AlphaBlendModeTest` | OPAQUE flat (spread **2.3/255**), BLEND ramp ×38.7 monotone on 98 % of steps, MASK cutoffs sharp (< 0.012 alpha) and spaced **0.236 / 0.245** against a declared 0.25 |
| `OrientationTest` | **6/6** arrows on their same-colour target — RGB (quaternions) *and* CMY (matrices) |
| `NormalTangentTest` | normal-mapped highlight within **17°** of geometry, same quadrant, no Y flip |
| `NormalTangentMirrorTest` | four columns within **3.8°**: Geometry −27.5°, Normal −27.4°, V Mirror −23.7°, U Mirror −23.7° |
| `TextureTransformTest` | offset ✓ rotation ✓ scale ✓ clamp ✓ — all three arrows on their green ✓ |
| `VertexColorTest` | pure R/G/B check marks, zero complementary ink |
| `EmissiveStrengthTest` | ratios **2.42 / 2.21 / 1.81 / 1.48** against a declared doubling |
| `MetalRoughSpheres` | the exposure/IBL control; matches the reference |
| `MetalRoughSpheresNoTextures` | metallic axis **10.8 → 119.1 → 204.9** (smooth) and **60.9 → 115.1 → 190.2** (rough), both monotone |
| `TransmissionTest` | the four declared hues — peaks at **0-10° / 60° / 120° / 200°** |
| `TransmissionRoughnessTest` | the Air row (IOR 1.00) does **not** blur (×1.34) while Sapphire/Glass/Water fall ×4.35 / ×4.55 / ×3.12 |
| `BoomBox` | matches the reference, crisp |
| `WaterBottle` | matches the reference, chirality correct |
| `VolumeAbsorptionProbe` (ours) | centre G/R **0.993 / 0.993 / 14.87**, backdrop (201, 200, 196) |
| `IridescenceDielectricSpheres` | **the grid is complete for the first time** — the 197 absent spheres are back and the pastel film sweep reads along the thickness axis |
| `IridescenceMetallicSpheres` | idem, with the saturated metal film; judged on the parametrisation, not the level (Khronos shoots it in a studio) |

⚠️ Reservation on `TransmissionRoughnessTest`: the Diamond row (IOR 2.42) decays only ×1.37 because
at that IOR the *reflection* dominates and a sharpness metric cannot separate a sharp reflection
from a sharp transmission. Not a transmission defect — an unresolvable row for this metric.

**FAIL (4), every cause pinned in code**

| model | cause | tracked |
|---|---|---|
| `SheenCloth` | **`KHR_materials_sheen`'s two textures are never read** — `GLTFLoader.cpp:1090-1107` reads `sheenColorFactor` and `sheenRoughnessFactor` only. The asset's whole point is its sheen texture (colour in RGB, roughness in alpha). | this file, below |
| `ClearCoatTest` | the three maps are read since 2026-09-14 and the `Roughness variations` row now carries the coating's stripes, but the **`Coat normal map` row still does not corrugate** and `Partial coating`'s bands are weak | this file, below |
| `SpecularTest` | 6 rows of 7 pass (leftmost sphere exactly **0.00**, each texture row matching its factor row to **0.06/255**); the 7th is flat because `specularColorFactor > 1` is clamped by `Color< float >` | [`specular-colour-factor-above-one-is-clamped.md`](specular-colour-factor-above-one-is-clamped.md) |
| `AnisotropyStrengthTest` | the axis works (lobe elongation **1.11 → 9.26**, monotone — a first), but it still acts at **roughness 1.0** (×1.84) where the extension says it must not | [`anisotropy-alpha-ignores-the-spec-formula.md`](anisotropy-alpha-ignores-the-spec-formula.md) |

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

## What remains

- [ ] **`KHR_materials_sheen`'s two textures** (`sheenColorTexture` RGB, `sheenRoughnessTexture`
      alpha) — never read. The only reason `SheenCloth` fails, and the same shape as the clearcoat
      and specular maps: check the material side first, it may well already be complete.
- [ ] **Clearcoat's residual**: the `Coat normal map` row does not corrugate and `Partial coating`'s
      bands are weak, although all three maps are now read and the roughness map demonstrably works.
      Look at the coat tangent frame (`Ncc`, `LightGenerator.PBR.cpp:808-814`) before the loader.
- [ ] **`specularColorFactor > 1`** — [`specular-colour-factor-above-one-is-clamped.md`](specular-colour-factor-above-one-is-clamped.md), owner decision.
- [ ] **The anisotropy alpha formula** — [`anisotropy-alpha-ignores-the-spec-formula.md`](anisotropy-alpha-ignores-the-spec-formula.md), owner decision.
- [ ] `KHR_texture_transform`'s per-`TextureInfo` **`texCoord` override** — the multi-UV gap
      (`GLTFLoader.cpp:1000`). Walls into
      [`vertex-attribute-presence-belongs-to-geometry.md`](vertex-attribute-presence-belongs-to-geometry.md).
- [ ] `KHR_texture_transform` on a **specular** or **clearcoat** map — no UV transform slot for those
      component types (`GLTFLoader.cpp:1295` and the clearcoat warning next to it). No conformance
      asset needs it.
- [ ] **`FBXLoader` reads neither specular nor ior, on purpose.** ufbx's
      `pbr.specular_factor`/`specular_color` mean the dielectric specular weight on an
      OpenPBR/Standard-Surface material but the **Phong** specular on a legacy `FbxSurfacePhong`,
      and the engine's legacy specular is a glossiness path. **Owner decision needed.**
- [ ] ⚠️ **`ModelViewer` misses the extents on `TextureTransformTest`** — *"published no extents in
      time, using the fallback framing"*, on every attempt, a `.gltf` with external textures. The
      capture stays usable but its framing is not the computed one, and the warning appears only in
      the engine log, never in `bench-report.json`.

## Closed since the previous revision of this file (2026-08-28 → 2026-08-29)

- `KHR_materials_iridescence`'s **thickness texture** (`cb4ad679`) — `ComponentType::IridescenceThickness`.
- `KHR_materials_volume`'s **thickness texture** and **one ambient Fresnel** (`809e0f78`): the
  ambient pass had four branches disagreeing on the Fresnel term, two of them hard-coding F0 = 0.04
  and ignoring `KHR_materials_ior`; iridescence reached zero ambient shaders and now reaches two.
  ⚠️ Both items' own files were deleted with the work, as the rule requires — this list exists so
  the next reader does not re-open them.

## ⚠️ Traps of this bench (they cost real time)

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
