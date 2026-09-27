## 15c. Vegetation Wind (Sept 2026)

A renderable flagged `HasVegetationWind` has its vertices displaced in the vertex stage by a
per-frame wind state, from the four colour channels the emeraude-base tree skinner writes:
**R** trunk bending weight, **G** branch bending weight (cumulative from the trunk), **B** limb
phase (shared by a first-order branch and all it carries), **A** baked occlusion.

⚠️⚠️ **Continuity is the contract** (2026-09-23): every term of the displacement is a continuous
function over the tree, so two vertices at a junction move together — `sway = windDir · (R ·
sin(0.9 t + s) + 0.45 G · sin(2.7 t + s + 2π B)) · amplitude`, with `s` a smooth function of the
position. The LEAF FLUTTER (`pos.y += (1 − v) · sin(9 t + φ(pos)) · 0.08 · amplitude`) runs only in the
programs of the renderable's FOLIAGE LAYER (`Renderable::setVegetationFoliageLayer()`, set to
`TreeMesh::LeafGroup` by `Toolkit::generateTreeRenderable()`; `AbstractVertexStage::enableVegetationFlutter()`,
in both cache keys), weighted by the card V, which is 1 at the petiole: a petiole never leaves its
twig. Before, the branch wave took a per-leaf phase and G restarted on every branch: the wind
dislocated the trees (emeraude-base `VertexFactory/AGENTS.md` § the channels).

**The displacement chain is `skinning → wind → every consumer`.** It follows the skinning
precedent exactly: a computed variable replaces the position attribute. The nine copies of
`m_skinningEnabled ? "skinnedPosition" : Attribute::Position` became
`VertexShader::vertexPositionExpression()` and `previousVertexPositionExpression()` — a third
displacement stage is learnt once now, not nine times.

**Where the state lives:** in the `Scenes::SceneInstanceTransforms` header, beside
`previousViewProjection` — per-frame state, and its PREVIOUS value where the motion-vector pass
already reads its own. `Scene::setVegetationWind(direction, strength, gustiness)` sets it;
**strength defaults to 0**, so no existing scene starts moving because the feature appeared.

⚠️ **The previous wind time is not optional.** The vertex stage builds the previous position with
`windTimes.y`; feeding it the current time reports zero velocity for a moving vertex and the
canopy smears under TAA. Verified on the `tree-generator` bench: with the wind on, the foliage
gradient energy is **13.05** against **13.18** still — a 0.9 % drop, inside the 0.4 % noise of a
static ground control. A broken motion vector would have collapsed it.

⚠️ **The wind direction is a WORLD direction applied to an OBJECT-space position.** Exact for a
tree standing unrotated, which is how `Toolkit::generateTreeInstance()` plants them; a tree rotated
around Y bends along a direction rotated with it. Fixing that means routing the model matrix into
the displacement, which every path would have to prepare.

⚠️ **Decided, not forgotten: the traced lane does NOT see the wind** (owner decision,
2026-09-21). The BLAS holds the undisplaced triangles, so a reflected or ray-traced-shadowed
canopy does not sway while the raster one does. A per-frame refit of 120 000 foliage triangles per
tree is the cost that buys consistency, and `blas-build-queue-saturation` is already open.

**The shadow pass sways too** (2026-09-22). `ShadowCasting` enables `PerSceneTransforms`
**per renderable**, exactly as it enables the skinning set, so the sealed pipeline layout of every
OTHER shadow caster is untouched. `castShadows()` carries the descriptor set,
`Scenes::Scene` hands it the one it already prepared, and `LightGenerator::ShadowMap` evaluates the
shadow term through `vertexShader.vertexPositionExpression()` so the two halves move together —
displacing one without the other reproduces the self-occlusion the skinning path already hit.

⚠️⚠️ **Enabling the set is NOT enough.** `onCreateDataLayouts()` must also hand the pipeline the
matching descriptor set layout, **in set-index order** (PerView, PerSceneTransforms, PerLight,
PerModel, PerModelLayer — the base class appends PerView before the hook runs). Without it every
shadow program of the renderable is refused with
`VUID-VkGraphicsPipelineCreateInfo-layout-07988`, "uses descriptor [Set 0, Binding 0, variable
ubInstanceTransforms] but was not declared in the pipeline layout", and the trees simply cast no
shadow at all.

Measured by A/B on the `tree-generator` bench, toggling ONLY the shadow-pass displacement:
**20.71 % → 36.39 %** of the shadow pixels move between two captures, at an unchanged foliage
motion (24.06 % against 24.44 %). The baseline is not zero because the measured band is not pure
cast shadow — the screen-space occlusion follows the moving geometry by itself — which is exactly
why the conclusion rests on the A/B and not on the absolute figure.

### The baked occlusion channel (Sept 2026)

The fourth channel, **A**, is folded into the DIFFUSE AMBIENT factor by
`LightGenerator::declareVegetationBakedOcclusion()`, beside a material AO texture rather than
instead of it. It never touches the direct lighting — a leaf in the sun is lit whatever its
neighbours do — nor the specular IBL, nor the emission.

⚠️⚠️ **It is nearly inert while a lighting lane is on**, because both lanes own the indirect
diffuse and the raster ambient leg they replaced is what this multiplies. Measured on the
`tree-generator` bench at a pinned exposure, declaration ON against OFF: **-0.37 %** mean foliage
luminance with a lane active, **-2.44 %** with `setLightingMode("None")` — 6.6x — and the foliage
contrast rises 35.11 → 36.15 in that case. Sky and grass controls moved 0.00 %. The channel itself
is rich (median 0.87, mean 0.84, 88 % of the vertices below 0.95), so the weakness is the term, not
the data. Open item: `vegetation-occlusion-is-inert-under-a-lighting-lane`.

⚠️⚠️ **`SceneRendering` requests the vertex color to the fragment stage itself**, since no
texture-based material ever asks for it. That is safe ONLY because the tree materials do not use
vertex colors: a material that does multiplies its albedo by the WHOLE vertex color
(`StandardResource.cpp`, `SurfaceAlbedoFinal`), which on a tree would tint every leaf by its
bending weights. Never enable vertex colors on a vegetation material without revisiting that
multiply.

**Code references:**
- `Saphir/AbstractVertexStage.cpp` — `generateVegetationWindCode()`, the two position accessors
- `Saphir/Generator/SceneRendering.cpp` — the enable and the cache-key contribution
- `Scenes/SceneInstanceTransforms.hpp` — `setWindState()` and the header layout
- `Graphics/Renderable/Abstract.hpp` — `HasVegetationWind`
