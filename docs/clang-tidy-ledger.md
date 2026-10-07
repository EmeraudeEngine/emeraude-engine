# clang-tidy ledger — emeraude-engine

> Owner decision 2026-09-30, plan Ave Robustus § 3 (projet-alpha `docs/plans/ave-robustus.md`). The configuration is
> this repository's `.clang-tidy`. Gate on every change: ZERO NEW finding on the touched translation units; a finding
> is fixed, never silenced. A FULL run is a long process ordered by the owner; its results are recorded here.

## How to run

**Version: clang-tidy 21.1.6** (owner decision 2026-09-30). clang-tidy 19 (Debian's) SEGFAULTS in
`modernize-use-designated-initializers` on engine `src/CoreTypes.hpp` (`EngineContext`: a reference member
brace-initialised with a forward-declared type) — 8 of the 12 Console TUs, most of the engine. 21 does not crash.
Without sudo, from PyPI (the official LLVM binaries) in a scratch virtual environment:

```bash
python3 -m venv <scratch>/ctvenv && <scratch>/ctvenv/bin/pip install "clang-tidy==21.1.6"
# One translation unit, with the compile database of a Claude build directory (never cmake-build-*) — cleaned of the
# GCC-only flags (-flto*, -fno-fat-lto-objects, -fuse-linker*) and with clang++ as the driver:
<scratch>/ctvenv/bin/clang-tidy -p <cleaned db dir> --quiet <file.cpp> > tidy.log 2>&1; echo EXIT=$?
```

Redirect, never pipe. Count the findings by check (`[check-name]` at the end of each warning line), keeping only
those whose file is inside the module (the header filter also reports every included header).

## Last full run per module

| Module | Date | Findings by check | Notes |
|---|---|---|---|
| ~38 files of `src/` (every `onNotification()` / `notify()` signature: `const Base::Any &` instead of `const std::any &`), checked through `src/Scenes/AbstractEntity.cpp` | 2026-10-07 | clang-tidy 21.1.6: 0 on the changed lines (signature-only change). | base rtti-free-observer-payload |
| `src/Net/TCPClient.cpp` (`connect()` through base `Network::connectFirstReachable()`, Happy Eyeballs) | 2026-10-07 | clang-tidy 21.1.6: 0 on the changed lines (the file's 5 older findings are outside them). | base httpsclient-happy-eyeballs |
| `src/Graphics/TextureCache.cpp` (directory, erase and streams through base `IO::`) | 2026-10-07 | clang-tidy 21.1.6: 0 on the changed lines. | base windows-long-paths |
| `src/Core.cpp`, `Tracer.cpp` / `TracerLogger.hpp`, `Audio/ExternalInput.*`, `Audio/Recorder.*`, `Audio/TrackMixer.*`, `Console/RemoteListener.*`, `Graphics/Recorder.*`, `PlatformSpecific/Helpers.linux.cpp`, `Scenes/Component/ParticlesEmitter.cpp` (threads on base `Base::Thread`) | 2026-10-07 | clang-tidy 21.1.6: 1 on the changed lines, fixed (readability-ambiguous-smartptr-reset-call in `ParticlesEmitter`). 0 left. `Notification.windows.cpp` not analysable on Linux. | base non-throwing-thread-start |
| `src/Saphir/ShaderManager.cpp`, `src/Graphics/Renderer.cpp` (cache commits through base `IO::renameFile()`), `src/Scenes/Editor/Manager.cpp` (comment) | 2026-10-07 | clang-tidy 21.1.6: 0 on the changed lines. | base windows-long-paths |
| `src/Scenes/Scene.physics.cpp` (`PhysicsStepMark`, bodies need a model), `Scene.entities.cpp` (deferred content notification, withdrawn model leaves the physics octree), `Scene.cpp` (the queue handled after the step), `AbstractEntity.cpp` / `.hpp` (`isCollisionModelWithdrawn()`), `Scene.hpp`, `Node.cpp` (a withdrawn model is not integrated) | 2026-10-07 | clang-tidy 21.1.6: 1 on the changed lines, fixed (readability-redundant-member-init on the `std::atomic< std::thread::id >`). 0 left. | Physics-step self-deadlock |
| `src/Graphics/Geometry/Interface.cpp` (identity RT index list for a non-indexed triangle list) | 2026-10-07 | clang-tidy 21.1.6: 0 on the changed lines. | RT NULL index address |
| `src/Resources/ResourceTrait.cpp` / `.hpp` (`checkDependencies()` claims `onDependenciesLoaded()`) | 2026-10-07 | clang-tidy 21.1.6: 0 on the changed lines (the TU's 5 misc-no-recursion + 1 use-enum-class predate them). | Resource finalization race |
| `src/Graphics/Renderable/MeshResource.cpp`, `MultiLayerMeshResource.cpp` (automatic LOD skips a mesh carrying its own levels) | 2026-10-07 | clang-tidy 21.1.6: 0 on the changed lines. | Automatic LOD |
| `src/Vulkan/PendingSubmissions.*` (new), the changed lines of `Queue.cpp` (timeline), `Device.cpp` (`destroyAfter()`), `Buffer.cpp`, `Image.cpp`, `ImageTransferOperation.cpp`, `BufferTransferOperation.cpp`, `Instance.cpp`, `Graphics/Renderer.cpp`; headers `DeferredDestructor.hpp`, `Queue.hpp`, `Device.hpp` | 2026-10-07 | clang-tidy 21.1.6: 6 on the changed lines — 1 fixed (prefer-member-initializer, the `PendingSubmissions` move constructor), 5 ON PURPOSE: readability-qualified-auto ×5 on Vulkan / VMA handles (`const auto` kept: a non-dispatchable handle is a pointer on some platforms only, caution-points § qualified-auto). | Upload lifetime (queue timelines) |
| `src/Graphics/DeferredLightResolve.cpp` (new) + the changed lines of `Renderer.cpp`, `Renderer.console.cpp`, `SceneRenderTarget.cpp`, `Material/StandardResource.cpp`, `Scenes/Scene.rendering.cpp`, `Saphir/LightGenerator.cpp` and their headers | 2026-10-04 | clang-tidy 21.1.6: 12 fixed in the new file (5 avoid-c-arrays: the GPU structs → `std::array`; 6 pro-bounds-constant-array-index: the published-block offsets made template parameters; 1 array-to-pointer-decay). 0 on the changed lines. 0 left. | Deferred punctual lights |
| `src/Scenes/Loaders/GLTFLoader.cpp` (the cutout override), `LoaderOptions.hpp` (`cutoutMaterialNames`) | 2026-10-04 | clang-tidy 21.1.6: 0 on the changed lines. | Sponza cypress cutout |
| `src/Vulkan/Device.cpp` (heap budgets, VMA report), `src/Graphics/Renderer.cpp` + `.console.cpp` (budget warning, `getGPUMemory`), `src/Scenes/Loaders/GeometryDeduplicator.*` (new), `USDLoader.cpp` (2026-10-04) | 2026-10-04 | clang-tidy 21.1.6 on new and changed lines: 1 ON PURPOSE — misc-no-recursion on `collectAllocations()` (`Renderer.console.cpp`): a walk of VMA's statistics JSON, bounded at depth 16, whose shape is VMA's (4 levels deep). Fixed: 17 designated initializers, a missing `std::forward`, 3 C-array subscripts and 2 decays (`std::span{array}.first(count)`). | Partial (touched files) |
| `src/Resources/SharingServer.cpp`, `PeerStore.cpp` (new), `Manager.cpp`, `Manager.console.cpp`, `src/Console/MCP/Server.cpp` (on base `HTTPServer`), `src/Net/Manager.cpp` (peers), `src/Scenes/Loaders/USDLoader.cpp` (resource sharing, 2026-10-04) | 2026-10-04 | clang-tidy 21.1.6 on new and changed lines: 1 ON PURPOSE — cppcoreguidelines-pro-type-reinterpret-cast in `SharingServer::fileSHA256()` (`char` → `uint8_t` for the hash: an iostream reads `char`, the hash takes bytes; the base ledger has the same pair). Fixed on the way: a moved `const` (performance-no-automatic-move), a by-value URI (unnecessary-value-param), a `std::move` into a const-reference parameter, a widening multiplication. `MCP/Server.cpp` lost its 21 misc-no-recursion: the asynchronous HTTP chain now lives in base (`HTTPServer`, its ledger). | Partial (touched files) |
| `src/Console` (+ `MCP/`), 12 TUs | 2026-09-30 | clang-tidy 21.1.6 after the triad pass: 24, all ON PURPOSE (below) — 21 misc-no-recursion, 1 avoid-c-arrays, 1 cppcoreguidelines-use-enum-class, 1 pro-type-reinterpret-cast. Before: 64 (clang-tidy 19, designated-initializers blind) / 66 (21). | Triad section 1, projet-alpha `docs/plans/triad-engine-pass-report.md` |
| `src/Resources`, 6 TUs (+ `Container.hpp` templates) | 2026-09-30 | clang-tidy 21.1.6 after the triad pass: 5, all ON PURPOSE (below) — 2 cppcoreguidelines-use-enum-class, 2 misc-no-recursion, 1 cppcoreguidelines-avoid-do-while. Before: 20. | Triad section 2 |
| `src/Scenes/Loaders`, 4 TUs (+ `SceneDataConsumer.cpp`) | 2026-09-30 | clang-tidy 21.1.6 after the triad pass: 172, all ON PURPOSE (below) — 141 pro-type-union-access, 18 pro-bounds-constant-array-index, 7 pro-type-reinterpret-cast, 3 avoid-const-or-ref-data-members, 1 Padding, 1 use-enum-class, 1 enum-size. Before: 221. | Triad section 3 |
| `src/Net`, 10 Linux TUs (`*.windows.cpp`, `*.mac.mm` read by hand) | 2026-09-30 | clang-tidy 21.1.6 after the triad pass: 39, all ON PURPOSE (below) — 20 reinterpret-cast, 7 make-member-function-const, 5 pro-type-vararg, 2 use-enum-class, 1 each array-to-pointer-decay, macro-usage, avoid-c-arrays, interfaces-global-init, constant-array-index. Before: 103. | Triad section 4 |
| `src/Input`, 6 TUs | 2026-09-30 | clang-tidy 21.1.6 after the triad pass: 40, all ON PURPOSE (below) — 28 pro-bounds-constant-array-index, 8 use-enum-class, 4 misc-confusable-identifiers. Before: 47. | Triad section 5 |
| `src/Scenes` 6a (scene graph core: Node, AbstractEntity, StaticEntity, controllers, OctreeSector, Scene.cpp / entities) | 2026-09-30 | clang-tidy 21.1.6 after the triad pass: 31, all ON PURPOSE (below) — 13 misc-no-recursion, 6 missing-std-forward, 4 use-enum-class, 4 constant-array-index, 2 static-cast-downcast, 1 avoid-const-or-ref-data-members. Before: 48. | Triad sub-section 6a |
| `src/Scenes` 6b (Scene rendering / lighting / physics / debug, LightSet, SceneInstanceTransforms, SceneMetaData, RenderBatch, InstanceCluster, BindlessTextureSet, CloudSet, influence areas, interfaces) | 2026-09-30 | clang-tidy 21.1.6 after the triad pass: 32, all ON PURPOSE (below) — 21 constant-array-index, 3 avoid-const-or-ref-data-members, 2 missing-std-forward, 2 reinterpret-cast, 2 const-cast, 1 use-enum-class, 1 static-cast-downcast. Before: 98. | Triad sub-section 6b |
| `src/Scenes` 6c (Manager + console, Toolkit, DefinitionResource) | 2026-09-30 | clang-tidy 21.1.6 after the triad pass: 7, all ON PURPOSE (below) — 5 constant-array-index, 1 misc-no-recursion, 1 use-enum-class. Before: 20. `Scene.hpp`'s 3 use-after-move (seen only from these TUs) fixed. The cascade-wide checked-JSON migration's touched TUs: 0 new finding on the changed lines but the one below (`Material/Helpers.cpp`). | Triad sub-section 6c |
| `src/Scenes` 6d (`Component/`) | 2026-09-30 | clang-tidy 21.1.6 after the triad pass: 40, all ON PURPOSE (below) — 22 constant-array-index, 9 use-enum-class, 3 non-private members, 2 non-const globals, 1 each mt-unsafe, missing-std-forward, const-ref member, static-cast downcast. Before: 75. | Triad sub-section 6d |
| `src/Scenes` 6e (`Editor/`, `AVConsole/`, `Viewers/`, `EffectsToolkit/`, `Debug/`) | 2026-10-01 | clang-tidy 21.1.6 after the triad pass: 38, all ON PURPOSE (below) — 11 constant-array-index, 9 pro-type-vararg, 6 non-private members, 5 switch-missing-default-case, 5 avoid-const-or-ref-data-members, 1 reinterpret-cast, 1 use-enum-class. Before: 78. | Triad sub-section 6e |
| `src/Graphics` 7a (resources read from disk: images, textures, KTX2, cubemaps, IBL, movies, font, cloud shape) | 2026-10-01 | clang-tidy 21.1.6 after the triad pass: 72, all ON PURPOSE (below) — 53 constant-array-index, 17 reinterpret-cast, 2 convert-member-functions-to-static. Before: 171. The touched resource-chain TUs: 3 misc-no-recursion (the failure propagation). | Triad sub-section 7a |
| `src/Graphics` 7b (`Material/`) | 2026-10-01 | clang-tidy 21.1.6 after the triad pass: 31, all ON PURPOSE (below) — 23 constant-array-index, 7 static-cast-downcast, 1 use-enum-class. Before: 56. | Triad sub-section 7b |
| `src/Physics` + `Scenes/Scene.physics.cpp` (physics overhaul P2: `SoftStepSolver`, `NarrowPhase`, the step) | 2026-10-01 | clang-tidy 21.1.6 on `SoftStepSolver.cpp`, `NarrowPhase.cpp`, `Scene.physics.cpp`, `MovableTrait.cpp`: 7 fixed (2 modernize-use-std-numbers, 1 nested conditional, 1 designated initializer, 3 parentheses); kept, in categories already ON PURPOSE below: 3 pro-type-static-cast-downcast (`NarrowPhase`, the `modelType()` tag switch, like the collision models), 2 misc-no-recursion (the octree walk), 3 avoid-const-or-ref-data-members (`GroundContactCollector`, a visitor that lives for one call). | P2 |
| `src/Physics` + `Scenes/Scene.physics.cpp` + `Scenes/GroundTriangles.hpp` (physics overhaul: the one-sided ground, its recovery, the continuous pass) | 2026-10-02 | clang-tidy 21.1.6 on `NarrowPhase.cpp`, `Scene.physics.cpp`, `Manager.console.cpp`, changed lines only: 5 fixed (2 constant-array-index and a throwing `std::array::at()` in the ground low points — since removed —, 1 designated initializer, 1 ranges sort, 1 DeMorgan, 1 nested min); kept ON PURPOSE: 5 avoid-const-or-ref-data-members (`GroundHeightProbe`, `ContinuousGroundSweep`: visitors that live for one call, like `GroundContactCollector`). | P2 ground / P5 |
| `src/Physics/CharacterController`, `Scenes/Component/CharacterController`, the character step of `Scene.physics.cpp` (physics overhaul P4) | 2026-10-02 | clang-tidy 21.1.6 on the new TUs and the changed lines: 0 to fix; kept ON PURPOSE: 1 use-enum-class (the component's `NotificationCode`, the Observer convention), 8 avoid-const-or-ref-data-members (`SceneCharacterWorld` and its two local visitors: they live for one call of the step). | P4 |
| `src/Input` (`KeyboardController::injectKeyState()`, the console `keyDown` / `keyUp`) and projet-alpha's walking actors (physics overhaul P4) | 2026-10-02 | clang-tidy 21.1.6, changed lines: 2 fixed (the injected-state copy as a `std::ranges::transform`); kept ON PURPOSE in the `src/Input` categories: 2 pro-bounds-constant-array-index (after the range check), 1 bugprone-dynamic-static-initializers (`s_injectedState`, declared like `s_deviceState`); projet-alpha `Player`, `Paladin`, `Fox`, `Abstract`: 0. | P4 |
| `Scenes/OctreeSector.hpp` (one element, one sector completed: `expand()` / `collapse()` / `update()` / `forEachElement()`), `Scene.cpp`, `Scene.physics.cpp`, `Physics/CharacterController.cpp` (flying), `Manager.console.cpp`; projet-alpha `Act`, `Player`, `Abstract` | 2026-10-02 | clang-tidy 21.1.6, changed lines: 1 fixed (missing-std-forward: `withPrimitive()` calls its callable once per path, now forwarded); kept ON PURPOSE in the `src/Scenes` 6a categories: misc-no-recursion (the octree walks `forEachElement` / `findOwnerAnywhere` / `gatherElements` / `subtreeElementCount` / `withPrimitive` → `insertWithPrimitive`, bounded by `DefaultMaxDepth`), 1 missing-std-forward (`forEachElement()` calls its callable once per element); `s_injectedState` (row above). | P4, octree |
| Physics P5 decision 14 (dynamic ↔ dynamic sweeps, box casts): `Scenes/Scene.physics.cpp`, `Physics/NarrowPhase.cpp`; projet-alpha `CollisionDebug` | 2026-10-02 | clang-tidy 21.1.6, changed lines: 1 fixed (modernize-use-emplace). | P5 |
| Physics wheeled vehicle (decisions 15): `Physics/Vehicle` (new), `Scenes/Component/Vehicle` (new), `Physics/SoftStepSolver` (the wheels), `Scenes/Scene.physics.cpp` (step 1d), `AbstractEntity`, `Manager.console.cpp`; projet-alpha `CollisionDebug` | 2026-10-02 | clang-tidy 21.1.6, new files and changed lines: 3 fixed (special-member-functions: `Component::Vehicle` deletes its copy / move like `Component::Abstract`; 2 avoid-const-or-ref-data-members: the wheel cast visitor holds its sphere and motion by value); kept ON PURPOSE in the category already below: 1 `cppcoreguidelines-pro-type-static-cast-downcast` (`TriangleMeshCollisionModel` after its `modelType()` tag, the wheel cast against a mesh). The base header: base `docs/clang-tidy-ledger.md`. The brake fix (the locked wheel, `SoftStepSolver.cpp`, 2026-10-02): changed lines 0. | Vehicle |
| `Graphics/IrradianceProbeVolume.cpp` (its ray rotation through base `PortableRandom::UniformReal`) | 2026-10-02 | clang-tidy 21.1.6, changed lines: 1 fixed (misc-const-correctness). 0 left. | Portable random |
| `Scenes/AbstractEntity.cpp` (`setCollisionModel()` notifies), `Node.cpp` / `StaticEntity.cpp` (`onContentModified()` through `weak_from_this()`) | 2026-10-02 | clang-tidy 21.1.6, changed lines: 0. | Spinner phase |
| `Scenes/Scene.cpp` (the crawl refiles a moved renderable node), `AbstractEntity.hpp` (`movedSinceFiled()` / `recordFiledFrame()`) | 2026-10-02 | clang-tidy 21.1.6, changed lines: 0. | Wheels refile |
| `Saphir/LightGenerator.cpp` (the thin-surface grab-pass transmission unit, the ambient diffuse weighted by the transmission) | 2026-10-03 | clang-tidy 21.1.6: 0 on the changed lines; the file's 10 findings (9 convert-to-static accessors, 1 branch-clone) predate them. | Glass fix |
| `Physics/Vehicle` (`VehicleSettings::maxSlopeAngle`), `Scenes/Scene.physics.cpp` (the wheel casts' slope limit) | 2026-10-03 | clang-tidy 21.1.6: 0 on the changed lines; the files' older findings are unchanged. | Vehicle slope |
| `Graphics/Material/StandardResource` (the emissive factor; the UV set in the UVW table, `setComponentUVWChannel()`, `usePrimaryTextureCoordinatesOnly()`), `Saphir/LightGenerator.cpp` (the ambient diffuse weight), `Scenes/Loaders/GLTFLoader` (`TEXCOORD_1`, the `…-uv0` variant), `Graphics/Geometry/IndexedVertexResource` / `VertexResource` | 2026-10-03 | clang-tidy 21.1.6, changed lines: 2 fixed (a nested conditional in `transformedTexCoords()`, a repeated branch in the per-primitive material choice); 5 left ON PURPOSE, the file's established patterns: 3 static-cast-downcast (a component to `Texture` after its `type()` check), 2 constant-array-index (the material property table at a computed offset). | Materials |
| `Vulkan/Utility` (`pipelineCreationFeedbackAvailable()`, `reportPipelineCreation()`), `GraphicsPipeline.cpp`, `ComputePipeline.cpp` (timed creations with feedback) | 2026-10-03 | clang-tidy 21.1.6: 0 on the new and changed lines; the pipelines' older findings unchanged. | Pipeline stalls |
| `Scenes/Scene.rendering.cpp` (instances prepared outside the components lock), `Tracer` (`currentThreadID()`), `Vulkan/Utility` + `GraphicsPipeline` (the report's label, thread, location), `Saphir/Generator/Abstract.cpp` (the label) | 2026-10-03 | clang-tidy 21.1.6: 0 on the new and changed lines (the reported `Tracer.hpp` lines are older). | Pipeline stalls |
| Physics P5: `Physics/TriangleMeshCollisionModel` (new), `NarrowPhase`, `MovableTrait`, `Scenes/Scene.physics.cpp` (islands, mesh pairs), `Node`, `AbstractEntity(.debug)`, `Editor/Manager`, the push modifiers, `Toolkit.hpp`; projet-alpha `CollisionDebug` | 2026-10-02 | clang-tidy 21.1.6, changed lines and new files: 1 fixed (missing-std-forward: `forEachWorldTriangle()` takes its visitor by const reference); kept ON PURPOSE in the categories already below: 4 `cppcoreguidelines-pro-type-static-cast-downcast` (`TriangleMeshCollisionModel` after its `modelType()` tag, as the other models); projet-alpha: 0. The base header's own findings: base `docs/clang-tidy-ledger.md`. | P5 |
| `src/Graphics` 7c (`Geometry/`, `Renderable/`, `MDI/`) | 2026-10-01 | clang-tidy 21.1.6 after the triad pass: 51, all ON PURPOSE (below) — 30 constant-array-index, 4 integer-division, 4 non-private members, 3 use-enum-class, 3 qualified-auto, 2 misc-no-recursion, 2 static-cast-downcast, 1 each special-member-functions, const-ref member, implicit-widening. Before: 95. | Triad sub-section 7c |
| `src/Graphics` 7d (`Renderer` + console, `RendererFrameScope`, `Recorder`, `FrameCapture`, `RenderDocCapture`) | 2026-10-01 | clang-tidy 21.1.6 after the triad pass: 91, all ON PURPOSE (below) — 28 SIMD intrinsics, 26 reinterpret-cast, 21 constant-array-index, 5 convert-to-static, 4 owning-memory, 3 union access, 2 qualified-auto, 1 each use-after-move (false positive), use-enum-class. Before: 226. | Triad sub-section 7d |
| `src/Graphics` 7e (`RenderTarget/`, `RenderableInstance/`, scene / intermediate / selection targets, view matrices, shared UBOs, bindless table, vertex formats, skinning, path overlay) | 2026-10-01 | clang-tidy 21.1.6 after the triad pass: 109, all ON PURPOSE (below) — 80 constant-array-index, 16 reinterpret-cast, 7 qualified-auto, 2 use-enum-class, 2 non-private members, 1 each convert-to-static, unused parameter. Before: 205. | Triad sub-section 7e |
| `src/Graphics` 7f (post-process: `PostProcessor`, `PostProcessStack`, the passes, `GIDenoiser`, `OverflowCensus`, `Effects/` Shared / Resolve / Camera / Style) | 2026-10-01 | clang-tidy 21.1.6 after the triad pass: 145, all ON PURPOSE (below) — 129 constant-array-index, 9 return-const-ref-from-parameter, 4 inefficient string concatenation, 2 macro-usage, 1 use-enum-class. Before: 192. | Triad sub-section 7f |
| `src/Graphics` 7g (lighting and atmosphere: `Effects/` Lighting, Atmosphere, `Compute/`, the probe volume, LTC, dummies, cloud shadow map, ocean waves, imposter atlas) | 2026-10-01 | clang-tidy 21.1.6 after the triad pass: 69, all ON PURPOSE (below) — 47 constant-array-index, 10 return-const-ref-from-parameter, 7 qualified-auto, 2 static-cast-downcast, 2 integer-division, 1 cert-msc51. Before: 184. | Triad sub-section 7g |
| `src/Saphir` 8a (shader core: `ShaderManager`, `Program`, the shader stages, `AbstractVertexStage`) | 2026-10-01 | clang-tidy 21.1.6 after the triad pass: 7, all ON PURPOSE (below). Before: 40. | Triad sub-section 8a |
| `src/Saphir` 8b (`LightGenerator`) | 2026-10-01 | clang-tidy 21.1.6 after the triad pass: 10, all ON PURPOSE (below) — 9 convert-member-functions-to-static, 1 branch-clone. Before: 15. | Triad sub-section 8b |
| `src/Saphir` 8c (`Declaration/`, `Generator/`) | 2026-10-01 | clang-tidy 21.1.6 after the triad pass: 10, all ON PURPOSE (below) — 7 reinterpret-cast, 2 branch-clone, 1 use-enum-class. Before: 23. | Triad sub-section 8c |
| `src/Vulkan` 9a (instance, device, presentation) | 2026-10-01 | clang-tidy 21.1.6 after the triad pass: 32, all ON PURPOSE (below) — 11 array-to-pointer decay, 10 reinterpret-cast, 5 convert-to-static, 4 constant-array-index, 1 each misplaced-const, dead store. Before: 59. | Triad sub-section 9a |
| `src/Vulkan` 9b (memory and resources) | 2026-10-01 | clang-tidy 21.1.6 after the triad pass: 48, all ON PURPOSE (below) — 24 reinterpret-cast, 10 qualified-auto, 5 const-correctness, 4 constant-array-index, 3 branch-clone, 2 array-to-pointer decay. Before: 116. | Triad sub-section 9b |
| `src/Vulkan` 9c (commands, pipelines, descriptors, sync) | 2026-10-01 | clang-tidy 21.1.6 after the triad pass: 21, all ON PURPOSE (below) — 15 reinterpret-cast, 2 constant-array-index, 2 vararg, 1 each convert-to-static, use-enum-class. Before: 45. | Triad sub-section 9c |
| `src/Audio` 10a (the audio core) | 2026-10-01 | clang-tidy 21.1.6 after the triad pass: 87, all ON PURPOSE (below) — 47 designated-initializers, 24 reinterpret-cast, 13 convert-to-static, 2 use-enum-class, 1 non-const global. Before: 121. | Triad sub-section 10a |
| `src/Audio` 10b (`Effects/`, `Filters/`, `EffectSlot`) | 2026-10-01 | clang-tidy 21.1.6 after the triad pass: 1, ON PURPOSE (below) — 1 convert-member-functions-to-static. Before: 1. | Triad sub-section 10b |
| `src/Physics` 11 | 2026-10-01 | clang-tidy 21.1.6 after the triad pass: 27, all ON PURPOSE (below) — 16 static-cast-downcast, 7 convert-member-functions-to-static, 2 constant-array-index, 1 use-enum-class, 1 special-member-functions. Before: 90. | Triad section 11 |
| `src/Animations` 12 (+ `Scenes/Loaders/GLTFLoader.cpp` changes) | 2026-10-01 | clang-tidy 21.1.6 after the triad pass: 0. Before: 5. `GLTFLoader.cpp`: 0 new finding on the changed lines (its 10 are the section 3 ones). The fix-its touched three base headers through the includes: reverted. | Triad section 12 |
| `src/Overlay` 13 | 2026-10-01 | clang-tidy 21.1.6 after the triad pass: 4, all ON PURPOSE (below) — 3 constant-array-index, 1 use-enum-class. Before: 22. | Triad section 13 |
| `src/PlatformSpecific` 14 (Linux TUs) | 2026-10-01 | clang-tidy 21.1.6 after the triad pass: 44, all ON PURPOSE (below) — 10 array-to-pointer decay, 10 union access, 10 vararg, 7 mt-unsafe, 4 non-private members, 1 branch-clone, 1 const-correctness, 1 reinterpret-cast. Before: 54. The Windows / macOS sources are not compiled here. | Triad section 14 |
| `src/Tool`, `src/Help`, root files 15 | 2026-10-01 | clang-tidy 21.1.6 after the triad pass: 38, all ON PURPOSE (below) — 10 vararg, 6 reinterpret-cast, 5 convert-to-static, 4 const members, 3 constant-array-index, 2 enum-class, 2 crtp-constructor, 2 non-private members, 1 mt-unsafe, 1 no-recursion, 1 branch-clone, 1 non-const global. Before: 69. | Triad section 15 |
| `src/Graphics/DeferredLightResolve.cpp`, `Renderer.cpp`, `Renderer.console.cpp`, `src/Scenes/LightSet.cpp`, `Scene.rendering.cpp` (the frame's light selection) | 2026-10-06 | clang-tidy 21.1.6, changed lines: 2 fixed (pro-bounds-constant-array-index: the classification read the published block at a runtime offset — the callers now read it at the constant offsets). 0 left. Then `src/Scenes/Component/AbstractLightEmitter.cpp` (`linkEmissiveMaterial()`), `src/Graphics/Material/StandardResource.cpp`, `BeamResource.cpp`, `PathResource.cpp` (atomic dirty flag): 0 on the changed lines. Then the tiled culling (`DeferredLightResolve.cpp`, `Renderer.console.cpp`): 0 on the changed lines. | Labyrinth lamps |
| `src/Console/RemoteListener.cpp` (graceful disconnect through base `GracefulCloser`) | 2026-10-06 | clang-tidy 21.1.6: 0 on the changed lines. | Graceful close |

## Findings kept ON PURPOSE

Each with its file:line, its check and the reason (an owner decision).

Known on purpose before this ledger: Saphir — the six warnings left on purpose
(`docs/subsystems/saphir/25-clang-tidy-the-six-warnings-that-are-left-on-purpose.md`).

### `src/Console` (owner decisions 2026-09-30)

- **misc-no-recursion ×21** — (a) the walks of the console tree (`Controller::dumpControllable`,
  `describeControllable`, MCP `appendTools`): the depth is the engine's own controllable hierarchy (~5 levels), never
  an input; (b) the asio async chains (`readRequest` → its completion handler → `readRequest`, `doRead`, `writeNext`,
  `startStream`, …): each call RETURNS before the next one runs, the stack does not grow.
- **avoid-c-arrays ×1** — `ControllableTrait::bindCommand(..., const Parameter (& parameters)[N], ...)`: the C-array
  reference is what deduces N from a brace list at the call site; `std::array` and `std::span` cannot.
- **cppcoreguidelines-use-enum-class ×1** — `Controller::NotificationCode`: the engine-wide Observer convention (24
  unscoped `NotificationCode` enums compared as `int` in `onNotification()`); changing one alone breaks the contract.
- **pro-type-reinterpret-cast ×1** — `RemoteListener.cpp` `setsockopt(..., SO_SNDTIMEO, reinterpret_cast< const char
  * >(&timeout), ...)`: the Windows signature of the system call.

### `src/Resources` (2026-09-30)

- **cppcoreguidelines-use-enum-class ×2** — `ResourceTrait::NotificationCode`, `Container::NotificationCode`: the
  Observer convention (see Console).
- **misc-no-recursion ×2** — `ResourceTrait::dependencyLoaded()` → `checkDependencies()` → a parent's
  `dependencyLoaded()`: the upward notification of a finished load. Its depth is the dependency chain, bounded by the
  resource TYPES (scene → mesh → material → texture → image), not by data. (The data-driven cycle check is iterative
  since 2026-09-30.)
- **cppcoreguidelines-avoid-do-while ×1** — `Manager::unloadUnusedResources()`: "one pass, then again while a pass
  frees something" (freeing a resource frees its dependencies on the next pass) — owner decision 2026-09-30.
- **portability-template-virtual-member-function ×17** (recorded 2026-10-04; 15 pre-existing, + `releaseIdleLocalData()`,
  `findResource()`)
  — `Container< resource_t >`: the `ContainerInterface` virtuals are implemented by the container TEMPLATE, the
  manager's type-erased access to every container (the design itself). Every specialisation is instantiated by
  `Manager::onInitialize()`, so no compiler leaves one out.

### `src/Scenes/Loaders` (2026-09-30)

- **pro-type-union-access ×143** — FBXLoader only: the ufbx API is unions (`ufbx_vec3::x`, …; +2 in
  `readEmbeddedTexture()`, 2026-10-04).
- **pro-bounds-constant-array-index ×18** — each one checked bounded: FBX (`texIdx` checked, `k < 3`, `wi < take ≤ 4`,
  ufbx indices consistent by construction), glTF (texture index checked, literal loops), USD (literal 4×4 loops), WAD
  (counts derived from the lump size; cross-references checked).
- **pro-type-reinterpret-cast ×7** — binary buffers (a WAD name, the file read, meshopt / Draco input — its offset is
  bounded upstream —, the USDZ asset map, Tydra's raw normal / UV buffers — sized by the vertex count check).
- **avoid-const-or-ref-data-members ×3** — each loader holds the `Resources::Manager &` it was built with: a
  short-lived, non-copyable object by design.
- **clang-analyzer-optin.performance.Padding ×1** — a lambda CLOSURE's layout (GLTFLoader).
- **use-enum-class + enum-size** — `LoaderCapabilityBits`: a bit-flag set combined with `|`, `uint32_t` on purpose.

### `src/Net` (2026-09-30)

- **pro-type-reinterpret-cast ×20** — the BSD socket API (`sockaddr_in` ↔ `sockaddr`, option buffers).
- **make-member-function-const ×7** — `UDPClient::bind` / `setBroadcast` / `setMulticast*`, `SerialPort::read` /
  `write`: they mutate the socket / port (logical mutation). Their fix-it also broke the Windows definitions.
- **pro-type-vararg ×5** — `ioctl()` / `fcntl()`, the POSIX system calls.
- **use-enum-class ×2** — `NotificationCode` (the Observer convention).
- **macro-usage, avoid-c-arrays** — `SerialPort.linux.cpp`'s private mirror of the kernel `termios2` (`BOTHER`, the
  `c_cc` array): it must match the kernel ABI byte for byte.
- **array-to-pointer-decay ×1** — a `char[]` buffer handed to a C API (`NetworkInterfaces.cpp`).
- **interfaces-global-init ×1** — `TCPServer::DefaultBacklog{asio::socket_base::max_listen_connections}`: that is
  `SOMAXCONN`, a constant expression (a false positive).
- **pro-bounds-constant-array-index ×1** — `UDPClient.cpp` NUL after `recvfrom(…, size - 1, …)`: bounded.

### `src/Input` (2026-09-30)

- **pro-bounds-constant-array-index ×28** — every device-state subscript now follows an explicit range check (the
  public-API rule of `docs/subsystems/input/01-…`).
- **use-enum-class ×8** — `Key`, `ModKey`, `MouseButton`, `Joystick*`, `Gamepad*`: they carry the GLFW integer codes
  and are compared / combined as such (a scoped enum would need a cast at every GLFW call).
- **misc-confusable-identifiers ×4** — `KeyI` / `Key1`, `KeyO` / `Key0` (and their `…String`): GLFW's own key names.

### `src/Scenes` 6a — scene graph core (2026-09-30)

- **misc-no-recursion ×13** — the node-tree walks (`Node::destroyTree` / `trimTree` / `destroyChildren` /
  `onLocationDataUpdate`) and the octree (`OctreeSector` insert / erase / expand / depth…): the node depth is bounded by
  `Node::MaxDepth` (256, owner decision 2026-09-30), the octree depth by its own maximum depth setting.
- **missing-std-forward ×6** — callables invoked once per element (`forEachComponent`, `forEachModifiers`,
  `OctreeSector` visitors): forwarding inside a loop would be the bug (use-after-move).
- **use-enum-class ×4** — `NotificationCode` (the Observer convention).
- **pro-bounds-constant-array-index ×4** — the render-state slot (`RenderStateSlotCount`, the frame sync's index):
  checked in Debug on both the write and the read side (hot path, the two-level rule).
- **pro-type-static-cast-downcast ×2** — `AbstractEntity.debug.cpp`: after a `CollisionModelType` switch (RTTI
  avoided: engine item `rtti-removal`).
- **avoid-const-or-ref-data-members ×1** — an entity's reference to its scene, by design.

### `src/Scenes` 6b — scene rendering / lighting / physics (2026-09-30)

- **pro-bounds-constant-array-index ×21** — engine-internal indices bounded by construction: the triple-buffer read /
  write slots, the render-list enum, LOD levels (clamped to `MaxLODLevels - 1`), clouds (`gatherClouds()` fills at most
  `MaxCloudVolumes`), a line light's points (`pointCount ≤ 9`, `setPolyline()`), light / cascade indices.
- **avoid-const-or-ref-data-members ×3** — `RenderBatch`'s const members: an immutable value object by design.
- **missing-std-forward ×2** — visitors invoked once per element (`CloudSet`, `LightSet`): never forwarded in a loop.
- **pro-type-reinterpret-cast ×2** — `RenderBatch` packs its sort key.
- **pro-type-const-cast ×2** — `SceneMetaData`: the RT skinned BLAS and a stale BLAS rebuilt lazily on objects the
  render path holds `const` (a deliberate mutable-cache point on the frame path).
- **use-enum-class, static-cast-downcast** — `NotificationCode`; a downcast after a type check (RTTI avoided).

### `src/Scenes` 6c — scene manager, toolkit, scene definitions (2026-09-30)

- **pro-bounds-constant-array-index ×5** — `Manager.console.cpp` `getRenderStatistics`: three `std::array` of the same
  `Graphics::Geometry::MaxLODLevels` size walked by one loop index.
- **misc-no-recursion ×1** — `DefinitionResource::readNodes()`: bounded by the parse's `stackLimit` (16) and by
  `Node::MaxDepth`.
- **use-enum-class ×1** — `NotificationCode` (the Observer convention).
- **pro-bounds-constant-array-index ×1** — `Graphics/Material/Helpers.cpp` `parseColorComponent()`: `index <
  min(4, size)` into a 4-slot array (it was a throwing `.at()` before the triad).

### `src/Scenes` 6d — `Component/` (2026-09-30)

- **pro-bounds-constant-array-index ×22** — bounded by construction: the triple-buffered render states (the frame
  slot), the cascade index (`< CascadeCount`), a light's colour-projection frame slot, the line light's ≤ 9 points,
  a beam's / path's station arrays walked by their own size.
- **use-enum-class ×9** — `NotificationCode` / `AnimationID` (the Observer and animation conventions).
- **non-private-member-variables ×3** — `AbstractLightEmitter`'s colour-projection state (bindless set, frame slot,
  cube-array flag), written by the derived lights; accessors would add nothing.
- **avoid-non-const-global-variables ×2** — `s_beamCount`, `s_pathCount`: `std::atomic` name counters.
- **concurrency-mt-unsafe ×1** — `std::lgamma` in `Beam::equivalentTubeLuminance()`: its only caller in the engine,
  on the logic thread (the non-thread-safe part is the `signgam` write, never read).
- **missing-std-forward ×1** — `forEachSuffix()` calls its callable in a loop.
- **avoid-const-or-ref-data-members ×1** — the console adapter's reference to the scene manager, by design.
- **pro-type-static-cast-downcast ×1** — `DirectionalLight`: after a type check (RTTI avoided).

### `src/Scenes` 6e — editor, AV console, viewers, effects toolkit, debug (2026-10-01)

- **pro-bounds-constant-array-index ×11** — the compass's six axes, the gizmos' three axes (enum-bounded loops).
- **pro-type-vararg ×9** — ImGui's printf-style API (`Text`, `BulletText`, `SetTooltip`) with CONSTANT formats.
- **non-private-member-variables ×6** — the gizmo base's program / geometry / frame / scale / highlight state, written by
  the three derived gizmos.
- **switch-missing-default-case ×5** — false positives: the switches cover every enumerator of `GizmoMode` /
  `TransformSpace` (`enum class`); a `default` would silence `-Wswitch` for a new enumerator.
- **avoid-const-or-ref-data-members ×5** — the viewers' references to the resource / scene managers and settings, by
  design (service references).
- **pro-type-reinterpret-cast ×1** — `AbstractVirtualDevice.hpp`: the device-type tag reinterpretation.
- **use-enum-class ×1** — `NotificationCode` (the Observer convention).

### `src/Graphics` 7a — resources read from disk (2026-10-01)

- **pro-bounds-constant-array-index ×53** — the cubemap's six faces (`faceIndex < 6` loops over `std::array`s: 25 of
  them were throwing `.at()` calls before the triad), the video converter's descriptor writes, the movie's frame
  colours (a `constexpr` table walked by its size).
- **pro-type-reinterpret-cast ×17** — binary I/O (`TextureCache` headers, `std::ifstream::read`), libktx's byte API,
  the video dump.
- **convert-member-functions-to-static ×2** — `TextureCompressor::compress()` / `compressSingle()`: the service's API,
  called on the instance.
- **misc-no-recursion ×3** (`ResourceTrait`) — a failure propagating up the dependency chain
  (`releaseLinksAfterFailure()` → `dependencyFailed()` → …): bounded by the chain's depth (image → texture → material
  → mesh → …).

### `src/Graphics` 7b — `Material/` (2026-10-01)

- **pro-bounds-constant-array-index ×23** — `StandardResource`'s UVW transform / rotation tables (`slot <
  MaxUVWSlots`, the slot allocated by the component) and `Helpers::parseColorComponent()` (`index < min(4, size)`).
- **pro-type-static-cast-downcast ×7** — a material component cast to `Component::Texture` right after a
  `type() == Type::Texture` check or right after its own `emplace()` of a `Texture` (RTTI avoided).
- **use-enum-class ×1** — `MaterialFlagBits` (a bit set, the flag convention).

### `src/Graphics` 7c — `Geometry/`, `Renderable/`, `MDI/` (2026-10-01)

- **pro-bounds-constant-array-index ×30** — the CDLOD clip levels and levels of detail (`level < m_clipLevelCount ≤
  MaxClipLevels`, `lod < m_levelOfDetailCount ≤ MaxLevelsOfDetail`), the ocean's FFT cascades (bounded loops over
  `std::array`s) and `ResourceGenerator`'s mapping-name tables (an enum's value).
- **bugprone-integer-division ×4** — exact by construction: `clipTexels / 2U` (a power of two, ≥ 256, checked),
  `patchQuads / 2U` (even, checked), the ocean quad-tree's `quarter / 2U` (the row of a quarter, an integer on purpose).
- **non-private-member-variables ×4** — `Geometry::Interface`'s RT members (`m_accelerationStructure`,
  `m_rtIndexBufferObject`, `m_BLASGeometryFirstIndices`, `m_accelerationStructureStale`), shared with every geometry
  class by design (protected, not public).
- **use-enum-class ×3** — `GeometryFlagBits`, `SubGeometryFlagBits`, `RenderableFlagBits` (bit sets, the flag
  convention).
- **readability-qualified-auto ×3** — `auto *const` suggested on Vulkan handles (`VkCommandBuffer`,
  `VkDescriptorSet`, `VkPipeline`): dispatchable handles are pointers on some platforms only — `const auto` stays
  (caution-points § qualified-auto).
- **misc-no-recursion ×2** — the CDLOD and ocean `selectNode()` quad-tree walks: bounded by the level-of-detail count.
- **pro-type-static-cast-downcast ×2** — `MeshResource` / `MultiLayerMeshResource` cast the manager they were built
  with (RTTI avoided).
- **special-member-functions ×1** — `Geometry::Interface` (a resource: never copied, held by `shared_ptr`).
- **avoid-const-or-ref-data-members ×1** — `ResourceGenerator::m_resources` (a non-owning service reference for the
  generator's lifetime).
- **implicit-widening-of-multiplication-result ×1** — a `static_assert` on `sizeof(Uniforms)` (compile-time constants).

### `src/Graphics` 7d — renderer and frame (2026-10-01)

- **portability-simd-intrinsics ×28** — the SSE4.1 / AVX2 BGRA→I420 converters (dispatched on the CPU features, the
  scalar path beside them).
- **pro-type-reinterpret-cast ×26** — the SIMD loads / stores (`__m128i *` / `__m256i *` over the pixel bytes), the
  console's image bytes, the RenderDoc API pointers.
- **pro-bounds-constant-array-index ×21** — the recorder's slot arrays (`index < AsyncBufferCount` /
  `HardwareSlotCount` loops), the renderer's clear colours and per-frame tables, the capture's file list.
- **convert-member-functions-to-static ×5** — `RenderDocCapture`'s build without RenderDoc: the same instance API as
  the real one.
- **owning-memory ×4** — `fopen()` / `fclose()` around `Recorder::OutputFile` (`std::unique_ptr< FILE, FileCloser >`
  IS the owner; gsl::owner is not used in the cascade).
- **pro-type-union-access ×3** — libvpx's `vpx_codec_cx_pkt_t::data.frame` (its API is a union).
- **readability-qualified-auto ×2** — Vulkan handles (`RendererFrameScope`), as in 7c.
- **clang-analyzer-cplusplus.Move ×1** — `Renderer.cpp` `m_sceneTarget`: retired by move (a moved `shared_ptr` is
  null), then `renderFrameWithInternal()` is only called when it is non-null — the analyzer does not model the guard.
- **use-enum-class ×1** — `Renderer::NotificationCode` (the observable notification convention).

### `src/Graphics` 7e — targets, instances, views, buffers (2026-10-01)

- **pro-bounds-constant-array-index ×80** — the view matrices' UBO float arrays (offsets named by constants, cascade and
  face loops bounded by their counts), the per-frame banks, the instance data blocks.
- **pro-type-reinterpret-cast ×16** — mapped GPU memory seen as floats / structures, Vulkan `pNext` chains.
- **readability-qualified-auto ×7** — Vulkan handles (`VkPipeline`, `VkBuffer`, `VkCommandBuffer`, `VkPipelineLayout`):
  a non-dispatchable handle is a 64-bit integer on 32-bit platforms, where `auto *` does not compile — `const auto`
  stays (caution-points § qualified-auto).
- **use-enum-class ×2** — `RenderableInstanceFlagBits`, `VertexBufferBindingFlagBits` (bit sets, the flag convention).
- **non-private-member-variables ×2** — `RenderableInstance::Abstract::m_localDataAccess` (the mutex the subclasses
  lock), `ViewMatricesInterface::m_nearestObjectDistance` (shared by the three view-matrix kinds).
- **convert-member-functions-to-static ×1** — `IntermediateRenderTarget::endRenderPass()` (the pair of
  `beginRenderPass()`, an instance API).
- **misc-unused-parameters ×1** — `RenderTarget::Abstract`'s `viewDistance`: dead across every render-target
  constructor, engine item `render-target-dead-view-distance-parameter`.

### `src/Graphics` 7f — post-process (2026-10-01)

- **pro-bounds-constant-array-index ×129** — the per-frame descriptor sets and targets (`frameIndex <
  framesInFlight`), the ping-pong pairs (`index < 2`), the effect slots (`slot < EffectSlotCount`), the census counters.
- **bugprone-return-const-ref-from-parameter ×9** — the chain's pass-through contract: an effect with nothing to do
  returns its INPUT texture, owned by a render target, never a temporary.
- **performance-inefficient-string-concatenation ×4** — `GIDenoiser` target names, built once per creation / resize.
- **cppcoreguidelines-macro-usage ×2** — `EMEN_CLOUD_VOLUME_GLSL`, `GIDENOISER_REPROJECTION_GLSL`: GLSL text spliced
  by adjacent-literal concatenation at compile time, which a `constexpr` pointer cannot do. Same reason, added
  2026-10-05: `EMEN_STAR_MASK_GLSL` (`Effects/Shared/StarMaskGLSL.hpp`, the in-texture body mask; the only finding on
  the lines that change touched in `IBLBaker`, `RTGI`, the probe volume, `GIDenoiser`, `PostProcessor`, `Renderer`).
- **use-enum-class ×1** — `OverflowCensus::CounterIndex` (indices into the counter array, now on `uint8_t`).

### `src/Graphics` 7g — lighting and atmosphere (2026-10-01)

- **pro-bounds-constant-array-index ×47** — per-frame sets and ping-pong pairs, the probe grid axes (`axis < 3`), the
  cascade matrices, the XRay corners.
- **bugprone-return-const-ref-from-parameter ×10** — the chain's pass-through contract (see 7f).
- **readability-qualified-auto ×7** — Vulkan handles (`ProbeConvolver`, `OceanWaves`), as in 7c / 7e.
- **pro-type-static-cast-downcast ×2** — `VolumetricClouds` / `VolumetricScattering`: the shadow map cast right after
  its type check (RTTI avoided).
- **bugprone-integer-division ×2** — `IrradianceProbeVolume`: `counts / 2` centres the grid on a whole cell.
- **cert-msc51-cpp ×1** — `IrradianceProbeVolume::m_random` seeded with a constant: the probe ray rotations must be
  reproducible run to run (A/B measurements).

### `src/Saphir` 8a — shader core (2026-10-01)

- **readability-qualified-auto ×2** — `FragmentShader`'s `std::ranges::find_if` over a `std::array`: an iterator, a
  class on MSVC (`auto *` broke the Windows build in 7a).
- **clang-analyzer-deadcode.DeadStores ×1** — `MeshShader`'s `topology = "triangles"` before a switch over every
  enumerator: the value an out-of-range enum gets.
- **misc-no-recursion ×1** — `AbstractVertexStage::requestSynthesizeInstruction()`: a synthesized variable may need
  another, bounded by the variable set.
- **pro-bounds-constant-array-index ×1**, **pro-bounds-array-to-pointer-decay ×1** — a bounded output loop; the
  `Code` stream's `operator<<` taking string literals.
- **use-enum-class ×1** — `ShaderManager::NotificationCode` (the observable notification convention).

### `src/Saphir` 8b — `LightGenerator` (2026-10-01)

- **convert-member-functions-to-static ×9** — `lightPositionWorldSpace()`… `lightColor()`: the generator's accessor API
  (today constant names, an instance API so a generator may vary them).
- **bugprone-branch-clone ×1** — the reflectivity priority chain: the "artistic reflection" branch and the fallback
  both publish `0.0`, kept apart because they say different things (documented in place).

### `src/Saphir` 8c — `Declaration/`, `Generator/` (2026-10-01)

- **pro-type-reinterpret-cast ×7** — a render pass / effect handle mixed into a program cache hash as an integer
  (64-bit targets only, where a non-dispatchable Vulkan handle is a pointer).
- **bugprone-branch-clone ×2** — `Declaration/Types.cpp`'s size table: distinct GLSL types of the same size, one case
  each so the table stays readable per type.
- **use-enum-class ×1** — `GeneratorFlagBits` (a bit set, the flag convention).

### `src/Vulkan` 9a — instance, device, presentation (2026-10-01)

- **pro-bounds-array-to-pointer-decay ×11** — the Vulkan C structures' fixed arrays (`deviceName`, `memoryTypes`,
  `extensionName`…) handed to the C API and to streams.
- **pro-type-reinterpret-cast ×10** — `vkGetInstanceProcAddr` / `vkGetDeviceProcAddr` function pointers, `pNext` chains.
- **convert-member-functions-to-static ×5** — `PhysicalDevice` query wrappers kept on the instance (their unused
  parameters mirror the Vulkan calls they will wrap) and the `Instance` compatibility checks.
- **pro-bounds-constant-array-index ×4** — memory-type and queue-priority loops bounded by the driver's counts.
- **misc-misplaced-const ×1** — `const VkSemaphore semaphore`: the handle is meant const, not its pointee.
- **clang-analyzer-deadcode.DeadStores ×1** — `SwapChain`'s present-mode `selectionReason` default: every branch
  overwrites it, the default is the fallback's wording.

### `src/Vulkan` 9b — memory and resources (2026-10-01)

- **pro-type-reinterpret-cast ×24** — Vulkan handles as `uint64_t` object names, mapped memory, device addresses.
- **readability-qualified-auto ×10** — Vulkan / VMA handles (`VkDevice`, `VmaAllocator`): `const auto` stays.
- **misc-const-correctness ×5** — mapped pointers handed back to the caller as WRITABLE memory, transfer operations
  reserved for writing.
- **pro-bounds-constant-array-index ×4** — mip / region loops bounded by their counts.
- **bugprone-branch-clone ×3** — `TransferManager`'s queue-family and format branches: distinct cases with the same
  values (documented per case).
- **pro-bounds-array-to-pointer-decay ×2** — `VideoEncoderH265`'s `memset` of the Std reference lists (C structures).

### `src/Vulkan` 9c — commands, pipelines, descriptors, sync (2026-10-01)

- **pro-type-reinterpret-cast ×15** — Vulkan handles as `uint64_t` object names and hash inputs, `pNext` chains.
- **pro-bounds-constant-array-index ×2** — per-frame profiler slots (`frameSlot < framesInFlight`).
- **pro-type-vararg ×2** — `std::snprintf` into the profiler's fixed label (bounded, truncating by design).
- **convert-member-functions-to-static ×1** — `ComputePipeline::getHash()`: the pipeline cache's instance API.
- **use-enum-class ×1** — `DescriptorSetLayout::Flag` (a bit set, the flag convention).

### `src/Audio` 10a — the audio core (2026-10-01)

- **modernize-use-designated-initializers ×47** — `MusicResource`'s 48-note jingle table: an aligned `{start, duration,
  frequency}` table reads better positional than with three member names per note.
- **pro-type-reinterpret-cast ×24** — OpenAL / WAV byte buffers, `alGetProcAddress` entry points.
- **convert-member-functions-to-static ×13** — the OpenAL wrappers' query API (an instance API over the device).
- **use-enum-class ×2** — `NotificationCode` of `Manager` and `TrackMixer` (the observable convention).
- **avoid-non-const-global-variables ×1** — `s_tsfMutex` (anonymous namespace): the lock serialising TinySoundFont.

### `src/Audio` 10b — `Effects/`, `Filters/`, `EffectSlot` (2026-10-01)

- **convert-member-functions-to-static ×1** — `EffectSlot::disable()`: it acts on the source it is given, but it is
  the slot's API, the pair of `enable()`.
- Not a finding but a trap: a range check written `!(value >= MIN && value <= MAX)` (which refuses NaN) draws
  `readability-simplify-boolean-expr`, and its fix-it rewrites it into `value < MIN || value > MAX`, which lets NaN
  through. Write `std::isnan(value) || value < MIN || value > MAX` instead.

### `src/Physics` 11 (2026-10-01)

- **pro-type-static-cast-downcast ×16** — the collision models' double dispatch: each `isCollidingWith()` switches on
  `other.modelType()` and casts to that exact type (a tag-checked downcast, no RTTI on this hot path). REMOVED with the
  models' MTV tests in P3.a (2026-10-02); the same tag-checked downcast lives on in `NarrowPhase.cpp` (its own row).
- **convert-member-functions-to-static ×7** — the three `ConstraintSolver` phases (`prepareContacts`,
  `solveVelocityConstraints`, `solvePositionConstraints` — that solver was REMOVED by the physics overhaul P2 on
  2026-10-01, these three findings with it) and the four
  `PointCollisionModel::collideWith*()` (a point has no shape: the same dispatch API as the other models; removed in
  P3.a, 2026-10-02).
- **pro-bounds-constant-array-index ×2** — `ContactPoint` tangent impulses: `tangentIndex` is always the literal 0 or 1.
- **use-enum-class ×1** — `BodyPhysicalProperties::NotificationCode` (the Observer convention).
- **special-member-functions ×1** — `CollisionModelInterface`: owned by `std::unique_ptr`, never copied through the
  interface (as `Geometry::Interface`).
- Trap met in this pass: a `--fix` run over several TUs applies a HEADER fix-it once per TU that includes it. Here it
  was harmless (`isolate-declaration` in base headers, idempotent), but a non-idempotent one (parentheses) nests
  again at every TU: run header fixes from one TU, and review every header in the diff.

### `src/Overlay` 13 (2026-10-01)

- **pro-bounds-constant-array-index ×3** — `Manager::m_programs[index]`: the index is built from two bits (premultiplied
  alpha, BGRA source), always below `ProgramCount` (4).
- **use-enum-class ×1** — `Manager::NotificationCode` (the Observer convention).

### `src/PlatformSpecific` 14 (2026-10-01)

- **pro-type-union-access ×10, pro-bounds-array-to-pointer-decay ×10, pro-type-vararg ×10** — the V4L2 API
  (`v4l2_format::fmt.pix`, `v4l2_buffer::m.offset`, `ioctl()`), `FD_SET`, `fgets` into a `std::array`.
- **concurrency-mt-unsafe ×7** — `system()`, `getenv()`, `strerror()` on the calling thread of a dialog / notification /
  capture (no concurrent caller).
- **non-private-member-variables ×4** — `Desktop::Notification`'s protected members, shared with its platform bodies.
- **branch-clone ×1** — `Message.linux.cpp`: the `MessageType` cases listed explicitly before their shared default.
- **const-correctness ×1** — `UserInfo.linux.cpp`: `getpwuid_r()` takes a `passwd **`.
- **pro-type-reinterpret-cast ×1** — `VideoCaptureDevice.linux.cpp`: the `v4l2_capability::card` byte array as text.

### `src/Tool`, `src/Help`, root files 15 (2026-10-01)

- **pro-type-vararg ×10** — `Core.cpp`: `ImGui::Text()` / `TextDisabled()`, ImGui's printf-style API.
- **pro-type-reinterpret-cast ×6** — `Window.linux.cpp`: `dlsym()` results cast to the libwayland-client function types.
- **convert-member-functions-to-static ×5** — `Window::initializeNativeWindow()` / `releaseNativeWindow()` /
  `drainDisplayConnection()` / `pumpEvents()` and `CursorAtlas::resetCursor()`: one declaration, a body per platform,
  and the Windows / macOS bodies use the instance.
- **avoid-const-or-ref-data-members ×4** — `Identification`: the application identity is immutable by design.
- **pro-bounds-constant-array-index ×3** — `CursorAtlas::setCursor()`: the index is checked against
  `StandardCursorCount` just above.
- **use-enum-class ×2** — `Core::NotificationCode`, `Window::NotificationCode` (the Observer convention).
- **crtp-constructor-accessibility ×2, non-private-member-variables ×2** — `Tracer.hpp` `T_TraceHelperBase`: its public
  constructors and protected `m_tag` / `m_location` are what the `TraceInfo` / `TraceError`… helpers build on.
- **concurrency-mt-unsafe ×1** — `Core.cpp`: `system()` in a Debug-only path, on the main thread.
- **misc-no-recursion ×1** — `Settings::readLevel()`: the depth is bounded by the JSON parser (stack limit 16).
- **bugprone-branch-clone ×1** — `Settings::settingValueToJson()`: `if constexpr` branches of different types, each
  `return v;`.
- **avoid-non-const-global-variables ×1** — `Window.linux.cpp` `s_waylandReader`: the libwayland functions resolved once
  for the process, used on the main thread only.

