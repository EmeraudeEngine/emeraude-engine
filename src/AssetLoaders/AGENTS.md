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
};
```

`LoaderOptions` controls resource loading behavior:
- `excludedNodeNames` — skip specific nodes and their subtrees
- `skipSkinning` — skip bone weights, skins, and animations

**Note:** `flattenHierarchy` is NOT in `LoaderOptions` — it only affects scene building and belongs in `Scenes::AssetDataConsumer`.

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

### FBXLoader (Stub)

Proves the interface architecture. Returns `false` with a warning. Ready for future implementation.

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
