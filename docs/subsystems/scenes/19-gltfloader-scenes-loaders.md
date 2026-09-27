## GLTFLoader → Scenes::Loaders (Refactored)

> **MOVED:** `Scenes::GLTFLoader` has been refactored into `Scenes::Loaders::GLTFLoader` (`src/Scenes/Loaders/`).
> The loader no longer depends on Scenes/ types. See [`@Scenes/Loaders/AGENTS.md`](../../../src/Scenes/Loaders/AGENTS.md) for the full loader documentation.
>
> Scene-side consumption is now handled by `Scenes::SceneDataConsumer`.

### Overview

`SceneDataConsumer` (`Scenes/SceneDataConsumer.hpp`) builds scene objects from an `Scenes::Loaders::SceneData`.

### Two Operating Modes

`SceneDataConsumer::build()` operates in one of two modes:

| Mode | Condition | Entity Type | Use Case |
|------|-----------|-------------|----------|
| **StaticEntity** | `parentNode == nullptr` | `StaticEntity` (flat, AABB culling) | Static scene geometry (buildings, props) |
| **Node** | `parentNode != nullptr` | `Node` (hierarchical, parent-relative) | Animated models, attachments, dynamic objects |

```cpp
// Step 1: Load resources (no Scene dependency)
Scenes::Loaders::GLTFLoader loader{act.resourceManager()};
Scenes::Loaders::SceneData sceneData;
loader.load(gltfPath, sceneData);

// Step 2: Build scene hierarchy
Scenes::SceneDataConsumer consumer;
consumer.setCreateLights(true);                  // OFF by default — see the warning below
consumer.setDirectionalLightShadows({.shadowMapResolution = 4096, .shadowCoverage = 200.0F});  // the caller's budget, never asset data
consumer.build(sceneData, scene);                // StaticEntity mode
consumer.build(sceneData, scene, parentNode);    // Node mode
```

> [!WARNING]
> **`setCreateLights()` is OFF by default, deliberately** (owner decision, 2026-08-08). A demo
> that lights its own scene must never have an asset's emitters appear behind its back: a
> photometric calibration is a whole, and uninvited lights silently rebalance the exposure the
> scene was tuned for. Turn it on only when the asset **is** the lighting authority.
>
> Two consequences inside the consumer, both easy to break:
> - a **light-only node survives flattening** — dropping it would drop the emitter with it;
> - in `StaticEntity` mode a node with a light but **no mesh still gets an entity**, since the
>   emitter needs an owner.
>
> **Three rules of `attachLight()` (2026-09-13, found on Intel's Sponza — the first asset-lit demo):**
> - **A directional light AIMS along its node's local -Z** (`KHR_lights_punctual` and UsdLux
>   alike): the component is built with `useDirectionVector(true)` so it reads the entity frame's
>   FORWARD vector. It used to keep the component default — "from my position toward the origin" —
>   which was 7° off on Sponza's `SUN`, a zero vector for a light node at the origin, and ignored
>   the frame the USD loader carefully bakes for a `DistantLight`.
> - **A light with `intensity <= 0` is NOT instantiated** (one `TraceWarning` naming the node).
>   Exporters do ship them: every one of Sponza's 24 lights left 3ds Max with `intensity: 0`, and
>   a dome light degrades to a null point light. Built anyway, a 0 cd point light gets a culling
>   radius of 0 = UNBOUNDED reach, bound to every draw for nothing. Fix the DATA
>   (`tools/gltf-lights.py --set NAME=VALUE` writes the intensities into a .glb/.gltf), never the
>   consumer.
> - **The shadow policy of the directional lights is the CALLER's**:
>   `setDirectionalLightShadows(DirectionalShadowOptions)` — the SAME struct
>   `Scene::applyBackgroundLighting()` takes for a sky's celestial bodies
>   (`Scenes/DirectionalShadowOptions.hpp`, one type, one dispatch `build()` between the three
>   `DirectionalLight` constructors). No format carries a shadow map; it is a runtime budget.
>   Default: no shadow map.
>
> Cameras declared by an asset are **never instantiated** — they stay data in `SceneData`.

### Configuration Options

**On the loader** (affects resource loading):

| Setter | Default | Effect |
|--------|---------|--------|
| `LoaderOptions::skipSkinning` | `false` | Skip phases 4-5, ignore bone weights (load as static mesh) |
| `LoaderOptions::excludedNodeNames` | empty | Skip named nodes and their subtrees entirely |

**On the consumer** (affects scene building):

| Setter | Default | Effect |
|--------|---------|--------|
| `setFlattenHierarchy(true)` | `false` | Skip intermediate nodes, attach all meshes directly to parent |
| `setCreateLights(true)` | `false` | Instantiate the asset's punctual lights (see the warning above) |
| `setDirectionalLightShadows(options)` | no shadow map | Shadow policy of the asset's directional lights (`DirectionalShadowOptions`: classic map or CSM) |
| `setInstanceTargetPerCell(n)` | `1024` | Population of one spatial cell of an instance set |

### Lighting Is Carried By the Descriptor (`MeshDescriptor::lightingEnabled`)

`SceneDataConsumer` used to call `visual.getRenderableInstance()->enableLighting()`
**unconditionally** at FIVE sites (both operating modes, hierarchy and flatten paths). All five now
honour `Scenes::Loaders::MeshDescriptor::lightingEnabled`, through
`Graphics::RenderableInstance::Abstract::setLightingState(bool)` — the symmetric form of
`enableLighting()`, implemented with `enableFlag`/`disableFlag` because
`Base::FlagTrait< uint32_t >` offers no `setFlag(flag, state)`, and adding one to emeraude-base is
barred by the "Ave robustus!" feature freeze.

- **Default is `true`** (boolean last in the struct layout) → **glTF and FBX behaviour is
  unchanged**: a mesh coming from a lit format expects the light set, the ambient pass and the
  environment IBL.
- `Scenes::Loaders::WADLoader` sets `lightingEnabled = false` on its level mesh — the Doom sector light
  levels are already baked into the vertex colors, so re-lighting would double-count them.

> [!WARNING]
> **This fixes a LATENT defect and is INERT today.** The dispatch test is
> `m_lightSet.isEnabled() && renderableInstance->isLightingEnabled()` (`Scene.rendering.cpp`), and
> the light set is only ever enabled by `Scene::applyBackgroundLighting()` (or a
> `DefinitionResource`, or the console). The WAD demo installs a background but never calls it, so
> the flag currently changes nothing that reaches the screen — **no measurement demonstrates it**,
> and it must not be presented as if one did. It would bite the moment any demo enabled the light
> set with a WAD level loaded. Owner decision: keep it, and document it as latent.

### Node Mode Behavior

**Default (hierarchy preserved):** `processNodeAsNode()` recursively walks the glTF node tree. Automatic **identity flattening** skips nodes that have no mesh and no transform, reducing unnecessary depth.

**Flatten mode:** All meshes attach directly to the `parentNode`, ignoring intermediate glTF structural nodes. The first mesh attaches to the parent itself; subsequent meshes create children.

**Joint node skipping:** Nodes that are skeleton joints (but carry no mesh) are skipped — their transforms are driven by `SkeletalAnimator`, not the scene graph.

> [!WARNING]
> **Node mode entities are Nodes, not StaticEntities.** Code that uses `findStaticEntity()` will NOT
> find entities created in Node mode. Use `scene.root()->findChild(name)` instead.

### Coordinate System Conversion — there is NONE (since Aug 2026)

glTF uses **Y-up, right-handed, `-Z` forward** coordinates. So does the engine. **The import is the
IDENTITY**: no rotation, no mirror, no per-asset flag.

> [!CAUTION]
> **Do not reintroduce the 180° X rotation, and do not reintroduce the winding swap.** Until Aug 2026
> the engine was Y-DOWN, the consumer applied a 180° X rotation to the root, and all four loaders
> swapped triangle indices 1↔2 — the latter justified by the claim that *"the 180° rotation flips the
> winding"*. That claim is **false**: a rotation has determinant +1 and NEVER inverts a winding. The
> swap was compensating an orientation-reversing projection, which is the defect the Y-up flip fixed
> at its root. Both the rotation and the swap are **deleted**.

If an imported asset looks mirrored or inside-out, the cause is **not** here — measure it
(`docs/coordinate-system.md` § *The measurement that proves it*) before compensating anywhere.

### Resource Naming Convention

All resources use a prefix derived from the filename: `glTF:{stem}/`

| Category | Pattern | Example |
|----------|---------|---------|
| Images | `glTF:Fox/Image/{name}` | `glTF:Fox/Image/Texture` |
| Materials | `glTF:Fox/Material/{name}` | `glTF:Fox/Material/fox_material` |
| Geometry | `glTF:Fox/Geometry/{name}` | `glTF:Fox/Geometry/fox1` |
| Meshes | `glTF:Fox/Mesh/{name}` | `glTF:Fox/Mesh/fox1` |
| Nodes | `glTF:Fox/Node/{name}` | `glTF:Fox/Node/root` |
| Skeletons | `glTF:Fox/skeleton/{name}` | `glTF:Fox/skeleton/Armature` |
| Animations | `glTF:Fox/animation/{name}` | `glTF:Fox/animation/Run` |

When a glTF object has no name, the numeric index is used as fallback.

### Default Resource on Every Error Path (MANDATORY)

Every resource slot must contain a valid resource — never nullptr. On any loading error, the loader stores the container's default resource and continues. This respects the engine's fail-safe philosophy.

### Lambda Capture Safety (CRITICAL)

GLTFLoader is stack-allocated and destroyed when `onBuilding()` returns. Async lambdas passed to `getOrCreateResource()` execute on the thread pool **after** the loader may be destroyed.

**Rules:**
1. **NEVER capture `this`** in async lambdas
2. **Pre-resolve** all `shared_ptr` data before the lambda
3. **Copy scalars by value** (colors, factors, indices)
4. **Move-capture** vectors of shared_ptr to avoid atomic refcount overhead

```cpp
// WRONG — dangling this
->getOrCreateResource(name, [this, idx] (auto & res) {
    return res.load(m_images[idx]);  // this is dead!
});

// CORRECT — self-contained lambda
->getOrCreateResource(name, [image = m_images[idx]] (auto & res) {
    return res.load(image);
});
```

### PBR Material Features

Textures are created **on-demand during material loading** with the correct sRGB flag based on material semantic. Supported components:
- Albedo (sRGB), Metallic-Roughness, Normal, Ambient Occlusion, Emissive (sRGB)
- Clear coat (KHR_materials_clearcoat), Sheen (KHR_materials_sheen)
- Transmission (KHR_materials_transmission), Iridescence (KHR_materials_iridescence)
- Alpha mode: OPAQUE / MASK / BLEND

### Performance Optimizations

- **String allocation**: `reserve + append` instead of concatenation temporaries
- **Tri-buffer streaming**: 3-element stack buffer replaces per-primitive heap vector for index building
- **Move-capture**: `[materialList = std::move(materialList)]` avoids N atomic refcount increments
- **Two-pass shape building**: first pass counts vertices/triangles, second pass fills

### Code References

- `Loaders/GLTFLoader.hpp/.cpp` — Resource loading (phases 1-6). See [`@Scenes/Loaders/AGENTS.md`](../../../src/Scenes/Loaders/AGENTS.md)
- `Loaders/SceneData.hpp` — Common intermediate format (NodeDescriptor, MeshDescriptor — the latter carries `lightingEnabled`, default `true`)
- `Graphics/RenderableInstance/Abstract.hpp:setLightingState()` — Symmetric form of `enableLighting()`, honoured at the consumer's five visual-setup sites
- `Loaders/Interface.hpp` — Loader interface + LoaderOptions
- `Scenes/SceneDataConsumer.hpp/.cpp` — Scene builder (StaticEntity/Node modes, Y-up conversion)
- `Graphics/Renderable/SimpleMeshResource.cpp:load(path)` — Transparent single-mesh glTF loading
- `Graphics/Renderable/MeshResource.cpp:load(path)` — Transparent multi-material glTF loading
