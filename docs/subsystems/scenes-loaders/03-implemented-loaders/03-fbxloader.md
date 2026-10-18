## Implemented Loaders

### FBXLoader (Phase 5 — Full Pipeline + LoaderOptions Plumbed)

Loads FBX files. Uses `ufbx` library (vendored as a git submodule at `dependencies/ufbx`, pinned to **v0.21.3**).

> [!CAUTION]
> **Geometry transforms — an FBX node transform that a loader reading `local_transform` alone
> DOES NOT SEE (fixed Aug 2026).** An FBX node can carry a *geometry transform* that affects its
> attached mesh but **not** its children. Left at ufbx's default
> `UFBX_GEOMETRY_TRANSFORM_HANDLING_PRESERVE`, it lives only in `ufbx_node.geometry_transform` /
> `geometry_to_node`, and `extractFrameFromNode()` — which reads `node.local_transform` — never
> compensated for it. **3ds Max and Maya emit them routinely**, so affected meshes imported at the
> wrong place, silently, with a perfectly clean log. The loader now passes
> `UFBX_GEOMETRY_TRANSFORM_HANDLING_MODIFY_GEOMETRY`, baking it into the vertices — the same
> strategy already used for the axis conversion — with helper nodes as ufbx's own fallback when
> one mesh is instanced several times with different geometry transforms. Set on **both**
> `load()` and `loadAnimationClipsOnly()` so the two never disagree.

> [!CAUTION]
> **`skinJointNodeIndices` is an index into `SceneData::nodes`, NOT a ufbx `element_id`
> (fixed Aug 2026).** `loadSkins()` used to insert `cluster->bone_node->element_id` — an index
> into `scene.elements[]`, all element types pooled together — while
> `SceneDataConsumer::build()` consumes the set as an index into `SceneData::nodes`, which
> `buildNodeDescriptors()` **COMPACTS** (the root and every excluded subtree are skipped). The two
> numbering schemes do not coincide, and the consumer's early `return` on a match **drops the
> matched node together with its entire subtree**. It survived because a node carrying a mesh is
> protected by the `!nodeDesc.meshIndex.has_value()` guard and Mixamo rigs put their meshes
> directly under the root — pure luck, not design. The skin analysis (`describeSkins()`) now
> collects **`ufbx_node` pointers** and the node analysis (`describeNodes()`) resolves them to real
> descriptor indices, which is the only place where the compaction is known. `GLTFLoader` was always
> correct here; the two loaders now agree.
>
> Related: since the split below, `FBXLoader` keeps **no per-asset state** at all (only the resource
> manager): every collection is a local of one `load()`, so nothing can leak from one file to the next.

**Two halves (2026-10-18, owner decision: analysis / materialization):** `FBX::analyse()`
(`FBXParser.hpp/.cpp`) turns a ufbx scene into an `FBX::Asset` of plain data — per texture its image (embedded bytes,
external path, or none), per material its factors and texture slots (index + colour space), per mesh its `Shape`
(triangulated, scaled, skin influences and weights, normals generated when absent), material parts and skin, per skin
deformer its skeleton and skin, the 30 Hz clips, the node descriptors and the bone-node indices — with NO engine
resource, service or device; `FBX::analyseClipsOnly()` does the same for a split-animation file against a target
skeleton, and `FBX::setLoadOptions()` holds the ufbx options both paths share. `FBXLoader` opens the file
(`ufbx_load_file()`, external files included), calls the analysis, then MATERIALIZES it: images, the two colour-space
textures on demand, materials, geometries (tangents computed on the loading thread, as before), meshes, skeletons,
clips, the skeletal attachment, the `SceneData`. The split exists so the whole analysis can be fuzzed:
`src/Fuzzing/fuzz_fbx.cpp` parses bytes with `ufbx_load_memory()` (no external file) and runs both analyses and the
tangent computation. Verified identical (2026-10-18): a dump of everything `load()` and `loadAnimationClipsOnly()`
deliver — every shape's vertices, triangles and groups hashed bit for bit, the mesh / geometry / material names, the
nodes (name, frame, mesh, children), roots, bone nodes, skeletons (joints, TRS, inverse bind matrices) and clips
(every key) — is byte-identical before and after on the animation-debug Paladin (4 meshes, 73 nodes, 4 skeletons, the
rig's 50 clips through `loadAnimationClipsOnly()`) and on Knight_USD_002.fbx + NewSponza_CypressTree_FBX_YUp.fbx through
`Core.openFiles()` (7 meshes, 967 nodes, 3 skeletons); 0 VUID.

Two behaviour changes came with the split, both on failure paths only: a mesh whose triangle count overflows the
32-bit vertex count (more than 1 431 655 765 triangles) is refused like a mesh without a valid triangle (the default
mesh, a warning) — the old 32-bit sum wrapped into a short vertex array the build then overran; and the skeletal data
is attached only to a mesh's OWN renderable, never to the shared default mesh a failed geometry or renderable falls back
to (the old element-id map attached it there too).

**Limits (2026-10-18, found by `fuzz_fbx`, owner decisions):**
- **ufbx memory** — a binary FBX declares the size of its arrays (often compressed) and ufbx allocated them as
  declared: a 50 KB file asked for 14 GB (332 reproducers, one cause). `FBX::setMemoryLimits()` caps each ufbx
  allocator (temporary and scene) at max(`MinimumMemoryLimit` 256 MiB, `MemoryLimitPerFileByte` 16 x the file size);
  beyond, ufbx fails with `UFBX_ERROR_MEMORY_LIMIT` and the load is refused with its message. Measured on the real
  assets: the temporary allocator peaks at 1x the file size, the scene at 6.5x (Knight_USD_002, 7.7 MB -> 50 MB; the
  1.7 GB IvyGrowth needs 1.36 GB). Applied to `load()`, `loadAnimationClipsOnly()` and the embedded-texture re-read.
- **Animation sampling** — the 30 Hz sampling sized itself on the stack duration the file declares (up to 10^12
  samples asked; a non-finite duration reached a `size_t` conversion, undefined behaviour). A stack of a non-finite
  time range, or whose frames x animated joints x 3 exceed `FBX::MaxClipKeyFrames` (4 M keyframes, ~200 MB — 10.7 min
  for 69 joints), is skipped with a warning; the model and the other clips load. Real data: 12.5 s and 69 joints at
  most (the Paladin's longest clip is 54 k keyframes).

Both verified identical on the dumps above (the same bytes with the limits on). Validated on macOS (M6: PARANOID 0 warning, `-Wdocumentation` clean;
animation-debug's 68 FBX resources identical by name to the pre-split run, the Paladins animating, the viewer's
Paladin identical; 0 VUID) and Windows (MSVC Release + Debug 0 warning; the 53 clips identical by name, the viewer on
the Paladin and on a copy under a non-ASCII path `Ω-日/Paladin-Ω.fbx`; 0 VUID).

**Axis/unit conversion is delegated to ufbx at load time** via `ufbx_load_opts`:
- `target_axes = ufbx_axes_right_handed_y_up` — output is Y-up right-handed like glTF.
- `target_unit_meters = 1.0F` — output is in meters (1.0 = 1 m engine convention).
- `space_conversion = UFBX_SPACE_CONVERSION_MODIFY_GEOMETRY` — conversion baked into geometry, keeping node transforms clean.
- `generate_missing_normals = true` — ufbx provides smooth normals when the FBX lacks them.

**6-Phase Pipeline (same contract as GLTFLoader)** — each phase is an analysis step of `FBXParser.cpp` (anonymous
namespace) materialized by `FBXLoader::load()`:

| Phase | Analysis | Status | Output (materialized) |
|-------|--------|--------|--------|
| 1 | `describeImages()` | **implemented** | `ImageResource` |
| 2 | `describeMaterials()` | **implemented** | `Material::StandardResource` + `Texture2D` on-demand |
| 3 | `describeMeshes()` | **implemented** (+ skin influences/weights) | `IndexedVertexResource` + `MeshResource`/`MultiLayerMeshResource` |
| 4 | `describeSkins()` | **implemented** | `SkeletonResource` + `Skin` |
| 5 | `describeAnimations()` | **implemented** (⚠ coord-space bug — see note below) | `AnimationClipResource` |
| 6 | `describeNodes()` | **implemented** | `NodeDescriptor` hierarchy in `SceneData` |

**Mesh loading specifics:**

- Per-face triangulation via `ufbx_triangulate_face` with a buffer sized by `mesh.max_face_triangles`.
- Multi-material meshes are split into sub-geometry groups (one per `ufbx_mesh_part`).
- Per-corner vertex emission (position/normal/UV via `ufbx_get_vertex_vec3`/`vec2`) — the same vertex is written 3 times per triangle. A deduplication pass via `ufbx_generate_indices` can be added later if the overhead becomes measurable.
- **UV V-flip on read** — FBX stores UVs with V=0 at the bottom (OpenGL convention) ; the engine and Vulkan use V=0 at the top, matching glTF. The loader stores `(u, 1.0F - v)` in the vertex stream so embedded textures sample the correct region. **Without this flip, FBX models render with shuffled / black-region UVs** (regression marker — see `Paladin` recette below).
- Winding pre-compensates for the 180° X rotation applied by `SceneDataConsumer` (indices 1 and 2 swapped), identical to GLTFLoader.
- Materials are resolved per-part via `mesh.materials.data[partIdx]->typed_id` → the materialized material, falling back to the default `StandardResource` when the FBX has no material connected.

**Image/material loading specifics:**

- `describeImages()` supports both **embedded content** (`ufbx_texture.content`) and **external file references** (tries `texture.filename`, `absolute_filename`, `relative_filename` in order). Format detected from filename extension (PNG/JPEG/Targa).
- `describeMaterials()` maps `ufbx_material.pbr` (`ufbx_material_pbr_maps`) to `StandardResource` components:
  - `base_color` → `setAlbedoComponent` (sRGB)
  - `roughness` → `setRoughnessComponent`
  - `metalness` → `setMetalnessComponent`
  - `normal_map` → `setNormalComponent`
  - `ambient_occlusion` → `setAmbientOcclusionComponent`
  - `emission_color` + `emission_factor` → `setAutoIlluminationComponent` (sRGB)
  - `opacity` < 1.0 or `opacity.texture != nullptr` → alpha blending enabled
- Texture resolution is **cached on demand** while the materials are materialized: each (texture `typed_id`, colour space) is turned into a `Texture2D` exactly once. Multiple materials sharing a texture reuse the same engine resource.

**Skinning specifics:**

- A single skin deformer per mesh is supported (`mesh.skin_deformers.data[0]`). Multiple deformers stacked on the same mesh fall back to the first one.
- **cluster_index equals joint_index in the Skeleton** — the skeleton is built in the exact same order as `skin.clusters[]`, so no remapping layer is needed between vertex influences and joint matrices.
- Per-vertex influences/weights are emitted during `describeMeshes()`: up to **4 bones per vertex**, sourced from ufbx's weight-sorted `skin_vertex.weights[]` (ufbx guarantees descending sort). Weights are normalised to sum to 1.0 since ufbx does not guarantee it.
- Parent joint resolution walks `bone_node->parent` until another cluster's bone is hit; isolated clusters map to `NoParent` (skeleton root).
- `cluster->geometry_to_bone` is used as the inverse bind matrix (local vertex → bone space), converted from ufbx's 3×4 affine into the engine's 4×4 column-major `Matrix<4, float>` via `convertUfbxMatrix`.
- A skinned mesh carries its skin deformer's index (`FBX::Mesh::skin`); at the end of `load()`, each skinned renderable gets `setSkeletalData(skeleton, skin, clips)` via `Renderable::SkeletalDataTrait`, exactly like GLTFLoader.
- The bone nodes' DESCRIPTOR indices are added to `SceneData::skinJointNodeIndices` so the scene consumer does not instantiate them as regular scene nodes — joint transforms are owned by the `SkeletalAnimator`.

**LoaderOptions support:**

- `skipSkinning` — bypasses `describeSkins()`, `describeAnimations()` and per-vertex influence emission in `describeMeshes()`. A mesh that would otherwise have been skinned is loaded as a static pose.
- `excludedNodeNames` — matched against `ufbx_node.name` during `describeNodes()`. Any node whose own name **or any ancestor's name** is in the set is dropped from `SceneData::nodes` along with its entire subtree. Handy for stripping rig helpers, dummies, LOD levels or debug locators.

**Animation specifics (étape 4):**

- One `AnimationClip<float>` is generated per `ufbx_anim_stack`, named after the stack.
- Keyframes are resampled at **30 Hz** via `ufbx_evaluate_transform(stack.anim, bone, time)` — game-engine canonical rate, matches Mixamo's default. High-frequency curves (> 30 Hz) alias, acceptable for skeletal motion.
- Every joint produces three channels (Translation / Rotation / Scale), with `jointIndex` = `cluster_index` inside the **first** skin deformer (`scene.skin_deformers.data[0]`). Mixamo and most pipelines keep bone ordering consistent across every skin of the same rig, so one clip set attaches cleanly to every skinned mesh.
- Clips are materialized flat and handed in bulk to every `SkeletalDataTrait::setSkeletalData(...)` call — identical to GLTFLoader's pattern.
- Per-stack sampling lives in the helper `sampleAnimStack(stack, jointToNode, uniformScale)` (`FBXParser.cpp`). `describeAnimations()` builds `jointToNode` from the reference skin's clusters; `FBX::analyseClipsOnly()` (behind `loadAnimationClipsOnly()`) builds it by name lookup against an external skeleton. Both call paths share the helper to guarantee identical 30 Hz / `ufbx_evaluate_transform` semantics.

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

**Resource naming:** `FBX:{stem}/{Category}/{name}-{index}` (e.g.,
`FBX:X Bot/Material/Alpha_Body_MAT-0`), textures adding `-srgb` / `-data`. ⚠️ Duplicate names are
*ordinary* in FBX — every instance of an authored object carries the same one — so the index is
what keeps two instances from sharing one renderable; see *The resource key* above. The unnamed
fallback is now the **loop index**, no longer `mesh.element_id`.

**Recette assets** (demo `./projet-alpha --load-demo fbx-loader`):
- **Option 0 — Mixamo X Bot** (`data/data-stores/FBX/Mixamo/X Bot.fbx`): validates materials color path, skinning pipeline (2 skin deformers + 2 meshes), animation clip construction (2 anim stacks). **Still exhibits the legacy dislocation bug** — kept as a regression marker.
- **Option 1 — Intel Knight** (`data/data-stores/FBX/Knight/...`): Maya USD Preview Surface quirk (textures stripped). Skinning guard filters it out → renders as clean static T-pose.
- **Option 2 — Paladin** (`data/data-stores/FBX/Paladin/`): full split-animation workflow. `base_model.fbx` (rig + skin + bind pose) + 48 per-action `.fbx` files in the same folder, loaded via `loadAnimationClipsOnly` and bound to the rig by joint name. `slash_1` is placed at index 0 and auto-loops at lazy-init time. **Animation pipeline validated end-to-end** (no dislocation). **UV bug resolved** (V-flip on read, see *Mesh loading specifics* above). **Dark-render investigation closed**: turned out to be the expected rendering with the default opt-out IBL — a material must explicitly call `setReflectionComponentFromEnvironmentCubemap()` to consume the scene's environment cubemap, otherwise it renders markedly darker under the same direct lighting. The `projet-alpha` demo wires this via the `onMeshLoaded` hook (see `src/Builtin/FBXLoader.cpp`).
