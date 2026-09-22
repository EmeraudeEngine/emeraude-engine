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

## What remains

1. ~~The per-target bake filter~~ **DONE** (2026-09-22). `RenderTarget::Abstract` gained
   `setBakeSubject()` (render ONE instance, `nullptr` = the whole scene as before) and
   `setClearColorOverride()` (the target's own clear, the renderer's being opaque). The filter sits
   at the single site that feeds every render list,
   `Scene::checkRenderableInstanceForRendering()`, so the MDI batches and the lighted selections
   inherit it for free. Cascade builds clean, 0 warning. ⚠ **Not yet exercised at runtime** — no
   caller sets a bake subject until the orchestration below exists. Described in
   `src/Graphics/AGENTS.md` § 15d.

2. **The N x N sequencing.** Secondary, and cheap either way now that the assembly is known to
   exist: `commandBuffer.blitImage()` / `copyImage()` are already used by `GrabPass`
   (`src/Graphics/GrabPass.cpp:709-736`) and `PostProcessor` (`src/Graphics/PostProcessor.cpp:760`),
   so N views are assembled into one atlas image with no new Vulkan work. The loop draws one
   target per pass, so it is either N^2 targets registered at once (one long hitch) or one target
   over N^2 frames (a small state machine). To be settled when the filter lands — it trades VRAM
   against hitch length and nothing else.
3. **The GLSL side.** No blocker: `Declaration::Function` is the mechanism, and
   `Graphics/Effects/Resolve/FXAA.cpp:46` is the working model (name, return type,
   `addInParameter`, `Code{fn, Location::Output}`, `shader.declare(fn)`). ⚠️ Functions are emitted
   BEFORE sampler declarations, so a declared function may not reference a sampler declared later
   — it takes the sampler as an in-parameter, exactly as FXAA does. The octahedral node is pure
   `vec3` math, so this costs nothing. ⚠️ **It will be a SECOND implementation of the mapping**,
   and the four tests guard the C++ one only: transcribe it line by line and say so in both files.

The mechanics already verified and available:
- `Scenes::Scene::createRenderToTexture2D()` (`src/Scenes/Scene.rendering.cpp:159`) creates and
  registers the target.
- `Renderer::renderRenderToTextures()` honours an **on-demand contract**: with
  `isAutomaticRendering()` false, a target renders only while `setRenderOutOfDate()` flags it
  (`src/Graphics/RenderTarget/Abstract.hpp:170-215`). That is the "bake once, then never again"
  the atlas wants.
- `Scenes::Toolkit::generateTexture2DRenderer()` (`src/Scenes/Toolkit.hpp:1185`) is the working
  precedent: a camera entity connected to the target through
  `AVConsoleManager().connectVideoDevices()`.

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
