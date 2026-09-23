---
id: vegetation-octahedral-imposter-atlas
title: Vegetation — bake an octahedral imposter atlas for the last LOD
status: open
priority: unranked
scope: Graphics
tags: [vegetation, lod, offscreen]
opened: 2026-09-21
---

# Vegetation — bake an octahedral imposter atlas for the last LOD

## Why

The last LOD of a tree is a card, and a card is only convincing if what it shows was rendered from
the real mesh. emeraude-base can only produce the card **geometry**; baking the atlas needs a
renderer, so it belongs here.

## What is DONE

**The parametrisation is delivered, tested and shared** (emeraude-base, 2026-09-22):
`Math/OctahedralMapping.hpp` carries `octahedralEncode()`, `octahedralDecode()`,
`octahedralCellDirection(cellX, cellY, gridSize)` — which direction the baker must render a cell
from — and `octahedralBlend(direction, gridSize)` — which three cells the shader must blend, with
barycentric weights. Both consumers read the SAME header on purpose: a baker and a shader that
disagree by one cell show a neighbouring view.

Four tests in `src/Testing/test_MathOctahedralMapping.cpp` (suite 2072, 2069 pass + 3 live-network
skips): direction round trip over a dense sweep, positive weights summing to 1, continuity under a
small rotation, and the blend landing back on the direction asked for.

The geometry side is also done: `TreeMesh::imposter()` is the crossed-quads card, kept apart from
the level chain because it carries ONE group — it samples the atlas this item bakes.

## Owner decisions taken (2026-09-22)

1. **The atlas is baked ONCE AT LOAD TIME**, not stored in the data store. A visible hitch is
   accepted; a baked atlas on disk is not wanted.
2. **The tree is isolated by a PER-TARGET FILTER added to the engine**, not by a throwaway bake
   scene and not by a visibility-layer system. A render target learns to draw ONE renderable over
   a transparent clear. The contract is the one a baker actually needs and it is reusable by any
   future bake (asset thumbnails, icons, per-object probes); a layer mask would touch every entity
   and every pass for a far wider scope than this item.

## Owner decisions taken (2026-09-23) — the `terrain` forest made it urgent

The `terrain` demo now plants ~210 000 trees (projet-alpha `src/Builtin/AGENTS.md` § 6d). Measured with
`Core.SceneManagerService.getRenderStatistics()`: every visible tree is at LOD 3 and LOD 3 is still ~16 000
triangles, 1.09 billion triangles in view. The owner chose (2026-09-23):

3. **Albedo + normal atlases, lit at RUNTIME** by the engine's pipeline (the sun of `terrain` is animated;
   a baked lit colour would be wrong the moment it moves) — the Ryan Brucks / Unreal approach.
4. **Hemi-octahedral** coverage: only the views above the horizon, all the resolution for them.
5. **8 × 8 views × 128 px** per variant: a 1024² atlas, ~8 MB a variant for albedo + normal.
6. **All views baked in ONE frame** (one submission, one hitch at load).

7. **The rung is a SIBLING visual with a draw-distance range**, not a fifth LOD slot: every LOD of a
   renderable shares its layer's material and program (`Renderable::Abstract::material(layer)` takes no
   LOD, the program key has none), and the imposter needs another material.
8. **Switch at a distance in metres with a HASHED cross-fade band** (reusing the hashed alpha test), not a
   hard cut and not screen coverage.
9. **Beyond the switch distance a tree also leaves the TLAS** (its BLAS is LOD 0); the imposter never
   enters it (a frozen, wrongly-facing quad).
10. **The 12 variants bake over 12 consecutive frames** with one reused target (~56 MB transient).

## The implementation plan (2026-09-23)

1. emeraude-base `Math/OctahedralMapping.hpp`: HEMI-octahedral encode / decode / cell direction / blend +
   the cell's camera frame; tests in `test_MathOctahedralMapping.cpp`.
2. `RenderableInstance` flag `BakeOnly`: such an instance renders ONLY into the target whose bake subject it
   is (never the view, the probes or the TLAS).
3. A G-buffer bake render target (colour, normals, material properties, albedo, depth — the scene pass's MRT
   order) whose attachments are copied to staging buffers in the same submission and read after the fence.
4. `Scenes::ImposterBaker`: one variant per bake, 64 rotated copies of the tree in ONE instanced subject under
   one orthographic camera, wind frozen during the bake; the CPU turns the read-back into two textures
   (albedo+coverage sRGB, object-space normal+depth linear).
5. A billboard vertex mode that keeps the instance's yaw and picks the three cells; the imposter mode of
   `StandardResource` (three weighted atlas samples, hashed cutout anchored on the atlas, the decoded normal
   fed to the lighting).
6. `RenderableInstance` draw-distance range + `DisableRayTracing`; the demo adds the imposter visuals.

## What remains

**The bake and the runtime imposter are DELIVERED (2026-09-23)** — `src/Graphics/AGENTS.md` § 15e: the atlas,
the G-buffer bake target, `Toolkit::bakeTreeImposter()`, the billboard vertex mode, the imposter material mode, the
draw-distance switch and the RT/shadow exclusion; `terrain` draws 117 186 imposters for 234 372 triangles. Open:

1. **The hashed CROSS-FADE band** (owner decision 8): today the switch at 250 m is a hard cut per cell. Needs a
   per-draw fade factor both the mesh materials and the imposter read, from the cell distance and the band.
2. **Visual validation of the imposter LIGHTING** against the mesh at the switch distance, pinned exposure and frozen
   wind (the atlas geometry and coverage are validated; the normals were not compared side by side yet).
3. **Depth / parallax**: the atlas stores no depth, so the three views are blended without per-view parallax
   (Brucks' ray–plane step) — a mild ghosting when the eye turns fast around a near imposter.
4. **Memory**: 12 variants × (albedo RGBA8 + normal RGBA16F, 5 mips) ≈ 200 MB uncompressed; BC7/BC5 would divide it.
5. **The wind during the bake**: the copies sway with the scene's wind; the 1.15 margin absorbs `terrain`'s, a
   stronger wind smears the views.

## Traps

- ⚠️⚠️ **The octahedral map is 2-to-1 on the BORDER.** Two border cells legitimately hold the same
  view — on an 8x8 grid, `(3, 7)` and `(4, 7)` both decode to `(0, -0.143, 0.857)`. The baker may
  skip a duplicate; it must never try to make the border cells distinct, and never "fix" the blend
  to force a cell onto itself. A test of mine asserted a cell recognises its own INDEX and failed
  on exactly that; assert on the DIRECTION the imposter shows. Full account in emeraude-base
  `docs/caution-points.md` § *The octahedral map is 2-to-1 on the BORDER*.
- ⚠️ The offscreen pass clears with `m_swapChainClearColors`, an OPAQUE colour. An imposter needs
  alpha 0 behind the tree, or every card shows a rectangle of sky.
- ⚠️ Bake at a **pinned exposure** (`Core.SceneManagerService.Act.setExposure()`). Baking through
  the auto-exposure burns whatever the camera happened to be metering into the atlas, and the
  imposter then never matches the mesh it replaces.
- ⚠️ The normals in the atlas are in view space of the baking direction. Write down which frame
  they are in, or the lighting on the far LOD will silently disagree with the near one.
- ⚠️ **Freeze the wind before comparing two bakes** (`Scene::setVegetationWind(dir, 0, 0)`). Two
  runs at different wind phases are two different trees; that already produced an impossible
  "13 % brighter under a floor" reading on the `tree-generator` bench.

## References

- Cigolle, Donow, Evangelakos, Mara, McGuire & Meyer, *A Survey of Efficient Representations for
  Independent Unit Vectors*, JCGT 3(2), 2014, § 3.3 — the parametrisation implemented.
- The renderable side is DONE: `Scenes::Toolkit::generateTreeRenderable()` builds a
  `MultiLayerMeshResource` from a `TreeMesh`. See `src/Graphics/AGENTS.md` § 15b and the
  projet-alpha `tree-generator` bench.
