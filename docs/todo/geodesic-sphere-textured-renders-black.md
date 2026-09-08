---
id: geodesic-sphere-textured-renders-black
title: A geodesic sphere renders flat black under a textured material — colour-only lights, geometry verified clean
status: open
priority: high
scope: Graphics/Renderable, Graphics/Material/StandardResource, Saphir (textured path on generated geometry)
opened: 2026-09-08
tags: [material, geometry, measured, owner-report]
---

# A geodesic sphere renders flat black under a textured material

## Why — and what is already EXCLUDED, each by a measurement

`light-and-shadow-debug`, `generateSphereInstance("SmoothMesh", 1.5F, "Grounds/Pavement002",
useGeodesic = true, lighting = true, quality = 4)`. Pinned pose `setPosition(-4, 2, 8)` /
`lookAt(-4, 2, 1)`, pinned exposure, region (1560, 700)-(1800, 980) of the 2880×1620 capture.

| Variable changed | Sphere region | Verdict |
|---|---|---|
| geodesic + `Pavement002` (normal map ×1.0, height 0.02, `Reflection: Automatic`) | **33.07** / 255, **flat black disc, no shading gradient**, its ground shadow present and correct | the defect |
| `useGeodesic = false`, same material | **61.80**, lit, texture visible | geometry-dependent |
| geodesic + `Parametrics/Basic` (colour only, no texture) | R **83.88**, max 211, lit **with a gradient** | lighting, normals and winding are FINE |
| geodesic, UV convention fixed (base `d1aa1b4`) | 32.35 → 33.07 | the UV transposition was real but is not this |
| `POMIterations` | 0 (default) when already black | not POM |

And the geometry itself, measured under the ENGINE's builder options `(false, false, false)` on
the compiled generator (emeraude-base test `geodesicSphereKeepsItsAttributesUnderEngineBuilderOptions`,
depth 4): **2619 vertices, 5120 triangles, U and V over their full range, 0 zero normals, 0 zero
tangents, 0 non-finite tangents, 0 tangents non-perpendicular to their normal.** The winding gates
pass. There is nothing left to blame in `ShapeGenerator`.

So: lit when untextured, black when textured, clean attributes, and the small UV sphere (32
triangles, and it even carries two zero pole tangents) renders the same material correctly. The
defect sits in the engine's **textured path on this geometry** — what differs between the two
spheres from the engine's point of view is vertex/triangle count (2619/5120 vs ~50/32), the
resource key, and the construction mode the shape came from (`Triangles` vs `TriangleStrip`).

## What remains

- [ ] **Albedo-texture-only A/B** (a material with an albedo texture and NO normal/height map) on
      the geodesic sphere: lit ⇒ the normal-map/TBN path is what fails on this geometry (the
      tangent ATTRIBUTE plumbing, not the tangent values, which are clean); black ⇒ texture sampling
      itself — see [`mdi-wrong-texture-after-first-frame.md`](mdi-wrong-texture-after-first-frame.md),
      whose symptom ("opaque meshes sample the wrong texture after the first frame") fits a flat
      black disc if the wrong texture is black or a dummy.
- [ ] **RenderDoc on the sphere's draw**: bound textures, vertex input layout, fragment output.
      ⚠️ Validation layers OFF and X11 for the capture, or no `.rdc` is written, silently
      ([`renderdoc-layer-present-rejected.md`](renderdoc-layer-present-rejected.md)).
- [ ] Check `Reflection: Automatic` on this material against
      [`reflection-automatic-component-missing.md`](reflection-automatic-component-missing.md) — the
      UV sphere shares the material and lights, so it is unlikely to be the cause, but it is on the
      path.

## ⚠️ Traps

- ⚠️⚠️ **Four attributions were already wrong on this sphere — do not re-run them**: POM, the UV
  transposition (fixed for its own sake), the baked vertex colour, an inverted winding. Every one
  was plausible and every one fell to a single-variable measurement. The next step is an
  instrument, not a hypothesis.
- ⚠️ Any A/B on a codegen setting is invalid while `POMIterations` is outside the program cache key
  ([`pom-setting-outside-program-cache-key.md`](pom-setting-outside-program-cache-key.md)); set
  both shader caches to false for such a measurement.
- The material is `Grounds/Pavement002`, NOT `Walls/Bricks001` (that is the cube's) — an earlier
  version of the base item had it wrong.

## References

- emeraude-base `docs/todo/geodesic-sphere-uv-convention-transposed.md` — the UV defect, fixed and
  pinned; its darkening section hands over to this item.
