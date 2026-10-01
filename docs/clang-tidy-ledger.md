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
| `src/Console` (+ `MCP/`), 12 TUs | 2026-09-30 | clang-tidy 21.1.6 after the triad pass: 24, all ON PURPOSE (below) — 21 misc-no-recursion, 1 avoid-c-arrays, 1 cppcoreguidelines-use-enum-class, 1 pro-type-reinterpret-cast. Before: 64 (clang-tidy 19, designated-initializers blind) / 66 (21). | Triad section 1, `docs/todo/triad-engine-pass.md` |
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
| `src/Graphics` 7c (`Geometry/`, `Renderable/`, `MDI/`) | 2026-10-01 | clang-tidy 21.1.6 after the triad pass: 51, all ON PURPOSE (below) — 30 constant-array-index, 4 integer-division, 4 non-private members, 3 use-enum-class, 3 qualified-auto, 2 misc-no-recursion, 2 static-cast-downcast, 1 each special-member-functions, const-ref member, implicit-widening. Before: 95. | Triad sub-section 7c |
| `src/Graphics` 7d (`Renderer` + console, `RendererFrameScope`, `Recorder`, `FrameCapture`, `RenderDocCapture`) | 2026-10-01 | clang-tidy 21.1.6 after the triad pass: 91, all ON PURPOSE (below) — 28 SIMD intrinsics, 26 reinterpret-cast, 21 constant-array-index, 5 convert-to-static, 4 owning-memory, 3 union access, 2 qualified-auto, 1 each use-after-move (false positive), use-enum-class. Before: 226. | Triad sub-section 7d |

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

### `src/Scenes/Loaders` (2026-09-30)

- **pro-type-union-access ×141** — FBXLoader only: the ufbx API is unions (`ufbx_vec3::x`, …).
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

