---
id: multi-layer-splat-material
title: Multi-layer splat material — blend N texture sets under a mask, on large surfaces only
status: open
priority: unranked
scope: Graphics/Material
opened: 2026-09-13
tags: [materials, terrain, saphir, shaders]
---

# Multi-layer splat material — blend N texture sets under a mask, on large surfaces only

## Why

Authoring a dirt path inside grass on a ground surface. Before PBR this was one material with two
albedo textures and a mask. The reflex is now "it must be multi-pass"; it must not be, and the
analysis below says why and what the alternatives actually are.

**Mixing two materials means mixing BRDF PARAMETERS, not two radiances.** `mix(L(grass), L(dirt), m)`
differs from `L(mix(params))` as soon as roughness, metalness or the normal differ — and between
grass and packed dirt they all do. The legacy trick was exact only because the pipeline blended
diffuse colour alone, and `N·L` is linear. GGX and Fresnel are not, and a normal has a direction.

## Owner decisions (2026-09-13)

- **N layers addressed through a `sampler2DArray`** — chosen over N discrete samplers, bindless
  indices and an atlas.
- **Weights come from an RGBA mask texture** (4 weights). Vertex colours, procedural slope/height
  and height-blend were NOT retained for this round.
- **Scope: large surfaces only (ground / terrain).** This *revises* the earlier "every
  `StandardResource`" answer taken the same day. The feature must not tax the 28 parametric
  materials nor the glTF corpus — which settles the UBO question below towards a gated,
  conditional storage rather than a longer common block.
- **Nothing is to be coded yet.** This item records the analysis so a later session does not
  restart from the multi-pass reflex.

## The option map (analysis, 2026-09-13)

Classified by WHERE the blend happens. O1, O2 and O4 need no engine work at all.

| | Available today | Soft edge | G-buffer correct | RT correct | Render cost | Engine work |
|---|---|---|---|---|---|---|
| **O1a** overlaid layer, CUTOUT mask | yes | **no**, binary | yes | yes | ×2 light passes | none |
| **O1b** overlaid layer, BLENDED mask | yes | yes | **no — destroyed** | no | ×2 light passes | none (unusable) |
| **O2** split the geometry (path = sub-geometry) | yes | no, follows edges | yes | yes | none | none (DCC tooling) |
| **O3** in-material splat | **no** | yes | yes | ⚠️ gap | N fetches × (1 + lights) | **substantial** |
| **O4** offline bake (unique mapping) | yes | yes | yes | yes | none | none (tooling + VRAM) |
| **O5** projected deferred decals | no | yes | yes | yes | low | blocked by `mrt-single-pass-deferred` |

### O1 — the overlaid layer IS reachable today

`MultiLayerMeshResource::load(geometry, materialList, rasterizationOptions)` takes ONE geometry and
a LIST of materials (`src/Graphics/Renderable/MultiLayerMeshResource.hpp:276`). On a geometry with
no sub-geometries — a terrain grid exactly — `subGeometryRange()` ignores the index and returns the
full range (`src/Graphics/Geometry/VertexGridResource.hpp:183`, same fallback in
`src/Graphics/Geometry/IndexedVertexResource.hpp:186`). So N layers are N full re-draws of the same
triangles, each with its own material and its own `RasterizationOptions` (depth bias included), each
dispatched independently into the opaque/translucent lists (`src/Scenes/Scene.rendering.cpp:1428`).

⚠️ **But a BLENDED overlay destroys the G-buffer.** In the ambient pass a translucent material gets
the MRT attachments in REPLACE (`blendEnable = FALSE`, ONE/ZERO) through `independentBlend`, on
purpose, so water writes its own normal instead of the bottom's
(`src/Saphir/Generator/SceneRendering.cpp:809-838`). A dirt layer covering the whole grid would
therefore stamp its normals/albedo/material-properties at 100 % over the ENTIRE terrain, including
where its alpha is 0 — RTGI, RTR, SSR, SSAO and TAA would read uniform dirt under a picture of
grass. Only the CUTOUT variant (`AlphaTestEnabled`, the documented binary-coverage contract) is
sound, and it buys a binary edge: `alphaToCoverageEnable` is hardcoded to `VK_FALSE`
(`src/Vulkan/GraphicsPipeline.cpp:450`), so alpha-to-coverage cannot soften it either.

### O3 — why the in-material splat fits the architecture

`setupLightGenerator()` hands the light generator **names of GLSL variables**, never values
(`src/Graphics/Material/StandardResource.cpp:2117`). Declaring one folded variable per surface
parameter makes the BRDF, the MRT, RTGI, SSR and TAA correct **by construction, with no change
downstream**. The precedent exists: vertex colours are already folded into a single
`SurfaceAlbedoFinal` for exactly that reason (`src/Graphics/Material/StandardResource.cpp:3478`).

## What remains

- [ ] Decide where the per-layer parameters live. The material UBO is a flat 104-float block
      dimensioning EVERY material element (`src/Graphics/Material/StandardResource.cpp:2551`, layout
      in `src/Graphics/AGENTS.md` § Material Property Layout). With the narrowed scope, a **second,
      conditional descriptor** (UBO or SSBO, present only when the multi-layer flag is armed) is the
      candidate; a longer common block would tax every simple material.
- [ ] Decide the ray-tracing answer (see the trap below): dominant layer, a new bindless
      `sampler2DArray[]` table, or reuse the per-layer 2D views `AnimatedTexture2D` already builds.
- [ ] Decide the mask's UV source: wake the second UV set, or divide the primary set by the grid's
      UV multiplier (which the material cannot currently read).
- [ ] Add a non-animated `Texture2DArray` resource + its JSON (list of images, on the
      `CubemapResource` model). Reusing `AnimatedTexture2D` is a contract fault: its `frameCount()`
      / `duration()` drive `Material::isAnimated()`, which drives the render Z-sort
      (`src/Graphics/Material/Interface.hpp:198`).
- [ ] Add the 4 component types (`LayerAlbedoArray`, `LayerNormalArray`, `LayerORMArray`,
      `LayerMask`) — four samplers whatever N, which is the whole point of the array choice, since
      `m_components` holds ONE component per type
      (`src/Graphics/Material/StandardResource.hpp:1700`).
- [ ] Make the layer index come from the material, not from a vertex attribute. Today the `.z` of 3D
      UVs is baked into a vertex attribute (sprites).
- [ ] Index-address the per-layer tiling: `transformedTexCoords()` is a `switch (ComponentType)`
      with one UBO key per component (`src/Graphics/Material/StandardResource.cpp:2824`) and does
      not scale to N.
- [ ] Take flag bit **19** (the only free one) for `MultiLayerEnabled` and make sure it reaches
      `ProgramCacheKey::materialFlags`.

## ⚠️ Traps

- **The RT lane does not follow.** `GPURTMaterialData` carries ONE texture index per role
  (`src/Graphics/Material/GPURTMaterialData.hpp:101`) and the `BindlessTextureManager` has no
  `sampler2DArray[]` table — its bindings are 1D/2D/3D/Cube/CubeArray (`src/Graphics/AGENTS.md`
  § Bindless Textures Manager). A splatted ground would be splatted in raster and uniform under
  RTR/RTGI/RTAO. RTGI demodulates by albedo, so this produces a disagreement between the indirect
  diffuse of primary surfaces and of reflected ones — the family of error that the pinned-exposure
  probes-vs-RTGI reading already cost a session.
- **Never a `sampler3D` instead of a 2D array.** `Texture3D` is right there and the substitution
  compiles: a 3D texture FILTERS BETWEEN layers (grass bleeding into dirt at every sample) and mips
  along depth. It must be a real `VK_IMAGE_VIEW_TYPE_2D_ARRAY` — per-layer mips, no inter-layer
  filtering.
- **At least TWO arrays, never one.** Albedo is sRGB, normal and ORM are UNORM. And the engine
  decides sRGB **by the GLSL variable name** (a `Color` suffix, `docs/material-json-format.md`):
  renaming one silently changes its colour space.
- **The material fragment code runs once per LIGHT PASS** in forward multi-pass. 4 layers × 3 arrays
  = 12 fetches × (1 + light count) per pixel, on a `ScenePass` that already costs 8.63 ms / 13.7 %
  of the frame (`docs/post-processing-pipeline.md`). `mrt-single-pass-deferred` is the real
  performance prerequisite of a rich splat.
- **`UVWChannel()` is dead.** `Component::Texture` stores it
  (`src/Graphics/Material/Component/Texture.hpp:277`) but `textCoords()` always returns
  `Primary2DTextureCoordinates` (`src/Graphics/Material/StandardResource.cpp:2807`). The second UV
  set is plumbed all the way to the vertex shader and no component ever selects it.
- **The grid's UV multiplier is BAKED into the vertex UVs** at generation time (default **500**,
  `src/Graphics/Geometry/VertexGridResource.hpp:73`, applied by emeraude-base's
  `VertexFactory/Grid::setUVMultiplier()`). The mask cannot reuse UV0 as is.
- **Array layers must share format, extent and mip count.** A 2048² grass and a 1024² dirt need
  resampling at import — the price of the array choice, and an argument for an assembly tool.
- **Vertex colours are already taken.** `TerrainResource` can load a vertex-colour map but the
  wiring is commented out (`src/Graphics/Renderable/TerrainResource.cpp:323`), and glTF `COLOR_0`
  reserves them for albedo modulation. Diverting them to splat weights is a contract conflict.

## Not retained this round

**Height-blend** — correcting the weights by each layer's height map, so dirt passes BETWEEN blades
of grass instead of cross-fading linearly. Nearly free once the splat exists (the engine already has
`ComponentType::Displacement` and POM), and it is the single detail that separates a credible splat
from a Photoshop fade. Worth reopening once the base splat lands.

## References

- Related item: `mrt-single-pass-deferred` (P1) — the deferred move that both reduces the splat's
  cost to once per pixel and unlocks option O5 (projected decals).
- `src/Graphics/AGENTS.md` § Material UBO System, § Material Component System, § Bindless Textures.
- `docs/material-json-format.md` — the JSON contract a layer set would extend.
