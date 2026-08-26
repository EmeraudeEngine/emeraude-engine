---
id: gltf-conformance-loader-gaps
title: glTF conformance — 8 loader gaps located line by line (bench re-run 2026-08-25)
status: open
priority: high
scope: Scenes/SceneLoaders/GLTF
opened: 2026-08-25
tags: [gltf, material, measured]
---

# glTF conformance — the loader gaps the bench located

## Why

Bench re-run on 2026-08-25 after the Y-up flip, against `glTF-Sample-Assets` @ `2bac6f8`
(19 models, now vendored in `dependencies/`): **7 PASS / 10 FAIL / 2 undecided** (v1 was 2/12/5).

Lifted by Y-up: `NormalTangentTest`, `AlphaBlendModeTest`, `OrientationTest`, `WaterBottle`,
`BoomBox`. Still failing: NormalTangentMirrorTest, TextureTransformTest, VertexColorTest,
SpecularTest, ClearCoatTest, TransmissionTest, TransmissionRoughnessTest, Iridescence ×2,
AnisotropyStrengthTest.

**8 of the 10 remaining failures are loader WIRING, identified line by line** — not missing
shading features.

## What remains

- [ ] **`KHR_materials_specular` + `KHR_materials_ior`: declared, NEVER READ.** fastgltf mask at
  `GLTFLoader.cpp:461`, but no access to `glTFMaterial.specular`;
  `setSpecularComponent/Factor/Color` exist on `Material::StandardResource` and are called
  NOWHERE in the cascade. `ior`: zero occurrences in the loader. Wiring, not implementation.
- [ ] **Iridescence**: `GLTFLoader.cpp:1066-1072` reads only `iridescenceFactor`;
  `iridescenceIor` and the film thickness (min/max/texture) are never parsed — and those are the
  two axes both test models sweep. The test CANNOT pass as it stands.
- [ ] **Clearcoat / transmission / sheen**: only the scalar factors are read, no texture (nor coat
  normal map). But clearcoat row 1 needs ONLY the factor and shows no second lobe ⇒ the
  factor→shading wiring must be verified too.
- [ ] **`KHR_texture_transform`**: offset ✓ and scale ✓ (SheenCloth tiles ×30 correctly),
  **rotation ✗** (the asset declares 0.39270 rad, the marker reads "not applied").
- [ ] **glTF sampler wrap mode ignored**: the asset declares `wrapS = wrapT = 33071`
  (CLAMP_TO_EDGE) and the render REPEATS. ⚠️ Transverse defect, far beyond this bench.
- [ ] Supplied tangents ignored (Mirror inverted 5/5).
- [ ] `COLOR_0` not multiplied (cyan/magenta/yellow X).
- [ ] Anisotropy with no elongation (16.2 % variation at roughness 1.0 where the spec mandates
  ZERO).

Undecided, blocked by another item: `MetalRoughSpheresNoTextures` is clipped by the hard-coded
near plane (see `hardcoded-near-plane.md`); `SheenCloth`'s rim was not isolated.

## ⚠️ Traps of this bench (they cost real time)

- Launch **without `--load-demo`** (an active demo makes `openFiles` no-op), then
  `Core.openFiles(<asset>)` → `+ModelViewer`. Camera imposed via `setNodePosition("ViewerCamera",
  …)` + `setNodeLookAt(…)`; the orbit controller only re-asserts on a pointer event.
- Screenshot filenames are timestamps in SECONDS — two captures in the same second collide.
- **Bit-exact (0,0,0) ≠ "crushed by exposure"**: exact zero = never drawn; low non-zero = drawn
  then crushed.
- **Near-grey pixel hue is NOISE**: the iridescence grids give a 130° hue spread at saturation
  0.03. Report SATURATION.
- A control exonerates the instrument: SpecularTest's 35 black spheres could have been an exposure
  defect; `MetalRoughSpheres` under the same conditions shows clean mirrors ⇒ IBL and exposure
  cleared.
- `AlphaBlendModeTest` **wrongly self-declares conformance** (green ticks mislead; the red X has
  alpha 0 and is discarded). Measure the ramp, never read the ticks.
- **Framing is COMPUTED**: radii from 0.0038 m to 14.5 m (×3800). Walk the glTF node hierarchy WITH
  its transforms — the mesh bbox lies.
- Each Khronos test ships NAMED failure images (`incorrect-flipped-y.png`,
  `supplied-tangents-ignored.png`) — that is the real diagnostic tool.

## References

- Full report: artifact `https://claude.ai/code/artifact/21daa10d-043b-4d4a-8566-4e4a2c85a4bc`.
- The owner's gallery verdict on the pre-merge bench run
  (`~/.local/share/LNIsle/projet-alpha/captures/bench-gltf-20260812/galerie-banc-gltf.html`) was
  the D5 gate of the material merge; it is still pending.
