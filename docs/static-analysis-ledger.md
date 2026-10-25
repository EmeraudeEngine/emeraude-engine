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
| Scene-colour pre-exposure (`defb1464` … `01f719b0` and the moments fix): `Graphics/Renderer.*`, `PostProcessor.*`, `IndirectPostProcessEffect.hpp`, `FrameDiagnostics.hpp`, `FrameCapture.*`, `ViewMatrices{2D,3D,Cascaded}UBO.*`, `ViewMatricesInterface.hpp`, `Effects/Camera/{ToneMapping,LensFlare,VeilingGlare}.*`, `Effects/Lighting/{RTGI,RTR,SSGI}.*`, `Effects/Atmosphere/*.cpp`, `Effects/Resolve/TAA.*`, `GIDenoiser.*`, `IrradianceProbeVolume.cpp`, `Effects/Shared/IrradianceProbesGLSL.hpp`, `DeferredLightResolve.cpp`, `Material/StandardResource.*`, `RenderTarget/Texture.hpp`, `Types.hpp`, `Saphir/{LightGenerator,Keys}.*`, `Saphir/Generator/{Abstract,SceneRendering}.cpp`, `SettingKeys.hpp` | 2026-10-24 | clang-tidy 21.1.6 on the changed lines, 6 gate runs, every finding fixed: 1 bugprone-easily-swappable-parameters (`setFramePreExposure(value, ratio)` → a struct, then a single float once the ratio plumbing went), 9 cppcoreguidelines-pro-bounds-pointer-arithmetic (the view UBOs' slot written through a `std::span` bounded to `ViewUBOElementCount`; `projMat[5]` and `prevInvView[12-14]` through the matrix subscript), 2 readability-static-accessed-through-instance (`sceneRadianceToNits()`). cppcheck 0, include-what-you-use clean. | graphics § 41 |
| The physical sky, lot 1: `Graphics/SkyAtmosphere.*`, `StorageTexture2D.*`, `Effects/Shared/AtmosphereGLSL.hpp`, `Material/SkyAtmosphereResource.*`, `Renderable/DynamicSkyResource.*` (new or rewritten), `Graphics/Renderer.*` (`renderSkyAtmosphere()`), `Scenes/SkyState.hpp`, `Scene.*` (the clock, `skyStateForRendering()`, the null environment cubemap), `CelestialClock.*` (`publishStateForRendering()`), `Manager.console.cpp`, `Resources/Manager.cpp`, `Saphir/Keys.hpp` | 2026-10-24 | clang-tidy 21.1.6 on the changed lines: 18 → 0. Fixed: 8 modernize-use-designated-initializers (the shared GLSL functions) and 1 pro-type-member-init (every `FunctionSource` member initialised); 2 readability-ambiguous-smartptr-reset-call (`= nullptr`); 1 pro-type-static-cast-downcast (a `dynamic_cast` instead of `is()` + cast); 1 implicit-widening (the block size in `uint32_t`); 4 pro-bounds-constant-array-index (the render-state slot and the frame-in-flight index through `std::next`, proven < the size; the moons published by iterators); 1 bugprone-easily-swappable-parameters (`StorageTexture2D` takes a `VkExtent2D`); 1 projet-alpha suspicious-missing-comma. cppcheck 0, include-what-you-use clean. | graphics § 42 |
| `src/Scenes/Component/CelestialClock.*` (new, replaces `SunCourse.*`), `EnvironmentConsoleAdapters.cpp`, `SkyFollowsSun.*`, `Scenes/Toolkit.*`, `tools/template-guard/TemplateInstantiation.cpp` | 2026-10-24 | clang-tidy 21.1.6 on the changed lines: 1 readability-redundant-member-init (`moons{}`) fixed. cppcheck 0, include-what-you-use clean. | scenes § 02 |
| `src/Graphics/RenderableInstance/Abstract.*` (`isReadyToRender()` on the instance's own record `m_readyTargets`; the draw functions read the instance count once and skip 0) | 2026-10-24 | clang-tidy 21.1.6 on the changed lines: 0. cppcheck 0, include-what-you-use clean. | caution-points § Scene Rendering |
| `src/Scenes/Component/ParticlesEmitter.*` (world box, `publishStateForRendering()`), `Scenes/Scene.*` (`snapshotParticlesEmitters()`, the emitters' own view / shadow passes), `Scene.entities.cpp`, `AbstractEntity.cpp` (`ParticlesEmitterDestroyed`), `Graphics/RenderableInstance/Multiple.hpp` (`activeInstanceCount()`) | 2026-10-24 | clang-tidy 21.1.6 on the changed lines: 2 fixed (pro-bounds-constant-array-index on the render-state slots: `std::next`, the slot proven < `RenderStateSlotCount` by the modulo). cppcheck 0, include-what-you-use clean. | caution-points § Scene Rendering |
| `src/Graphics/RenderableInstance/{Abstract,Multiple}.*` (`setDeferredDestructor()`, `~Multiple()` retires its VBO, the count constructor without its unused flags), `Geometry/{IndexedVertex,RawIndexedVertex,RawVertex,VertexGrid,Vertex}Resource.cpp` (buffers retired), `Vulkan/DeferredDestructor.hpp` (`close()`), `Graphics/Renderer.cpp`, `Scenes/Component/{ParticlesEmitter,MultipleVisuals}.hpp` | 2026-10-24 | clang-tidy 21.1.6 on the changed lines: 1 fixed (bugprone-easily-swappable-parameters on `Multiple (…, instanceCount, flagBits)`: the flags parameter removed — its only caller passed none). cppcheck 0 (11 TUs), include-what-you-use clean, thread-safety analysis clean (`m_closed` guarded). | vulkan § 12 |
| `src/Scenes/Scene.*` (`playSoundAt()`, `acquireWorldSoundVoice()`, `releaseWorldSounds()`), `Scene.entities.cpp`, `Manager.console.cpp` (`getWorldSoundCount`), `Audio/Source.hpp` (`isFinished()`), `Scenes/Component/SoundEmitter.cpp` | 2026-10-24 | clang-tidy 21.1.6 on the changed lines: 1 fixed (readability-qualified-auto on a `std::array` iterator: the slot search became a helper returning a pointer from range-for, never `auto *` — MSVC's iterator is a class). cppcheck 0, include-what-you-use clean, thread-safety analysis (`EMEN_GUARDED_BY` / `EMEN_REQUIRES`) clean. | audio § 14 |
| `src/Input/PointerController.*` (`injectButtonState()`), `Manager.*` (`injectMouseWheelEvent()`, `mouseWheel`, `mousePress` / `mouseRelease` hold the polled state) | 2026-10-23 | clang-tidy 21.1.6 on the changed lines: 2 fixed (readability-qualified-auto and pro-bounds-pointer-arithmetic on the copy loop's `std::array` iterators: indexed through `std::next`, like the accessors). cppcheck 0, include-what-you-use clean. | input § 4 |
| `src/Audio/Source.*` (`resetToDefaults()`), `Manager.*` (`releaseSource()`), `Scenes/Component/SoundEmitter.*` (one-shot voices), `AudioConsoleAdapters.cpp` (`oneShots`) | 2026-10-23 | clang-tidy 21.1.6 on the changed lines: 1 fixed (pro-bounds-constant-array-index: the round-robin voice through `std::next`, the cursor proven < `MaxOneShotVoices`). cppcheck 0. include-what-you-use clean. | audio § 14 |
| `src/Graphics/Material/StandardResource.*` (the setters by source, `onLoadFinishing()`), `Resources/ResourceTrait.*` (the hook) — graphics § 40 L5 | 2026-10-21 | clang-tidy 21.1.6 on the changed lines: 0. cppcheck 0. include-what-you-use clean. The gate's clang build caught 13 `-Wdouble-promotion` (a `float` assigned to a `Json::Value`, which holds `double`), made explicit; GCC does not warn. |
| `src/Graphics/{GPUTextureSet,KTX2Decoder,KTX2Writer}.*` (the fingerprint), `TextureCache.hpp` (`version()`), `Material/StandardResource.cpp` (the check), `SettingKeys.hpp`, `tools/texture-bake/*` — graphics § 40 L4 | 2026-10-21 | clang-tidy 21.1.6 on the changed lines: 0. cppcheck 0. include-what-you-use clean. The gate's clang build caught a `-Wdocumentation` error (a function inserted between a Doxygen block and its declaration) that GCC does not see; the tool (`EXCLUDE_FROM_ALL`) built by hand with GCC and clang, 0 warning. |
| `src/Graphics/TexturePacker.*` (`plan()`), `tools/texture-bake/{TextureBake,MaterialBake}.cpp` + `TextureBake.hpp` (the material mode) — graphics § 40 L3 | 2026-10-21 | clang-tidy 21.1.6 on the changed lines: 5 fixed (2 qualified-auto on `std::array` iterators — loops instead, `const auto *` would not compile against MSVC's class iterators; 2 pointer arithmetic around `std::from_chars` — a bounded digit loop; 1 container-size-empty). cppcheck 0. include-what-you-use clean. ⚠️ The tool is `EXCLUDE_FROM_ALL`: built by hand in the GCC and clang trees (0 warning). |
| `src/Graphics/GPUTextureSet.*` (new), `Material/StandardResource.*` (`planGPUTextures()`), `Material/Component/Texture.cpp`, `tools/template-guard/TemplateInstantiation.cpp` — graphics § 40 L2 | 2026-10-21 | clang-tidy 21.1.6 on the changed lines: 1 fixed (readability-use-anyofallof in `GPUTextureSet::empty()`). cppcheck 0. include-what-you-use clean. ⚠️ The gate's clang tree must be RECONFIGURED after a new globbed engine source, or the clang link misses it (undefined `GPUTextureSet::*`). |
| `src/Graphics/CompressedImageResource.cpp` (the `GPUTextures` name check), `KTX2Decoder.*` (`channelCount()`), `ImageResource.cpp` (GPU containers refused), `Resources/Manager.cpp` (the store) — graphics § 40 L1 | 2026-10-21 | clang-tidy 21.1.6 on the changed lines: 0. cppcheck 0. include-what-you-use: the five files brought clean (`Manager.cpp` +17 −1 and two forward declarations; a `<features.h>` IWYU asked for was refused — base mapping). |
| `src/Graphics/MipChain.*` (new), `TextureCompressor.*`, `TextureCache.*` (version 6), `TextureResource/{Texture2D,AnimatedTexture2D}.cpp` (the uncompressed chain), `Vulkan/Image.*` (`createFromMipChain()`), `SettingKeys.hpp` (`BlockCompression`), `tools/texture-bake/TextureBake.cpp`, `tools/template-guard/TemplateInstantiation.cpp` (the mip filter, graphics § 39) | 2026-10-21 | clang-tidy 21.1.6 on the changed lines: 1, ON PURPOSE — `TextureCompressor::compress()` could be static (§ below, convert-member-functions-to-static: the instance proves the encoders are initialized). cppcheck 0 on 10 TUs. include-what-you-use: `TextureCompressor.cpp`, `MipChain.cpp` clean. |
| `src/Graphics/ImageResource.cpp`, `TextureCompressor.*`, `TextureResource/{Texture1D,Texture2D,AnimatedTexture2D}.*`, `MovieResource.*`, `CursorAtlas.cpp` (D8 step 2: a grey-scale image file keeps one channel) | 2026-10-21 | clang-tidy 21.1.6 on the changed lines: 13 → 1. Fixed: the block gather through `std::span` (no pointer arithmetic, no array subscript, no `std::array` iterator — readability-qualified-auto would want `auto *`, which MSVC's class iterators refuse); `compress()` split (`dilateHoles()`, `compressChainLevel()`) below the complexity threshold. 1 ON PURPOSE: readability-convert-member-functions-to-static on `TextureCompressor::compress()` (the 2026-10-21 row above: the instance proves the encoders' tables are set up). cppcheck 0; include-what-you-use clean. | graphics § 39 § Block format per kind, step 2 |
| `src/Graphics/KTX2Writer.*` (new), `ImageResource.*` (`PackRecipe::fillChannel()`), `TextureCompressor.cpp` (the colour dilation), `TextureCache.hpp` (version 5), `tools/texture-bake/TextureBake.cpp` (new), `tools/template-guard/TemplateInstantiation.cpp` (the baked assets, graphics § 39) | 2026-10-21 | clang-tidy 21.1.6 on the changed lines: 30 → 3 (base + engine). Fixed: the bake tool's argument parsing split below the complexity threshold, `argv` through a `std::span`, a `uint8_t` printed as a number; base `Processor::fillTransparentColour()` split into pull / push helpers with index arithmetic; its tests split. 3 ON PURPOSE in `KTX2Writer.cpp`, the libktx C interface: cppcoreguidelines-pro-type-reinterpret-cast in `asBaseTexture()` (libktx's own `ktxTexture()` macro is that cast, `KTX2Decoder` has the same helper), cppcoreguidelines-no-malloc + owning-memory on the `std::free()` of the buffer `ktxTexture_WriteToMemory()` malloc's — owned by a `std::unique_ptr` with that deleter, the only way to release it. cppcheck 0. | graphics § 39 § Baked assets |
| `src/Graphics/TextureCompressor.*`, `TextureCache.*`, `TextureResource/{Abstract,StoreTexture,Texture2D,AnimatedTexture2D}.*`, `Material/Component/Texture.*`, `Material/StandardResource.cpp`, `Effects/Lighting/RTR.cpp`, `Scenes/Loaders/{GLTF,FBX,USD}Loader.*`, `Scenes/Toolkit.cpp` (block format per kind, D8) | 2026-10-20 | clang-tidy 21.1.6 on the changed lines: 10 → 1. 6 pro-bounds-constant-array-index fixed (the loaders' usage slot through `std::next`, proven < `LoaderUsageCount`); 2 pro-type-reinterpret-cast fixed (the cache header's format word through `std::bit_cast` to a `std::array< char, 4 >`); 1 bugprone-easily-swappable-parameters gone with `compressedSize()`, removed (no caller in the cascade). 1 ON PURPOSE: readability-convert-member-functions-to-static on `TextureCompressor::compress()` — the service instance is the proof that `onInitialize()` set the encoders' global tables up (`bc7enc_compress_block_init()`, `rgbcx::init()`); a static would be callable before it. cppcheck: 0. | graphics § 39 § Block format per kind |
| `src/Input/KeyboardController.*`, `PointerController.*`, `GamepadController.*`, `JoystickController.*` (the input state, one atomic per input), `tools/stress/InputStateStress.cpp` (new) | 2026-10-16 | clang-tidy 21.1.6 on the changed lines: 31 → 6. 24 pro-bounds-constant-array-index fixed (the checked index through `std::next`, the walks as range-for), 1 clang-analyzer-core.CallAndMessage fixed (`GamepadController::getRawState()`, an index loop over the atomics, now a range-for); 6 ON PURPOSE: readability-qualified-auto on `std::array` iterators walked beside a range-for (`readDeviceState()` of the keyboard, the pointer and the joystick) — an iterator is a class on MSVC, `auto *` breaks the Windows build (§ 7a above). The earlier P5 lots (handle owners' special members, `Scene::enable()`'s guard, the lot 3 statics): 0 on the changed lines. | Ave Robustus II P5 |
| `src/Audio/TrackMixer.*`, `Recorder.*`, `ExternalInput.*`, `src/Console/RemoteListener.*`, `src/Graphics/Recorder.*`, `src/Tracer.cpp` / `TracerLogger.hpp`, `src/Core.*`, `src/Notifier.*` (dedicated threads on their stop token) | 2026-10-15 | clang-tidy 21.1.6, changed lines of the 8 touched TUs: 1, ON PURPOSE — cppcoreguidelines-pro-type-reinterpret-cast in `ExternalInput::captureAvailableSamples()` (the 16-bit samples written to the WAV `std::ostream`, which takes `char`; the line existed, moved out of the loop). | engine `docs/subsystems/source-tree/07-dedicated-threads-stop-tokens.md` |
| `src/Graphics/Renderable/MeshResource.*`, `MultiLayerMeshResource.*` (`m_LODJob`), `src/Resources/ResourceTrait.*` (`submitResourceJob()`, `LocalDataReloadJob`), `Container.*` (`m_tasks`, `submitTask()`, `DiscardedTaskGuard`, `stopBackgroundWork()`), `Manager.*` (`stopBackgroundWork()`, the release-pass handle), `src/Core.cpp` (no drain, no cancel flag), `src/Net/Manager.*`, `APIClient.*`, `src/Graphics/FrameCapture.*`, `Geometry/CDLODTerrainResource.cpp`, new `tools/stress/ResourceJobsStress.cpp` | 2026-10-15 | clang-tidy 21.1.6, changed lines of the 10 touched TUs + the stress: 8; 6 fixed (performance-unnecessary-value-param: `DiscardedTaskGuard` takes a const reference; pro-type-member-init, math-missing-parentheses, redundant-member-init ×3 in the stress). 2 left, ON PURPOSE: portability-template-virtual-member-function on `Container::stopBackgroundWork()` (the `ContainerInterface` category below, now ×18); misc-const-correctness on the stress's `main()` local (a false positive: `Core::run()` is not `const`; `TrackMixerStress.cpp` has the same). | engine thread-pool-jobs-owned-through-a-task-handle (closed 2026-10-15) |
| `src/Graphics/RenderableInstance/PathPoints.*` (`Footprint`, the old shape's box), `src/Scenes/SceneInstanceTransforms.*` (`invalidateTemporalHistory()`), `src/Graphics/Renderer.*` (`markInvalidTemporalHistory()`), `SceneRenderTarget.*` (`reactiveColorAttachmentIndex()`), `src/Scenes/Component/Path.cpp`, `Beam.cpp` | 2026-10-13 | clang-tidy 21.1.6 on the changed lines: 4 fixed (2 pro-bounds-constant-array-index: the footprint slot through `std::next`; 2 modernize-use-designated-initializers: `Footprint`). 0 left; `clang -fsyntax-only` clean on the 6 TUs. | path-reactive-tag-on-point-count-change |
| `src/Graphics/Renderable/MeshResource.cpp`, `MultiLayerMeshResource.cpp` (the decimator gets the cancellation flag), `src/Resources/ResourceTrait.hpp` (`backgroundWorkCancellation()`) | 2026-10-08 | clang-tidy 21.1.6: 0 on the changed lines. | lod-job shutdown follow-up |
| `src/Core.cpp` (`cancelBackgroundWork()`, second pool drain), `src/Resources/ResourceTrait.hpp` (the flag), `src/Graphics/Renderable/MeshResource.cpp`, `MultiLayerMeshResource.cpp` (weak job capture, the check) | 2026-10-08 | clang-tidy 21.1.6: 0 on the changed lines. | lod-job-outlives-its-mesh |
| `src/Graphics/Renderable/MeshResource.*`, `MultiLayerMeshResource.*` (lock-free level reads, `publishLevel()`) | 2026-10-07 | clang-tidy 21.1.6: 0 on the changed lines. | engine geometry() lock (profile) |
| ALL `src/` TUs — **move / copy audit** (`modernize-pass-by-value`, `performance-unnecessary-value-param`, `performance-move-const-arg`, `performance-unnecessary-copy-initialization`, nothing else enabled) | 2026-10-07 | clang-tidy 21.1.6: **0** findings — emeraude-base 132 TUs, emeraude-engine 460, projet-alpha 109 (canary file checked: the four checks fire). The triad pass (Allocatus Reduxus, 2026-09-30 / 10-01) had already removed them. | base std-move-function-args (closed) |
| ALL `src/` TUs — **Ave Robustus II warning pass**, findings on the CHANGED lines (`git diff -U0 HEAD`) | 2026-10-08 | clang-tidy 21.1.6, 460 TUs: 26 findings on changed lines, all pre-existing on those lines (subscripts, ImGui varargs, ufbx / rusage unions, socket `reinterpret_cast`: the pass only added a cast on them) except two: `OceanWaves.cpp:498` `bugprone-misplaced-widening-cast` (the pass had moved the widening after the multiplication — restored before it) and `KTX2Decoder.cpp` `asBaseTexture()` `reinterpret_cast` (kept on purpose: it replaces libktx's `ktxTexture()` C-style-cast macro; `ktxTexture2` starts with the `ktxTexture` members). | projet-alpha `docs/plans/ave-robustus-ii.md` § 6.3 |
| `src/Graphics/Renderable/SpriteResource.cpp` / `.hpp` (`computeBillboardVolumes()` in `onDependenciesLoaded()`), `src/Saphir/AbstractVertexStage.cpp` (GLSL `computeYAxis()`) | 2026-10-07 | clang-tidy 21.1.6: 0 on the changed lines. | sprite-disappears-at-some-angles |
| `src/Scenes/Scene.physics.cpp` (step 4b: the sweep reports its impacts) | 2026-10-07 | clang-tidy 21.1.6: 0 on the changed lines. | engine physics-continuous-collision follow-up |
| ~38 files of `src/` (every `onNotification()` / `notify()` signature: `const Base::Any &` instead of `const std::any &`), checked through `src/Scenes/AbstractEntity.cpp` | 2026-10-07 | clang-tidy 21.1.6: 0 on the changed lines (signature-only change). | base rtti-free-observer-payload |
| `src/Net/TCPClient.cpp` (`connect()` through base `Network::connectFirstReachable()`, Happy Eyeballs) | 2026-10-07 | clang-tidy 21.1.6: 0 on the changed lines (the file's 5 older findings are outside them). | base httpsclient-happy-eyeballs |
| `src/Graphics/TextureCache.cpp` (directory, erase and streams through base `IO::`) | 2026-10-07 | clang-tidy 21.1.6: 0 on the changed lines. | base windows-long-paths |
| `src/Core.cpp`, `Tracer.cpp` / `TracerLogger.hpp`, `Audio/ExternalInput.*`, `Audio/Recorder.*`, `Audio/TrackMixer.*`, `Console/RemoteListener.*`, `Graphics/Recorder.*`, `PlatformSpecific/Helpers.linux.cpp`, `Scenes/Component/ParticlesEmitter.cpp` (threads on base `Base::Thread`) | 2026-10-07 | clang-tidy 21.1.6: 1 on the changed lines, fixed (readability-ambiguous-smartptr-reset-call in `ParticlesEmitter`). 0 left. `Notification.windows.cpp` not analysable on Linux. | base non-throwing-thread-start |
| `src/Saphir/ShaderManager.cpp`, `src/Graphics/Renderer.cpp` (cache commits through base `IO::renameFile()`), `src/Scenes/Editor/Manager.cpp` (comment) | 2026-10-07 | clang-tidy 21.1.6: 0 on the changed lines. | base windows-long-paths |
| `src/Scenes/Scene.physics.cpp` (`PhysicsStepMark`, bodies need a model), `Scene.entities.cpp` (deferred content notification, withdrawn model leaves the physics octree), `Scene.cpp` (the queue handled after the step), `AbstractEntity.cpp` / `.hpp` (`isCollisionModelWithdrawn()`), `Scene.hpp`, `Node.cpp` (a withdrawn model is not integrated) | 2026-10-07 | clang-tidy 21.1.6: 1 on the changed lines, fixed (readability-redundant-member-init on the `std::atomic< std::thread::id >`). 0 left. | Physics-step self-deadlock |
| `src/Graphics/Geometry/Interface.cpp` (identity RT index list for a non-indexed triangle list) | 2026-10-07 | clang-tidy 21.1.6: 0 on the changed lines. | RT NULL index address |
| `src/Resources/ResourceTrait.cpp` / `.hpp` (`checkDependencies()` claims `onDependenciesLoaded()`) | 2026-10-07 | clang-tidy 21.1.6: 0 on the changed lines (the TU's 5 misc-no-recursion + 1 use-enum-class predate them). | Resource finalization race |
| `src/Graphics/Renderable/MeshResource.cpp`, `MultiLayerMeshResource.cpp` (automatic LOD skips a mesh carrying its own levels) | 2026-10-07 | clang-tidy 21.1.6: 0 on the changed lines. | Automatic LOD |
| `src/Core.*` (system stop, device-loss stop), `src/PlatformSpecific/TerminationSignals.posix.cpp` (new), `Helpers.*`, `Window.*`, `Graphics/Renderer.*`, `Vulkan/Device.*`, `Queue.cpp`, `Sync/Fence.cpp` (release fixes, 2026-10-11) | 2026-10-11 | clang-tidy 21.1.6 on new and changed lines: 6 fixed (5 pro-bounds-constant-array-index: the caught signals as an array of structs walked by range-for), 2 ON PURPOSE — cppcoreguidelines-avoid-non-const-global-variables on `s_pendingSignal` (`TerminationSignals.posix.cpp`): a signal handler cannot reach an object, a lock-free `std::atomic< int >` at namespace scope is the one async-signal-safe channel (the sanctioned exception to the no-global-state rule, like the libjpeg error callback); misc-use-anonymous-namespace on `onTerminationSignal` (`static` inside `extern "C" { }`): an `extern "C"` function in an anonymous namespace keeps an external C name and fails Apple clang's `-Wmissing-prototypes -Werror` (macOS peer, 2026-10-11) — `static` in the linkage block is the standard way to get a C-linkage handler with internal linkage. | OS end of session, device-loss fail-fast |
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
| Physics wheeled vehicle (decisions 15): `Physics/Vehicle` (new), `Scenes/Component/Vehicle` (new), `Physics/SoftStepSolver` (the wheels), `Scenes/Scene.physics.cpp` (step 1d), `AbstractEntity`, `Manager.console.cpp`; projet-alpha `CollisionDebug` | 2026-10-02 | clang-tidy 21.1.6, new files and changed lines: 3 fixed (special-member-functions: `Component::Vehicle` deletes its copy / move like `Component::Abstract`; 2 avoid-const-or-ref-data-members: the wheel cast visitor holds its sphere and motion by value); kept ON PURPOSE in the category already below: 1 `cppcoreguidelines-pro-type-static-cast-downcast` (`TriangleMeshCollisionModel` after its `modelType()` tag, the wheel cast against a mesh). The base header: base `docs/static-analysis-ledger.md`. The brake fix (the locked wheel, `SoftStepSolver.cpp`, 2026-10-02): changed lines 0. | Vehicle |
| `Graphics/IrradianceProbeVolume.cpp` (its ray rotation through base `PortableRandom::UniformReal`) | 2026-10-02 | clang-tidy 21.1.6, changed lines: 1 fixed (misc-const-correctness). 0 left. | Portable random |
| `Scenes/AbstractEntity.cpp` (`setCollisionModel()` notifies), `Node.cpp` / `StaticEntity.cpp` (`onContentModified()` through `weak_from_this()`) | 2026-10-02 | clang-tidy 21.1.6, changed lines: 0. | Spinner phase |
| `Scenes/Scene.cpp` (the crawl refiles a moved renderable node), `AbstractEntity.hpp` (`movedSinceFiled()` / `recordFiledFrame()`) | 2026-10-02 | clang-tidy 21.1.6, changed lines: 0. | Wheels refile |
| `Saphir/LightGenerator.cpp` (the thin-surface grab-pass transmission unit, the ambient diffuse weighted by the transmission) | 2026-10-03 | clang-tidy 21.1.6: 0 on the changed lines; the file's 10 findings (9 convert-to-static accessors, 1 branch-clone) predate them. | Glass fix |
| `Physics/Vehicle` (`VehicleSettings::maxSlopeAngle`), `Scenes/Scene.physics.cpp` (the wheel casts' slope limit) | 2026-10-03 | clang-tidy 21.1.6: 0 on the changed lines; the files' older findings are unchanged. | Vehicle slope |
| `Graphics/Material/StandardResource` (the emissive factor; the UV set in the UVW table, `setComponentUVWChannel()`, `usePrimaryTextureCoordinatesOnly()`), `Saphir/LightGenerator.cpp` (the ambient diffuse weight), `Scenes/Loaders/GLTFLoader` (`TEXCOORD_1`, the `…-uv0` variant), `Graphics/Geometry/IndexedVertexResource` / `VertexResource` | 2026-10-03 | clang-tidy 21.1.6, changed lines: 2 fixed (a nested conditional in `transformedTexCoords()`, a repeated branch in the per-primitive material choice); 5 left ON PURPOSE, the file's established patterns: 3 static-cast-downcast (a component to `Texture` after its `type()` check), 2 constant-array-index (the material property table at a computed offset). | Materials |
| `Vulkan/Utility` (`pipelineCreationFeedbackAvailable()`, `reportPipelineCreation()`), `GraphicsPipeline.cpp`, `ComputePipeline.cpp` (timed creations with feedback) | 2026-10-03 | clang-tidy 21.1.6: 0 on the new and changed lines; the pipelines' older findings unchanged. | Pipeline stalls |
| `Scenes/Scene.rendering.cpp` (instances prepared outside the components lock), `Tracer` (`currentThreadID()`), `Vulkan/Utility` + `GraphicsPipeline` (the report's label, thread, location), `Saphir/Generator/Abstract.cpp` (the label) | 2026-10-03 | clang-tidy 21.1.6: 0 on the new and changed lines (the reported `Tracer.hpp` lines are older). | Pipeline stalls |
| Physics P5: `Physics/TriangleMeshCollisionModel` (new), `NarrowPhase`, `MovableTrait`, `Scenes/Scene.physics.cpp` (islands, mesh pairs), `Node`, `AbstractEntity(.debug)`, `Editor/Manager`, the push modifiers, `Toolkit.hpp`; projet-alpha `CollisionDebug` | 2026-10-02 | clang-tidy 21.1.6, changed lines and new files: 1 fixed (missing-std-forward: `forEachWorldTriangle()` takes its visitor by const reference); kept ON PURPOSE in the categories already below: 4 `cppcoreguidelines-pro-type-static-cast-downcast` (`TriangleMeshCollisionModel` after its `modelType()` tag, as the other models); projet-alpha: 0. The base header's own findings: base `docs/static-analysis-ledger.md`. | P5 |
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
| Bug hunt 2026-10-12, lots 1-2: the peers' fixes (`Core.cpp`, `Vulkan/Instance.cpp`, `Device.cpp`, `GPUProfiler.cpp`, `Graphics/Renderer.cpp`, `RenderableInstance/Abstract.cpp`, `Window.cpp`), the TBN refusal, sample counts, KHR meshopt, `ResourceTrait.cpp`, `FileSystem.cpp`, `Texture2D.cpp`, `ParticlesEmitter.cpp`, the tick clocks (`Sequence`, `AmbienceChannel`, `Particle`, `Scene`, `Node`, `Core`), `DirectionalLight.cpp` / `LightSet.cpp`, `Console/JSONNumber.hpp`, the 17 `.at()` sites, rule C steps 1-2 | 2026-10-12 | clang-tidy 21.1.6, changed lines: 6 fixed (pro-bounds-constant-array-index in `ViewMatrices3DUBO.cpp` and `CubemapResource.cpp`: `std::next`, the view copy as `std::ranges::copy_n`). 0 left. | Bug hunt |
| `src/Vulkan/Device.cpp` / `.hpp` (`createPipelineCache()`, `restorePipelineCache()`, `getPipelineCacheData()` merging into a temporary cache, the published handle), `src/Graphics/Renderer.cpp` / `.hpp` (`loadPipelineCache()` on a `Base::Thread`, the join before the save) | 2026-10-12 | clang-tidy 21.1.6: 4 on the changed lines, ON PURPOSE: readability-qualified-auto ×4 on `VkPipelineCache` handles (`const auto` kept: a non-dispatchable handle is a pointer on some platforms only; `const VkPipelineCache` trades it for misc-misplaced-const). | Asynchronous pipeline cache restore (P2) |
| `Vulkan/Image` (`deviceMemoryBytes()`), `Graphics/Material/StandardResource` (`textureUsages()`), `Graphics/Renderer.console.cpp` (`getTextureMemory`, the texture memory report) | 2026-10-20 | clang-tidy 21.1.6, changed lines: 1 fixed (readability-function-cognitive-complexity: the report split into `reportTextures()` / `reportMaterials()` / `measureMaterial()` / `materialJSON()`); kept ON PURPOSE in the 7b category: 1 pro-type-static-cast-downcast (`textureUsages()`, a component cast to `Texture` after its `type()` check). cppcheck 0. include-what-you-use: the three touched files brought clean (`Image.hpp` keeps `ExternalImageDescriptor.hpp`, needed by the `IS_WINDOWS` / `IS_MACOS` import factories only). | Texture packing L0 |
| `Graphics/Material/Component/Texture` (the JSON constructor's default source channel; split into `parseUsage()` / `findStoreTexture()` / `parseVector3()` / `parseSamplingOptions()`), `Graphics/Material/StandardResource` (`defaultSourceChannel()`, every scalar component's swizzle) | 2026-10-20 | clang-tidy 21.1.6, changed lines: 1 fixed (readability-function-cognitive-complexity 43 on the `Texture` JSON constructor, a baseline brought clean because its signature changed). cppcheck 0. include-what-you-use: `Component/Texture.hpp` / `.cpp` brought clean (forward declarations of `FillingType` and `AbstractServiceProvider`). | Texture packing L1 |
| `Graphics/Material/StandardResource` (`createTextureComponents()`, `distinctTextureCount()`, one binding per distinct texture, the layout identifier), `Component/Texture` (`shareSamplerOf()`), `Saphir/AbstractShader.cpp` (an identical sampler re-declaration is silent) | 2026-10-20 | clang-tidy 21.1.6, changed lines: 2 fixed (qualified-auto, designated initializer); kept ON PURPOSE in the 7b category: 1 pro-type-static-cast-downcast (`createTextureComponents()`, after the `type()` check). cppcheck 0. include-what-you-use: `AbstractShader.cpp` brought clean. | Texture packing D1 part 1 |
| `Graphics/ImageResource` (`PackRecipe`, the packed image), `Graphics/Material/StandardResource` (`planPackedTextures()` and its helpers, `parseComponents()` split out of `load()`), `Component/Texture` (the `Packed/` lookup), `Scenes/SceneMetaData.cpp` + `Effects/Shared/RTAlphaTestGLSL.hpp` (the RT opacity channel), `SettingKeys.hpp` | 2026-10-20 | clang-tidy 21.1.6, changed lines: 13 constant-array-index fixed (the recipe reads its channels through a `switch` and range-for), 3 cognitive complexity fixed (`compose()`, `planPackedTextures()`, `load()` split). cppcheck 0. include-what-you-use: the touched files brought clean (`ImageResource.hpp` forward-declares `StaticVector`). | Texture packing L2 |
| `Graphics/TexturePacker` (new), `Material/StandardResource` (the planner on the packer), `Scenes/Loaders/FBXLoader.cpp`, `Scenes/Toolkit.cpp`, `Component/Texture` (`hasExpectedColorSpace()` channel), `Graphics/TextureCache` (the mip count in the key) | 2026-10-20 | clang-tidy 21.1.6, changed lines: 2 qualified-auto fixed (the `std::array` lookup became a loop: `const auto *` on a `std::array` iterator would not compile with MSVC, whose iterator is a class; the `StaticVector` one is a pointer everywhere). cppcheck 0. include-what-you-use: the six touched files brought clean (two forward declarations in `FBXLoader.cpp` / `Toolkit.cpp`). | Texture packing L2c |
| `Graphics/TextureResource/AnimatedTexture2D` (`createCompressedImage()`, BC7 frames), `Vulkan/Image` (`createFromCompressed()` with layers) | 2026-10-20 | clang-tidy 21.1.6, changed lines: 0. cppcheck 0. include-what-you-use: the two files brought clean; `AnimatedTexture2D.hpp` keeps `Graphics/MovieResource.hpp` as `// IWYU pragma: export` (its `load()` takes a `MovieResource`: removing it broke projet-alpha's `AbstractDemo.cpp`, which relied on it). | Animated textures BC7 |
| `Graphics/Material/StandardResource` (`sharesFetch()`, `texelExpression()`, `declareSharedFetches()`: one fetch per canonical texture, D10) | 2026-10-20 | clang-tidy 21.1.6, changed lines: kept ON PURPOSE in the 7b category: 1 pro-type-static-cast-downcast (a component cast to `Texture` after its `type()` check). cppcheck 0. include-what-you-use clean. | Texture packing D10 |
| `Scenes/Loaders/GLTFLoader.cpp` (the data maps through the texture packer, L2d) | 2026-10-20 | clang-tidy 21.1.6, changed lines: 2 fixed (nested conditional operators → `if`). cppcheck 0. include-what-you-use: the file brought clean (7 unused includes removed, one forward declaration). | Texture packing L2d |
| `Graphics/ImageResource` (`PackRecipe`, the packed image), `Graphics/Material/StandardResource` (`planPackedTextures()` and its helpers, `parseComponents()` split out of `load()`), `Component/Texture` (the `Packed/` lookup), `Scenes/SceneMetaData.cpp` + `Effects/Shared/RTAlphaTestGLSL.hpp` (the RT opacity channel), `SettingKeys.hpp` | 2026-10-20 | clang-tidy 21.1.6, changed lines: 18 fixed (13 constant-array-index — the recipe reads its channels through a `switch` and range-for, the resample unrolls its four taps; 3 cognitive complexity — `compose()`, `planPackedTextures()`, `load()` split; parentheses, const, widening in the base tests). cppcheck 0. include-what-you-use: the touched files brought clean (`ImageResource.hpp` forward-declares `StaticVector`). | Texture packing L2 |

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
- **portability-template-virtual-member-function ×18** (recorded 2026-10-04; 15 pre-existing, + `releaseIdleLocalData()`,
  `findResource()`; + `stopBackgroundWork()` 2026-10-15)
  — `Container< resource_t >`: the `ContainerInterface` virtuals are implemented by the container TEMPLATE, the
  manager's type-erased access to every container (the design itself). Every specialisation is instantiated by
  `Manager::onInitialize()`, so no compiler leaves one out.

### `src/Scenes/Loaders` (2026-09-30)

- **pro-type-union-access ×143** — FBXLoader only: the ufbx API is unions (`ufbx_vec3::x`, …; +2 in
  `readEmbeddedTexture()`, 2026-10-04).
- **pro-bounds-constant-array-index ×18** — each one checked bounded: FBX (`texIdx` checked, `k < 3`, `wi < take ≤ 4`,
  ufbx indices consistent by construction), glTF (texture index checked, literal loops), USD (literal 4×4 loops), WAD
  (counts derived from the lump size; cross-references checked).
- **pro-type-reinterpret-cast ×7** — binary buffers (a WAD name, the file read — checked since 2026-10-18 —, meshopt / Draco input — its offset is
  bounded upstream —, the USDZ asset map, Tydra's raw normal / UV buffers — sized by the vertex count check).
- **avoid-const-or-ref-data-members ×3** — each loader holds the `Resources::Manager &` it was built with: a
  short-lived, non-copyable object by design.
- **clang-analyzer-optin.performance.Padding ×1** — a lambda CLOSURE's layout (GLTFLoader).
- **use-enum-class + enum-size** — `LoaderCapabilityBits`: a bit-flag set combined with `|`, `uint32_t` on purpose.
- **WAD split (2026-10-18)** — the WAD analysis moved verbatim to `WADParser.cpp` (`WAD::parse()`): its findings moved
  with it, unchanged — `pro-bounds-constant-array-index` ×5 (the grey palette fill, the palette lookups and a corner index,
  bounded as above) and `pro-type-reinterpret-cast` ×1 (`readName8()`, a WAD name). `WADLoader.cpp` keeps the file
  read's cast.
- **FBX split (2026-10-18)** — the FBX analysis moved verbatim to `FBXParser.cpp` (`FBX::analyse()`,
  `FBX::analyseClipsOnly()`): its findings moved with it — `FBXLoader.cpp` had 195 `pro-type-union-access` and 7
  `pro-bounds-constant-array-index`; `FBXParser.cpp` + `FBXLoader.cpp` now have 188 (185 + 3: ufbx's vector, matrix and
  string types are unions, read member by member — 4 of them in the two new animation-stack warnings) and 7 (4 + 3: the joint / weight arrays of a vertex and the two colour-space
  slots of a texture, indexed by bounded loop counters), plus the loader's `Resources::Manager &` member. The one new
  finding (an `operator+` chain building a clip's resource name) was fixed with `String::concatenate()`.
- **glTF split (2026-10-18)** — `GLTFLoader.cpp` split into `GLTFParser.cpp` (the analysis) and the materializing
  `GLTFLoader.cpp`: the same findings as before, moved — `pro-bounds-constant-array-index` ×5 (2 + 3),
  `pro-type-reinterpret-cast` ×2 (the Draco decoder's byte buffer, the GLB header bytes), the `optin.performance.Padding`
  of a material closure. Three new `performance-inefficient-string-concatenation` of the split (skeleton and clip
  resource names) were fixed with `String::concatenate()`.
- **USD split (2026-10-18)** — `USDLoader.cpp` split into `USDParser.cpp` (the analysis), `USDZArchive.cpp` (the
  archive) and the materializing `USDLoader.cpp`: the same findings as before, moved — `pro-bounds-constant-array-index`
  ×2 and `pro-type-reinterpret-cast` ×3 (lightusd's raw byte buffers and the archive's entry bytes), plus the loader's
  `Resources::Manager &` member. Four new ones of the split itself were fixed: a moved-from string read (the dome
  image path, now `std::exchange`d), an rvalue `USD::Mesh` never moved (the consumer takes it by value) and two
  indexed copies of a USD matrix (`toArray()` spells the sixteen elements).

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
- **pro-type-static-cast-downcast ×10** — a material component cast to `Component::Texture` right after a
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
  constructor, engine item `render-target-view-distance-ignored` (the merged one, 2026-10-11).

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
  Same reason, added 2026-10-19: `EMEN_RT_SCENE_BUFFERS_GLSL`, `EMEN_RT_SCENE_BINDLESS_OPEN_GLSL`,
  `EMEN_RT_SCENE_ACCESSORS_GLSL` (`Effects/Shared/RTAlphaTestGLSL.hpp`: `EMEN_RT_SCENE_DATA_GLSL` split so that each macro
  holds one raw string — § cppcheck).
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
- **Vulkan::Loader + VMA through volk (2026-10-11), ON PURPOSE ×4** — `Loader.cpp`: `reinterpret_cast` of `dlsym()` /
  `GetProcAddress()` to `PFN_vkGetInstanceProcAddr` (the only way to fetch a symbol; POSIX guarantees the
  conversion), `dlerror()` (`concurrency-mt-unsafe`: main thread, during initialisation, and glibc keeps it per
  thread); `Device.cpp`: `VMA_STATIC_VULKAN_FUNCTIONS` / `VMA_DYNAMIC_VULKAN_FUNCTIONS` (`macro-usage`: VMA's own
  configuration contract, defined before its implementation like `VMA_IMPLEMENTATION`).
- **concurrency-mt-unsafe ×3 (2026-10-11), ON PURPOSE since the fix** — `getenv()` / `setenv()` in `PlatformSpecific::prependVulkanLayerDirectory()` (`Helpers.linux.cpp`): called only by `PlatformManager::prepareVulkanLoaderEnvironment()`, which the consumer's `main()` runs before CEF and before any engine thread (owner decision 2026-10-11); `PlatformManager::onInitialize()` only reads the environment.
- **concurrency-mt-unsafe ×1 (2026-10-11), ON PURPOSE** — `getenv()` in `PlatformSpecific::readEnvironmentVariable()` (`Helpers.linux.cpp`, same in `Helpers.mac.cpp`), which `PlatformManager::onInitialize()` uses to READ the loader environment written single-threaded by `prepareVulkanLoaderEnvironment()`; concurrent reads do not race, only a write does, and none happens after `main()`'s call. Windows reads the process block with `GetEnvironmentVariableW` instead (MSVC C4996 refuses `getenv`, and the CRT copy never sees a `SetEnvironmentVariableW`).
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
- **cppcoreguidelines-pro-type-reinterpret-cast ×1 (2026-10-11), ON PURPOSE** — `entryPoint()` in `Vulkan/PhysicalDeviceProbe.cpp`: `vkGetInstanceProcAddr` answers a generic `PFN_vkVoidFunction` that the Vulkan binary interface requires to be cast to the entry point's real signature. The probe fetches its own entry points on purpose (it must never touch volk's global pointers, `Loader::VolkBinding::None`).

## Clang Static Analyzer — first run (2026-10-17, Ave Robustus II P-W)

How: `analyze-build-19 --cdb <db> --sarif --enable-checker optin.cplusplus.UninitializedObject --enable-checker
optin.cplusplus.VirtualCall`, the database being projet-alpha's clang tree (`.claude-build-asan`) filtered to the
cascade's OWN sources: 629 TUs (emeraude-base `src/`, the engine `src/`, projet-alpha `src/`; no third-party TU). The
default checkers do not report inside headers (`--analyze-headers` not used). Result: 11 findings — emeraude-base 0,
the engine 10, projet-alpha 1.

Fixed (3, minor; 0 finding left on their TUs after the fix):
- `Graphics/VolumetricImageResource.cpp` `core.DivideZero` — unreachable (the members are set to 32 just before), but a
  `uint8_t` write may alias them: the gradient divides by the local `DefaultSize` now (no reload per texel either).
- `Vulkan/SwapChain.cpp` `deadcode.DeadStores` — the present-mode reason's "default fallback" text was never shown
  (every branch names its own): initialised to `nullptr`.
- `Saphir/MeshShader.cpp` `deadcode.DeadStores` — the `Triangles` case rewrote the initial value: it keeps it.

False positives (8), kept with their reason:
- `Graphics/Renderer.cpp:2472` `cplusplus.Move` — `m_sceneTarget` moved into a deferred retirement: the call is
  reached only through `m_sceneTarget != nullptr` (`renderFrame()`), a moved-from `shared_ptr` is null; the checker
  does not take the comparison as a re-initialisation.
- `Scenes/Loaders/USDLoader.cpp:855 / 933 / 943 / 948 / 955` (in `USDParser.cpp` since the 2026-10-18 split) `cplusplus.NewDeleteLeaks` — the allocation is
  lightusd's `Animatable` copy constructor (`_ts.reset(new TimeSamples(...))`) inside the `std::optional` returned by
  `get_value()`: owned by a `unique_ptr`, freed with the copy.
- `Scenes/Loaders/USDLoader.cpp:1961` (`USD::forEachMesh()` in `USDParser.cpp` since the 2026-10-18 split) `core.NullDereference` — the vertex loop reads `points[index]` under
  `index < points.size()` and a structured binding captured by reference, alive for the iteration.
- projet-alpha `src/UI/WebView.CefRenderHandler.cpp:383` `core.NullDereference` — `srcBuffer`, a local captured by
  reference in a lambda that `writeActiveBufferWithMapping()` runs synchronously (through a `std::function` the
  analyzer does not follow); the popup buffer and its size are stored together (`width × height × 4`).

## CodeChecker — cross-TU analysis (2026-10-19, Ave Robustus II P-W (2))

**CodeChecker 6.29.1** (pipx), its `clangsa` analyzer (clang 19.1.7) with `--ctu`: the call graph is followed ACROSS
translation units, which the first run above (one TU at a time) could not. The database: projet-alpha's GCC database
cleaned for clang (the clang-tidy runner's), filtered to the cascade's OWN sources — 632 TUs (emeraude-base `src/`, the
engine `src/`, projet-alpha `src/`), a skip file keeping the reports inside them:

```bash
cat > skip.txt <<'SKIP'
+*/projet-alpha/src/*
+*/emeraude-engine/src/*
+*/emeraude-base/src/*
-*
SKIP
CodeChecker analyze <filtered db>/compile_commands.json --analyzers clangsa --ctu -j 16 -i skip.txt -o reports
CodeChecker parse reports -i skip.txt
```

17 minutes on Linux (`-j 16`: the CTU phase is memory-hungry, 2.8 GB peak per process). **15 reports**:
- 7 already triaged above as false positives (the `Renderer` move, the five lightusd `NewDeleteLeaks` in `USDParser`,
  projet-alpha's popup copy) — cross-TU found NO new defect in them;
- 8 `security.FloatLoopCounter` (a checker of CodeChecker's default profile, off in `analyze-build`), all in projet-alpha
  demos: fixed there (its ledger), the three TUs at 0 report after.

The other CodeChecker analyzers are run on their own (clang-tidy, cppcheck: their sections). `gcc` (GCC's
`-fanalyzer`, C++ support experimental in GCC 14) has not been run: an owner decision pending.

## include-what-you-use (owner decision 2026-10-19, Ave Robustus II P-W (2))

**Version: include-what-you-use 0.23**, the rules of emeraude-base's ledger § include-what-you-use (the files a lot
touches come out IWYU-clean; forward declarations allowed, each verified by compiling; never IWYU's
`fix_includes.py`). **Configuration**: `iwyu-mapping.imp` at the repository root — it `ref`s the base's mapping, then:
Vulkan through `"volk.h"` only (`VK_NO_PROTOTYPES`), jsoncpp through `"json/json.h"`, lightusd through `"lightusd.hh"`
/ `"tydra/render-data.hh"`, fastgltf's math / util through `"fastgltf/types.hpp"`, Draco through its decoder header,
libvpx through `"vpx/vpx_encoder.h"`, OpenAL quoted as the engine writes it, the x86 intrinsics through
`<immintrin.h>`, the generated `emeraude_*.hpp` quoted, and emeraude-base's configuration through
`"emeraude_config.hpp"`.

**The command** (Linux; projet-alpha's database cleaned for clang — the clang-tidy runner's; the touched `.cpp` files as
TUs, the touched headers through the template guard, which includes every header):

```bash
iwyu_tool -p <clean db> <touched .cpp files> -- -Wno-unknown-warning-option \
	-Xiwyu --mapping_file=iwyu-mapping.imp -Xiwyu --cxx17ns > pass.txt
iwyu_tool -p <clean db> tools/template-guard/TemplateInstantiation.cpp -- -Wno-unknown-warning-option \
	-Xiwyu --mapping_file=iwyu-mapping.imp -Xiwyu --cxx17ns -Xiwyu --check_also=<each touched header> > guard.txt
python3 <projet-alpha>/tools/iwyu-apply.py . <touched list> pass.txt guard.txt
```

Then the forward declarations it prints (in the file's `/* Forward declarations. */` block), a build of the three
cascades (GCC Release and Debug, clang with `-Wdocumentation`), and a second IWYU pass: it must converge.

⚠️ **Traps met on the first lot (2026-10-19):**
- `vpx/vpx_integer.h` only includes `<stdint.h>`: mapped to `"vpx/vpx_encoder.h"`, it sent EVERY integer type to
  libvpx (`AnimationClipResource.hpp` asked for the video encoder). Mapped to `<cstdint>`.
- emeraude-base's `Network/asio_throw_exception.hpp` defines asio's `throw_exception()` under `-fno-exceptions`;
  nothing names its symbols, IWYU removed it from `Net/Manager.cpp` and `Net/TCPServer.cpp` and the LINK failed. The
  header carries `// IWYU pragma: always_keep` now (base), and it stays FIRST in its section (it sets the ASIO macros).
- IWYU on Linux sees neither the `#ifdef _WIN32` nor the macOS branches: a POSIX header it asks for (`<sys/select.h>`
  in `Net/UDPClient.cpp`) goes BY HAND inside the file's POSIX block, never into the unconditional STL section; a
  removed include is checked against the platform branches before it goes (none were used there).
- A header that used a type through another header's include breaks when that include becomes a forward declaration
  (`Scenes/AbstractEntity.hpp` used `Resources::Manager`): it declares what it uses itself.
- `tools/iwyu-apply.py` (projet-alpha): an emeraude-base header is LOCAL for the engine; a section it creates goes in
  the cascade's order; a section title followed by a preprocessor conditional is kept.

⚠️ **2026-10-25 — IWYU had not run since 2026-10-21** (the base mapping's `<features.h>` entry made it abort every TU;
base ledger § include-what-you-use). The physical sky's files were brought clean on its return: `SkyAtmosphere.cpp`
+6, `CelestialClock.cpp` +5, `EnvironmentConsoleAdapters.cpp` +12 and a forward declaration of `Scenes::Manager`,
`CelestialClock.hpp` two forward declarations. Known IWYU false suggestion, kept: `CelestialClock.hpp` asked to replace
`"Scenes/SkyState.hpp"` by a forward declaration, but `m_renderStates` is a `std::array< SkyState, N >` member (a
complete type) — `// IWYU pragma: keep (<reason>)`. The rest of the window's debt: item `iwyu-debt-since-2026-10-21`.
The same day, the sky's RGBA32F tables (`SkyAtmosphere.{hpp,cpp}`, `StorageTexture2D.{hpp,cpp}` — `setFormat()`):
clang-tidy 21.1.6 on the changed lines 0, cppcheck 0, IWYU brought clean (`StorageTexture2D.hpp` forward-declares
`Vulkan::Image` / `ImageView` / `Sampler`, the `.cpp` `Settings`). macOS-PA's IWYU (Apple libc++) still asks
`StorageTexture2D.cpp` for `<string_view>` (an implicit use through a string_view parameter); Linux does not —
an Apple-libc++-only suggestion, not applied. The planet identity lot touched `Scenes/Manager.console.cpp` (in the
debt list): brought clean, +54 −2 and a forward declaration of `Console::Argument`; IWYU's removal of
`Component/Camera.hpp` / `Microphone.hpp` refused (`componentBuilder< Component::Camera >` instantiates a template
that needs the complete type: the build failed) — `// IWYU pragma: keep (<reason>)`.
The sky's own IBL lot (same day; `Graphics/SkyEnvironment` new, `Compute/IBLBaker`, `IBLTexture`, `SkyAtmosphere`,
`Material/SkyAtmosphereResource`, `Renderable/DynamicSkyResource`, `Renderer`, `Scene.lighting.cpp`, `Constants.hpp`):
clang-tidy 21.1.6 on the changed lines 8 fixed (swappable parameters → named structs `IBLBaker::CubeBarrier`,
`SkyEnvironment::Inputs`, a reordered `recordPrefilterMip()`; `auto * const` for the Vulkan handles; a ring index
through `std::next`; an ambiguous `reset()` → `= nullptr`), cppcheck 0, IWYU brought clean with seven forward
declarations. The template guard gained the headers it lacked: `SkyAtmosphere.hpp`, `StorageTexture2D.hpp`
(forgotten in lot 1), `SkyEnvironment.hpp`, `TexturePacker.hpp`.

**First lot (2026-10-19)**: the 49 files of the engine's cppcheck lot (34 `.cpp`, 15 headers): about 800 includes added
and 115 removed, 41 forward declarations, converged at the second pass (three residual lines applied); the three
cascades 0 error, 0 warning. Section layouts that predate the pass were left as they are (`GLTFParser.cpp`'s leading
fastgltf block, `SceneRendering.cpp`'s two local blocks, `UDPClient.cpp`'s third-party block after the local one).

## The template guard (owner decision 2026-10-19, Ave Robustus II P-W (2))

A member of a class template is compiled only when something calls it, a header only when a translation unit includes
it (emeraude-base found 19 never-compiled members this way, its caution points § Templates).
`tools/template-guard/TemplateInstantiation.cpp` includes EVERY engine header (an OS-suffixed one on its OS only) and
explicitly instantiates the 12 class templates with every argument the cascade uses (the three RenderTarget templates
for their view-matrix UBOs, `Resources::Container` for the 33 containers `Resources::Manager` registers, `OctreeSector`
and its crawler, `NodeCrawler`, `ConsoleAdapter` / `ComponentBuilder` for the components, `BuiltEntity`, the Tracer
helpers).

It is the OBJECT target `EmeraudeTemplateGuard` (owner's choice: always built, never linked — the library gains
nothing). On macOS its source is `TemplateInstantiation.mac.mm`, which includes the `.cpp` and is compiled as
Objective-C++ with ARC like the engine's `.mm` files: `PlatformSpecific/StringConversion.mac.hpp` refuses plain C++
(`#error`, `#import`), which broke the first macOS build of the guard (macOS peer, 78ce1936). It links the engine and the engine's own link libraries (their include directories stay SYSTEM, as in the
engine's translation units). The engine is an `EXCLUDE_FROM_ALL` subproject and the guard links it, so the engine
cannot depend on it (a cycle): an APPLICATION adds `add_dependencies(<its target> EmeraudeTemplateGuard)` — projet-alpha
does.

Rules: a new header joins the list, a new class template its instantiation(s); a member valid for some arguments only
takes a `requires` clause; a specialization a `.cpp` instantiates is `extern template` in its header, never repeated
in the guard (MSVC C4661). A `Container` is instantiated only for a REGISTERED resource type.

First run: the 546 headers compiled together (GCC, clang with `-Wdocumentation`), 4 of them included by no translation
unit before (`Audio/Speaker.hpp`, `Physics/PointCollisionModel.hpp`, `Scenes/OctreeSectorCrawler.hpp`,
`Graphics/RenderTarget/AbstractFilter.hpp`). Two `OctreeSector` members had never compiled, fixed:
`getRootSector()` (both) assigned the parent's `weak_ptr` to a `shared_ptr` (now `lock()`, stopping at an expired
parent), `getDeepestSubSectorForPosition()` called a `center()` that does not exist (`centroid()`; its slot formula
matches the subsector layout). Read on the way, a defect fixed the same day (owner decision: register the container):
`Texture3D::load()` dereferenced the null `container< VolumetricImageResource >()` — no such container was registered.
`Resources::Manager` registers the "Volumetric image manager" (store `VolumetricImages`), `Texture3D` refuses a provider
without it, and the guard instantiates its `Container`. Verified on a test instance (validation layer loaded, 0 VUID):
`loadResource(Texture3DResource, <name>)` is Loaded, both containers report one loaded resource (the generated default
volume: volumetric files are not read yet).


## cppcheck (owner decision 2026-10-19, Ave Robustus II P-W (2))

**Version: cppcheck 2.22.0** — installation, the per-OS defines and the general traps: emeraude-base's
`docs/static-analysis-ledger.md` § cppcheck. **Configuration**: `cppcheck-suppressions.txt` at the repository root — the
base's list, plus `duplInheritedMember` (owner: the `ClassId` / `getClassUID()` naming pattern; `-Woverloaded-virtual`
catches the dangerous case), `funcArgNamesDifferent` (gated by clang-tidy's
`readability-inconsistent-declaration-parameter-name`), `constParameterCallback` (a third-party C callback's
`void * userdata`) and the engine's third-party directories. **Gate**: zero finding on the touched TUs (error, warning,
style, performance, portability; `information` lines are not findings). The command (Linux, about 10 minutes for the
whole engine with `-j 30`):

```bash
cppcheck --project=<projet-alpha build>/compile_commands.json --file-filter='*/emeraude-engine/src/*' \
	--library=posix --platform=unix64 -D__x86_64__ -D__linux__ -D__GNUC__=14 --std=c++20 \
	--check-level=exhaustive --enable=all --inconclusive --inline-suppr \
	--suppressions-list=cppcheck-suppressions.txt --cppcheck-build-dir=<scratch dir> -j $(nproc) \
	--template='{file}:{line}:{column}: {severity}: {message} [{id}]' > cppcheck.out 2> cppcheck.txt
```

⚠️ **Three ways to a false zero or a skipped TU**, all met on the first sweep:
- the base's former `*/dependencies/*` third-party pattern matched the engine's own path to emeraude-base: EVERY engine
  finding suppressed (sweep 1: 0). The patterns name each third-party directory;
- `template< … > requires (…) using X = …;` (base `Space2D::Point`, `Space3D::Point`, `OBJTriangle`) is a `syntaxError`
  that abandons every TU including it; now type-constraints;
- **a macro that continues after a multi-line raw string** with a literal or a macro name
  (`)GLSL" EMEN_RT_SUBGEOMETRY_GLSL R"GLSL(`), expanded at namespace scope: `syntaxError` reported at an unrelated
  namespace line (`FNV1a.hpp:33`, `NameableTrait.hpp:32`), 5 TUs abandoned (RTAO, RTGI, RTR, RTContactShadows,
  IrradianceProbeVolume). Reduced to four lines; 2.23-dev too. `EMEN_RT_SCENE_DATA_GLSL` now only chains macros that each
  hold ONE raw string (the GLSL is byte-identical, 6265 bytes compared). A `#param` after a raw string is fine. Rule: a
  GLSL macro holds exactly one raw string, closing the macro; a composite macro chains names.

**First full sweep (2026-10-19)**: 111 findings and 7 `syntaxError` → 99 and 3 → 13 and 0 → **0** after the lot. Fixed:
- a null dereference in Debug (`AbstractDeviceDependentObject`: "Device is null", then `m_device->isCreated()`);
- `TCPServer`'s moves dropped `m_lastAcceptTimedOut`;
- an unchecked material in `SceneRendering::onGraphicsPipelineConfiguration()` — a renderable without a material (which
  the generator supports: neutral output) dereferenced null; `configureColorBlendState()` takes a `const
  Material::Interface *`, `nullptr` = opaque (owner decision);
- `parseSpecularComponent()`: a non-object `"Specular"` aborted in jsoncpp, and the factor was read from a `"Factor"` key
  no file wrote — every `Data` ignored since 0.7.1 (owner: read the documented `Type`/`Data`, refuse a wrong shape;
  `docs/material-json-format.md`, verified at runtime there);
- `generateGrabPassTransmissionFragmentShader()` hid two `declare()` failures; `buildLevelOfDetailTables()` clamped to a
  distance ≤ 0 instead of refusing (owner: false when no distance fits);
- functions that cannot fail return `void` (owner): `prepareMDIModelMatrix()` / `prepareInstanceModelMatrix()` (12 dead
  branches), `Core::openImageViewer()` / `openAudioTrack()`;
- `accessMoved` ×10 (`std::exchange`, or a moved-from smart pointer left as the standard leaves it), the FBX loader's
  self-assignment left by the Y-up migration (and its three stale flip comments), USD's `erase(unique)` idiom, the
  `ssdpDiscover()` socket by RAII (`ScopedSocket`), 5 records with uninitialized members, an unsigned `<= 0`, two
  write-only variables (GLTF cycle check, FrameCapture), redundant conditions (Renderer post-process block, ShadowMap /
  Texture, SpotLight, CommandBuffer, FileSystem ×3, Scene), two overrides identical to the base, an `std::forward` in a
  loop (`AbstractVirtualDevice`).

Kept ON PURPOSE (inline, each with its reason): `knownEmptyContainer` (`VideoEncoderH265`: `enumerate()` fills it),
`knownConditionTrueFalse` ×7 (a const member's default read instead of the constructor's value ×2, flags set by a
by-reference lambda or another method ×2, the RenderDoc and fault-injection stubs ×3), `duplicateAssignExpression`
(`Sequence`), `redundantInitialization` (`Net::Manager`: every variable initialized), `duplicateCondition` ×5 (two
documented generation phases or concerns), `unreadVariable` ×2 (ShadowMap / Texture `capture()`: the download ignores
the layer — engine item `render-target-capture-ignores-the-layer-index`).

**Windows (Windows peer, 2026-10-19, the corrected defines of the base ledger):** 0 finding outside the
`*.windows.cpp` (which Linux never analyses), 8 in them, fixed: `createConsole()` / `attachToParentConsole()` ignored
six `freopen_s()` results (a console its streams cannot reach is reported unusable now), `WorkerContext`'s members
initialized (`Helpers.windows.cpp`); two `danglingLifetime` in `SaveFile.windows.cpp` kept inline — a
`std::filesystem::path` assigned from a `wchar_t *` copies the characters. macOS (`-D__attribute__(x)=`): 0.

## D11 — the disabled checks ruled (owner decision 2026-10-19, Ave Robustus II P-W (5))

The `.clang-tidy` (identical in the three repositories) switches ON `cppcoreguidelines-pro-bounds-pointer-arithmetic`,
`cppcoreguidelines-pro-bounds-avoid-unchecked-container-access` (0 finding in the cascade: free), `bugprone-easily-swappable-parameters`
and `readability-function-cognitive-complexity` (default threshold 25), and sets `WarningsAsErrors: '*'` (a finding is
an error when the tool runs: the check names carry `,-warnings-as-errors`). **Gate**: zero NEW finding on the changed
lines, as for every check; the code already there is the BASELINE below, brought clean file by file when a lot touches
it — no mass campaign (owner).

Baseline (2026-10-19, clang-tidy 21.1.6, the cascade's own TUs, findings deduplicated across TUs):

| Check | emeraude-base (55 TUs) | emeraude-engine (471) | projet-alpha (106) |
|---|---|---|---|
| `pro-bounds-pointer-arithmetic` | 170 | 804 | 219 |
| `easily-swappable-parameters` | 81 | 422 | 253 |
| `function-cognitive-complexity` | 37 | 281 | 82 |
| `pro-bounds-avoid-unchecked-container-access` | 0 | 0 | 0 |

Still OFF: `cppcoreguidelines-avoid-magic-numbers` / `readability-magic-numbers` and `readability-identifier-length`
(style, not defects — D11). The others predate the base's extraction from the engine with no written reason (the
history says nothing); the reasons below are INFERRED, not ruled by the owner yet: `misc-include-cleaner`
(include-what-you-use is the gate), `misc-multiple-inheritance` (the trait design), `modernize-use-trailing-return-type`,
`readability-trailing-comma`, `portability-avoid-pragma-once` (the cascade's written style),
`readability-suspicious-call-argument` (names compared, not defects), `clang-diagnostic-unreachable-code` (presumably
the `if constexpr` platform branches).

## clang-tidy on macOS (owner decision 2026-10-19, Ave Robustus II P-W (6))

The macOS peer ran clang-tidy 21.1.6 (pipx, the same version as on Linux) with the repository's `.clang-tidy` over the
code Linux never analyses: the 14 engine `.mm`, the `.mac.cpp`, the other `PlatformSpecific/` TUs, and every shared
TU with a macOS branch — 41 TUs, the AppleClang compile database of a Claude build tree.

⚠️ **Add `--extra-arg=-resource-dir=$(/usr/bin/c++ -print-resource-dir)`**: with clang-tidy's own builtin headers, five
TUs (Core, RendererFrameScope, Overlay/Surface, Vulkan/Image, SwapChain) stop on the SDK's libc++
(`__random/clamp_to_integral.h: use of undeclared identifier 'INFINITY'`) and are only partly analysed; AppleClang's
resource directory matches the SDK.

**Result (2026-10-19)**: 659 findings; 194 only macOS sees (a `.mac.` / `.mm` file, or a macOS branch of a shared file),
the 465 others are Linux's too. No `bugprone-*`, `performance-*` or `pro-type-member-init` in the macOS code. Fixed:
`Net/WiFiScanner.mac.mm` built `std::string`s from `-UTF8String` (NULL for a string without a UTF-8 form: UB; 3
`NullablePassedToNonnull` reported, a fourth site alike) — through `PlatformSpecific::toStdString()` now, the helper
the 2026-10-01 triad wrote for exactly this; `PlatformSpecific/SystemInfo.mac.cpp`'s `realpath()` result freed by hand
— owned by a `unique_ptr` now. Kept ON PURPOSE: `Window.mac.mm` `cppcoreguidelines-owning-memory` ×2 (the Apple Event
handler is owned by a `unique_ptr` whose custom deleter deletes it — RAII, the check wants `gsl::owner<>`), and
`SystemInfo.mac.cpp`'s `owning-memory` + `no-malloc` on the deleter's `std::free()` (memory `realpath()` allocates
with `malloc()` is released by `free()` only; the call sits in the RAII owner's deleter — verified by the macOS peer,
engine 92b0f965: the nullability findings gone, nothing new on the changed lines, `getRealApplicationDir()` resolving
a symbolic link to the real bundle). The rest is
the macOS BASELINE, brought clean file by file when a lot touches it (owner, § D11's rule): 71
`misc-const-correctness`, 15 `pro-bounds-array-to-pointer-decay`, 11 `use-designated-initializers`, 9
`use-anonymous-namespace`, 9 `init-variables`, 8 `optin.osx.cocoa.localizability` (the engine has no localization
system: a product decision, not a defect), and 65 more findings of 22 style checks.

## MSVC /analyze (owner decision 2026-10-19, Ave Robustus II P-W (6))

The Windows peer ran MSVC 19.44's `/analyze` with Microsoft's `NativeRecommendedRules` rule set over the cascade's own
sources (emeraude-base `src/`, the engine `src/` with every `*.windows.cpp`, projet-alpha `src/`), in a separate build
tree: `CMAKE_CXX_FLAGS` gets `/analyze /analyze:external- /analyze:WX- /analyze:ruleset NativeRecommendedRules.ruleset`
(CMake already passes the SYSTEM includes as `/external:I`, so `/analyze:external-` skips the third-party headers;
`/analyze:WX-` keeps the C6xxx warnings non-fatal under `/WX`). About 11 minutes; the normal build is untouched. The rule
set has no Core Guidelines check (C26xxx): clang-tidy gates those.

**First run (2026-10-19)**: 13 unique findings — emeraude-base 11, the engine 2, projet-alpha 0. Fixed:
- engine `Graphics/MDI/BatchBuilder.cpp` C6011: `dispatch()` dereferenced the batch's geometry on the strength of a
  check made by `buildBatches()` only — a batch without geometry takes the individual path now, which checks it;
- engine `Scenes/AbstractEntity.debug.cpp` C6387: an out-of-range `VisualDebugType` (an enum cast from an integer)
  left `label` null — refused with the mesh;
- base `StaticVector.hpp` C6031 ×2 (`swap()`): the moved-from element destroyed through `operator[]` — through `data()`,
  as the file's other destructions;
- base `Hash/CRC32.cpp` C28020: the table loop bounded by `table.size()` instead of the literal 256.
Kept ON PURPOSE: C6262 ×5 (a frame above 16 KiB: `LZMA::Compressor::compress()`, `Decompressor::decompress()`,
`HTTPSClient::performHop()`, `Font`'s glyph-array emplace, `Synthesizer::generateSinTable()`) — the smallest thread
stack of the cascade is macOS's 512 KiB for a secondary thread, the frames stay well under it; C6993 ×3 (`Shape.hpp`,
"OpenMP constructs ignored": informational).
