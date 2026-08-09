# Scene Loaders & OpenUSD Integration — Design Document

> Status: **design approved, implementation not started**.
> Owner decisions recorded below are binding. Any deviation requires a new decision.
>
> Related: [`scene-graph-architecture.md`](scene-graph-architecture.md),
> [`renderable-instance-system.md`](renderable-instance-system.md),
> [`resource-management.md`](resource-management.md),
> [`coordinate-system.md`](coordinate-system.md),
> [`../src/AssetLoaders/AGENTS.md`](../src/AssetLoaders/AGENTS.md).

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

**Three upstream defects**, all in the v0.9.4 release tag:

| Defect | Effect | Handling |
|--------|--------|----------|
| `LoadUSDFromFile()` composes nothing | A 19-sublayer stage returns **2 prims** and reports success | Use the explicit pipeline: `LoadLayerFromFile` → `CompositeSublayers` → `LayerToStage` |
| The asset resolver's state is never restored after recursion (upstream leaves a `TODO` right above the mutation) | After descending into one sublayer, every following **sibling** resolves against the child's directory; composition aborts | RAII save/restore guard, `patches/tinyusdz.patch` |
| `CompositeSublayersInPlace()` is declared in the header but never implemented | Undefined reference at link time | Use the plain `CompositeSublayers()`; `LayerToStageInPlace()` IS implemented and is used |

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

### 4.5 PointInstancer — measured blocker, and the instancing design it forces

**The vegetation is unreachable today.** Every `PI_*.usd` file is a USDC crate, and the
`PointInstancer` prim is silently absent from the composed stage. Measured on
`elements/Anthurium/PI_Anthurium.usd`, the smallest of them:

| Composition | Prims | `/World` (holds the instancer) | `/_class_` (holds the prototypes) |
|---|---|---|---|
| sublayers only | 8 | **empty** | 6 `Model` stubs |
| + references resolved | 56 | **empty** | 6 prototypes, meshes and materials filled |

So composition is not the culprit: the prim is missing from the very first read. tinyusdz
**does** register a `GeomPointInstancer` handler on the crate path
(`usdc-reader-prim.cc:195` and `:553`), and no warning is raised. This needs a focused session
against the library's own debug output — not another guess.

> ⚠️ Do NOT conclude "tinyusdz does not support PointInstancer" from grepping `usdc-reader.cc`:
> the crate reader is split across `usdc-reader-prim.cc`, `-property.cc` and `-reconstruct.cc`,
> and the support lives in the first. That mistake was made twice in one session.

**The design this forces, once the data is reachable.** A 40 km² forest must never become one
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

## 5. Contract Changes

### 5.1 Rename — `AssetLoaders` becomes `SceneLoaders` ✅ **DONE (2026-08-08)**

The layer loads *scenes*; the format decides whether a given file yields a single model or a
complete scene. It lives engine-side, not in emeraude-base, precisely because emeraude-base
only knows raw geometry formats.

| Current | New |
|---------|-----|
| `EmEn::AssetLoaders` / `src/AssetLoaders/` | `EmEn::SceneLoaders` / `src/SceneLoaders/` |
| `AssetLoaders::Interface` | `SceneLoaders::Interface` |
| `AssetData` | `SceneData` |
| `Scenes::AssetDataConsumer` | `Scenes::SceneDataConsumer` |

Done **before** USD lands, as an isolated mechanical pass, so the name never lies in the
interval.

### 5.2 `SceneLoaders::Interface` — unification ✅ **DONE (2026-08-08)**

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

> **Scoping note.** `pointInstancers[]` is deliberately **NOT** part of this delivery. No loader
> can fill it yet, and designing an instancer descriptor against a format not yet parsed would
> produce a structure to redo at milestone 6 — the placeholder § Rule 1 forbids. The glTF path
> also revealed that Sponza's 24 lights all carry `intensity: 0.0`, which is why the
> hand-authored `PhotometricProbe` asset exists (§ 7.2, milestone 2).

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

1. **Rename** — `SceneLoaders` / `SceneData` / `SceneDataConsumer`, mechanical pass, no
   behaviour change.
2. **Contract extension** — lights, cameras, instancers in `SceneData`; capability declaration
   on `Interface`; **wired to glTF first** to validate against a known format.
3. **tinyusdz integration** ✅ **DONE on Linux (2026-08-08)** — `ext-deps-generator`,
   `Setup*.cmake`, stage inventory dump (§ 3.5) as the first functional output.
   Windows and macOS builds are untried. See § 4.1 for what this cost.
4. **Geometry and materials** 🔶 **translation works, rendering crashes (2026-08-08)** — meshes,
   `UsdPreviewSurface` → PBR, cutout, two-sided, axis and unit baking. See § 4.3.
5. **Texture residency** — BC7 transcoding, mip generation, disk cache; enough for the scene
   to fit in VRAM.
6. **PointInstancer** — mapping to `Multiple`. **No culling.** Draw everything.
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