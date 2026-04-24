# AssetLoaders — Composite Format Loading

Context for developing composite asset format loaders in the Emeraude Engine.

## Module Overview

Format-agnostic namespace for loading multi-resource asset files (glTF, FBX, etc.) into engine resource containers. Produces a common intermediate representation (`AssetData`) that can be consumed by scene builders or mesh resources independently.

## Architecture

### Design Philosophy

AssetLoaders sits between `Libs/` (raw data) and `Scenes/` (scene graph). Each loader:
1. Parses a composite file format (glTF, FBX, USDZ...)
2. Creates engine resources in containers (images, textures, materials, geometry, meshes, skeletons, clips)
3. Attaches skeletal data to renderables automatically via `SkeletalDataTrait`
4. Produces a format-agnostic `AssetData` describing the node hierarchy — no Scene/Node/Entity types

### Layer Rules

- **CAN depend on:** `Resources/`, `Graphics/`, `Animations/`, `Libs/`
- **CANNOT depend on:** `Scenes/`, `Physics/`, `Audio/`, `Input/`
- **No Scene types:** `AssetData` uses `NodeDescriptor` (pure data), never `Node`, `StaticEntity`, or `Component::Visual`

### Common Interface

All loaders implement `Interface` (`Interface.hpp`):

```cpp
class Interface {
    void setOptions(LoaderOptions options) noexcept;
    virtual bool load(const std::filesystem::path & filepath, AssetData & output) noexcept = 0;
    virtual bool supportsExtension(std::string_view extension) const noexcept = 0;

    /* Default: returns false. Loaders override to opt in. */
    virtual bool loadAnimationClipsOnly(
        const std::filesystem::path & filepath,
        const Animations::SkeletonResource & targetSkeleton,
        std::vector<std::shared_ptr<Animations::AnimationClipResource>> & output) noexcept;
};
```

`LoaderOptions` controls resource loading behavior:
- `excludedNodeNames` — skip specific nodes and their subtrees
- `skipSkinning` — skip bone weights, skins, and animations

**Note:** `flattenHierarchy` is NOT in `LoaderOptions` — it only affects scene building and belongs in `Scenes::AssetDataConsumer`.

`loadAnimationClipsOnly()` covers the **split-animation workflow** (Mixamo per-action exports, Maya/Blender per-action FBX). The asset file is opened, every `anim_stack` is sampled against the bones of `targetSkeleton` resolved **by joint name**, and the produced clips are appended to `output`. Joints with no matching node are silently dropped (kept at bind pose). See FBXLoader section for the concrete implementation.

### AssetData — Common Intermediate Format

`AssetData` (`AssetData.hpp`) is the format-agnostic output:

| Field | Type | Description |
|-------|------|-------------|
| `meshes` | `vector<MeshDescriptor>` | Loaded renderables + geometry + materials |
| `skeletons` | `vector<shared_ptr<SkeletonResource>>` | Skeletal data |
| `animationClips` | `vector<shared_ptr<AnimationClipResource>>` | Animation clips |
| `nodes` | `vector<NodeDescriptor>` | Format-agnostic hierarchy (name, localFrame, meshIndex, childIndices) |
| `rootNodeIndices` | `vector<size_t>` | Root node indices |
| `skinJointNodeIndices` | `unordered_set<size_t>` | Joint nodes to skip in scene building |

Helper methods:
- `isSingleMesh()` — true if exactly one node has a mesh (skeleton joints don't count)
- `singleMeshNodeIndex()` — index of the single mesh-bearing node

### Two Consumption Paths

```
AssetData
    ├── Scenes::AssetDataConsumer  → Scene hierarchy (Nodes / StaticEntities)
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
| 6 | `buildNodeDescriptors()` | `NodeDescriptor` hierarchy in `AssetData` |

Phases 4-5 skipped when `skipSkinning = true`.

**Resource naming:** `glTF:{stem}/{Category}/{name}` (e.g., `glTF:Fox/Mesh/fox1`)

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
| 6 | `buildNodeDescriptors()` | **implemented** | `NodeDescriptor` hierarchy in `AssetData` |

**Mesh loading specifics:**

- Per-face triangulation via `ufbx_triangulate_face` with a buffer sized by `mesh.max_face_triangles`.
- Multi-material meshes are split into sub-geometry groups (one per `ufbx_mesh_part`).
- Per-corner vertex emission (position/normal/UV via `ufbx_get_vertex_vec3`/`vec2`) — the same vertex is written 3 times per triangle. A deduplication pass via `ufbx_generate_indices` can be added later if the overhead becomes measurable.
- Winding pre-compensates for the 180° X rotation applied by `AssetDataConsumer` (indices 1 and 2 swapped), identical to GLTFLoader.
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
- Bone element ids are added to `AssetData::skinJointNodeIndices` so the scene consumer does not instantiate them as regular scene nodes — joint transforms are owned by the `SkeletalAnimator`.

**LoaderOptions support:**

- `skipSkinning` — bypasses `loadSkins()`, `loadAnimations()` and per-vertex influence emission in `loadMeshes()`. A mesh that would otherwise have been skinned is loaded as a static pose.
- `excludedNodeNames` — matched against `ufbx_node.name` during `buildNodeDescriptors()`. Any node whose own name **or any ancestor's name** is in the set is dropped from `AssetData::nodes` along with its entire subtree. Handy for stripping rig helpers, dummies, LOD levels or debug locators.

**Animation specifics (étape 4):**

- One `AnimationClip<float>` is generated per `ufbx_anim_stack`, named after the stack.
- Keyframes are resampled at **30 Hz** via `ufbx_evaluate_transform(stack.anim, bone, time)` — game-engine canonical rate, matches Mixamo's default. High-frequency curves (> 30 Hz) alias, acceptable for skeletal motion.
- Every joint produces three channels (Translation / Rotation / Scale), with `jointIndex` = `cluster_index` inside the **first** skin deformer (`scene.skin_deformers.data[0]`). Mixamo and most pipelines keep bone ordering consistent across every skin of the same rig, so one clip set attaches cleanly to every skinned mesh.
- Clips are stored flat in `m_animationClips` and handed in bulk to every `SkeletalDataTrait::setSkeletalData(...)` call — identical to GLTFLoader's pattern.
- Per-stack sampling lives in the static helper `FBXLoader::sampleAnimStack(stack, jointToNode)`. `loadAnimations()` builds `jointToNode` from the reference skin's clusters; `loadAnimationClipsOnly()` builds it by name lookup against an external skeleton. Both call paths share the helper to guarantee identical 30 Hz / `ufbx_evaluate_transform` semantics.

**Multi-file animation pipeline (`loadAnimationClipsOnly`):**

For the **split-animation workflow** (a rig FBX + many per-action FBX next to it — Mixamo, Maya, Blender per-action exports):

```cpp
AssetLoaders::FBXLoader loader{resources};
AssetLoaders::AssetData assetData;
loader.load("Paladin/base_model.fbx", assetData);   // rig + skin + bind pose

const auto & skeleton = *assetData.skeletons[0];

std::vector<std::shared_ptr<AnimationClipResource>> clips;
for ( const auto & entry : std::filesystem::directory_iterator{"Paladin/"} ) {
    if ( entry.path().extension() == ".fbx" && entry.path().stem() != "base_model" ) {
        loader.loadAnimationClipsOnly(entry.path(), skeleton, clips);
    }
}

/* Replace (don't append): base_model.fbx ships with a bind-pose anim_stack
 * which would otherwise sit at index 0 and freeze the auto-play. */
for ( const auto & meshDesc : assetData.meshes ) {
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
- **Option 2 — Paladin** (`data/data-stores/FBX/Paladin/`): full split-animation workflow. `base_model.fbx` (rig + skin + bind pose) + 48 per-action `.fbx` files in the same folder, loaded via `loadAnimationClipsOnly` and bound to the rig by joint name. `slash_1` is placed at index 0 and auto-loops at lazy-init time. **Animation pipeline validated end-to-end** (no dislocation). Known cosmetic issues unrelated to animation: model renders dark (likely PBR albedo / normals) + UV alignment looks off — see follow-up tasks.

## Consumers

### Scenes::AssetDataConsumer (`Scenes/AssetDataConsumer.hpp`)

Transforms `AssetData` into scene objects:

```cpp
AssetDataConsumer consumer;
consumer.setFlattenHierarchy(false);  // optional
consumer.build(assetData, scene);               // StaticEntity mode
consumer.build(assetData, scene, parentNode);   // Node mode
```

Handles Y-up → Y-down conversion (180° X rotation on parentNode).

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

- `AssetLoaders/AssetData.hpp` — Common intermediate format (NodeDescriptor, MeshDescriptor, AssetData)
- `AssetLoaders/Interface.hpp` — Loader interface + LoaderOptions
- `AssetLoaders/GLTFLoader.hpp/.cpp` — glTF/GLB implementation
- `AssetLoaders/FBXLoader.hpp/.cpp` — FBX stub
- `Scenes/AssetDataConsumer.hpp/.cpp` — Scene-side consumer (Node/StaticEntity builder)

## Critical Rules

1. **Never add Scene dependencies** to this namespace — that's the whole point of the separation
2. **Lambda capture safety** — same rules as before: never capture `this`, pre-resolve shared_ptr
3. **Default resource on every error path** — never leave a nullptr slot
4. **Y-up conversion is NOT done here** — it's the consumer's responsibility (AssetDataConsumer or the actor code)
