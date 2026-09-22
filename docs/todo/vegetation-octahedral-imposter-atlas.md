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

## What remains

1. **The bake orchestration.** The engine already renders offscreen on demand, and this was
   verified rather than assumed:
   - `Scenes::Scene::createRenderToTexture2D(name, w, h, colorCount, viewDistance, ortho)`
     (`src/Scenes/Scene.rendering.cpp:159`) creates the target and registers it on the scene.
   - `Graphics::Renderer::renderRenderToTextures()` (`src/Graphics/Renderer.cpp:2457`) walks
     `scene.forEachRenderToTexture()` once per frame and **honours an on-demand contract**: with
     `isAutomaticRendering()` false, a target renders only while `setRenderOutOfDate()` has
     flagged it (`src/Graphics/RenderTarget/Abstract.hpp:170-215`). That is exactly the "bake
     once, then never again" the atlas needs.
   - `Scenes::Toolkit::generateTexture2DRenderer()` (`src/Scenes/Toolkit.hpp:1185`) is the working
     precedent: a perspective camera entity connected to the target through
     `AVConsoleManager().connectVideoDevices()`.
   ⚠️ The loop renders **one target per frame pass**, so an N x N atlas is an N² multi-frame
   sequence unless N² targets are registered at once. Decide which: a long hitch, or a lot of
   simultaneous framebuffers. **Owner decision.**
2. **The shading path.** A camera-facing quad that calls `octahedralBlend()` for the view
   direction and blends the three cells. The mapping header is ready; the GLSL side is not
   written, and Saphir has no octahedral node yet.
3. **When the atlas is baked**: offline into the data store, or once at load time. A load-time
   bake costs a visible hitch; a stored atlas costs disk and a pipeline step. **Owner decision.**

## Traps

- ⚠️⚠️ **The octahedral map is 2-to-1 on the BORDER.** Two border cells legitimately hold the same
  view — on an 8x8 grid, `(3, 7)` and `(4, 7)` both decode to `(0, -0.143, 0.857)`. The baker may
  skip a duplicate; it must never try to make the border cells distinct, and never "fix" the blend
  to force a cell onto itself. A test of mine asserted a cell recognises its own INDEX and failed
  on exactly that; assert on the DIRECTION the imposter shows. Full account in emeraude-base
  `docs/caution-points.md` § *The octahedral map is 2-to-1 on the BORDER*.
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
