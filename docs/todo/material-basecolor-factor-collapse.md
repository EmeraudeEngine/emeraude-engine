---
id: material-basecolor-factor-collapse
title: Materials differing only by baseColorFactor all render with the first one's colour
status: open
priority: high
scope: Graphics/Material
opened: 2026-08-27
tags: [material, gltf, measured]
---

# Materials differing only by `baseColorFactor` all render with the first one's colour

## Why

Found while re-running the glTF conformance bench on 2026-08-27. It had been mis-attributed until
now: the failures of `ClearCoatTest`, `TransmissionTest` and `SpecularTest` were all charged to
un-wired material extensions, and part of what they show is this instead.

**Measured.** `ClearCoatTest` declares three different base colours — red `(0.5, 0.02, 0.01)` on
row 1, dark blue `(0.013, 0.022, 0.109)` on rows 2-6, and **black `(0, 0, 0)`** for the whole
`Coating Only` column. All eighteen cells render red:

```
row                        Base layer            Coated           Coating Only
Simple coating        (132.2, 26.2, 20.8)  (132.2, 26.5, 21.2)  (131.9, 26.8, 21.6)
Partial coating       (135.3, 26.2, 20.6)  (134.8, 26.3, 20.9)  (134.3, 26.6, 21.2)
Roughness variations  (137.6, 26.0, 20.3)  (137.2, 26.2, 20.6)  (136.3, 26.4, 20.7)
Base normal map       (140.7, 26.3, 20.5)  (139.9, 26.5, 20.6)  (138.8, 26.5, 20.7)
Shared normal map     (142.9, 26.4, 20.3)  (142.0, 26.4, 20.4)  (140.9, 26.5, 20.7)
Coat normal map       (144.8, 26.9, 20.7)  (144.2, 26.9, 20.8)  (143.3, 26.9, 20.8)
```

De-gamma'd, that is a linear ratio of `1 : 0.045 : 0.029` against material 0's declared
`1 : 0.04 : 0.02`. Every mesh is wearing material 0's base colour, including the ones that ask for
black.

`TransmissionTest` corroborates: six distinct declared base colours (red, yellow, blue, green…),
twelve identically salmon spheres.

**Not a regression.** The 2026-08-12 v1 capture
(`captures/bench-gltf-20260812/ClearCoatTest.png`) already shows the same uniform red. The defect
predates the Y-up flip and every material-merge lot.

**No passing model exonerates it.** This is why it survived two bench runs: not one of the seven
PASS models exercises a per-material `baseColorFactor`. `EmissiveStrengthTest`'s five emissive
materials all declare `baseColorFactor = [0, 0, 0, 1]` and differ only in `emissiveStrength` —
which is why that test passes and proves nothing here. Every `AlphaBlendModeTest` material declares
no `baseColorFactor` at all and goes through a texture. `MetalRoughSpheres`, the exposure control,
has exactly **one** material.

## What remains

- [ ] Isolate where the collapse happens. The obvious loader-side cause is already ruled out:
  material resources are keyed `<prefix>Material/<glTF material name>`
  (`Scenes/Loaders/GLTFLoader.cpp:850-861`) and ClearCoatTest's nineteen material names are all
  distinct, so `getOrCreateResource()` is not colliding. The loader does read `baseColorFactor`
  (`:1018`) and hands it to `setAlbedoColor()` / `setAlbedoComponent()` inside a per-material,
  fully self-contained `configure` lambda (`:1136-1167`). Suspicion therefore points downstream —
  material-properties buffer indexing, or a shared descriptor set where the dynamic offset is what
  distinguishes the materials.
- [ ] Decide it with a runtime check rather than by reading: load `ClearCoatTest` through
  `Core.openFiles()` and enumerate the created material resources over the console. Nineteen
  resources carrying nineteen different albedo colours means the loader is right and the renderer
  binds one of them; one resource means the opposite.
- [ ] Re-judge `ClearCoatTest`, `TransmissionTest` and `SpecularTest` afterwards — their extension
  verdicts in `gltf-conformance-loader-gaps.md` cannot be trusted while every mesh wears the same
  base colour.

## ⚠️ Traps

- **A control only exonerates what it actually exercises.** `MetalRoughSpheres` was used across two
  bench runs to clear "the IBL and the exposure". It does clear those, and nothing else: it carries
  a single material, so it can say nothing about per-material data reaching the shader. Check what
  a control declares before letting it close a question.
- The symptom looks exactly like an un-wired extension when the model under test uses one. Read the
  asset's declared factors before charging a defect to the extension.

## References

- Captures: `~/.local/share/LNIsle/projet-alpha/captures/bench-gltf-20260827/ClearCoatTest_front.png`
  and `TransmissionTest_front.png`; the 2026-08-12 counter-dated capture in
  `captures/bench-gltf-20260812/ClearCoatTest.png`.
- [`gltf-conformance-loader-gaps.md`](gltf-conformance-loader-gaps.md) — the bench run that
  surfaced it.
