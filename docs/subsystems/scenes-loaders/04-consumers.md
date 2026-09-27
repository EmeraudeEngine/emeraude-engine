## Consumers

### Scenes::SceneDataConsumer (`Scenes/SceneDataConsumer.hpp`)

Transforms `SceneData` into scene objects:

```cpp
SceneDataConsumer consumer;
consumer.setFlattenHierarchy(false);  // optional
consumer.build(sceneData, scene);               // StaticEntity mode
consumer.build(sceneData, scene, parentNode);   // Node mode
```

Applies **no** axis conversion — the engine world is Y-up like glTF/USD/FBX, so the import is the IDENTITY. ⚠️ It used to apply a 180° X rotation on `parentNode`; that rotation is **deleted in both branches** (`SceneDataConsumer.cpp`). Do not restore it.

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
