# Scenes::Loaders — Composite Format Loading

Context for developing composite asset format loaders in the Emeraude Engine.

## Module Overview

Format-agnostic namespace for loading multi-resource asset files (glTF, FBX, etc.) into engine resource containers. Produces a common intermediate representation (`SceneData`) that can be consumed by scene builders or mesh resources independently.

> [!IMPORTANT]
> **This layer loads *scenes*.** Whether a given file yields a single model or a complete scene
> (lights, cameras, instancers) is decided by **the format**, not by the caller — hence
> `Loaders` / `SceneData` (renamed from `AssetLoaders` / `AssetData`, 2026-08-08). It lives
> engine-side rather than in emeraude-base precisely because emeraude-base only knows raw,
> classic geometry formats; composite scene description belongs here.
>
> **Absorption rule.** A loader translates *everything* into native engine scene logic. When
> the scene layer cannot express a source concept, the missing capability is added to `Scenes`
> — never a foreign construct kept alive, never a workaround in the loader. See
> [`../../docs/scene-loaders-usd.md`](../../docs/scene-loaders-usd.md).

## Architecture

### Design Philosophy

Scenes::Loaders sits between `Base/` (raw data) and `Scenes/` (scene graph). Each loader:
1. Parses a composite file format (glTF, FBX, USDZ...)
2. Creates engine resources in containers (images, textures, materials, geometry, meshes, skeletons, clips)
3. Attaches skeletal data to renderables automatically via `SkeletalDataTrait`
4. Produces a format-agnostic `SceneData` describing the node hierarchy — no Scene/Node/Entity types

### Layer Rules

- **CAN depend on:** `Resources/`, `Graphics/`, `Animations/`, `Base/`
- **CANNOT depend on:** `Scenes/`, `Physics/`, `Audio/`, `Input/`
- **No Scene types:** `SceneData` uses `NodeDescriptor` (pure data), never `Node`, `StaticEntity`, or `Component::Visual`

### Capability declaration (added 2026-08-08)

`LoaderCapabilityBits` (`Interface.hpp`): `Geometry`, `Skinning`, `Animations`, `Lights`,
`Cameras`.

> [!WARNING]
> The mask describes **the loader**, not the file format. FBX carries lights and cameras; our
> FBX loader does not read them, so it must not advertise them. Current state:
>
> | Loader | Capabilities |
> |--------|--------------|
> | `GLTFLoader` | `Geometry \| Skinning \| Animations \| Lights \| Cameras` |
> | `FBXLoader` | `Geometry \| Skinning \| Animations` |
> | `WADLoader` | `Geometry` (a Doom level's lighting is BAKED, by design) |
>
> **Why it exists:** probing the produced `SceneData` for an empty light table cannot tell
> *"this loader ignores lights"* from *"this asset declares none"* — and both happen. Ask the
> capabilities before concluding anything about a scene's lighting.

### Common Interface

All loaders implement `Interface` (`Interface.hpp`):

```cpp
class Interface {
    void setOptions(LoaderOptions options) noexcept;
    virtual bool load(const std::filesystem::path & filepath, SceneData & output) noexcept = 0;
    virtual bool supportsExtension(std::string_view extension) const noexcept = 0;

    /* Mask of LoaderCapabilityBits. Pure virtual: a loader cannot inherit a lie. */
    virtual uint32_t capabilities() const noexcept = 0;

    /* Default: returns false. Loaders override to opt in. */
    virtual bool loadAnimationClipsOnly(
        const std::filesystem::path & filepath,
        const Animations::SkeletonResource & targetSkeleton,
        std::vector<std::shared_ptr<Animations::AnimationClipResource>> & output) noexcept;
};
```

`LoaderOptions` controls resource loading behavior:
- `excludedNodeNames` — skip specific nodes and their subtrees
- `onMeshLoaded` — `std::function<void(MeshDescriptor &)>` callback invoked once per mesh, right after the renderable, geometry and materials have been registered in their containers and pushed to `output.meshes` (before nodes are wired). Lets the caller patch the descriptor in place — typical use cases: enable IBL reflection on PBR materials (`pbr->setReflectionComponentFromEnvironmentCubemap`), swap a renderable, override geometry. Symmetric across `FBXLoader` and `GLTFLoader` (5 invocation sites total: 4 in FBXLoader including the fallback paths, 1 in GLTFLoader).
- `materialMode` — `MaterialMode { PBR, Standard }`, default `PBR`. Selects which material container the loader populates. Both loaders go through the same generic configuration lambda thanks to the cross-material setter aliases on `StandardResource` (see *Cross-material setters* below). Both PBR and Standard paths are validated end-to-end on the Paladin asset.
- `skipSkinning` — skip bone weights, skins, and animations
- `forceDoubleSided` — `bool`, default `false`. Forces **every** material part of the loaded model to `RasterizationOptions{ CullingMode::None }`, OR-ed with the per-material asset flag (see *Double-sided materials* below). Symmetric across `FBXLoader` (applied per material part, default-material parts included) and `GLTFLoader` (per primitive). Use it when the asset's format/exporter cannot carry a double-sided flag the loader can read — the canonical case is **Mixamo FBX rigs**, which ship plain `FbxSurfacePhong` materials: ufbx only surfaces `double_sided` for glTF-style materials, so these models always import single-sided and thin shells (inner armour, cloth) render with see-through holes. Blunt instrument (whole model); for per-mesh selectivity use the `onMeshLoaded` hook instead. Two-sided lighting (back-face normal flip) is already engine-side, so forced back-faces are correctly lit. Validated on the Paladin (`src/Actor/Paladin.cpp`).
- `uniformScale` — `float`, default `1.0F`. Uniform scale applied at load time, **coherently across the full skinned-mesh pipeline**: vertex positions (in `loadMeshes`), joint local TRS translations + inverse bind matrix translation columns (in `loadSkins`), and animation translation keyframes (in `sampleAnimStack`, covers both `load()` embedded clips AND `loadAnimationClipsOnly()` external clips). Rotations and scales of joint TRS plus per-vertex influence weights are never touched. Linear (rotation + uniform 1×1 scale) parts of the inverse bind matrix are unaffected by uniform scaling around origin, so only the translation column needs scaling there. **Critical**: the same factor must be passed to BOTH the rig load (`load()`) AND every subsequent `loadAnimationClipsOnly()` against that rig — otherwise animation translation keyframes describe positions in a different unit than the scaled bind pose, joints snap to wrong positions on every keyframe, and the rig visually collapses on the first animated frame. Also propagates to the renderable's bounding box, so collision shapes derived from the bbox reflect the scaled size automatically. Use cases: enlarging a Mixamo humanoid that ships at 1.7 m to 1.9 m for a knight silhouette (validated end-to-end on the Paladin); shrinking oversized Maya/Blender assets without re-export.
- `stripRootMotion` — `bool`, default `false`. When set, `loadAnimationClipsOnly()` zeroes the **horizontal (X, Z) components** of every translation keyframe on every root joint of the produced clips. Rotation + scale of the root and *all* channels of every other joint stay intact. The vertical (Y) component is preserved on purpose: it carries both the bind-pose hip-height offset (~0.85 m on a Mixamo humanoid — wiping it would sink the model halfway into the ground) and the natural up/down bounce of walking, jumping or crouching. Idiomatic "convert per-action FBX into in-place clip" pass at load time. Required for any FBX (Mixamo, Maya/Blender per-action) where the root bone carries forward locomotion AND the actor's displacement is also driven by gameplay code (physics force, navmesh) — without this, the two motions stack and the model snaps backward at every clip loop. Has no effect on `load()` (full-pipeline import) — only on `loadAnimationClipsOnly()`. **Future work — Option C (root-motion mode):** instead of stripping, extract the root delta per frame and feed it back to the actor as actual displacement (foot-planting, no foot-sliding, animation-driven speed). Would replace the actor-side `addForce` for animation-driven characters; tracked as a TODO for the locomotion subsystem.

**Note:** `flattenHierarchy` is NOT in `LoaderOptions` — it only affects scene building and belongs in `Scenes::SceneDataConsumer`.

### Double-sided materials (honored since Jun 2026)

Both loaders read the standard per-material **double-sided** flag and translate it to a per-layer
`RasterizationOptions{ CullingMode::None }` passed into `MeshResource::load(geometry, materialList, rasterizationOptions)` / `SimpleMeshResource::load(geometry, material, rasterizationOptions)`:
- **glTF** — `fastgltf::Material::doubleSided` (glTF 2.0 standard), read in `GLTFLoader::loadMeshes` per primitive.
- **FBX** — `ufbx_material.features.double_sided.enabled`, read in `FBXLoader` per material part.

> [!WARNING]
> **The FBX asset-driven path only fires for glTF-style materials.** ufbx maps
> `UFBX_MATERIAL_FEATURE_DOUBLE_SIDED` solely from the `main|DoubleSided` property in
> `ufbxi_gltf_material_features` — the `UFBX_SHADER_FBX_PHONG`/`FBX_LAMBERT` presets do **not**
> include it. So a standard FBX (Maya/Mixamo `FbxSurfacePhong`) **never** reports
> `double_sided.enabled == true`, no matter how it was authored: the FBX double-sided intent lives
> in the *Model node* `Culling` property (default `"CullingOff"` on every node — not a reliable
> per-material signal, and not exposed by ufbx's public API anyway). This is a format/exporter
> limitation, **not** an engine bug — the loader honours everything ufbx surfaces. For these assets
> use `LoaderOptions::forceDoubleSided` (above). Diagnosed Jun 2026 on the Paladin (Mixamo Phong).

Without this, back-faces were culled and thin double-sided surfaces (Sponza curtains, foliage,
cloth, inner armour shells) rendered front-face-only with see-through holes. The models were correct
(Sponza curtains declare `"doubleSided":true`); the loaders simply ignored the flag.

> [!WARNING]
> **The multi-material `MeshResource::load(geometry, materialList, rasterizationOptions)` overload
> previously IGNORED its `rasterizationOptions` argument** (every layer got defaults). It now applies
> `rasterizationOptions[i]` per layer (defaults when the vector is shorter). Keep it that way.

> [!NOTE]
> **Two-sided lighting (the other half) is implemented — view-based (`dot(N,V)`).** Geometry
> double-sidedness alone renders back-faces, but they need their shading normal flipped or they
> are lit with an inward-pointing normal. The lighting fragment shaders orient the normal toward
> the viewer: `N = dot(N, V) < 0.0 ? -N : N` — in `LightGenerator.PBR.cpp` (both the normal-mapped
> and geometric N) and `LightGenerator.PerFragment.cpp` (a shared `twoSidedN`/`twoSidedV` used by
> diffuse + specular). The legacy normal-mapped Phong path
> (`LightGenerator.PerFragment.NormalMap.cpp`) applies the same correction to its "facing away →
> discard" test; its tangent-space back-face shading stays approximate.
>
> **Why `dot(N,V)` and not `gl_FrontFacing`:** `gl_FrontFacing` keys off the triangle *winding*,
> so a surface whose visible face is wound "backwards" (e.g. a Perlin ground rasterized
> back-facing) gets its correct normal wrongly flipped → its lighting collapses to zero. The
> view-based test keys off the actual geometry (normal vs eye), so any surface facing the camera
> is lit correctly regardless of winding. (This replaced an earlier `gl_FrontFacing` version that
> broke point-light illumination on the ground.)

**Default behaviour preserved:** if neither `onMeshLoaded` nor `materialMode` is set, the loader behaves exactly like before (PBR container, no callback, every existing call site unaffected). Both fields are gated: `onMeshLoaded` is invoked under `if ( m_options.onMeshLoaded )` (a default-constructed `std::function` evaluates to `false`); `materialMode` defaults to `MaterialMode::PBR`.

### Cross-material setters on StandardResource

To keep the loaders' material-configuration lambda generic (one code path that works for both `PBRResource` and `StandardResource`), `Graphics::Material::StandardResource` exposes PBR-named alias setters that map onto its Phong/Blinn parameters order-independently:

| Alias setter (Standard) | Forwarded to / mapping |
|-------------------------|------------------------|
| `setAlbedoComponent(Color)` | `setDiffuseComponent(Color)` + tracks `m_pbrAlbedoColor` |
| `setAlbedoComponent(texture)` | `setDiffuseComponent(texture)` (color tracked stays at previous value) |
| `setRoughnessComponent(value)` | tracks `m_pbrRoughness`, recomputes specular |
| `setRoughnessComponent(texture, value, invert)` | texture **ignored**, only the factor is honored |
| `setMetalnessComponent(value)` | tracks `m_pbrMetalness`, recomputes specular |
| `setMetalnessComponent(texture, value)` | texture **ignored**, only the factor is honored |
| `setAmbientOcclusionComponent`, `setClearCoatComponent`, `setSheenComponent`, `setTransmissionComponent`, `setIridescenceComponent` | no-op stubs (no Phong/Blinn equivalent) |

`recomputeSpecularFromPBR()` derives `(specularColor, shininess)` from the tracked triple at every call:
- `shininess = (1 - roughness)² × MaxPBRShininess` (default `MaxPBRShininess = 128`)
- `specularColor = mix(vec3(DielectricF0), albedo, metalness)` with `DielectricF0 = 0.04`

It bails early if no `ComponentType::Diffuse` has been registered yet — emitting Specular alone would produce a shader sampling an undeclared `SurfaceDiffuseColor`. Diffuse is always created by `setAlbedoComponent` before any roughness/metalness setter in normal usage, so the early-out is a safety net.

`loadAnimationClipsOnly()` covers the **split-animation workflow** (Mixamo per-action exports, Maya/Blender per-action FBX). The asset file is opened, every `anim_stack` is sampled against the bones of `targetSkeleton` resolved **by joint name**, and the produced clips are appended to `output`. Joints with no matching node are silently dropped (kept at bind pose). See FBXLoader section for the concrete implementation.

### SceneData — Common Intermediate Format

`SceneData` (`SceneData.hpp`) is the format-agnostic output:

| Field | Type | Description |
|-------|------|-------------|
| `meshes` | `vector<MeshDescriptor>` | Loaded renderables + geometry + materials |
| `skeletons` | `vector<shared_ptr<SkeletonResource>>` | Skeletal data |
| `animationClips` | `vector<shared_ptr<AnimationClipResource>>` | Animation clips |
| `lights` | `vector<LightDescriptor>` | Punctual lights, in **photometric units** (see below) |
| `cameras` | `vector<CameraDescriptor>` | Authored camera viewpoints — **data only** |
| `instanceSets` | `vector<InstanceSetDescriptor>` | One renderable drawn N times — see below |
| `nodes` | `vector<NodeDescriptor>` | Format-agnostic hierarchy (name, localFrame, meshIndex, lightIndex, cameraIndex, childIndices) |
| `rootNodeIndices` | `vector<size_t>` | Root node indices |
| `skinJointNodeIndices` | `unordered_set<size_t>` | Joint nodes to skip in scene building |

#### Lights — the unit contract (added 2026-08-08)

> [!IMPORTANT]
> `LightDescriptor::intensity` carries the unit **the engine itself uses**, which is also what
> glTF `KHR_lights_punctual` specifies — they agree term for term, so the glTF path applies **no
> conversion factor at all**:
>
> | Type | Unit | Engine setter |
> |------|------|---------------|
> | `Directional` | **lux** (illuminance) | `DirectionalLight::setIlluminance()` |
> | `Point` | **candela** (luminous intensity) | `PointLight::setIntensity()` |
> | `Spot` | **candela** | `SpotLight::setIntensity()` |
>
> A loader whose source format uses another unit MUST convert **once, here**. A descriptor whose
> unit depended on the producing format would defeat the point of a format-agnostic contract.
>
> Cone angles are stored in **DEGREES** (glTF authors radians — `GLTFLoader` converts).
> `range` is a **culling bound, never a dimmer**: the falloff is carried by the inverse square.
> `0.0F` means the asset declared none, and the engine default is left alone.

#### Instance sets — redundancy is a HINT, not a draw order (added 2026-08-09)

`InstanceSetDescriptor` = `{ name, instances (vector<CartesianFrame<float>>), meshIndex }`.

It states the INTENT — *the same renderable, N times, here* — never the encoding. USD carries it
as a `PointInstancer`, glTF as `EXT_mesh_gpu_instancing`, FBX as duplicated nodes a loader may
fold. **Not a USD tax**: one consumer path serves every format.

> [!IMPORTANT]
> **How the instances reach the GPU is the CONSUMER's call**, because only the scene knows its own
> culling machinery. `Scenes::SceneDataConsumer` splits each set into spatial cells via
> `buildInstanceClusters()` (cell size through `setInstanceCellSize()`, default 32 units), so the
> rendering octree culls whole cells with no new culling path. A loader deciding that would be
> re-implementing the renderer.

> [!WARNING]
> **The mesh a set points at must NOT appear in `nodes`.** A prototype exists to be instanced;
> giving it a node draws one stray copy at the asset's origin — the classic instancing bug, and it
> looks like a random object floating in the scene rather than a loader mistake.

> [!WARNING]
> **Frames live in the same space as the meshes of the same `SceneData`.** A loader baking its own
> axis conversion into vertices MUST bake the very same one into these frames — and for a
> TRANSFORM that bake is a **conjugation** `C·T·C⁻¹`, never a permutation of the position. Get it
> wrong and every instance sits in the right place, rotated wrong.

> [!CAUTION]
> An asset can hold **no drawable node at all** and still be complete — every `PI_*.usd` element
> of Jungle Ruins is pure instances. `SceneDataConsumer::build()` returned early on an empty node
> table and would have dropped them while reporting success. It now checks both collections.

#### Cameras are data, never instantiated

An authored camera is a viewpoint, not a game camera (owner decision, 2026-08-08). The consumer
never turns one into a `Component::Camera` on its own — the caller decides. `yFieldOfView` is in
**degrees**; feed it to `Camera::setFieldOfView()`, which derives the focal length through the
camera's own sensor height, so the framing stays a lens.

Helper methods:
- `isSingleMesh()` — true if exactly one node has a mesh (skeleton joints don't count)
- `singleMeshNodeIndex()` — index of the single mesh-bearing node

**`MeshDescriptor::lightingEnabled`** (`bool`, default `true`, booleans last in the struct layout) declares whether the consumer must put this mesh on the **LIT path**. Default `true` means glTF and FBX behaviour is unchanged — a mesh coming from a lit format expects the light set, the ambient pass and the environment IBL. A loader that bakes its own lighting into the vertex colors on unlit materials — `WADLoader` is the reference case — sets it to `false`. `Scenes::SceneDataConsumer` previously called `visual.getRenderableInstance()->enableLighting()` **unconditionally at five sites**; all five now honor the descriptor's flag through the new `Graphics::RenderableInstance::Abstract::setLightingState(bool)`, the symmetric form of `enableLighting()`. It is implemented with `enableFlag`/`disableFlag` because `Base::FlagTrait< uint32_t >` offers no `setFlag(flag, state)` — and adding one to emeraude-base is barred by the *"Ave robustus!"* feature freeze.

### Two Consumption Paths

```
SceneData
    ├── Scenes::SceneDataConsumer  → Scene hierarchy (Nodes / StaticEntities)
    └── SimpleMeshResource::load(path) / MeshResource::load(path)  → Single mesh resource
```

## Implemented Loaders

### GLTFLoader

Loads glTF 2.0 / GLB files. Uses `fastgltf` library (vendored, static).

**6-Phase Pipeline:**

| Phase | Method | Output |
|-------|--------|--------|
| 1 | `loadImages()` | `ImageResource` |
| 2 | `loadMaterials()` | `Material::PBRResource` + `Texture2D` on-demand |
| 3 | `loadMeshes()` | `IndexedVertexResource` + `SimpleMeshResource`/`MeshResource` |
| 4 | `loadSkins()` | `SkeletonResource` + `Skin` |
| 5 | `loadAnimations()` | `AnimationClipResource` |
| 6 | `buildNodeDescriptors()` | `NodeDescriptor` hierarchy in `SceneData` |

Phases 4-5 skipped when `skipSkinning = true`.

**Resource naming:** `glTF:{stem}/{Category}/{name}` (e.g., `glTF:Fox/Mesh/fox1`)

### USDLoader (OpenUSD via tinyusdz — geometry, materials, sky, INSTANCING, Aug 2026)

Reads `.usd` / `.usda` / `.usdc` / `.usdz`. Its reference asset, Intel's Jungle Ruins, is the
engine's **GOLD GOAL** (owner: *"le Saint Graal"*) — see
[`../../docs/scene-loaders-usd.md`](../../docs/scene-loaders-usd.md) § 8. It now translates
meshes, materials, dome lights and **point instancers**. **`capabilities()` still returns
`None`** — a known lag between the mask and what is delivered, to be corrected. Full design:
[`../../docs/scene-loaders-usd.md`](../../docs/scene-loaders-usd.md).

> [!CAUTION]
> **`tinyusdz::LoadUSDFromFile()` composes NOTHING.** It reads the root layer, parses the
> `subLayers` metadata, and returns success — a 19-sublayer stage comes back holding 2 prims.
> Composition is an explicit, separate pipeline and the loader uses it:
>
> ```
> LoadLayerFromFile → CompositeSublayers → LayerToStage
> ```
>
> **`CompositeAllArcs()` is deliberately NOT called.** It resolves references, payloads,
> inherits and variants eagerly; measured on Intel Jungle Ruins that is **24 minutes and 15 GB
> resident with no convergence**. Sublayers alone take seconds at ~3 GB and yield the whole
> non-instanced scene. Prototype references are resolved on demand, per element.
> ⇒ **`inherits` and `variants` are not applied on this path.**

> [!WARNING]
> **An unresolved reference fails SILENTLY and takes its whole prim with it.** tinyusdz stores
> each sublayer's working directory as the raw relative path written in the file, never joined
> with the root, so a reference made from inside a sublayer is looked up relative to the
> *process* working directory. Symptom measured before the fix: 84 meshes loaded, **0
> PointInstancer prototypes, no error raised**. The loader defends against it by (a) composing
> from an absolute path and (b) seeding the resolver with **every** directory of the stage tree.
>
> This is why the loader's first functional output is an **inventory**: byte-scanning a USDC
> crate cannot tell you what a stage contains (crate files compress their token table), and a
> plausible-looking scene with missing prototypes is indistinguishable from a correct one
> without counting prims.

> [!CAUTION]
> **ALWAYS PRINT tinyusdz's `warn` STRING.** Every composition entry point returns one, and it is
> where the library says what it silently gave up on. `LayerToStage` was discarding **every
> `PointInstancer` of the asset with its entire subtree** — the prim type was missing from its
> reconstruction table — and it said so, every single time, in a string the loader threw away.
> That cost a full session of diagnosis. All four steps now trace it.

#### Point instancers — read from the PRIMS, never from Tydra

`PointInstancer` appears **zero times** in the whole of `src/tydra/`: Tydra will never report
instances, and `RenderScene::instances` is about something else. `collectInstancers()` walks the
prims directly. Tydra does *descend* into a prototype and convert its meshes, exposing
`RenderMesh::abs_path` — which is what ties an instancer to the renderable it draws, with no
patch to Tydra.

Three traps, all paid on this asset (details in `docs/scene-loaders-usd.md` § 4.6):

1. **A transform is conjugated, not permuted.** `C·T·C⁻¹` where `C` is the +90°-about-X axis
   change. Closed form: translation through `C`, orientation quaternion conjugated by `C`'s
   quaternion, scale with **Y and Z swapped**. Permuting the position alone yields a forest in the
   right places, every plant rotated wrong.
2. **`/_class_` subtrees are skipped.** USD classes are abstract templates; Tydra converts them
   anyway. 12 meshes out for 6 real prototypes on one element — drawing them stacks a copy of
   every species at the origin.
3. **A prototype gets NO node**, only a resource. A node draws it one extra time, alone.

⚠️ A prototype root is assumed to sit at the identity (Tydra exposes absolute matrices only).
The assumption is **checked and logged**, never trusted silently.

`patches/tinyusdz.patch` in ext-deps-generator carries four fixes: the missing install rules
(upstream installs only its optional C API); an RAII guard restoring the asset resolver's state
after recursion — without it, every sibling sublayer after the first resolves against the wrong
directory, and upstream left a `TODO` asking for exactly that; a header include path only valid
inside the source tree; and the **missing PrimSpec→Prim table rows** for `GeomPointInstancer` and
five UsdLux types, all implemented upstream yet never listed, whose absence deletes the prim and
everything under it.

### FBXLoader (Phase 5 — Full Pipeline + LoaderOptions Plumbed)

Loads FBX files. Uses `ufbx` library (vendored as a git submodule at `dependencies/ufbx`, pinned to **v0.21.3**).

**Axis/unit conversion is delegated to ufbx at load time** via `ufbx_load_opts`:
- `target_axes = ufbx_axes_right_handed_y_up` — output is Y-up right-handed like glTF.
- `target_unit_meters = 1.0F` — output is in meters (1.0 = 1 m engine convention).
- `space_conversion = UFBX_SPACE_CONVERSION_MODIFY_GEOMETRY` — conversion baked into geometry, keeping node transforms clean.
- `generate_missing_normals = true` — ufbx provides smooth normals when the FBX lacks them.

**6-Phase Pipeline (same contract as GLTFLoader):**

| Phase | Method | Status | Output |
|-------|--------|--------|--------|
| 1 | `loadImages()` | **implemented** | `ImageResource` |
| 2 | `loadMaterials()` | **implemented** | `Material::PBRResource` + `Texture2D` on-demand |
| 3 | `loadMeshes()` | **implemented** (+ skin influences/weights) | `IndexedVertexResource` + `SimpleMeshResource`/`MeshResource` |
| 4 | `loadSkins()` | **implemented** | `SkeletonResource` + `Skin` |
| 5 | `loadAnimations()` | **implemented** (⚠ coord-space bug — see note below) | `AnimationClipResource` |
| 6 | `buildNodeDescriptors()` | **implemented** | `NodeDescriptor` hierarchy in `SceneData` |

**Mesh loading specifics:**

- Per-face triangulation via `ufbx_triangulate_face` with a buffer sized by `mesh.max_face_triangles`.
- Multi-material meshes are split into sub-geometry groups (one per `ufbx_mesh_part`).
- Per-corner vertex emission (position/normal/UV via `ufbx_get_vertex_vec3`/`vec2`) — the same vertex is written 3 times per triangle. A deduplication pass via `ufbx_generate_indices` can be added later if the overhead becomes measurable.
- **UV V-flip on read** — FBX stores UVs with V=0 at the bottom (OpenGL convention) ; the engine and Vulkan use V=0 at the top, matching glTF. The loader stores `(u, 1.0F - v)` in the vertex stream so embedded textures sample the correct region. **Without this flip, FBX models render with shuffled / black-region UVs** (regression marker — see `Paladin` recette below).
- Winding pre-compensates for the 180° X rotation applied by `SceneDataConsumer` (indices 1 and 2 swapped), identical to GLTFLoader.
- Materials are resolved per-part via `mesh.materials.data[partIdx]->typed_id` → `m_materials[...]`, falling back to the default PBR resource when the FBX has no material connected.

**Image/material loading specifics:**

- `loadImages()` supports both **embedded content** (`ufbx_texture.content`) and **external file references** (tries `texture.filename`, `absolute_filename`, `relative_filename` in order). Format detected from filename extension (PNG/JPEG/Targa).
- `loadMaterials()` maps `ufbx_material.pbr` (`ufbx_material_pbr_maps`) to `PBRResource` components:
  - `base_color` → `setAlbedoComponent` (sRGB)
  - `roughness` → `setRoughnessComponent`
  - `metalness` → `setMetalnessComponent`
  - `normal_map` → `setNormalComponent`
  - `ambient_occlusion` → `setAmbientOcclusionComponent`
  - `emission_color` + `emission_factor` → `setAutoIlluminationComponent` (sRGB)
  - `opacity` < 1.0 or `opacity.texture != nullptr` → alpha blending enabled
- Texture resolution is **cached on demand** inside `loadMaterials()`: each `ufbx_texture*` is turned into a `Texture2D` exactly once, indexed by `typed_id`. Multiple materials sharing a texture reuse the same engine resource.

**Skinning specifics:**

- A single skin deformer per mesh is supported (`mesh.skin_deformers.data[0]`). Multiple deformers stacked on the same mesh fall back to the first one.
- **cluster_index equals joint_index in the Skeleton** — the skeleton is built in the exact same order as `skin.clusters[]`, so no remapping layer is needed between vertex influences and joint matrices.
- Per-vertex influences/weights are emitted during `loadMeshes`: up to **4 bones per vertex**, sourced from ufbx's weight-sorted `skin_vertex.weights[]` (ufbx guarantees descending sort). Weights are normalised to sum to 1.0 since ufbx does not guarantee it.
- Parent joint resolution walks `bone_node->parent` until another cluster's bone is hit; isolated clusters map to `NoParent` (skeleton root).
- `cluster->geometry_to_bone` is used as the inverse bind matrix (local vertex → bone space), converted from ufbx's 3×4 affine into the engine's 4×4 column-major `Matrix<4, float>` via `convertUfbxMatrix`.
- Skinned meshes are tracked in `m_meshToSkinIndex` at emission time; at the end of `load()`, each skinned renderable gets `setSkeletalData(skeleton, skin, clips)` via `Renderable::SkeletalDataTrait`, exactly like GLTFLoader.
- Bone element ids are added to `SceneData::skinJointNodeIndices` so the scene consumer does not instantiate them as regular scene nodes — joint transforms are owned by the `SkeletalAnimator`.

**LoaderOptions support:**

- `skipSkinning` — bypasses `loadSkins()`, `loadAnimations()` and per-vertex influence emission in `loadMeshes()`. A mesh that would otherwise have been skinned is loaded as a static pose.
- `excludedNodeNames` — matched against `ufbx_node.name` during `buildNodeDescriptors()`. Any node whose own name **or any ancestor's name** is in the set is dropped from `SceneData::nodes` along with its entire subtree. Handy for stripping rig helpers, dummies, LOD levels or debug locators.

**Animation specifics (étape 4):**

- One `AnimationClip<float>` is generated per `ufbx_anim_stack`, named after the stack.
- Keyframes are resampled at **30 Hz** via `ufbx_evaluate_transform(stack.anim, bone, time)` — game-engine canonical rate, matches Mixamo's default. High-frequency curves (> 30 Hz) alias, acceptable for skeletal motion.
- Every joint produces three channels (Translation / Rotation / Scale), with `jointIndex` = `cluster_index` inside the **first** skin deformer (`scene.skin_deformers.data[0]`). Mixamo and most pipelines keep bone ordering consistent across every skin of the same rig, so one clip set attaches cleanly to every skinned mesh.
- Clips are stored flat in `m_animationClips` and handed in bulk to every `SkeletalDataTrait::setSkeletalData(...)` call — identical to GLTFLoader's pattern.
- Per-stack sampling lives in the static helper `FBXLoader::sampleAnimStack(stack, jointToNode)`. `loadAnimations()` builds `jointToNode` from the reference skin's clusters; `loadAnimationClipsOnly()` builds it by name lookup against an external skeleton. Both call paths share the helper to guarantee identical 30 Hz / `ufbx_evaluate_transform` semantics.

**Multi-file animation pipeline (`loadAnimationClipsOnly`):**

For the **split-animation workflow** (a rig FBX + many per-action FBX next to it — Mixamo, Maya, Blender per-action exports):

```cpp
Scenes::Loaders::FBXLoader loader{resources};
Scenes::Loaders::SceneData sceneData;
loader.load("Paladin/base_model.fbx", sceneData);   // rig + skin + bind pose

const auto & skeleton = *sceneData.skeletons[0];

std::vector<std::shared_ptr<AnimationClipResource>> clips;
for ( const auto & entry : std::filesystem::directory_iterator{"Paladin/"} ) {
    if ( entry.path().extension() == ".fbx" && entry.path().stem() != "base_model" ) {
        loader.loadAnimationClipsOnly(entry.path(), skeleton, clips);
    }
}

/* Replace (don't append): base_model.fbx ships with a bind-pose anim_stack
 * which would otherwise sit at index 0 and freeze the auto-play. */
for ( const auto & meshDesc : sceneData.meshes ) {
    if ( auto * trait = dynamic_cast<Renderable::SkeletalDataTrait *>(meshDesc.renderable.get()) ) {
        trait->setAnimationClips(clips);
    }
}
```

Implementation details:

- Same `ufbx_load_opts` as `load()` (Y-up, 1 m, `MODIFY_GEOMETRY`) so animation curves live in the same coord space as the bind pose baked into the target skeleton.
- Bone resolution: for every joint of `targetSkeleton`, `ufbx_find_node_len(scene, jointName, len)` looks up the FBX node by exact name. Joints with no match are silently kept at bind pose (warning logged when partial). Aborts only if **zero** joints resolve.
- Clip naming: file stem (`idle_1.fbx` → clip name `"idle_1"`). When a single file holds multiple anim_stacks, suffixed by stack index (`idle_1_0`, `idle_1_1`). Resource name: `FBX:{stem}/Animation/{clip_name}`.
- Mixamo names every stack `mixamo.com` (useless) — using the filename gives meaningful clip names for `SkeletalAnimator::play("slash_1")`.
- The runtime auto-plays `animationClips()[0]` at lazy-init time (see `Scenes::Component::Visual` lazy animator setup) — the **order** of the clip vector matters. Demos that want a specific clip to auto-loop must place it at index 0 (e.g. by inserting it first in the `directory_iterator` walk).

**Coord-space bug status (2026-04-24):**

The dislocation bug from étape 4 is **resolved on recent FBX exports**: the Paladin asset (`data/data-stores/FBX/Paladin/`, latest Mixamo export) plays its split animations correctly with no limb dislocation. The X Bot asset (older Mixamo export, `data/data-stores/FBX/Mixamo/X Bot.fbx`) still exhibits the bug — kept as a regression marker. Hypothesis: Mixamo updated its FBX writer (different `adjust_pre_*` / pivot baking) and the legacy export hits a code path the new exports avoid. Investigation deferred until/unless an asset producer asks for legacy-export support.

**Historical — coord-space dislocation bug (resolved on recent FBX exports):**

Étape 4 had a dislocation bug: skinned characters rendered upside-down with limbs splayed from the pivot. Hypothesis at the time: bind pose TRS sourced from `bone.local_transform` (raw, no `adjust_pre_*` baked) versus animation keyframes from `ufbx_evaluate_transform` (adjust_pre baked) — the blend mixed two different spaces.

**Confirmed resolved on Paladin (latest Mixamo export, 2026-04-24)** — `slash_1.fbx` plays correctly with no dislocation, all bones stay coherent through the full clip. Likely Mixamo updated its FBX writer (different `adjust_pre_*` / pivot baking) and recent exports avoid the broken code path.

**Still observed on the legacy X Bot asset** (`data/data-stores/FBX/Mixamo/X Bot.fbx`, older Mixamo export) — kept in the demo as a regression marker. Investigation deferred until/unless an asset producer asks for legacy-export support; the new format covers all current production needs.

**Known FBX-exporter quirk — Maya 2022 USD Preview Surface:**
Maya's USD → FBX converter drops all texture connections. The material still exists under its USD name (`usdPreviewSurface_*`) but `shader_type = UFBX_SHADER_FBX_LAMBERT`, `mat.textures.count = 0`, and all `pbr.base_color.texture` pointers are null. **Not a loader bug** — the texture data is absent from the FBX binary. Blender/Max exports preserve texture connections correctly.

**Resource naming:** `FBX:{stem}/{Category}/{name}` (e.g., `FBX:X Bot/Material/Alpha_Body_MAT`)

**Recette assets** (demo `./projet-alpha --load-demo fbx-loader`):
- **Option 0 — Mixamo X Bot** (`data/data-stores/FBX/Mixamo/X Bot.fbx`): validates materials color path, skinning pipeline (2 skin deformers + 2 meshes), animation clip construction (2 anim stacks). **Still exhibits the legacy dislocation bug** — kept as a regression marker.
- **Option 1 — Intel Knight** (`data/data-stores/FBX/Knight/...`): Maya USD Preview Surface quirk (textures stripped). Skinning guard filters it out → renders as clean static T-pose.
- **Option 2 — Paladin** (`data/data-stores/FBX/Paladin/`): full split-animation workflow. `base_model.fbx` (rig + skin + bind pose) + 48 per-action `.fbx` files in the same folder, loaded via `loadAnimationClipsOnly` and bound to the rig by joint name. `slash_1` is placed at index 0 and auto-loops at lazy-init time. **Animation pipeline validated end-to-end** (no dislocation). **UV bug resolved** (V-flip on read, see *Mesh loading specifics* above). **Dark-render investigation closed**: turned out to be the expected PBR rendering with the default opt-out IBL — PBR materials must explicitly call `setReflectionComponentFromEnvironmentCubemap()` to consume the scene's environment cubemap, otherwise they render markedly darker than `StandardResource` for the same direct lighting. The `projet-alpha` demo wires this via the `onMeshLoaded` hook (see `src/Builtin/FBXLoader.cpp`).

### WADLoader (level MATERIALIZER — Jul 2026, sky + photometric anchor Aug 2026, masked middle textures Aug 2026)

Loads a classic Doom-engine WAD (IWAD/PWAD) and materializes **one map** as static textured
geometry. Deliberately NOT a game loader: no things, no sprites, no mechanics — walls,
floors, ceilings, original textures, sector light levels baked as vertex colors.

**API:** `setMapIndex(n)` (1-based, WAD directory order — works across both naming schemes
ExMy and MAPxx) or `setMapName("E1M1")`. After `load()`: `loadedMapName()`,
`playerStartPosition()` / `playerStartDirection()` — already in ENGINE WORLD space (Y-down,
post-consumer), at the sector floor (eye height up to the caller). Scale: 32 map units per
meter (`MapUnitsPerMeter`), multiplied by `LoaderOptions::uniformScale`.

**Pipeline:**
1. Whole-file read → directory (name/offset/size per lump), map marker lookup (`ExMy`/`MAPxx`
   followed by `THINGS`). Every 8-char name is produced by `readName8()`, which **UPPERCASES** —
   see *readName8 uppercasing* below.
2. Map lumps: `VERTEXES`, `LINEDEFS`, `SIDEDEFS`, `SECTORS`, `SEGS`, `SSECTORS`, `NODES`, `THINGS`.
   The LINEDEF **flags** word (16-bit, record offset 4) is parsed, with named constants
   `LinedefTwoSided` (0x0004), `LinedefDontPegTop` (0x0008), `LinedefDontPegBottom` (0x0010) —
   only the two pegging flags concern a materializer, the rest are collision/sound/automap.
3. **Walls** from linedefs: one-sided → full quad; two-sided → lower/upper step quads seen from
   each side (sky-hack: no upper quad between two `F_SKY1` ceilings — see *Sky sectors* below),
   **plus the two-sided MIDDLE texture** (grates, fences, fake walls — see *Masked two-sided
   middle textures* below). UVs in texel space from sidedef offsets.
4. **Floors/ceilings**: per-subsector convex polygons reconstructed EXACTLY by
   **Sutherland-Hodgman clipping of the level bbox through the BSP node planes** down to each
   leaf, then by the leaf's segs (their right side faces the subsector — Doom convention).
   ⚠️ Neither fanning the seg vertices nor angular-sorting them works: subsector corners
   created by two partition lines carry no seg vertex → holes. The BSP clipping is the only
   correct source. A sector whose ceiling flat is `F_SKY1` emits **NO ceiling at all** — see
   *Sky sectors* below.
5. **Textures**: `PLAYPAL` palette → flats (raw 64×64) and composite wall textures
   (`TEXTURE1/2` + `PNAMES` + picture-format patch blitting, transparent texels alpha 0). The
   per-texel **coverage canvas** written while composing is also reduced to a single per-texture
   verdict cached in `textureHasHoles` — the basis of the masked classification below.
   No sky texture is resolved: the WAD's own `SKY*` lumps are never materialized.
6. One multi-material mesh: `VertexFactory::Shape` groups (one per **bucket**, triangle
   offset/count, winding swap i1↔i2 like GLTF/FBX) → `IndexedVertexResource`
   (`EnableNormal|EnablePrimaryTextureCoordinates|EnableVertexColor`) →
   `MultiLayerMeshResource` with per-layer `CullingMode::None` (double-sided on purpose).
7. Materials: unlit `BasicResource` per bucket, `setTextureResource()` + `enableVertexColor()`,
   `enableAlphaTest()` on the masked ones, plus the absolute-luminance anchor — see
   *Photometric anchor* below.

**Sky sectors — the engine skybox shows through (Aug 2026):**

A sector whose ceiling flat is `F_SKY1` emits **nothing**: no sky plane, no sky texture, no
stencil, no portal. The scene **background** is what fills the opening.

Why the hole is not a hole (verified in `Scenes::Scene::registerSceneVisualComponents()`): the
background renderable gets `setUseInfinityView(true)` + `disableDepthTest(true)` +
`disableDepthWrite(true)`, is inserted into the **Opaque** render list with distance `0.0F` (the
`isSpecial` / `isUsingInfinityView()` branch) and is drained first. Its geometry is a 512 m cuboid
with flipped winding drawn on the translation-free view, so it fills every pixel the level does not
cover; level geometry drawn afterwards with depth test **and** depth write ON simply overwrites it.
This is the modern equivalent of vanilla's sky visplane. With **no** background installed the
pixels are opaque black — never garbage.

The dead **episode sky-texture selection** (`SKY<e>` / `RSKY1-3` picked from the map name) was
**DELETED**.

The pre-existing **upper-wall suppression was already correct** and is untouched: `skyOnBothSides`
in `WADLoader.cpp` suppresses **BOTH** upper quads when both sectors' ceilings are the sky flat.
That is exactly vanilla Doom's sky hack — `R_StoreWallRange` (`r_segs.c`) sets
`worldtop = worldhigh`, so the `worldhigh < worldtop` guard is false and the upper texture is never
assigned. Sky adjacent to a **non-sky** ceiling still emits its upper wall — also vanilla.

Demo side (`projet-alpha`, `src/Builtin/DoomLoader.cpp`): the constructor calls
`enableBasicBackground({}, true)` — empty name = random pick from the sky store, `true` = the
generic KeyPad3 sky cycle. `enableBasicLighting()` is **deliberately NOT** called: installing a sky
must not put the level on the lit path (see *Unlit on purpose* below).

**Masked two-sided middle textures — grates, fences, fake walls (Aug 2026):**

Doom's two-sided linedefs can carry a **middle** texture in addition to the upper/lower steps. It is
not a wall: it hangs in the *opening* between the two sectors and whatever of the opening it does not
cover stays see-through. This is now materialized, as an **alpha-tested CUTOUT**.

*Engine side — new material contract* (canonical home:
[`src/Graphics/AGENTS.md`](../Graphics/AGENTS.md#alpha-test--the-binary-cutout-contract-aug-2026) § 5,
"Alpha Test — the Binary Cutout Contract" — keep the two in sync, and prefer editing that one):

- New flag **`MaterialFlagBits::AlphaTestEnabled = 1U << 16`** (`Graphics/Material/Interface.hpp`) —
  a binary **CUTOUT**: the fragment shader discards below a cutoff and the material **STAYS
  OPAQUE** (opaque render list, depth write kept, no back-to-front sorting, state-sorted batching
  preserved). `BasicResource::enableAlphaTest()` sets it.
- The discard now fires on **that flag, INDEPENDENTLY of the blending mode**. Gating it on blending
  was exactly what used to force a cutout out of the opaque list.
- ⚠️ **`isOpaque()` is deliberately UNCHANGED and must stay so** — an alpha-tested material **IS**
  opaque. Returning `false` there would push it into the distance-sorted **translucent** list AND
  enable colour blending (`GraphicsPipeline::configureColorBlendState` keys on that predicate),
  defeating the flag entirely.
- `isAlphaTest()` returns `true` for the flag, so the **RT pipeline** alpha-tests at hit time.
- `requiresAlphaTestedShadows()` now **also** returns `true` for the flag: a cutout must cast a
  **CUTOUT shadow**, not a solid rectangle. (It previously required `BlendingMode::Normal`, so a
  cutout would have shadowed solid.)

> [!WARNING]
> **The cutoff is FIXED at 0.5 and NOT configurable, on purpose.** Two reasons:
> 1. The **shader program cache keys on the material's DESCRIPTOR LAYOUT hash**, not on its flags or
>    values — a per-material cutoff literal baked into the generated GLSL could serve one material's
>    program to another sharing the same layout.
> 2. All **twelve floats** of `BasicResource`'s material-properties buffer are already claimed, so a
>    uniform-borne cutoff would require growing the block.
>
> 0.5 is the right value for a **coverage** mask, and it now agrees across all three paths: colour
> discard, shadow discard, and `GPURTMaterialData::alphaCutoff` (which already defaulted to 0.5).
> **A configurable cutoff needs the cache key fixed first.**

*Loader side (`WADLoader.cpp`):*

- **Compound bucket key.** The geometry bucket key became `SurfaceKey{name, SurfaceClass}` with
  `SurfaceClass::Opaque | Masked`. Required because the same texture **NAME** can be consumed both
  ways: measured in `doom.wad`, **WOOD1, GSTONE1, MARBLE2, SP_ROCK1, MARBFACE and FIREMAG3** are each
  used BOTH as an ordinary wall AND as a two-sided middle texture. Keying on the name alone would
  force one material for both — a grate turning its solid twin see-through, or the reverse.
- ⚠️ **The class is also part of the MATERIAL RESOURCE NAME** (`.../Material/{NAME}` vs
  `.../Material/{NAME}/Masked`): the resource container is keyed by name, so without it the two
  variants would collide and whichever loaded **first** would impose its mode on the other.
- ⚠️ **Index-alignment invariant.** `materialOrder`, the sub-geometry groups, `materialList` and
  `rasterizationOptions` stay index-aligned **BECAUSE they are all produced by one ordered iteration
  of the bucket map**: a sub-geometry index **IS** a layer index **IS** a material index. Keep it
  that way.
- **`emitQuad` / `emitWall` split.** `emitQuad` was extracted from `emitWall`: it takes the **V range
  EXPLICITLY** plus a `SurfaceClass`. `emitWall` delegates to it and keeps the **pegged-to-top** rule
  for ordinary walls (which DO tile vertically).
- **`emitMiddle` implements vanilla's rules** (`linuxdoom-1.10` `R_RenderMaskedSegRange`): the image
  is drawn **EXACTLY ONCE**, **NEVER tiled vertically**, anchored in **WORLD** space then **CLIPPED**
  to the opening — whatever of the opening it does not cover stays see-through.

  ```
  opening          = [max(floors), min(ceilings)]
  anchor (texel 0) = min(ceilings) + yOffset             (default, pegged to the top)
                   = max(floors) + texHeight + yOffset   (with ML_DONTPEGBOTTOM, 0x0010)
  V(z)             = (anchor - z) / texHeight            (V grows downward, 1 texel per map unit)
  ```

  Clipping the quad to **both** the one-texture-height span **and** the opening keeps V inside
  `[0,1]` **BY CONSTRUCTION**, so the image is **CLIPPED, never squashed**. A **POSITIVE** `yOffset`
  slides the visible image **UP**.
  ⚠️ **Reproducing this with `emitWall()` would stretch or tile it** and misalign a large share of
  the faces.
- **Masked classification is AUTOMATIC**, from the **coverage canvas already computed while
  composing**: a texture is `Masked` only when its **COMPOSED image leaves texels uncovered**. That
  is Doom's own notion of masked — transparency is the **ABSENCE of a patch post**, never a colour
  key (palette index 0 is an ordinary opaque colour). Cached per texture in `textureHasHoles`.
  Measured: **6 of the 20** textures used as middle textures in `doom.wad` are fully opaque "fake
  wall" decoration and correctly stay `Opaque`, **paying nothing** for the cutout.
- **Duplicate-side rule.** The LEFT side's middle quad is **skipped when it would exactly duplicate
  the right one** (same texture AND same offsets): the material is already double-sided
  (`CullingMode::None`), so two coincident quads would only **z-fight**. **Differing offsets DO make
  two genuinely different images** and both are emitted — as in vanilla, where each sidedef is drawn
  from its own viewing side.
- **Vanilla glitches deliberately NOT reproduced:** *Medusa* (a multi-patch texture used as a middle
  texture) and *tutti-frutti* (a texture shorter than 128 tiling with garbage).
- Middle textures do **not** block movement in vanilla unless `ML_BLOCKING` is also set. This
  materializer has **no collision at all**, so the point is moot — stated for completeness.

*Validation (runtime, measured):*

- Validated on **E4M3** (map index **30** in `doom.wad`, **28 MIDGRATE faces**) — chosen because
  **E1M1 has only 13 middle faces and ALL of them are fully-opaque `BRNBIG*` decoration**, so E1M1
  does not exercise the cutout at all.
- **MIDGRATE** is 128×128 with **7480 uncovered texels (45.7%)**, correctly classified `Masked`.
- Confirmed visually: the grate's gaps are **genuinely see-through** (the wall and floor behind are
  visible), the image is **anchored correctly** in the doorway with **no stretching**, and there is
  **no z-fighting**.
- E4M3 was scanned for hole-bearing textures **by slot**: MIDGRATE in the middle slot is the ONLY
  one — no opaque wall on that map uses a hole-bearing texture, so nothing there is misclassified.

> [!WARNING]
> ⚠️ **STILL A KNOWN GAP — do not claim otherwise.** A hole-bearing texture used on an **ORDINARY
> wall** (upper, lower or one-sided) is classified `Opaque`, so its uncovered texels render **BLACK**
> (the palette index 0 colour, alpha ignored). Vanilla showed the *tutti-frutti* glitch there
> instead. **No map validated so far exercises it.**

**`readName8` uppercasing (bonus fix, Aug 2026):**

`readName8()` now **UPPERCASES** the 8-char names it produces. It is the **single site where every
WAD name is born**, so the lump directory, `PNAMES`, the sidedef texture names and the sector flat
names all become comparable.

Why it matters: WAD names are uppercase **by convention**, but the convention is **NOT enforced** —
`doom.wad`'s **TEKWALL4** references its patch through a **LOWERCASE** `PNAMES` entry (`w94_1`) while
the lump is `W94_1`. The case-sensitive lookup **silently failed**, the composed texture kept its
**cleared canvas**, and the wall rendered **BLACK** — **15 faces in E1M1 alone**.

**ASCII only, deliberately**: a WAD name is ASCII by format, and `std::toupper` would drag a
**locale** into a hot parsing loop.

**Photometric anchor — `FullBrightLuminance` + the demo's fixed exposure, ONE JOINT CALIBRATION (Aug 2026):**

Public constant `WADLoader::FullBrightLuminance = 2000.0F` (nits, cd/m²). Every map material
(`Material::BasicResource`, one per bucket) also does:

```cpp
materialResource.setAutoIlluminationAmount(1.0F);
materialResource.setEmissiveStrength(FullBrightLuminance);
```

The emitted quantity is therefore **texel × vertexColor × 2000 nits**. The surfaces **EMIT, they are
not LIT** — the sector light level is already baked into the vertex colors, so re-lighting would
double-count it. Exactly the same reasoning `SkyBoxResource` uses for a sky.

Rationale: a Doom map carries **no photometry**. Its sector light levels are 0-255 ordinals authored
for a CRT, not luminances. Emitted raw into a photometric pipeline they landed at **0.038 mean
output**, so the map needs an ABSOLUTE anchor.

**Why 2000 nits and not 250:** a fully-lit Doom surface is treated as a **SUNLIT surface**
(~2000-5000 nits in the real world) rather than as **white on an SDR monitor** (~250 nits), for
exactly one reason — the map has to **share the frame with a sky**. 250 nits read well on its own,
but it sat **5 stops under a daylight sky**, so no single exposure could hold both.

> [!WARNING]
> ⚠️⚠️ **`FullBrightLuminance` AND THE DEMO'S EXPOSURE TRIAD ARE ONE JOINT CALIBRATION.** Moving
> either alone breaks the other. The anchor lives in `WADLoader.hpp`, the triad in
> `projet-alpha`'s `DoomLoader::onEnabled()` — both carry the rule in their own comments. Change
> them together, or not at all.

The triad, **derived** (engine photometry, `Graphics/Photometry.hpp`):
`display = L / (MeterCalibration · 2^EV100)` with `MeterCalibration = 1.2`, and
`EV100 = log2(N²/t · 100/S)`.

| Term | Value |
|------|-------|
| Aperture / shutter / sensitivity | f/8 · 1/125 s · **ISO 125** |
| EV100 | `log2(6400)` = **12.64** |
| Clipping point | **7680 nits** |
| Fully-lit surface (2000 nits) | **0.26** display-linear |
| Clear sky at 8000 nits | **1.04** — just at the top |
| Skies that still clip | **2 of 28**: JeGray (25500) and AutumnField (31800) |

**The owner's principle, now confirmed:** ONE fixed configuration suffices for EVERY Doom map,
because all Doom maps are authored on the same scale (sector light 0-255, same meaning everywhere).
Nothing about a map justifies a per-map exposure. **The variable was never the map — it was the
sky**, which is not part of Doom's calibration and spans 1 to 31800 nits across the engine's store.

> [!WARNING]
> ⚠️⚠️ **DIAGNOSTIC LESSON — the failure mode of the superseded 250 nits + ISO 1000 pairing.**
> That pairing clipped at **960 nits**, so **16 of the 28 skies went PURE WHITE** and their glare
> flooded the frame on any **outdoor** map. The MAP ITSELF was correctly and **identically** exposed
> the whole time — which is precisely what made it confusing: it read as *"the fixed lighting failed
> on some maps"* when the map was never the variable. **INDOOR maps (E4M3) hid it completely**, so an
> indoor validation target proves **nothing** about this. Generalisable: when a fixed-exposure scene
> looks wrong on *some* content only, first check what ELSE shares the frame and its absolute range,
> before touching the thing that changed appearance.

> [!WARNING]
> **`setAutoIlluminationAmount` is the MASK and is clamped to `[0,1]`.** The luminance belongs to
> `setEmissiveStrength`. Passing the nits through the amount **silently clamps them to 1**.

Measured on the **superseded 250 nits + ISO 1000** revision, identical viewpoint, sky = Forrest
(3000 nits), before → after the anchor. Kept because it demonstrates the **anchor mechanism** (an
absolute anchor is what lifts the map out of near-black); the numbers do **not** describe the
current calibration:

| Metric | Before | After |
|--------|--------|-------|
| Geometry mean | 0.03843 | **0.39782** (×10.4) |
| Geometry p95 | 0.05490 | **0.56350** |
| Frame clipped to pure white | 5.34% | **0.06%** |
| Crushed pixels (≤ 0.02) | 0.00% | 0.00% |

An earlier revision instead **capped the random sky pick at 300 nits**. Abandoned: it treated the
symptom, and the luminance criterion also picked photometrically-valid but thematically absurd
backgrounds (abstract chrome interiors over E1M1) because **luminance cannot express "looks like a
sky"**. No cap now — the whole sky store is fair game. The cap survives as the optional third
parameter `maxLuminance` of `projet-alpha`'s
`AbstractDemo::enableBasicBackground(materialName = {}, cyclable = false, maxLuminance = 0.0F)`
(zero = no cap; bounds ONLY the random pick — the KeyPad3 cycle stays generic over every sky on
purpose, it is the affordance for inspecting the whole store).
⚠️ **No demo uses `maxLuminance` any more** — it defaults to a no-op; do not assume it is
load-bearing.

**Unlit on purpose — `MeshDescriptor::lightingEnabled = false`:**

The loader sets `meshDescriptor.lightingEnabled = false` on its level mesh (field documented under
*SceneData* above). The level is self-illuminating by construction — baked vertex colors on unlit
`BasicResource` — and the lit path's ambient/IBL term is scaled by the background luminance, so
putting the level on it would multiply every surface by the sky brightness and destroy the baked
look.

> [!NOTE]
> **Honest status: this fixes a LATENT defect only.** `Scene`'s test is
> `m_lightSet.isEnabled() && instance->isLightingEnabled()`, and the light set is enabled only by
> `Scene::applyBackgroundLighting()` (or `DefinitionResource` / the console) — which the
> `doom-loader` demo never calls. The flag is therefore currently **INERT** for that demo. It would
> bite the moment any demo enabled the light set with a WAD level loaded. Kept and documented as
> latent by owner decision — do **not** present it as something a measurement demonstrated.

**Critical lessons (engine-wide):**
- ⚠️ **Resource lambdas run on the resource-manager loading threads** — capture every local
  buffer BY VALUE (`[pixels = std::move(rgba)]`), never by reference. By-ref captures caused
  intermittent load failures AND a segfault (use-after-free of the stack).
- ⚠️ **The sky does NOT light this level — MEASURED.** With the **camera exposure fixed** (the
  `doom-loader` demo pins `setAutoExposure(false)` + f/8 · 1/125 s · **ISO 125**, the current triad —
  see *Photometric anchor* above; this measurement was taken on the superseded ISO 1000 revision),
  the framing held
  fixed and NO sky pixels in frame, the level's pixels were **bit-identical across the entire sky
  store** (1 nit to 31800 nits). Any future claim that "the sky brightens the map" **must be
  re-measured with the camera pointing DOWN and the exposure pinned**.
- ⚠️ **An auto-exposing camera invalidates that measurement**, and this was also measured: with
  auto-exposure ON the metering tracks the sky and the map's own rendered brightness swings by
  **5.7×** across the store (mean 0.86 under a 1-nit sky, 0.15 with 13 % clipping under a
  31800-nit one) — a **sensor** effect, no light reaching the geometry.
- ⚠️ **`UP = -Y` in this engine**: a `lookAt` target with a **more negative** Y looks **UP**. Getting
  that backwards produced a bogus *"the sky lights the level ×41"* conclusion in the session that
  introduced the skybox. That ×41 figure still appears in two code comments —
  `SceneData.hpp` (`MeshDescriptor::lightingEnabled`) and `WADLoader.cpp` (the
  `meshDescriptor.lightingEnabled = false` block) — the *mechanism* they describe is real, the
  **number is not measured**. Treat it as unverified, and reword those comments when the file is
  next touched. (`RenderableInstance::Abstract::setLightingState()` describes the same mechanism
  **without** the number — that one is fine.)
- **Perceptual color chain for unlit retro content**: textures are loaded with
  `enableSRGB(false)` and sector light stays raw — the direct swap-chain path does not
  re-encode, so keeping everything perceptual (texel × light) reproduces the original
  renderer's look. Decoding to linear darkened the whole map.

**V1 limitations (future work):** no collision (the demo flies through); fine texture alignment
still approximate on ordinary walls. **Two-sided middle textures are NO LONGER skipped** — they are
materialized as alpha-tested cutouts (see *Masked two-sided middle textures* above), and the
**pegging flags are now parsed** (`ML_DONTPEGTOP` / `ML_DONTPEGBOTTOM`), `ML_DONTPEGBOTTOM` being
honored for the middle-texture anchor. What remains on that front is the
**hole-bearing-texture-on-an-opaque-wall gap**: such a texture on an upper, lower or one-sided wall
is classified `Opaque` and its uncovered texels render black.
The sky is no longer a limitation either: sky sectors emit no ceiling and the scene background shows
through (see *Sky sectors* above).

**Resource naming:** `WAD:{stem}/Texture/{NAME}` and `WAD:{stem}/Material/{NAME}` (shared per WAD,
the masked variant suffixed `/Masked` — see the compound key above), `WAD:{stem}/{MAP}/...`
(per map: geometry, mesh).

## Consumers

### Scenes::SceneDataConsumer (`Scenes/SceneDataConsumer.hpp`)

Transforms `SceneData` into scene objects:

```cpp
SceneDataConsumer consumer;
consumer.setFlattenHierarchy(false);  // optional
consumer.build(sceneData, scene);               // StaticEntity mode
consumer.build(sceneData, scene, parentNode);   // Node mode
```

Handles Y-up → Y-down conversion (180° X rotation on parentNode).

Honors `MeshDescriptor::lightingEnabled` at **all five** visual-creation sites via
`RenderableInstance::Abstract::setLightingState(bool)` — it no longer calls `enableLighting()`
unconditionally (see *SceneData* above).

### SimpleMeshResource::load(path) / MeshResource::load(path)

Transparent single-mesh loading for `.gltf`/`.glb` files:

```cpp
auto mesh = resources.container<SimpleMeshResource>()
    ->getOrCreateResource("Fox", [](auto & res) {
        return res.load(std::filesystem::path{"Fox.glb"});
    });
```

Checks `isSingleMesh()` — refuses multi-mesh assets. Transfers skeletal data automatically.

## Important Files

- `Scenes/Loaders/SceneData.hpp` — Common intermediate format (NodeDescriptor, MeshDescriptor, SceneData)
- `Scenes/Loaders/Interface.hpp` — Loader interface + LoaderOptions
- `Scenes/Loaders/GLTFLoader.hpp/.cpp` — glTF/GLB implementation
- `Scenes/Loaders/FBXLoader.hpp/.cpp` — FBX implementation (ufbx)
- `Scenes/Loaders/WADLoader.hpp/.cpp` — Doom WAD level materializer (`FullBrightLuminance` lives in the header)
- `Scenes/SceneDataConsumer.hpp/.cpp` — Scene-side consumer (Node/StaticEntity builder, honors `lightingEnabled`)

## Critical Rules

1. **Never add Scene dependencies** to this namespace — that's the whole point of the separation
2. **Lambda capture safety** — same rules as before: never capture `this`, pre-resolve shared_ptr
3. **Default resource on every error path** — never leave a nullptr slot
4. **Y-up conversion is NOT done here** — it's the consumer's responsibility (SceneDataConsumer or the actor code)
