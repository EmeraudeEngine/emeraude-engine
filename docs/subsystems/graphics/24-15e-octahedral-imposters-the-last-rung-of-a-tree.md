## 15e. Octahedral imposters — the last rung of a tree (Sept 2026)

Owner decisions 2026-09-23 (`docs/todo/vegetation-octahedral-imposter-atlas.md`): ALBEDO + NORMAL atlases lit at
RUNTIME (the `terrain` sun is animated), HEMI-octahedral views, 8 × 8 views of 128 px, the 64 views of a variant in
ONE render, the 12 variants over consecutive frames, a SIBLING visual gated by distance rather than a fifth LOD slot
(every LOD shares its layer's material), a hard switch today (the hashed cross-fade band is still open), and the
imposter out of shadows and ray tracing.

The pieces, and the one-line reason for each:

- **`Graphics::ImposterAtlas`** — two GPU-only textures, cleared at creation (coverage 0: an imposter drawn before
  its bake discards everything). ALBEDO `RGBA8_SRGB`, rgb PREMULTIPLIED by coverage (the bake clears to 0, the mips
  average both — the shader divides); NORMAL `RGBA16F`, the normal in the view space of the cell's own camera.
  `recordBake()` copies a bake into mip 0 and builds 5 mips by blits in the SAME submission (no read-back); the chain
  stops at 8 px per view, below which neighbouring views bleed together.
- **`RenderTarget::ImposterBake`** — the scene pass's MRT order (colour, normals, material properties, albedo) plus
  depth under an orthographic camera: the Saphir generator emits its G-buffer outputs by COUNTING the colour
  attachments, so the subject is baked by the very programs the mesh uses (alpha test, normal map, wind). One per
  scene (`Scene::imposterBakeTarget()`), registered in the render-to-texture list WITHOUT the AV console (a console
  registration would prepare every instance of the scene for its render pass). A job queue, one bake per frame, two
  warm-up renders per job (the entity's published state catches up). ⚠️ `setClearColorOverride()` to TRANSPARENT is
  mandatory — without it every empty texel was a covered black one (measured: coverage 100 %). ⚠️ It says
  `isRefreshedWhenContentArrives() = false`: a streaming forest re-flagged every on-demand target every frame.
- **`Scenes::Toolkit::bakeTreeImposter(label, tree, bounds)`** — its atlas, material, mesh and quad are named
  `"Imposter/<scene>/<label>"` (per scene since 2026-09-30: a shared name handed the second scene the first one's
  atlas; engine caution-points § "An imposter atlas is owned by its MATERIAL only"); the material is the atlas's only
  long-term owner, and the bake target retires the atlas when its job ends. It builds a BAKE-ONLY entity (`RenderableInstance::
  setBakeOnly()`, shown only to the target whose bake subject it is — never the view, the probes, the shadows or the
  TLAS) holding 64 copies of the tree, each rotated so its view direction faces the camera (`imposterCellFrame()` of
  `hemiOctahedralCellDirection()`, emeraude-base `Math/OctahedralMapping.hpp`) and scaled to `ImposterRigRadius`
  (20 m). ⚠️⚠️ The camera stands FAR (280 m): the LOD is chosen by DISTANCE, not by image size — an orthographic
  camera does not change the latter — so the bake draws the COARSEST level, the one the imposter replaces. Baked at
  LOD 0, the small cards of an aspen or a pine fell under a pixel of a 128 px view and vanished (atlas coverage 3.9 %
  and 2.1 %, against 18.3 % for the broadleaf). Returns the atlas and a `MeshResource` quad with the imposter material.
- **`StandardResource::setImposterAtlas(atlas, bounds)`** (flag `ImposterAtlasEnabled`, UBO `imposterBounds` /
  `imposterGrid`) — the vertex stage builds the billboard (`AbstractVertexStage::enableImposterBillboarding()`,
  Saphir `AGENTS.md`); the albedo lambda blends the three views premultiplied then divides by the blended coverage;
  the normal lambda brings each view's normal back to object space through that view's frame, blends, and expresses
  it in the billboard's tangent space, which the standard TBN turns into the world normal. Hashed cutout anchored on
  the quad corner.
- **`RenderableInstance::setDrawDistanceRange(near, far)`** + **`disableRayTracing()`** — the switch: the mesh visual
  [0, d], its imposter sibling [d, 0]; the ray-tracing lists honour the far limit too (what is no longer drawn as a
  mesh leaves the TLAS). Console `Core.SceneManagerService.writeImposterAtlases()` writes every baked albedo atlas.

Measured on `terrain` (210 000 trees, switch at 250 m, validation ON, 3070 Ti, 2880 × 1620): from 800 m above the
forest 117 186 instances for **234 372 triangles** (1.09 billion at LOD 3 without imposters), frame **31 ms**
(46 ms before). Atlas coverage (8 × 8 × 128 px): aspen 14.9 %, broadleaf 19.8 %, conifer 11.6 %, colonized 12.2 %.

⚠️⚠️ **The mip chain of every BC7 texture was a bilinear resample, not a mip filter** — found through the bare pines
of the conifer atlas, fixed in emeraude-base (`Processor::downsample()`, `TextureCompressor` `generateMip()`,
`TextureCache::Version` 3). See `docs/caution-points.md` § *The BC7 mip chain lost the mean*.
