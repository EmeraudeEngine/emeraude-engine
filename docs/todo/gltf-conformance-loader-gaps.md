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

## The count, re-judged 2026-09-14 (third pass, after four owner decisions)

**49 captures, ZERO VUID, zero `VK_ERROR`, zero shared-UBO failures.** base suite **2049/2049**.

| | 2026-08-27 | first pass | second pass | **final** |
|---|---|---|---|---|
| PASS | 7 | 14 | 16 | **18** |
| FAIL | 10 | 1 | 4 | **2** |
| blocked by the instrument | 2 | 5 | 0 | **0** |

On the nineteen Khronos models alone (the twentieth is our own volume probe): **17 / 19 = 89 %**,
against 7 / 19 = 37 % in August.

⚠️ **What that number is not**: 89 % of what this bench drives, which is nineteen models chosen
because each isolates one defect — not a percentage of glTF 2.0. The corpus is a partial clone
(30 materialised of ~180 upstream), and the gaps it does NOT exercise are listed in
[`src/Scenes/Loaders/AGENTS.md`](../../src/Scenes/Loaders/AGENTS.md) § *Known gaps*: no multi-UV,
four skin influences, no morph targets, `TRIANGLES` only, no rigid-node animation, no GPU
instancing. Nothing here measures them, so nothing here fails on them.

**Repaired in this pass**

| model | was | now |
|---|---|---|
| `SpecularTest` | 6 rows of 7; the 7th flat at 9.33 / 9.35 / 9.05 / 8.87 | **7/7** — the `color factor > 1.0` row is a monotone ramp **10.22 / 26.42 / 47.13 / 70.95**, the six others identical to the hundredth |
| `AnisotropyStrengthTest` | the axis acted at roughness 1.0 (×1.84) where the spec forbids it, and was unmeasurable at roughness 0 | **both clauses met** — roughness 1.0 flat (**×0.85**), roughness 0.0 now the strongest column (**18.73 → 9.61**) |

**FAIL (2)**

| model | cause | tracked |
|---|---|---|
| `SheenCloth` | its two maps are READ since this pass and demonstrably reach the shader (13.1 % of pixels, cloth mean 158.2 → 80.7), but it tiles them **30×** through `KHR_texture_transform` and no sheen component type has a UV transform slot | [`uv-transform-slots-for-extension-maps.md`](uv-transform-slots-for-extension-maps.md) |
| `ClearCoatTest` | **the coat normal map corrugates since 2026-09-14** (the coat reflects the environment along its own normal now); the residual is the **`Partial coating`** row, whose band boundary is still not readable | this file, below |

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

- [ ] **Clearcoat's residual is now the `Partial coating` row alone.** The coat FACTOR map reaches
      the ambient pass (28 generated shaders read `SurfaceClearCoatFactor` rather than the UBO
      scalar), and the row changes when it is wired, so it is not unread. What is not established is
      whether the band boundary is *rendered too weakly* or simply *unreadable at this contrast*:
      the coat's Fresnel is 0.04 at normal incidence, so coat-present versus coat-absent is a 4 %
      step in the reflection. The bench now poses this test under Kloppenheim05 with no flat ambient,
      as its own README asks ("an environment with distinctive bright light sources"), and the band
      is still not obvious. **Measure the step across the boundary before touching any code** — a
      4 % reflection difference may be correct and simply invisible, and calling that a defect is
      the mistake this row invites.
- [ ] **A clear coat WITHOUT a normal map still samples the environment at the BASE roughness.**
      Wrong for the same reason the normal was: a coat is typically far smoother than what it covers,
      so its reflection comes out as blurry as the base's. Scoped out of the 2026-09-14 fix on
      purpose, to keep every row without a coat normal map bit-exact as the control. Fixing it moves
      `Simple coating` and `Roughness variations`, so it needs its own before/after.
- [ ] **UV transform slots for the extension maps** —
      [`uv-transform-slots-for-extension-maps.md`](uv-transform-slots-for-extension-maps.md). The
      only thing between `SheenCloth` and a PASS, and it needs a design decision, not a patch.
- [ ] **`KHR_materials_transmission`'s texture** — the last extension still reading only its scalar
      factor. No conformance model in the bench fails on it today.
- [ ] `KHR_texture_transform`'s per-`TextureInfo` **`texCoord` override** — the multi-UV gap
      (`GLTFLoader.cpp:1000`). Walls into
      [`vertex-attribute-presence-belongs-to-geometry.md`](vertex-attribute-presence-belongs-to-geometry.md).
- [ ] ⚠️ **The FBX specular's PBR branch is unexercised.** `FBXLoader` reads
      `pbr.specular_factor`/`specular_color` since this pass, gated on `ufbx_material::shader_type`
      so a legacy Phong material is left alone. Verified that the Paladins still render unchanged
      with zero VUID — which proves the gate is CLOSED where it must be and nothing about the open
      branch. Judge it when a Standard-Surface / OpenPBR FBX exists to judge it with.
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
