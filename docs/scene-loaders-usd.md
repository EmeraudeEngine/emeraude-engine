# Scene Loaders & OpenUSD Integration — Design Document

> Status: **phase 1 in progress** — geometry, materials, sky and INSTANCING render.
> Milestones and what remains: § 7.
> Owner decisions recorded below are binding. Any deviation requires a new decision.
>
> Related: [`scene-graph-architecture.md`](scene-graph-architecture.md),
> [`renderable-instance-system.md`](renderable-instance-system.md),
> [`resource-management.md`](resource-management.md),
> [`coordinate-system.md`](coordinate-system.md),
> [`../src/Scenes/Loaders/AGENTS.md`](../src/Scenes/Loaders/AGENTS.md).

## 1. Purpose

Add OpenUSD as a **first-class scene format** to Emeraude-Engine, alongside glTF, FBX and
WAD. USD is not another mesh format: it describes a *composed scene* — layers, instancing,
lights, cameras, materials, time samples. Integrating it exposes the fact that the existing
`AssetLoaders` layer has always been a **scene-loading layer** wearing the wrong name.

The reference asset is Intel's **Jungle Ruins** sample scene (see § 3), which the engine must
render at production quality. That asset has a second, larger role: it is the engine's
**GOLD GOAL** — the owner's *Saint Graal* — the single scene whose completion says the runtime
has arrived, and the benchmark it is measured against until then (see § 8).

## 2. The Absorption Rule

> **Owner directive.** Loading a USD scene must translate *everything* into native
> Emeraude-Engine scene logic. Where the scene layer cannot express a USD concept, it is the
> **`Scenes` subsystem that gains the missing logic** — never USD constructs that survive in
> memory, and never a workaround in the loader.

Consequences, which drive the whole design:

- **Nothing USD survives `load()`.** The loader is a throwaway function, exactly like
  `GLTFLoader`. No persistent stage, no prim-to-node mapping kept alive, no USD subsystem.
- **Every capability added for USD is a native scene capability**, therefore reusable by
  glTF, FBX, the editor and the JSON scene format. USD is the client that reveals the gap,
  not the owner of the solution.
- **Composition is resolved at load time**, producing one flat native hierarchy.

## 3. Reference Asset — Measured Facts

`projet-alpha.data/data-stores/USD/JungleRuins` — 7.0 GB total (USD 1.8 GB, textures 3.5 GB,
Blender sources 1.7 GB).

### 3.1 Stage structure

Root layer `USD/JungleRuins_Karma.usda`:

| Metadata | Value | Impact |
|----------|-------|--------|
| `subLayers` | 19 layers | Root composition arc is a sublayer, not a reference |
| `metersPerUnit` | `0.01` (centimetres) | Unit conversion required |
| `upAxis` | `"Z"` | Engine uses the **Y-DOWN** convention (UP = -Y) — axis conversion required |
| `startTimeCode` / `endTimeCode` | `0` / `4800` | 200 s of animation at 24 fps |

Content: one `DomeLight` (equirectangular HDR, 8K, 67 MB), one `Camera` layer exported from
Blender 4.2, and 18 element directories.

### 3.2 File formats and sizes

All 18 `.usd` files are **`PXR-USDC` crate (binary)**; the 14 `.usda` files are ASCII.

| File | Size | Note |
|------|------|------|
| `elements/Banyan/Banyan.usd` | 313 MB | crate |
| `elements/QueenForest/queenforest_classes.usda` | 247 MB | **ASCII** |
| `elements/RiverForest/riverforest_classes.usda` | 206 MB | **ASCII** |
| `elements/Terrain/Terrain_Cinematic.usd` | 118 MB | crate |
| `elements/RiverForest/PI_S_RiverForest.usd` | 77 MB | crate |
| `elements/Pyramid_Moss/PI_S_Moss.usd` | 65 MB | crate |

Naming convention observed: `PI_*` files hold `PointInstancer` prims; `*_classes.usda` files
hold the prototypes they instance.

### 3.3 Materials

Shader inventory across the ASCII layers — **no MaterialX, no UDIM**:

| Shader `info:id` | Count |
|------------------|-------|
| `UsdUVTexture` | 298 |
| `UsdPreviewSurface` | 105 |
| `UsdPrimvarReader_float2` | 93 |

This is exactly the subset tinyusdz supports, which removes the main feature risk.
Materials use `inputs:opacityThreshold = 0.5` (**alpha cutout**, already implemented for the
WAD loader) and are named `*_TwoSided` (**two-sided lighting**, fixed in `5bec23db`).

### 3.4 Textures

263 files, 3.5 GB: Terrain 2.0 GB, Plants 1.1 GB, Trees 260 MB, Pyramid 217 MB.
Formats: 159 JPEG, 104 PNG, 10 TIFF, 2 EXR, 1 HDR.

Largest measured: `Terrain/Terrain_Cinematic_Normal_3_4.jpg` — 129 MB, **8192×8192**, 8-bit.
Terrain textures are manually split into a grid (`_3_3`, `_3_4`, `_4_4`), not USD UDIM.

> An 8K RGBA8 texture costs **256 MB of VRAM** (341 MB with mips). In BC7 the same texture
> costs ~85 MB with mips. On the target hardware (8 GB), BC7 is not an optimisation — it is
> the condition for the scene to fit at all.

### 3.5 Composition arcs actually used

The ASCII layers contain **no `variantSet`, no `payload`, no `references`, no `inherits`**.

> **Unverified.** The same check on the USDC crates is worthless: crate files compress their
> token table, so a byte scan cannot detect a token whether it is present or not. The
> definitive inventory requires a real parser. **The first task of `USDLoader` is therefore to
> dump a stage inventory** (arcs used, prim types, instance counts) and report it, before any
> conclusion is drawn about what this asset does or does not use.

### 3.6 Licensing and attribution

Jungle Ruins 1.0b — **Creative Commons Attribution 4.0 International**. Created by Cristiano
Siqueira; artistic support Charlene Teets; published as part of the Intel Sample Library
(Siqueira, Teets, Herholz, Sochenov, Kaplanyan, 2024).

Any publication using this scene — including screenshots in documentation or promotional
material — **must carry the attribution and the BibTeX citation** found in
`JungleRuins/credits_license.txt`. The asset lives in the data repository only; it is never
redistributed with the engine.

## 4. Implementation Choice — tinyusdz

| | Decision |
|---|---|
| Library | **tinyusdz** (`lighttransport/tinyusdz`) |
| License | Apache 2.0 / MIT — compatible with the engine's LGPLv3 |
| Dependencies | None beyond the C++ STL |
| Exceptions / RTTI | Advertised as requiring neither — matches `-fno-exceptions` |
| Integration path | `ext-deps-generator`, `Setup*.cmake` owned by emeraude-base, like `ufbx` and `fastgltf` |

Rejected: **OpenUSD (pxr)** — several hundred MB, TBB imposed, a probable conflict with
`-fno-exceptions`, and a build to industrialise across three platforms. Rejected: **a
hand-written USDA/USDC parser** — the crate format plus the composition engine is a
multi-month effort before the first triangle.

> **Accepted risk (owner decision).** The `-fno-exceptions` compatibility of tinyusdz was
> **not** verified experimentally before design. If the advertised property is false, it
> surfaces at integration, not now. Known and assumed.

Secondary risks, in order of likelihood:

1. **Crate reader coverage** — all the heavy geometry is USDC. A gap here blocks everything.
2. **Composition correctness** — advertised as experimental. Mitigated by § 3.5: the asset
   appears to use only `subLayers`, the simplest arc.
3. **ASCII parsing throughput** — 450 MB of `.usda` to parse, including two files above
   200 MB.

### 4.1 What integration actually cost — measured, 2026-08-08

The library builds with `TINYUSDZ_CXX_EXCEPTIONS=Off`, with no third-party dependency and no
source patch needed to compile: **the accepted `-fno-exceptions` risk is retired on Linux.**
Everything below was found afterwards, against the real asset.

**Four upstream defects**, all in the v0.9.4 release tag:

| Defect | Effect | Handling |
|--------|--------|----------|
| `LoadUSDFromFile()` composes nothing | A 19-sublayer stage returns **2 prims** and reports success | Use the explicit pipeline: `LoadLayerFromFile` → `CompositeSublayers` → `LayerToStage` |
| The asset resolver's state is never restored after recursion (upstream leaves a `TODO` right above the mutation) | After descending into one sublayer, every following **sibling** resolves against the child's directory; composition aborts | RAII save/restore guard, `patches/tinyusdz.patch` |
| `CompositeSublayersInPlace()` is declared in the header but never implemented | Undefined reference at link time | Use the plain `CompositeSublayers()`; `LayerToStageInPlace()` IS implemented and is used |
| **`GeomPointInstancer` missing from the PrimSpec→Prim table** of `composition-reconstruct.cc`, though `ReconstructPrim<GeomPointInstancer>` is fully implemented in `prim-reconstruct.cc` | `LayerToStage` drops **every** `PointInstancer` **with its entire subtree**. The whole vegetation of the asset disappeared, and the only trace was a `warn` string the loader did not print | Missing table rows added in `patches/tinyusdz.patch`, together with the five UsdLux types in the same situation (§ 4.5) |

**One behaviour to design around, not a defect:** tinyusdz stores each sublayer's working
directory as the raw relative path written in the file (`elements/Anthurium`), never joined
with the root. A reference made from inside such a layer is looked up relative to the
*process* working directory and fails **silently, taking its whole prim with it**. The loader
therefore seeds the resolver with every directory of the stage tree.

> ⚠️ That failure mode is the dangerous one: the hard geometry arrives, the scene looks
> plausible, and only the referenced prototypes are missing. Measured on Jungle Ruins before
> the fix: 84 meshes in, **0 PointInstancer prototypes**, no error raised.

### 4.2 Eager composition does not converge — owner decision

`CompositeAllArcs()` resolves references, payloads, inherits and variants in one eager pass.
On Jungle Ruins that means ingesting 450 MB of ASCII prototype layers up front. **Measured:
24 minutes, 15 GB resident, growing linearly at ~7 MB/s with no convergence** before the run
was killed.

Sublayer composition alone completes in seconds at ~3 GB and already yields the entire
non-instanced scene: **773 prims, 84 meshes, 87 materials, 434 shaders, 1 camera, 1 dome
light**. The loader therefore stops there (owner decision, 2026-08-08), and prototype
references become an **on-demand, per-element** resolution step — the deferred strategy chosen
in § 7.

> Consequence: `inherits` and `variants` are not applied on this path. The `LoaderOptions`
> variant selection of § 5.4 lands together with the on-demand resolver, not before.

### 4.3 Tydra, and where milestone 4 currently stands

**Tydra is the right layer to build on.** It returns renderer-ready data — triangulated faces,
indexed vertices, resolved material bindings, and a `RenderInstance` array that milestone 6
will need. Re-deriving that from raw prims would be duplicated work; the engine's own job
starts at the translation into native scene logic, not at mesh plumbing.

> ⚠️ **Two Tydra front-ends exist and they are NOT equivalent.**
> - `LayerToRenderSceneConverter` (Layer-based) is experimental: its in-place entry point is
>   **refused at runtime** ("destructive source transfer is not implemented safely yet"), and
>   its plain entry point returned the node hierarchy with **zero meshes** on Jungle Ruins.
> - `RenderSceneConverter` (Stage-based) is the mature path and is the one used.

A fourth patched defect: `tydra/shape-to-mesh.hh` includes `"../../src/math-util.inc"`, a path
that only resolves inside the source tree — the installed header cannot compile. Fixed to
`"../math-util.inc"` in `patches/tinyusdz.patch`.

**Measured on the full stage:** 84 source meshes → **84 built**, 87 materials, 324 textures,
198 images, 4 root nodes, 1 camera, 1 light. The scene reports `successfully loaded`.

> 🔶 **Open: the renderer stalls and segfaults after the scene loads.** The swap chain reports
> `The acquisition of the next image was canceled by the 60000000000 ns timeout`, then the
> process dies with SIGSEGV. **No Vulkan validation error, no `DEVICE_LOST`, no allocation
> failure is logged** — so the cause is not yet attributed. Next step is a Debug build with a
> backtrace, not more guessing. Candidates to rule out in order: the sheer size of the uploads
> (one source mesh is 313 MB), the winding/orientation bake, and the `RasterizationOptions`
> path for double-sided meshes.

> ⚠️ Tydra's own texture loading is refused by its security policy — `Unsafe asset path` on any
> `../` — which is harmless here: the engine loads textures itself. It does mean the asset paths
> must be resolved on our side at milestone 5.

### 4.4 Materials and the asset's own sky — state at 2026-08-09

**Materials**: `UsdPreviewSurface` maps term for term onto the engine's PBR — base colour,
roughness, metalness, normal. Verified on screen: the pyramid renders in its authored limestone
with per-terrace wear.

> ⚠️ **tinyusdz never loads a texture from this asset.** Its security policy rejects any asset
> path containing `..`, which every Jungle Ruins texture uses. Only `TextureImage::asset_identifier`
> survives; the loader resolves it against the stage directory and the engine reads the file.
> sRGB for base colour, linear for roughness/metalness/normals — inverting that reads as a
> lighting bug, not a colour-space one.

> ⚠️⚠️ **`getOrCreateResource()` runs its factory ON THE THREAD POOL.** Capturing the render
> scene by reference in a material factory crashed the process far from the cause. Textures are
> now resolved *before* the lambda; only `shared_ptr`s and plain values cross over.

**Environment (DomeLight)**: read from the stage — Tydra's `RenderLight` carries colour and
intensity but **not** the dome image path, so the `DomeLight` prim is read directly.
`SceneData` gained `LightType::Environment` + `textureAssetPath` for it. `CubemapResource`
already performs the equirectangular → cubemap projection (2:1 auto-detected) and
`FileFormatHDR` reads Radiance, so no conversion code was needed.

> ⚠️⚠️ **Two silent traps, both fixed, both worth knowing:**
> 1. The sky chain must be built **synchronously**. `applyBackgroundLighting()` MEASURES the
>    sky's texels; called while the cubemap still loads, the scene silently falls back to
>    `+DefaultTextureCubemap` — the log says so, nothing else does.
> 2. **`SkyBoxResource::load(material)` did not declare its IBL source.** Only the name-based
>    load paths set `m_environmentCubemap`, so a sky built from a material rendered correctly
>    while every surface was lit by the *default* environment. Fixed in the engine with
>    `SkyBoxResource::setEnvironmentCubemap()`.

> 🔶 **Two things still open.** The derived ambient comes out **black**
> (`Ambient light color : Color(0,0,0,1)`), so the IBL derivation yields no light even with the
> correct cubemap installed. And the full-stage run still dies on the **swap-chain 60 s
> acquisition timeout** — the same Wayland surface loss as § 4.2, now caused by the load plus
> the 8K HDR projection. Both need a session of their own; neither is in the loader.

### 4.5 PointInstancer — root cause found and fixed ✅ **VEGETATION ON SCREEN (2026-08-09)**

**Where the prim was actually lost.** Not in the crate reader, and not in composition. The
`PointInstancer` reaches the `Layer` perfectly — all six of them in `PI_Anthurium.usd`, each
with five properties and its prototype subtree — and is destroyed one step later, by
`LayerToStage`. Its PrimSpec→Prim table, in `composition-reconstruct.cc`, simply had **no entry
for `GeomPointInstancer`**, while `ReconstructPrim<GeomPointInstancer>` is fully implemented two
files away in `prim-reconstruct.cc`.

> ⚠️⚠️ **A prim that table cannot reconstruct is dropped WITH ITS ENTIRE SUBTREE.**
> `ReconstructPrimFromPrimSpecRec()` only recurses into children when the parent was
> reconstructed, so one missing table row deletes a whole branch of the scene.

Measured on `elements/Anthurium/PI_Anthurium.usd`, the smallest element:

| Stage | Layer | Stage before fix | Stage after fix |
|---|---|---|---|
| `/World` | 6 `PointInstancer`, 5 props each | **empty** | 6 `PointInstancer`, prototypes intact |

**It said so all along.** tinyusdz emitted
`TODO or unsupported prim type: PointInstancer`, six times, through the `warn` string of
`LayerToStage`. `USDLoader` never printed that string. **This is what cost a full session**, and
why the loader now traces the warning of *every* composition step — see § 4.6.

> ⚠️ Do NOT conclude "tinyusdz does not support PointInstancer" from grepping `usdc-reader.cc`:
> the crate reader is split across `usdc-reader-prim.cc`, `-property.cc` and `-reconstruct.cc`,
> and the support lives in the first. That mistake was made twice before the real cause showed up
> somewhere else entirely.

**The fix** (`patches/tinyusdz.patch`, fourth hunk): the missing table rows. `GeomPointInstancer`
plus the five UsdLux types in the same situation — `DomeLight_1`, `GeometryLight`, `PortalLight`,
`LightFilter`, `PluginLightFilter` — all implemented upstream, none listed. Only the first is
exercised by an asset we own; the other five are closed because their *absence* is what is known
to be harmful.

#### What the asset actually holds, measured element by element

Every `PI_*.usd` composes, prototypes included, with `LoaderOptions::resolveReferences`:

| Element | Instancers | Instances | Time | Peak RSS |
|---|---:|---:|---:|---:|
| `PI_S_RiverForest` | 195 | 2 407 967 | 605 s | 14.4 GB |
| `PI_S_RiverSeedling` | 80 | 2 266 462 | 9.2 s | 563 MB |
| `PI_S_Moss` | 138 | 2 034 610 | 1.1 s | 399 MB |
| `PI_S_ShrubSorrel` | 133 | 630 176 | 0.8 s | 150 MB |
| `PI_S_QueenForest` | 195 | 613 806 | 737 s | 18.0 GB |
| `PI_Grass_B` | 5 | 339 865 | 0.3 s | 74 MB |
| `PI_Grass_A` | 6 | 280 985 | 0.9 s | 68 MB |
| `PI_RiverSapling` | 5 | 45 000 | 16.6 s | 328 MB |
| `PI_Pyramid_GrassB` | 5 | 44 000 | 0.02 s | 16 MB |
| `PI_Shrub` | 4 | 11 337 | 0.6 s | 25 MB |
| `PI_Nettle` | 6 | 330 | 0.9 s | 27 MB |
| `PI_Anthurium` | 6 | 138 | 1.9 s | 46 MB |
| **Total** | **778** | **8 674 676** | **~23 min** | **18 GB (peak, one element)** |

The cost is **not** proportional to instance count: `RiverSeedling` delivers 2.2 M instances in
9 seconds, while `QueenForest` needs 12 minutes for 0.6 M. What it follows is the size of the
`*_classes.usda` prototype layer (236 MB and 196 MB for the two slow ones). The bottleneck is
**ASCII prototype parsing**, not instancing.

⚠️ `PI_Pyramid_GrassB` composes in 0.02 s and yields **0 mesh** — its reference does not resolve.
Its 44 000 instances therefore have nothing to draw. Not yet diagnosed.

#### The full stage sees the instancers but cannot draw them

On `JungleRuins_Karma.usda` with sublayers only (the default path), the loader now reports
**778 point instancers, 773 prototypes** where it previously reported zero — and 778 warnings
saying each prototype produced no mesh, because references are not resolved on that path. The
positions of the entire vegetation are available; the prototypes are not.

⚠️⚠️ **Do not "fix" this by turning `resolveReferences` on for the whole stage.**
`CompositeAllArcs()` on the root layer was measured at 24 minutes and 15 GB with **no
convergence** (§ 4.4). Per element it terminates, and it is bounded — which is why the
element-by-element route is the one that works.

**The design the data forces.** A 40 km² forest must never become one
`Multiple` holding every transform. The engine already owns the right structure:

- `Scenes/OctreeSector` culls ENTITIES by frustum. Splitting a PointInstancer's transforms into
  spatial buckets — one `Component::MultipleVisuals` per cell, each with its own bounding box —
  makes whole cells disappear from the batch without a single new culling system.
- `Component::MultipleVisuals` is exactly the shape a PointInstancer has: one prototype, N
  transforms. There is **no automatic grouping** in the engine — `RenderBatch` keys on
  `(renderableInstance, coordinates, subGeometry, LOD)` and nothing merges separate entities
  sharing a renderable. That is a feature here: the loader decides the grouping.

What `RenderableInstance::Multiple` already does, measured, so nobody rebuilds it: a per-instance
VBO carrying the model matrix, optionally the normal matrix, optionally the previous model matrix
(64–192 bytes/instance, feeding motion vectors), uploaded **only when dirty**, never per frame.

What it does not do, in the order it will hurt:
1. **No per-instance culling.** `setActiveInstanceCount()` truncates the FIRST N — a particle
   mechanism used only by `ParticlesEmitter`, not a spatial one.
2. **No per-instance LOD.** The level is chosen per draw call, so the tree at 2 m and the tree at
   800 m render at the same detail.
3. **Motion history paid on static content** — 64 bytes/instance that never change.

Beyond cell-level culling, the next step is GPU-driven: a compute pass culling and selecting LOD
per instance into a compacted list, behind `vkCmdDrawIndexedIndirect`. The engine already has
dormant MDI infrastructure (`Core/Graphics/MDI/Enabled`, default false, one known bug).

### 4.6 Translating an instancer — the three traps, all paid

**Tydra is no help here and never will be.** `PointInstancer` appears **zero times** in the whole
of `src/tydra/`. Instances are therefore read straight from the prims, by
`USDLoader::collectInstancers()`. Tydra does, however, *descend* into a prototype and convert its
meshes like any other, handing back `RenderMesh::abs_path` — which is exactly what ties an
instancer to the renderable its instances draw, without patching Tydra.

**Trap 1 — the axis bake does not apply to a transform.** Vertices go through
`C : engine = (usd.x, usd.z, -usd.y)`. An instance carries a *transform*, and a transform changes
basis by **conjugation**, `C·T·C⁻¹`, because the prototype's vertices are already baked:

```
C·(T·p) = (C·T·C⁻¹)·(C·p)
```

`C` is the rotation of +90° about X (determinant +1), so each part has a closed form and no
matrix is needed: the translation goes through `C`, the orientation quaternion is conjugated by
`C`'s own quaternion, and the scale — being sign-blind — simply has **Y and Z swapped**.
Permuting the position alone puts every plant in the right place with the wrong rotation, which
reads as a broken asset rather than a broken conversion.

**Trap 2 — `/_class_` is not scene content.** USD classes are abstract templates, and Tydra
converts them like anything else. On one Anthurium element that is 12 meshes out for 6 real
prototypes: drawing them stacks a full copy of every species at the asset's origin. The loader
skips any node whose `abs_path` starts with `/_class_`.

**Trap 3 — a prototype must NOT get a node.** It is drawn by its instances; giving it a node in
`SceneData::nodes` draws it one extra time, alone, wherever the asset parks its prototypes. It is
built as a resource, recorded in the path→mesh map, and deliberately left out of the hierarchy.

⚠️ A prototype root is assumed to sit at the identity — true of every Jungle Ruins element, since
Tydra only exposes absolute matrices. The assumption is **checked and logged**, never trusted
silently: a prototype carrying its own transform says so instead of shifting the forest.

### 4.7 Why the vegetation rendered WHITE — three defects behind one symptom

Instances placed correctly and still wrong on screen. Each cause looked like the previous one's
consequence, and none of them was where the symptom pointed.

**1. Instance cells were UNLIT — black silhouettes.** `buildInstanceClusters()` built its
`Component::MultipleVisuals` and stopped there. A renderable instance is born with
`EnableLighting` OFF; every one of `SceneDataConsumer`'s five Visual sites sets it, this one did
not. `InstanceClusterOptions::lightingEnabled` now carries `MeshDescriptor::lightingEnabled`
through — never hard-coded, because baked-lighting content must stay off the lit path.

**2. Texture paths differ by CASE.** The material asks for
`anthurium_botany_01_BaseColor.tif`; the file is `Anthurium_Botany_01_BaseColor.tif`. The normal
map, in the same material, is spelled correctly. On Windows or macOS nothing shows; on Linux the
base colour vanishes and the normal map does not — so the plants render lit, detailed and pure
WHITE. `USDLoader::findCaseInsensitive()` retries a failed path against one directory listing and
logs loudly, because the asset's spelling is genuinely wrong.

**3. A broken image resource takes the MATERIAL down with it.** Once found, the TIFF still failed
to decode, `ImageResource` errored, the texture built on it failed, and the plants stopped
rendering ENTIRELY — strictly worse than never finding the file, which merely falls back to a
flat colour. The loader now checks the extension BEFORE handing anything to the resource manager.

> ⚠️⚠️ Fixing (2) without (3) is a REGRESSION: the file starts being found, and the object
> disappears. That is exactly what happened, and it is the shape to expect whenever a fallback
> path is more forgiving than the real one.

### 4.8 TIFF — the format the whole vegetation is written in

All 10 TIFF files of the asset sit under `textures/Plants/`: the **base colour and translucency of
seven species** (Anthurium, Grass_Medium ×2, Moss, Nettle, Shrub_04, Shrub_Sorrel), 4096×4096,
**16 bits per channel**, uncompressed, ~98 MB each. `PixelFactory` read JPEG, PNG, Targa and HDR.

So no amount of instancing work could ever have produced a coloured plant.

**Owner decision (2026-08-09): libtiff, through `ext-deps-generator`**, the same path as libpng
and libjpeg — a targeted exception to the *"Ave robustus!"* feature freeze on emeraude-base.
Added: `libraries/libtiff.yaml` (pinned to the **v4.7.2 release**, no RC), `SetupTIFF.cmake`, and
`PixelFactory::FileFormatTIFF`.

⚠️ **WebP and JBIG codecs are OFF in the build.** libtiff enables them when it finds the
libraries, and the engine then fails to link on `WebPGetFeaturesInternal` and `jbg_dec_in`.
Deflate, LZMA, Zstd and JPEG ride on libraries the cascade already links, so they stay on.

⚠️ Decoding goes through `TIFFReadRGBAImageOriented()` rather than the strip/tile API: TIFF is a
container, not a format, and that entry point collapses every variant to 8-bit RGBA. **A 16-bit
source is down-converted** — acceptable for a texture, and a separate reader's job if full
precision is ever needed. `ORIENTATION_TOPLEFT` is what keeps the output canonical; the plain
`TIFFReadRGBAImage()` returns the image bottom-up, which renders vertically mirrored with nothing
in the log to say so.

**Verified on screen (2026-08-09)**, `--load-demo jungle-ruins --demo-options 2`: 6 instancers,
138 instances, 6 sets, 42 cells of 32 units; 12 source meshes for 6 built (the 6 `/_class_`
duplicates correctly dropped). Plants upright, each with its own rotation and scale.

## 5. Contract Changes

### 5.1 Rename — `AssetLoaders` becomes `Loaders` ✅ **DONE (2026-08-08)**

The layer loads *scenes*; the format decides whether a given file yields a single model or a
complete scene. It lives engine-side, not in emeraude-base, precisely because emeraude-base
only knows raw geometry formats.

| Current | New |
|---------|-----|
| `EmEn::AssetLoaders` / `src/AssetLoaders/` | `EmEn::Scenes::Loaders` / `src/Scenes/Loaders/` |
| `AssetLoaders::Interface` | `Scenes::Loaders::Interface` |
| `AssetData` | `SceneData` |
| `Scenes::AssetDataConsumer` | `Scenes::SceneDataConsumer` |

Done **before** USD lands, as an isolated mechanical pass, so the name never lies in the
interval.

### 5.2 `Scenes::Loaders::Interface` — unification ✅ **DONE (2026-08-08)**

The interface is the single point every format conforms to. USD conforms like the others.
Beyond the rename it gains a **capability declaration**: a caller must be able to ask what a
format claims to deliver (geometry only? lights? cameras? instancers?) *before* loading, so a
consumer can decide how to treat the result without hard-coding format names.

### 5.3 `SceneData` — extension ✅ **lights + cameras DONE (2026-08-08)**

> **Frozen with two known loose ends** (owner decision to jump to milestone 3 early). Neither
> affects correctness of the shipped contract; both are cheap to close when someone passes by:
> 1. **`PhotometricProbe.gltf` has an inverted winding order** — its quad's front face ends up
>    pointing down after the Y-up → Y-down conversion, so it is only visible from below. The
>    loader is not at fault; the generator that authored the asset is. Reverse the index order.
> 2. **The per-light / per-camera values were never read back.** They are traced with
>    `TraceDebug`, which is `#ifdef DEBUG` — compiled out of Release, where the check was run.
>    Promote those two traces to `TraceInfo` (they are a bounded load inventory, not debug
>    noise — and they would have revealed Sponza's all-zero intensities immediately), or repeat
>    the run on a Debug build.
>
> What IS verified end to end on Release: the inventory line (`3 lights, 1 cameras`), the
> component types created (`PointLight` / `SpotLight` / `DirectionalLight`), and the poses after
> axis conversion (glTF `(0, 3, 0)` → engine `(0, -3, 0)`).

> **Instancing landed later, on purpose.** It was deliberately kept out of the lights/cameras
> delivery — designing an instancer descriptor against a format not yet parsed would have
> produced a structure to redo. It was designed on 2026-08-09 against real data, once the prims
> were actually reachable. See `InstanceSetDescriptor` below. The glTF path also revealed that
> Sponza's 24 lights all carry `intensity: 0.0`, which is why the hand-authored
> `PhotometricProbe` asset exists (§ 7.2, milestone 2).

#### `InstanceSetDescriptor` ✅ **DONE (2026-08-09)**

```cpp
struct InstanceSetDescriptor
{
    std::string name;
    std::vector< Base::Math::CartesianFrame< float > > instances;
    size_t meshIndex{0};
};
```

Carried by `SceneData::instanceSets`. It states the INTENT — *the same renderable, N times,
here* — and never the encoding, so USD's `PointInstancer`, glTF's `EXT_mesh_gpu_instancing` and
folded FBX duplicates all reach a consumer through one path.

> ⚠️ **A set is a hint about redundancy, not an instruction to draw.** How the instances reach
> the GPU belongs to the consumer, because only the scene knows its own culling machinery. A
> loader deciding that would be re-implementing the renderer.

> ⚠️ **The mesh a set points at must NOT appear in the node hierarchy.** A prototype exists to be
> instanced; a node for it draws one stray copy at the asset's origin.

> ⚠️ Frames live in the SAME space as the meshes of the same `SceneData`. A loader baking its own
> axis conversion into vertices MUST bake the very same one into these frames — see § 4.6,
> trap 1, which is a conjugation and not a permutation.

`Scenes::SceneDataConsumer` turns each set into spatial cells through
`buildInstanceClusters()`, applying the asset's root frame exactly as it does to a mesh node.
Cell size via `setInstanceCellSize()` (default 32 units).

> ⚠️⚠️ `SceneDataConsumer::build()` used to return early on an empty node table. An asset made
> **entirely** of instances — every `PI_*.usd` element of Jungle Ruins — has no drawable node at
> all, and would have been dropped while reporting success. The guard now checks both.

`SceneData` currently carries meshes, skeletons, animation clips and a flat node hierarchy.
It gains, as optional collections mirroring the existing `meshIndex` pattern:

- `lights[]` — punctual and dome/environment lights;
- `cameras[]` — with physical parameters;
- `pointInstancers[]` — a prototype set plus per-instance transforms;
- corresponding optional indices on `NodeDescriptor`.

> This is **not a USD tax**. glTF carries `KHR_lights_punctual` lights and cameras that the
> engine currently **discards at parse time**. The extension is validated against glTF first
> (milestone 2), on a format whose behaviour is already known, before USD depends on it.

### 5.4 `LoaderOptions` — variant selection

Variants are **load-time options** (owner decision). The option set gains a variant selection
expressed by prim path — `/Environment/Pyramid` → `state` = `ruined`. **Empty by default**,
in which case the selection authored in the asset is honoured.

There is deliberately **no runtime variant switching**. Should it ever be wanted, the correct
form under the absorption rule is a native *switchable alternative branch* capability in
`Scenes`, with USD as one producer among several — not a USD document kept alive.

## 6. Engine Gaps to Fill

Each row is native scene work, useful beyond USD.

| USD concept | Native target | Gap |
|-------------|---------------|-----|
| `subLayers`, composition | Flattened `Node` hierarchy | — resolved at load |
| `PointInstancer` + `class` prototypes | `RenderableInstance::Multiple` + `SceneInstanceTransforms` | **Per-instance culling** |
| `UsdPreviewSurface` | PBR metal-rough material | Direct mapping; cutout and two-sided already exist |
| `UsdUVTexture` (8K, 3.5 GB) | `ImageResource` | **BC7 transcoding + mip generation with a disk cache** |
| `DomeLight` + equirect HDR | Photometric sky contract | **Equirectangular → cubemap conversion** |
| `Camera` | Physical camera | Photometric mapping |
| `TimeSamples` on transforms | `AnimatableInterface` + `KeyFrame`/`Sequence` | — mechanism exists |
| `upAxis=Z`, `metersPerUnit` | Engine **Y-DOWN** convention (UP = -Y) | Baked at import, **normals and tangents included** |
| Lights / cameras / instancers | — | `SceneData` extension (§ 5.3) |

### 6.1 Per-instance culling

`RenderableInstance::Multiple` supports LOD levels (`bindInstanceModelLayer(…, LODLevel)`) and
can truncate the drawn count (`m_activeInstanceCount`), but there is **no per-instance frustum
culling** — only "draw the first N". For vegetation spread across a terrain, the entire jungle
is submitted every frame regardless of framing.

> **Phase two, not a prerequisite** (§ 7.1). Submitting everything is accepted for the first
> delivery. When it is addressed, it is addressed **in the engine** per the co-development
> rule, and only once measurement on the rendered scene justifies the design chosen.

### 6.2 BC7 transcoding and mips

`bc7enc` is already an engine dependency. **No mip generation utility exists** anywhere in the
cascade — a known gap, previously deferred by the `AnimatedTexture2D` work, that this project
must close. Strategy: transcode on first load, cache to disk, read the cache thereafter.

### 6.3 Axis and unit baking

Positions, normals, tangents and transforms converted once at import, so physics, picking, the
octree, the editor and ray tracing all see one coherent convention.

> **Classic failure mode:** converting positions and forgetting normals and tangents. Lighting
> then looks subtly wrong in a way that is easy to blame on the material system. Every
> vector-valued field must be enumerated explicitly.

## 7. Milestones

### 7.1 Definition of done for the first delivery

> **Owner directive.** The first delivery succeeds when the scene is **on screen**. Frame rate
> is explicitly **not** a criterion — *"even at 0.5 FPS, I don't care; displaying it is a
> success in itself."* Performance work is a second phase, driven by measurement on the scene
> once it renders.

This reorders the work, and one distinction must not be blurred:

- **Per-instance culling is no longer a prerequisite.** The whole jungle is submitted every
  frame; that is accepted. It moves to phase two.
- **Texture residency remains a prerequisite** — not for speed, but for existence. 3.5 GB of
  decoded textures do not fit in 8 GB of VRAM: without BC7 or a resolution cap there is no
  image at all, at any frame rate.

### 7.2 Phase one — get it on screen

Each milestone compiles and is verifiable on its own.

1. **Rename** — `Loaders` / `SceneData` / `SceneDataConsumer`, mechanical pass, no
   behaviour change.
2. **Contract extension** — lights, cameras, instancers in `SceneData`; capability declaration
   on `Interface`; **wired to glTF first** to validate against a known format.
3. **tinyusdz integration** ✅ **DONE on Linux (2026-08-08)** — `ext-deps-generator`,
   `Setup*.cmake`, stage inventory dump (§ 3.5) as the first functional output.
   Windows and macOS builds are untried. See § 4.1 for what this cost.
4. **Geometry and materials** ✅ **renders (2026-08-09)** — meshes,
   `UsdPreviewSurface` → PBR, cutout, two-sided, axis and unit baking. See § 4.3.
5. **Texture residency** — BC7 transcoding, mip generation, disk cache; enough for the scene
   to fit in VRAM.
6. **PointInstancer** ✅ **DONE (2026-08-09)** — root cause of the missing prims found and
   fixed upstream (§ 4.5), `InstanceSetDescriptor` contract (§ 5.3), translation into
   spatial cells (§ 4.6). Vegetation verified on screen on one element. **No culling** beyond
   what the octree already does per cell. REMAINS: the full stage needs every element
   composed separately (~23 min, 8.67 M instances), and `PI_Pyramid_GrassB` resolves to 0 mesh.
7. **Sky and camera** — equirect → cubemap, photometric mapping of `DomeLight` and `Camera`.
8. **`JungleRuins` demo** in projet-alpha, with its `--demo-options` and CC-BY attribution.

**→ Phase one ends the moment the scene is visible, whatever the frame rate.**

### 7.3 Phase two — make it fast

Nothing here starts before phase one renders and has been **measured** (§ 8.3): per-instance
culling, automatic geometry LOD, texture streaming, and whatever else the numbers actually
demand. No blind optimisation.

## 8. JungleRuins — the Engine's Benchmark Scene, and its Holy Grail

> [!IMPORTANT]
> **This scene is the GOLD GOAL** — the owner's own words, *"le Saint Graal"*.
>
> Rendering Intel's Jungle Ruins at a quality that stands next to the reference path-traced
> renders is **the** target the runtime is measured against. Not a test case among others, not
> a loader demo: the single scene whose completion says the engine has arrived.
>
> Everything the engine still lacks, this scene names out loud — massive instancing, automatic
> geometry LOD, texture streaming, alpha-cutout foliage under ray tracing, an environment
> dome carrying the whole lighting. Each of them is a chapter, and the Grail is reached when
> none of them is missing.

> **Owner directive.** This scene **must load** — that is not negotiable, and a failure to
> load is an engine defect to be fixed in the engine, never an asset to be rejected or
> pre-processed away. It is expected to *hurt*. Its role is not to prove the loader works: it
> becomes the engine's **definitive benchmark**, the scene against which rendering quality and
> performance are improved **statistically**, over time.

### 8.1 Why this scene is hard

The asset was authored for offline rendering, not real time. The root layer is named
`JungleRuins_Karma.usda` and the `DomeLight` carries `HoudiniViewportGuideAPI` — this is a
**Houdini/Karma path-tracing scene**. Uncompressed 8K normal maps, a 313 MB single mesh and
3.5 GB of textures are offline budgets; a game-ready equivalent would be an order of magnitude
smaller.

It therefore stresses, simultaneously, every axis the engine is weakest on:

- **massive instancing** — the entire scene is `PointInstancer` vegetation;
- **alpha-cutout foliage** — already measured on this project as roughly **doubling ray-traced
  cost, even at half resolution**;
- **texture residency** far beyond VRAM;
- **environment lighting only** — one `DomeLight`, so IBL and GI carry the whole image.

### 8.2 Capability gaps this scene exposes

Beyond the gaps listed in § 6, comparison with what a mature real-time engine brings to an
asset of this class identifies two **larger, previously unplanned** gaps:

| Capability | What it would solve here | Engine state |
|------------|--------------------------|--------------|
| Automatic geometry LOD | The 313 MB banyan costs nothing when off-screen or distant | **WIP** |
| Texture streaming / virtual texturing | Only visible texels resident, instead of 3.5 GB | **Absent** |

Neither is a prerequisite for the milestones in § 7 — the scene must first load and render at
*some* frame rate before either can be justified by measurement. They are recorded here so the
next session does not rediscover them, and so no one concludes from a poor frame rate that the
loader is at fault.

### 8.3 Benchmark protocol

For measurements to be comparable across sessions, they must be reproducible:

- **Canonical viewpoints.** The USD `Camera` gives one authored viewpoint; a small fixed set
  must be defined (pyramid wide shot, dense foliage close-up, terrain vista) and used
  unchanged. Framing is set through the console, never by hand.
- **Per-pass attribution** via `Core.RendererService.getGPUTimings()` — never a single frame
  time, which attributes nothing.
- **A/B by runtime toggle**, one term at a time, removing the effect first to establish the
  floor before attributing any cost to it.

> **Two traps already paid for on this project.** `getStatus()` reports a polluted average —
> read the *instantaneous* frame rate. And `PixelDoubling` persisted in the user's
> `settings.json` silently produced a 10 FPS Sponza and a completely wrong attribution table:
> **verify the effective render settings before recording any baseline.**

## 9. Verification

Per the project rule, in order: (1) projet-alpha compiles across the whole cascade,
(2) the emeraude-base unit suite passes. Visual verification through the remote console
(screenshot, `getGPUTimings()`), never by assumption.

> **Measurement discipline.** Two traps documented in `caution-points.md` apply directly here:
> a `lookAt` target with a **more negative Y looks UP**, and a conclusion drawn from pixels
> without confirming the framing has already produced a bogus result on this project. Confirm
> the surface under measurement fills the crop before concluding anything.

## 10. Open Points

- Actual composition arcs and instance counts in the crate files — answered by milestone 3.
- Whether the 4800-frame animation drives anything besides the camera.
- Whether the terrain's manually split texture grid needs a dedicated material path.
- tinyusdz `-fno-exceptions` behaviour in practice (§ 4).
- Whether automatic geometry LOD and texture streaming (§ 8.2) become projects of their own
  once the scene renders and has been measured.
---

## 11. USDZ Archives — World Lobby (2026-08-10, IN PROGRESS)

Second reference asset, complementary to JungleRuins: where JungleRuins is a stage spread over a
**directory tree**, `WorldLobby.usdz` is a **single 1.6 GB file** — an NVIDIA Omniverse Kit export
holding 353 entries, 38 USD layers and 285 images. Demo: `projet-alpha --load-demo world-lobby`.

> [!WARNING]
> **STATUS: THE SCENE IS LIT — CONFIRMED BY MEASUREMENT (2026-08-11). The open question is now
> EXPOSURE, not lighting.** Three causes were found and fixed; the third (§ 11.3, every fixture
> culled on a null radius) is what held the frame black.
>
> | Measure | Before (2026-08-10) | After (2026-08-11) |
> |---|---|---|
> | Pixels at exactly zero | **97.9 %** | **8.5 %** |
> | Floor, display-linear | ≈ 0.0012 | **0.301** (target 0.245) |
> | Frame clipped at 1.0 | — | **18.8 %** (was 41.5 % before the triad was re-derived) |
>
> The exposure was re-derived from the MEASURED room illuminance and now reads
> **`f/4 · 1/50 s · ISO 160`** — § 11.6. A night sky is installed as **scenery only**, with the
> diffuse-IBL separation verified by measurement.
>
> **The live open point is now § 11.5 item 1: half the frame sits at or above 0.9 sRGB with the
> floor correct**, which points at baked lighting being counted twice rather than at the exposure.

### 11.1 The archive is MAPPED, never extracted

`USDZArchive` (defined in `USDLoader.cpp`) memory-maps the file and reads the asset table with
`assetOnMemory = true`, so the table holds **pointers into the mapping** instead of copying the
archive. Layers are parsed and images decoded in place — a 1.6 GB archive costs its table (21 MB),
not its bytes. The archive is held through a `shared_ptr` because image factories run on the
**thread pool** and outlive `load()`.

Two tinyusdz gaps make this necessary, and both are quiet:

- `SetupUSDZAssetResolution()` registers handlers for **images only** (its own TODO says
  `[ ] USD: usda, usdc, usd`), so a nested `.usd` layer is never read out of the archive.
- `USDZResolveAsset()` compares the table **verbatim**, never joining the asking layer's directory.

Hence three handlers of our own, registered for every extension plus the `*` wildcard.

> [!CAUTION]
> **Asset resolution matches a PATH SUFFIX, never a basename, and refuses an ambiguous one.** A Kit
> export bakes materials into `Materials/Bake/baked_textures_<hash>/` and names every file inside
> identically: **285 images under 119 distinct filenames**, with `mtl-base_color.jpg` appearing
> **sixty times**. A basename match resolves all sixty to whichever entry came first — sixty
> materials wearing one skin, no error raised, a scene that merely looks "wrong". When even the
> suffix is ambiguous the answer is **refused**, so the material keeps its flat colour.

> [!CAUTION]
> **Tydra carries its OWN resolver, and it is not the one composition used.** Left as a filesystem
> resolver on an archive, a texture is not merely unresolved: Tydra drops the whole `UsdUVTexture`,
> `renderScene.textures` comes back **empty**, and the materials look like the asset never had any.
> `env.asset_resolver = resolver` is load-bearing.

### 11.2 Composition is PAYLOAD-driven here, not sublayer-driven

The root layer's whole body is two `prepend payload` arcs. Sublayer composition alone resolves
neither and returns **ten prims while reporting success** — which is what demo option `0` shows on
purpose. `resolveReferences` is therefore mandatory, and this is the **opposite trade-off from
JungleRuins**: a USDZ is bounded by its own file, so arc resolution costs **1.75 s and 2.6 GB peak**
for the whole stage, against 24 minutes and 15 GB there.

Composed result: 2806 prims, depth 9, **942 meshes, 155 materials**, 5 cameras, 25 DiskLight,
4 SphereLight, 1 DomeLight. `upAxis Z`, `metersPerUnit 0.01`.

### 11.3 Punctual lights — `buildLights()`

**The photometric anchor** (owner decision, 2026-08-10) reads `inputs:intensity` as a luminance in
cd/m², multiplies by the emitter's **area** — which is what `normalize = false` means — and
normalizes by 4π:

```
candela = intensity * 2^exposure * area / (4 * pi)
```

Verified in the running light set: the 25 ceiling DiskLights (intensity 60000, radius 0.5 m after
`metersPerUnit`, 4 m above the floor) come out at **3750 cd**, i.e. 234 lux at floor level — the
real range of a building lobby (200-500 lux). The two rejected readings gave 2945 and 3750 **lux**,
outdoor levels.

> [!NOTE]
> **The 4 SphereLights come out at 0.0032 cd, and that is a faithful reading, not a bug.** Their
> authored radius is 0.5 stage unit = **5 mm**, and the area enters the conversion. A 5 mm emitter
> is physically dim. Given their colour (1.0, 0.60, 0.30 ≈ 2900 K, matching the asset's own
> `Light_2900K` material) the author clearly meant something else — an open point, not a defect.
> Owner decision 2026-08-10: recorded, left alone; 25 disks at 3750 cd carry the room.

> [!CAUTION]
> **⚠️⚠️ TYDRA NEVER PLACES A LIGHT. `render-light-converter.cc` writes seventeen fields — colour,
> intensity, exposure, temperature, cone, shadows — and contains ZERO occurrences of `transform`,
> `position` and `direction`.** Those members keep the struct defaults: position `(0,0,0)`,
> direction `(0,-1,0)`.
>
> Cost, measured: 29 fixtures read at their correct 3750 cd, **all stacked on the world origin**
> twenty metres from the room they belong to, aiming horizontally once baked. **Pure black frame**,
> a light set truthfully reporting 25 spots and 4 points, and **not one warning anywhere** — the
> loader had faithfully read fields nobody wrote.
>
> The placement now comes from `collectLightPlacements()`, which walks the tree built by
> `tinyusdz::tydra::BuildXformNodeFromStage()` (the library's own equivalent of pxrUSD's
> `GetLocalToWorldMatrix`). Joined on **`abs_path`**, which the converter DOES fill and which is
> unique by construction — **the element name is NOT usable, all 25 ceiling fixtures of this asset
> are named `LightBloomDisc`**. A light with no entry is **dropped and reported**, never silently
> placed at the origin.

Two conventions carried by that walk, each of which fails silently if got wrong:

- **USD is a ROW-VECTOR convention**: translation in the matrix's **last row** (`m[3][0..2]`), basis
  vectors as **rows**. Reading it column-major transposes the rotation — it does not fail, it aims
  every light somewhere plausible and wrong.
- **A UsdLux light emits along its LOCAL -Z.** Not -Y, and not the stage's up axis — this stage is
  Z-up, which makes the two easy to confuse.

Verified in the scene graph dump: spots land at `Y ≈ -3.8 … -4.5` (≈ 4 m above the floor, `UP = -Y`)
with direction `(0, 1, 0)`, i.e. straight down. 29/29 placed, 0 dropped.

> [!CAUTION]
> **⚠️⚠️ USD DECLARES NO RANGE ON ANY LIGHT TYPE, AND A RANGELESS LIGHT USED TO BE CULLED FROM EVERY
> DRAW.** This was the third cause of the black frame, and the one that survived the two fixes above.
> `SceneDataConsumer::attachLight()` called `setRadius()` only when the descriptor carried a range,
> so all 29 fixtures kept `AbstractLightEmitter::DefaultRadius` — **zero** — and a zero radius made
> `touch()` build an invalid sphere that `isColliding()` refuses outright. Correctly placed,
> correctly valued, enabled, listed in the light set, and **bound to not one draw call**.
>
> The consumer now derives a culling bound from the photometry when the asset declares none —
> `Graphics::Photometry::cullingRadiusFromIntensity()`, `r = sqrt(I / E)` at 1 lux, giving **~61 m**
> for a 3751 cd ceiling disk — and the engine now reads a null radius as **unbounded** on the CPU
> too, matching the shader. Full account, both halves and why neither is optional:
> [`caution-points.md` § "a radius of ZERO meant …"](caution-points.md).
>
> ⚠️ **What isolated it was a known-good control, not a code read**: the player's flashlight lit the
> same scene perfectly, and its only relevant difference was an explicit `setRadius(30.0F)`. When
> asset lights fail while an engine-made light succeeds in the same frame, **compare the setters**.

### 11.4 Opacity — six materials decide whether the building has windows

A `UsdPreviewSurface` glass pane declares `inputs:diffuseColor = (0,0,0)` with `inputs:opacity = 0`
— black **because** it is meant to be seen through. Read as opaque, a whole curtain wall renders as
a solid **black block**, and the failure reads as a lighting or material bug rather than a missing
input. Exactly 6 of the 141 materials carry opacity 0 (`Glass` ×4, `Clear_Glass`, `Tinted_Glass`),
each with a real `ior` (1.20 to 1.52).

`opacityThreshold` is read **first**: a non-zero threshold is a **cutout** — an alpha test that
keeps the material opaque — never blending. No material of this asset exercises it (all 141 report
0); the branch exists so the next asset does not silently lose its cutouts.

Not read yet, deliberately: **`ior`**. Refraction and its Fresnel belong to the transmission path
(`setTransmissionComponent()`), a separate calibration.

### 11.5 Still open on this asset

1. **⚠️⚠️ SUSPECTED DOUBLE-COUNTED BAKED LIGHTING — half the frame sits at or above 0.9 sRGB.**
   Measured at EV100 8.97 (clipping ceiling 600 cd/m²): the floor reads 194 cd/m² and is correct,
   but the right-hand wall reads **≥ 570** and the far end **≥ 555** — about **3× the floor**. Downward
   spots cannot do that to a VERTICAL surface: a wall under a downlight gets grazing light and must
   be DIMMER than the floor, not three times brighter. 49.8 % of the frame is above 0.9 sRGB and
   18.8 % clips outright.

   The hypothesis, untested: a Kit export commonly bakes lighting into the base colour, and
   re-lighting such a surface counts the term twice — the documented rule is that content carrying
   its own baked lighting must be declared **EMITTING, never lit**. The symptom matches exactly:
   bright surfaces take off while the floor stays correct. **The check needs no run** — sample those
   materials' base-colour textures and see whether they carry shading gradients rather than a flat
   material colour.

   ⚠️ This is NOT an exposure problem. The floor is right, so the triad must not move; what would
   collapse if the hypothesis holds is the clipping figure, which is the scene's real health metric.
2. **THE FLOOR ALBEDO HAS NEVER BEEN MEASURED** — 0.7 is an assumption, and it is one of the two
   unpinned terms in the photometric chain (§ 11.6). Closing it means sampling the floor's baked
   base-colour texture; 0.86 would account for the whole `+0.30 EV` residual on its own.
3. **85 × `Unsafe asset path: ../../Materials/Bake/…`** — the patch fixed the **composition** path
   (`ValidateAndNormalizeRelativeAssetPath`) but **not Tydra's image loader**, which still calls the
   strict validator. Identical count across every run to date.
4. **28 meshes have no `st` UV set** (`ConvertMesh: Failed to get texture coordinate`).
5. **The DomeLight carries no image** (`intensity 1000, image '<none>'`), so there is nothing to
   install as an environment.
6. **USD cameras are not translated at all** — `SceneData` carries no camera. The demo's viewpoint
   is placed by hand.

### 11.6 The measured floor — and why the DISAGREEMENT points at the prediction, not the anchor

First frame ever measured with the room actually lit (2026-08-11, 2880×1620, flashlight off —
verified two ways, see below):

| Quantity | Value |
|---|---|
| Unclipped foreground floor patch | **0.631 display-linear** |
| Predicted (§ 11.3 anchor + `f/2.8 · 1/50 · ISO 200`) | 0.222 |
| Ratio | **2.87× — `+1.52 EV`** |
| Frame clipped at 1.0 | **41.5 %** |

Back-computed from the measurement: `L = 0.631 × 1.2 × 2^7.61 ≈ 148 cd/m²`, so with the assumed 0.7
albedo the floor receives **≈ 664 lux**.

> [!IMPORTANT]
> **The 234 lux figure was ONE fixture at nadir; the floor is lit by TWENTY-FIVE.** That is almost
> certainly the whole discrepancy. A ceiling grid of 25 disks at 4 m, each 3751 cd, delivers several
> times the single-nadir value at any given floor point once the off-axis contributions are summed —
> and 2.87× is a modest, physically ordinary figure for such a grid. **664 lux is also a credible
> building lobby** (200-500 lux nominal, a bright entrance hall runs higher).
>
> So the reading is: **the anchor in `buildLights()` is probably sound, and the DERIVATION of the
> exposure triad used a per-fixture illuminance where it needed a whole-room one.** Correcting that
> input is not "tuning the triad by eye" — the prohibition stands against eyeballing, not against
> fixing a wrong input to the derivation.

**Proven by the floor profile, without another run.** Sampling the floor across the frame gives
0.617 – 0.654 over the whole left half — **flat to within 3 %**, rising monotonically only towards
the bright right-hand wall, with **no periodic bright pools anywhere**. Twenty-five narrow
hard-edged cones would print twenty-five crisp discs and a floor alternating light and dark. They do
not. The overlap is total, so the summing explanation holds and the anchor is exonerated.

**Re-derived and re-measured (owner decision 2026-08-11).** The triad became **`f/4 · 1/50 s ·
ISO 160`** (EV100 8.97), taking the correction on the aperture and the sensitivity and leaving the
shutter alone so the 180-degree motion blur is untouched — f/4 is also the honest register for an
architectural interior, where f/2.8 was a portrait depth of field. Result, five unclipped floor
patches at the demo's default viewpoint:

| | Before | After |
|---|---|---|
| Floor, display-linear | 0.631 | **0.301** (median; 0.288 – 0.323 over five patches) |
| Frame clipped at 1.0 | 41.5 % | **18.8 %** |

The floor now reads as grey stone with its tile joints and texture; the fixtures still clip, which is
correct for a luminaire seen directly.

⚠️ **The residual is `+0.30 EV` above the 0.245 target, and that is inside the method's own error
bars — do not chase it.** The 0.245 came from back-computing 664 lux out of a SINGLE patch in a
capture taken at a different viewpoint, and the room is not uniform. Two candidates account for it
entirely: the 0.7 albedo assumption (0.86 would close it exactly) and the choice of measurement
spot. **Closing it properly means sampling the floor's baked base-colour texture** — the albedo has
never been measured, and it cancels out of neither the prediction nor the back-computation.

⚠️ **Two captures from different viewpoints cannot be divided.** The ratio between the before and
after frames is 2.09 while the triad moved by 1.35 EV (2.55×) — the difference is the framing, not
the sensor. Compare exposures on ABSOLUTE values against the target, or hold the viewpoint fixed
(`Act.getOrientation()` prints a replayable `setPosition` / `lookAt` pair for exactly this).

⚠️ **41.5 % of the frame clips**, mostly the right-hand wall and the fixture bodies themselves, so
**the frame mean is a floor, not a value**. Any exposure work here must be measured on unclipped
patches only.

⚠️ **How the flashlight was ruled out**, since it is the project's known-good control and would have
invalidated the whole measurement: (1) the floor chroma is **neutral everywhere** (R/G 0.99-1.00,
B/G 0.98-1.00) while the flashlight is `LightYellow` and carries a projection texture; (2) floor
luminance **rises** with distance from the player (0.631 → 0.646 → 0.776) instead of decaying by
inverse square from the eye. Owner confirmed it was off.
