## Important Files

- `Manager.cpp/.hpp` - SceneManager, multiple Scenes management + ActiveScene
- `Scene.hpp` - Scene class declaration (~2260 lines), organized by concept
- `Scene.cpp` - Core lifecycle, audio, octree management
- `Scene.entities.cpp` - Node tree, static entities, modifiers
- `Scene.lighting.cpp` - `applyBackgroundLighting()` and its deferred apply, ambient light properties, CSM cascades, environment IBL
- `Scene.physics.cpp` - Collision detection, boundary clipping, sleep/wake collision. See [`@Physics/AGENTS.md`](../../../src/Physics/AGENTS.md) for normal convention
- `Scene.rendering.cpp` - Render targets, shadow casting, rendering pipeline
- `Scene.debug.cpp` - Debug displays (compass, ground zero, boundary planes, octrees)
- `Debug/Compass.cpp/.hpp` - Orientation compass, drawn **after** the post-process chain. ⚠️ **NOT a scene entity** — see "Debug Helpers and the Exposure Trap"
- `NodeController.cpp/.hpp` - Keyboard/gamepad debug node manipulator (value member of Scene)
- `OrbitController.cpp/.hpp` - Pointer-driven camera orbit around a fixed target (value member of Scene). See "Controllers"
- `Viewers/ImageViewer.cpp/.hpp` - Builds the `+ImageViewer` scene (unlit picture quad). See "Viewer Scenes"
- `Viewers/ModelViewer.cpp/.hpp` - Builds the `+ModelViewer` scene (composite asset showcase). See "Viewer Scenes".
  Also owns the viewer's animation contract: the asset opens **at rest** (`enableAutoPlayFirstClip(false)`
  on every skeletal renderable), `clipNames()` hands the deduplicated clip list to `Core`, and the static
  `applyAnimation(scene, clipNames, index)` drives **whichever evaluator answers** — index 0 being the rest
  pose. ⚠️ The names collected are the CLIPS' own names (`clip().name()`), never the loaders' prefixed
  resource keys: feeding a key to `play()` looks up something that never existed and returns false, silently.
  ⚠️ Both `animationClips` and `nodeAnimationClips` are read and deduplicated by name — one glTF animation
  driving skin joints AND plain nodes comes out of the loader SPLIT into two clips sharing a name
- `Node.cpp/.hpp` - Hierarchical dynamic entity (tree)
- `NodeCrawler.hpp` - Header-only tree iterator. ⚠️ **Never yields the base node** — see "Node Tree Iteration — NodeCrawler Contract"
- `StaticEntity.cpp/.hpp` - Optimized static entity (flat map)
- `AbstractEntity.cpp/.hpp` - Common base for Component management
- `LocatableInterface.cpp/.hpp` - Interface for coordinates/movement
- `Toolkit.cpp/.hpp` - High-level scene construction helper. See [`@docs/toolkit-system.md`](../../toolkit-system.md)
- `Component/Abstract.hpp` - Base class for all Components (pure virtual onSuspend/onWakeup)
- `Component/SoundEmitter.cpp/.hpp` - Audio emitter with suspend/wakeup source management
- `InfluenceAreaInterface.hpp` - Pure virtual interface for modifier influence zones
- `SphericalInfluenceArea.cpp/.hpp` - Spherical influence with inner/outer radius falloff
- `CubicInfluenceArea.cpp/.hpp` - Oriented box influence with local space transform
- `Component/SphericalPushModifier.cpp/.hpp` - Radial push force modifier
- `Component/DirectionalPushModifier.cpp/.hpp` - Directional push force modifier
- `@docs/scene-graph-architecture.md` - **Complete detailed architecture**
- `@docs/coordinate-system.md` - Y-up convention (CRITICAL)
