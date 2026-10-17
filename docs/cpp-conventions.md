# C++ Conventions - Emeraude Engine

This document describes the C++ coding conventions used in Emeraude Engine.

## General Rules

- **Standard:** C++20
- **Language:** All code, comments, and documentation in English

---

## Naming Conventions

### Classes and Structs

- **PascalCase** for class and struct names
- Suffix with purpose when relevant (e.g., `Resource`, `Manager`, `Builder`)

```cpp
class PhysicalDevice;
class TextureResource;
struct HybridGPUConfig;
```

### Methods and Functions

- **camelCase** for method and function names
- Prefix accessors with `get`, mutators with `set`, predicates with `is`/`has`/`can`

```cpp
std::string deviceName() const noexcept;      // Simple accessor (no get prefix)
Vendor vendor() const noexcept;               // Simple accessor
void setEnabled(bool state) noexcept;         // Mutator
bool isFullscreenMode() const noexcept;       // Predicate
bool hasExtension(const char* name) const;    // Predicate
```

### Variables

- **camelCase** for local variables and parameters
- **m_** prefix for member variables
- **s_** prefix for static member variables
- **g_** prefix for global variables (avoid when possible)

```cpp
class Device
{
    VkDevice m_device{VK_NULL_HANDLE};        // Member variable
    static size_t s_instanceCount;             // Static member

    void create(bool useVMA)                   // Parameter: camelCase
    {
        const auto deviceInfo = getInfo();     // Local: camelCase
    }
};
```

### Constants and Enums

- **PascalCase** for enum values
- **PascalCase** or **UPPER_SNAKE_CASE** for constants (project uses PascalCase)
- Always use `enum class` for type safety

```cpp
enum class DeviceAutoSelectMode : uint8_t
{
    DontCare = 0,
    Performance = 1,
    PowerSaving = 2,
    Failsafe = 3
};

enum class Vendor : uint32_t
{
    Unknown = 0,
    AMD = 0x1002,
    Nvidia = 0x10DE,
    Intel = 0x8086
};
```

### Acronyms

- **Acronyms are always fully uppercase** in variable names, method names, and member names
- This applies to well-known graphics/engine acronyms: BLAS, TLAS, VBO, IBO, UBO, SSBO, MVP, HDR, PBR, LOD, CSM, PCF, BDA, MDI, etc.
- Non-acronym compound words remain camelCase

```cpp
// CORRECT — acronyms uppercase
AccelerationStructure m_TLAS;
const auto & BLAS () const noexcept;
VkBuffer m_VBO{VK_NULL_HANDLE};
ShaderStorageBufferObject m_SSBO;

// WRONG — acronyms lowercased or mixed
AccelerationStructure m_tlas;     // Should be m_TLAS
const auto & blas () const;       // Should be BLAS()
VkBuffer m_vbo{VK_NULL_HANDLE};   // Should be m_VBO
```

### Trademarks and Brand Names

Trademarks and official brand names (e.g. **macOS**, **iOS**, **iPadOS**, **OpenGL**, **DirectX**) have a canonical casing owned by the brand. Two rules apply depending on whether the text is user-visible or purely internal to the code:

- **User-visible text** — log messages, UI strings, console output, documentation comments, any string literal that may be displayed or printed publicly: **respect the official casing** of the trademark (e.g. `macOS`, not `MacOS` or `MACOS`).
- **Internal code identifiers** — class names, method names, variable names, enum values, setting key C++ constants: follow the engine's standard **PascalCase / camelCase** rules. The trademark exception does **not** apply here; consistency of the naming scheme takes priority (e.g. `MacOS` in a PascalCase identifier, `macOS` in a setting path string that is displayed/logged).

```cpp
// Setting path — user-visible (printed in config files, logs): official casing
constexpr auto CompatibilityMacOSSampleKey{"Core/Compatibility/macOS/Sample"};

// C++ identifier — internal only: PascalCase, no trademark exception
constexpr auto DefaultCompatibilityMacOSSample{false};

// Comment — user-visible: official casing
// Use this on macOS to bypass the Cocoa dialog path.

// Log message — user-visible: official casing
Tracer::info(ClassId, "Running on macOS, enabling legacy path.");
```

The distinction: code identifiers are read by compilers and follow a *grammar*; displayed strings are read by humans and must respect the *brand*. Mixing the two breaks either readability or trademark fidelity.

### Encapsulation — No Public Data Members

- **All data members must be private** (or protected when inheritance requires it)
- Expose via public accessors (getters) and mutators (setters)
- This includes static service pointers — they go private with a setter

```cpp
// WRONG — public member
class Renderer
{
    public:
        bool m_vsyncEnabled{false};  // Exposed directly
};

// CORRECT — private member + accessor
class Renderer
{
    public:
        [[nodiscard]]
        bool
        vsyncEnabled () const noexcept
        {
            return m_vsyncEnabled;
        }

    private:
        bool m_vsyncEnabled{false};
};
```

### Namespaces

- **PascalCase** for namespaces
- Main namespace: `EmEn` (Emeraude Engine)

```cpp
namespace EmEn::Vulkan
{
    class Instance;
}
```

---

## Member Layout and Padding

### Boolean Members Must Be Grouped at the End

To avoid memory padding waste, **all boolean members must be placed at the end** of struct/class member declarations, **regardless of logical domain grouping**. Booleans are 1 byte, but the compiler may add padding bytes to align the next member.

When moving booleans to the end breaks logical readability, **use intercalated section comments** to clarify which domain each boolean belongs to.

```cpp
// WRONG - padding waste between booleans and larger types
struct BadLayout
{
    bool m_enabled{false};           // 1 byte + 7 bytes padding
    uint64_t m_size{0};              // 8 bytes
    bool m_visible{true};            // 1 byte + 7 bytes padding
    void* m_pointer{nullptr};        // 8 bytes
};  // Total: 32 bytes (with 14 bytes wasted)

// CORRECT - booleans grouped at the end with section comments
struct GoodLayout
{
    /* Rendering state. */
    uint64_t m_size{0};              // 8 bytes
    void* m_pointer{nullptr};        // 8 bytes
    uint64_t m_captureDelayUs{0};    // 8 bytes

    /* Boolean flags (grouped for padding). */
    bool m_enabled{false};           // Rendering state
    bool m_visible{true};            // Rendering state
    bool m_capturing{false};         // Capture state
};  // Total: 32 bytes (only 5 bytes padding)
```

### General Member Ordering

Order members by size (largest first) to minimize padding:

1. Pointers, `uint64_t`, `double` (8 bytes)
2. `uint32_t`, `float`, `int` (4 bytes)
3. `uint16_t`, `short` (2 bytes)
4. `uint8_t`, `char`, `bool` (1 byte)

```cpp
class Instance
{
    // Large types first
    std::shared_ptr<Device> m_graphicsDevice;
    std::shared_ptr<Device> m_computeDevice;
    std::vector<const char*> m_requiredValidationLayers;
    std::vector<const char*> m_requiredInstanceExtensions;
    HybridGPUConfig m_hybridConfig{};

    // Booleans at the end
    bool m_showInformation{false};
    bool m_debugMode{false};
};
```

---

## Function Attributes

### Use `[[nodiscard]]` for Return Values

Mark functions with `[[nodiscard]]` when ignoring the return value is likely a bug:

```cpp
[[nodiscard]]
bool create() noexcept;

[[nodiscard]]
std::optional<uint32_t> findQueueFamily() const noexcept;
```

### Use `noexcept` When Appropriate

Mark functions `noexcept` when they don't throw exceptions:

```cpp
void destroy() noexcept;
const char* deviceName() const noexcept;
```

### No Exceptions — Anywhere, On Every Platform

The whole cascade is built **without exceptions** (`EMERAUDE_DISABLE_EXCEPTIONS=On` in
emeraude-base: `-fno-exceptions` on GCC/Clang, `/EHs- /EHc-` on MSVC). A `throw` that reaches one
of our frames does not unwind, it terminates the process. Therefore:

- **No `try`, `catch` or `throw` in engine code**, not even in a platform-specific file. A failure is
  a value: `bool`, or `std::optional< T >`, with the reason on the diagnostics channel (Tracer, or
  `std::cerr` for bootstrap services). The full contract is emeraude-base's
  [`docs/error-handling.md`](../dependencies/emeraude-base/docs/error-handling.md).
- ⚠️ **Standard-library calls that report failure by throwing are forbidden too**, and Windows is the
  usual trap: `std::thread`'s constructor throws `std::system_error` from inside the CRT when the OS
  refuses a thread. Use the API that returns the failure instead (`_beginthreadex()` returns `0`,
  `errno` set — see `runFileDialogOnDedicatedThread()` in `Helpers.windows.cpp`; on POSIX the
  Linux helper validates `joinable()` after construction). Same family: `.at()`, `std::stoi`,
  `std::stoul`, `std::regex` — validate first, then use the non-throwing form (`operator[]` after a
  bounds check, `std::from_chars`).
- **`.at()` is never a checked access here**: out of range it throws `std::out_of_range`, i.e. the
  process aborts. Since 2026-10-12 the engine's `src/` and projet-alpha's `src/` hold zero `.at()`
  call (`/usr/bin/grep -rn -E '(\.|->)at ?\(' src`; plain `grep` is ugrep and skips the engine — the
  one match left is a history comment in `JoystickController.cpp`). The
  replacements, by case: an index proven by a loop bound, a size check just before or a modulo of
  the size → `operator[]` on a `std::vector` (a one-line comment when the proof is not on the
  screen); on a `std::array` (clang-tidy `pro-bounds-constant-array-index`) a range-for — with a
  C++20 init-statement `for ( std::size_t index = 0; const auto & item : items )` when the index is
  needed, parallel arrays merged into one array of structs — or, for a computed pick,
  `*std::next(array.cbegin(), static_cast< std::ptrdiff_t >(index))`; a constant
  index into a `std::array` → `std::get< N >()` (checked at compile time); a `std::map` /
  `std::unordered_map` key → `find()` and an explicit `end()` branch (reuse the iterator of a
  `find()` already made, never look the key up twice); an index from data, the console, a page or
  a public API → an explicit `size()` check and a refusal that follows the function's contract
  (`nullptr` in `MusicResource::buffer()`, an error + face #0 in `CubemapResource::hdrFaceData()`
  like its sibling `data()`, `false` + `TraceError`).
- MSVC warning **C4530** ("exception handler used, but unwind semantics are not enabled") is the
  compiler telling you a `try` slipped into a `/EHs-` build. It is a defect report, not noise.

### Use `const` Correctly

- Mark methods `const` when they don't modify the object
- Prefer `const&` for input parameters that won't be modified

```cpp
[[nodiscard]]
Vendor vendor() const noexcept;

void processDevice(const std::shared_ptr<PhysicalDevice>& device);
```

---

## Error Handling

### Use `value_or()` with `std::optional`

When parsing user input or settings, always provide a fallback:

```cpp
// WRONG - crashes if setting contains invalid value
const auto mode = magic_enum::enum_cast<DeviceAutoSelectMode>(modeString).value();

// CORRECT - safe fallback
const auto mode = magic_enum::enum_cast<DeviceAutoSelectMode>(modeString)
    .value_or(DeviceAutoSelectMode::Failsafe);
```

### Never Return nullptr for Resources

The engine uses fail-safe resources. Always return a valid fallback:

```cpp
// WRONG
Texture* getTexture(const std::string& name)
{
    auto it = m_textures.find(name);
    return (it != m_textures.end()) ? it->second : nullptr;  // Dangerous!
}

// CORRECT
std::shared_ptr<Texture> getTexture(const std::string& name)
{
    auto it = m_textures.find(name);
    return (it != m_textures.end()) ? it->second : m_neutralTexture;  // Safe
}
```

---

## Initialization

### Use Brace Initialization with Default Values

```cpp
class Device
{
    VkDevice m_device{VK_NULL_HANDLE};
    uint32_t m_queueCount{0};
    bool m_initialized{false};
};
```

### Use Designated Initializers for Structs (C++20)

```cpp
VkApplicationInfo appInfo{
    .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
    .pNext = nullptr,
    .pApplicationName = "MyApp",
    .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
    .pEngineName = "Emeraude",
    .engineVersion = VK_MAKE_VERSION(0, 8, 0),
    .apiVersion = VK_API_VERSION_1_3
};
```

---

## Ownership and Lifetime — Ave Robustus II (owner decision, 2026-10-08)

Every new addition follows the C++ Core Guidelines' ownership model scrupulously, at a mission-critical bar. The
nine rules are written once, in emeraude-base `docs/error-handling.md` § "Ownership and lifetime — Ave Robustus II";
they apply to the engine unchanged. The engine's consequences:

- An object that starts an asynchronous job (thread pool task, loader, LOD generation, capture, network request) owns
  that job and its destructor stops and waits for it. `Core`, the service providers and the shutdown sequence never
  compensate for a job they do not own.
- A `ServiceInterface` releases in its destructor what it acquired, even when `terminate()` was never called (an
  initialization failure path); native handles (Vulkan, OpenAL, GLFW, FreeType, platform) live in RAII holders.
- Reuse the engine's lifetime mechanisms before adding one: `Vulkan::DeferredDestructor`, `ResourceTrait`
  dependencies, `LocalDataLease`, `Container::DeferredSyncLoad`, Observer auto-detach.

### Mutexes are annotated (decision D12, the engine pass started 2026-10-16)

A member a mutex protects is declared `EMEN_GUARDED_BY(theMutex)`, and the mutex is an annotated emeraude-base type,
so Clang's `-Wthread-safety` (paranoid set, the pre-push `clang -fsyntax-only` check, the macOS build) refuses every
unlocked access at compile time. Base `docs/subsystems/source-tree/26-thread-safety-annotations.md` has the types.

- `Base::Mutex` + `Base::MutexLock` (exclusive); `Base::SharedMutex` + `SharedMutexReadLock` / `SharedMutexWriteLock`;
  `Base::ConditionVariable` to wait on a `MutexLock` (`std::condition_variable_any` only for a `std::stop_token` wait);
  a non-blocking attempt is `if ( !mutex.try_lock() ) { return; } const Base::MutexLock lock{mutex, std::adopt_lock};`.
- A private helper called with the lock held: `EMEN_REQUIRES(theMutex)`; a predicate of a wait: the lambda carries
  `EMEN_REQUIRES(theMutex)`. An initialization or termination that touches guarded members takes the lock too (cheap,
  and the analysis then holds everywhere).
- Never two mutexes held at once by one `std::scoped_lock` (the analysis cannot follow it): take them one after the
  other when the operation allows it (`Vulkan::PendingSubmissions`' move assignment).
- A mutex that only serializes calls on an externally-synchronized handle and protects NO member has nothing to
  annotate: it may stay a `std::mutex`. Today: `Vulkan::Device::m_logicalDeviceAccess` (handed out as a
  `std::unique_lock` by `lockAccess()` to the queues and the overlay), and the `Base::Mutex` of `Vulkan::CommandPool`
  and `Vulkan::Buffer` (host writes) guard calls, not members.
- Lot Vulkan (2026-10-16): `DescriptorPool`, `LayoutManager`, `AccelerationStructureBuilder`, `DeferredDestructor`,
  `PendingSubmissions`, `Device` (the deferred destructor pointer), `GPUProfiler`, `TransferManager` annotated; the
  analysis found the initialization / termination paths touching the guarded members without the lock
  (`TransferManager`, `LayoutManager`, `AccelerationStructureBuilder`), now locked.
- Lot Resources (2026-10-16): `Container` (the loaded and parked resources; its `@pre`-documented helpers now
  `EMEN_REQUIRES`), `Manager` (the local stores; the index readers run at initialization under the lock, now
  `EMEN_REQUIRES`; the directory scan collects the store directories in its walk and fills them in the function body —
  the analysis cannot follow a lambda through the `IO::forEachDirectoryEntry()` template), `PeerStore` / `SharingServer`
  (`Base::ConditionVariable`), `ResourceTrait` (the dependency lists and the local-data lease state; the graph lock a
  `Base::Mutex`). Found: `isTopResource()` / `dependencyCount()`, documented thread-safe, read the lists unlocked
  (now locked); `addDependency()` locked the resource and its dependency with one `std::scoped_lock{a, b}` — for a
  resource given ITSELF as a dependency, the same mutex twice (undefined behaviour), now refused as a cycle before any
  lock, and the two locks are taken one after the other under the graph lock (no path holds a node's lock while taking
  another's). Found while reading, no caller does it today: no runtime repro.
- Lot Scenes A (2026-10-17): `AbstractEntity` (no longer recursive — the components run on a copy of the list WITHOUT
  the lock, CP.22; the dispatching thread's structural changes refused, another thread waits on a condition variable;
  `forEachComponent()`'s callback still under the lock: it must not call the entity's component API), `Camera`,
  `AVConsole::AbstractVirtualDevice` (its two-device locks a `Base::MutexPairLock`; a device connecting to itself is
  refused — it locked its own mutex twice), `AVConsole::Manager` (its `*NoLock` helpers `EMEN_REQUIRES`),
  `Editor::Manager`, `GeometryDeduplicator`, `BindlessTextureSet`, `CloudSet`, `LightSet` (the set accessors
  `EMEN_REQUIRES`, the mutex accessor `EMEN_RETURN_CAPABILITY`; every caller already held it), `PhysicsRecorder`. Cost
  of the non-recursive dispatch, interleaved A/B (8 pairs, `balls-of-steel`'s logic cycle): +0.9 % median, inside the
  noise (±4 %); a first NON-interleaved comparison said +2.5 %: machine drift. Not exercised at run time: the automatic
  removal (`shouldBeRemoved()`: a broken instance, an expired sun course).
- Lot Scenes B (2026-10-17): `Scene` (the static entity map, the three render target lists, the active camera, the
  highlighted entities, the state-sync statistics, the two octrees with the overflow set and the gather stamp; every
  mutex a `Base::Mutex`; the unused `m_stateCopyLock` removed) and `Scenes::Manager` (the
  scene list a `Base::Mutex`, the active scene a `Base::SharedMutex`, the exclusive-access announcement a
  `Base::ConditionVariable`). Found — real concurrent accesses, now locked:
  - `createStaticEntity()` / `removeStaticEntity()` changed the static entity map WITHOUT its lock while the render
    thread walked it UNDER the lock; `findStaticEntity()`, `getStaticEntitySystemStatistics()`, the two debug-display
    queries and `publishStateForRendering()`'s walk read it unlocked. `removeStaticEntity()` holds the lock around the
    map operations only (the unlinking notifies observers and takes the octree locks) and erases by identity at the
    end. The publication takes the list lock, then each entity's component lock: the order of the render loops.
  - The ConsoleAdapter's entity collection called `findStaticEntity()` inside `forEachStaticEntities()` — with the
    lookup locked, the same non-recursive mutex twice: it collects the names first and looks them up after.
  - `updateVideoMemory()` tested `empty()` on two render target lists unlocked (the iteration helpers lock).
  - The manager read `m_activeScene` without its shared access from the main thread (enable / disable / delete, the
    editor toggle, 28 console commands): a private `activeSceneSnapshot()` copies the pointer under the read lock;
    `enableScene()` checks for an active scene under its exclusive access (check and change atomic),
    `disableActiveScene()` re-checks under it. Lock order: `m_sceneListAccess`, then `m_activeSceneSharedAccess`.
  - Three lists erased under one `std::scoped_lock{a, b, c}`: one lock at a time now (no path holds two).
- Lot Graphics A (2026-10-17): the managers and registries — `Renderer` (the frame diagnostics, the pending material /
  geometry updates, the surface geometries), `BindlessTextureManager` (its mutex guards no member: it serializes every
  descriptor-set write, so the two write helpers are `EMEN_REQUIRES`), `SharedUBOManager`, `SharedUniformBuffer` (the
  seat table; the bank growth `EMEN_REQUIRES`), `VertexBufferFormatManager` (the list and the format-building state;
  the builder helpers `EMEN_REQUIRES`), `PostProcessStack`, `OverflowCensus` and `FrameCapture`
  (`Base::ConditionVariable`), `ImposterBake`, `Recorder` (the sessions; each session's queue under its own
  `Base::Mutex`, its `std::condition_variable_any` kept for the stop token). Restructured for the analysis:
  - `FrameCapture::writeFiles()` (a pool task) read the capture unlocked while `Writing`, safe by its state protocol: it
    now takes the frames out and copies the parameters under the lock, then works on its own copies.
  - `Recorder::onTerminate()` walked and cleared the finishing sessions after releasing the lock: they are taken out
    under it and joined outside it. The hardware encoding thread read `m_hardwareSession`: it receives its session at
    its start (created before the thread, destroyed after its join). `EncodingSession::finalize()` empties its queues
    under its lock.
  - The readback / snapshot slots are NOT guarded by the recorder's lock as a whole: each slot's atomic state hands it
    over (the rendering thread records and retires under the lock, the encoding thread reads the slot it was handed).
  - `SharedUniformBuffer` grows its banks under the seat lock, while `uniformBufferObject()`, `descriptorSet()` and
    `writeElementData()` (hot: every instance, every frame) read the bank vectors' `size()` unlocked —
    `reserve(MaxBankCount)` prevented the reallocation, not the concurrent read of the size. Owner decision: an atomic
    bank count, published (release) once a bank is complete; the readers bound their index with it (acquire), no lock.
    Found with it: a bank whose buffer failed to create stayed in the vector (`usable()` counted it), and the dynamic
    flavour dereferenced a descriptor set without checking it — a failed bank is now removed.
- Lot Graphics B (2026-10-17): the per-object locks.
  - `Renderable::Abstract`: its program cache under a `Base::SharedMutex` (read / write locks).
  - `MeshResource`, `MultiLayerMeshResource`: `Base::Mutex`; the level array keeps its lock-free read contract
    (append-only, count published release / acquire), so only the append helper is `EMEN_REQUIRES`.
  - `RenderableInstance::Abstract` (the skinning staging) and `Multiple` (its local data; `m_localDataAccess` moved
    from the base, where it was a protected member only `Multiple` used — a clang-tidy finding on the touched line). Found: `setActiveInstanceCount()` (a particle emitter, logic thread, every tick) wrote the
    count `instanceCount()` reads on the render thread — now an atomic; the public `resetModelMatrices()` rewrote the
    local data without the lock (no caller today) — now locked; the instance count is `const` (constructor only) and the
    vertex buffer object documented as frozen after the constructor (`createOnHardware()` has no other caller).
  - `CDLODTerrainResource`: the pending ray-tracing proxy.
  - `ViewMatrices2DUBO`, `ViewMatrices3DUBO`, `ViewMatricesCascadedUBO`: their mutex REMOVED (owner decision). It
    locked only `updateVideoMemory()` and `destroy()` ("Maybe useless", its own comment), while every access is
    either before the render target is published (`create()`) or on the render thread (`updateVideoMemory()`,
    `descriptorSet()`, `destroy()` from `destroyRenderTarget()`, called by the renderer and the selection outline),
    and the destructor runs once nothing uses the object. The contract is stated at the members.
- Lots Overlay, Console, Net and the engine root (2026-10-17) — the last members of the census:
  - `Overlay::Surface` (the geometry state under `m_geometryAccess`, the requested transition size; `updateModelMatrix()`
    `EMEN_REQUIRES`; its try-lock pass is `try_lock()` + `MutexLock(adopt_lock)`; `activeBufferMutex()` returns the
    `Base::Mutex`, `EMEN_RETURN_CAPABILITY`), `UIScreen` (the stack, the input-exclusive surface, the move tap;
    `recomputeDepths()` `EMEN_REQUIRES`), `Overlay::Manager` (the screens; its two-lock pass a `Base::MutexPairLock`;
    `onTerminate()` emptied the screens without the lock — taken out under it, destroyed outside it). The two
    framebuffers stay unannotated: the content providers write them in their overrides (`onActiveBufferReady()`,
    `onTransitionBufferReady()`) with the lock held by contract, and an annotation would reach every downstream
    override. `m_physicalRepresentationUpdateMutex` serializes a pass, it guards no member.
  - `Console::RemoteListener` (the clients, the command queue; `m_writeMutex` serializes the socket writes) and
    `Console::MCP::Server` (the pending requests).
  - `Net::APIClient` and `Net::Manager` (the tickets, the events, the cache index, the peers; the helpers documented
    "caller holds m_itemsAccess" now `EMEN_REQUIRES`; the cache index loads under the lock at initialization).
  - `Settings` (the stores under a `Base::SharedMutex`; `readLevel()` / `readFile()` `EMEN_REQUIRES`, `writeFile()` /
    `getVariable()` `EMEN_REQUIRES_SHARED`), `Audio::Manager` (the source pool; its creation at initialization now under
    the lock), `TracerLogger` (the entry queue; its `condition_variable_any` kept for the stop token), `Tracer` (its
    console mutex serializes the output, no member) and `Core` (the redraw mutex: no member, the budget is atomic).
  - Left `std::mutex` on purpose: `Device::m_logicalDeviceAccess` (serializes handle calls), `MusicResource`'s
    file-static `s_tsfMutex` (serializes a library that is not thread-safe, TinySoundFont). Every
    `std::condition_variable_any` waited with a `stop_token` stays, over a `Base::MutexLock`.
  - The octree pointers were tested OUTSIDE their lock, then used under it (statistics, removal, the per-frame
    refiling, the collision step, the two render-list gathers) while `rebuild*Octree()` / `destroyOctrees()` replace
    them under it, and `buildOctrees()` created them unlocked: every test is now inside the lock; the render paths
    choose their path with `hasRenderingOctree()` (locked), and `gatherRenderingCandidates()` tests again under it.

## Paths and Text Encoding — rule C (owner decision, 2026-10-12)

Every `std::string` of the cascade is UTF-8: JSON, settings, the command line, console / MCP arguments and replies, logs,
resource names, URIs. On Windows a `std::filesystem::path` is UTF-16, and its NARROW conversions go through the ANSI code
page: `path::string()` THROWS on a character outside it (an abort, exceptions are off) and writes invalid UTF-8 for the
others; a path built from a narrow string decodes that string as ANSI. So:

- **Into a path**: `Base::IO::u8path(text)` — never `std::filesystem::path{text}`, `dir / text`, `p = text`,
  `p.append(text)` or a function taking a `const path &` handed a `std::string` (all implicit narrow conversions). A
  string literal or a `constexpr` constant is ASCII text the author controls: it may stay as is.
- **Out of a path**: `Base::IO::toU8String(p)` / `toGenericU8String(p)` — never `.string()`, `.generic_string()`,
  `.u8string()`, nor `std::ostream << p` (std's `operator<<` writes `string()` quoted). A `Trace*` / `BlobTrait` line takes
  the path as is: emeraude-base's `BlobTrait` overload writes it UTF-8, quoted.
- **To a Windows or CEF API**: the native / wide string (`p.native()`, `p.c_str()`), never a narrow conversion. A C
  library that documents UTF-8 file names (ufbx, tinyusdz, RenderDoc) gets `toU8String()`; one that opens a narrow name
  with `fopen()` gets the CONTENT instead (`IO::fileGetContents()`, then a memory API).
- **Untrusted text bound for a third-party path API** is checked with `Base::String::isValidUTF8()` first: MSVC's
  `char8_t` path conversion throws on invalid UTF-8 (the glTF uri pre-check, `GLTFLoader::load()`).
- The application declares `<activeCodePage>UTF-8</activeCodePage>` as a second line of defence only: a DLL cannot impose
  it, it hides defects (a per-site fix is verified WITHOUT it), and only third-party code relies on it.

**The guard — `tools/check-path-encoding.py`**, a pre-push step for every change to C++ (like `tools/mcp-conformance.py`
for the console):

```bash
python3 tools/check-path-encoding.py --build-dir <release build> --build-dir <debug build> [--root <application src>]
```

clang-query over each `compile_commands.json` is the authority (implicit conversions included, Release and Debug
`#if` branches both); a grep gate covers the sources no database compiles (POSIX-only files are skipped: their native
string IS UTF-8) and refuses a non-ASCII narrow literal on a path line. `--grep-only` is a coarse quick net. A legitimate
site goes in `tools/path-encoding-allowlist.txt` with a one-line reason. clang-query traps: `docs/caution-points.md`
§ "On MSVC, `std::filesystem::path::string()` is ANSI".

## Template Patterns

### CRTP (Curiously Recurring Template Pattern)

Use CRTP for zero-overhead polymorphism when the derived types are known at compile time. This avoids virtual function overhead while enabling code reuse.

**Naming convention:** Prefix template class names with `T_` to indicate a template base class.

```cpp
// CRTP base class - uses T_ prefix
template< typename derived_t, Severity severity_t >
class T_TraceHelperBase : public Libs::BlobTrait
{
    public:
        explicit T_TraceHelperBase(const char* tag,
            const std::source_location& location = std::source_location::current()) noexcept
            : m_tag{tag}, m_location{location} {}

        ~T_TraceHelperBase()
        {
            Tracer::getInstance().trace(severity_t, m_tag, this->get(), m_location);
        }

    protected:
        const char* m_tag;
        std::source_location m_location;
};

// Derived classes inherit constructors via using declaration
class TraceInfo final : public T_TraceHelperBase<TraceInfo, Severity::Info>
{
    public:
        using T_TraceHelperBase::T_TraceHelperBase;  // Inherit all constructors
};

class TraceError final : public T_TraceHelperBase<TraceError, Severity::Error>
{
    public:
        using T_TraceHelperBase::T_TraceHelperBase;
};
```

**When to use CRTP:**
- Multiple classes share identical logic with only a compile-time constant difference
- Virtual dispatch overhead is unacceptable (hot paths)
- The set of derived types is fixed and known

**Reference implementation:** `src/Tracer.hpp:T_TraceHelperBase`

---

## Comments

### Use Doxygen for Public APIs

```cpp
/**
 * @brief Returns the vendor enum.
 * @note Shortcut to PhysicalDevice::properties().
 * @return Vendor
 * @version 0.8.38
 */
[[nodiscard]]
Vendor vendor() const noexcept;
```

### Use `/* NOTE: */` for Implementation Comments

```cpp
/* NOTE: Detect hybrid GPU configuration (Optimus, etc.) */
m_hybridConfig = this->detectHybridGPUConfiguration();
```

---

## AI-Friendly Code Guidelines

This codebase aims to be **AI-friendly** to maximize collaboration with AI assistants.

### Clarity Over Cleverness

1. **Clear naming over ambiguous parameters**: If a function takes a parameter that could be interpreted multiple ways, rename it or split into explicit methods.

   ```cpp
   // AMBIGUOUS - is offset an element index or byte offset?
   getDescriptorInfo(uint32_t offset, uint32_t range);

   // CLEAR - explicit naming removes ambiguity
   getDescriptorInfoForElement(uint32_t elementIndex);
   getDescriptorInfoAtByteOffset(VkDeviceSize byteOffset);
   ```

2. **Single responsibility for conversions**: When a value needs conversion (e.g., element index → byte offset), do it in ONE place, not scattered across multiple files.

   ```cpp
   // GOOD - conversion happens in the class that understands the concept
   VkDeviceSize SharedUniformBuffer::getByteOffsetForElement(uint32_t elementIndex) const
   {
       return static_cast<VkDeviceSize>(elementIndex % m_maxElementCountPerUBO) * m_blockAlignedSize;
   }
   ```

### Proactive Refactoring Rule

> **When an AI identifies an unclear concept or confusing interface, it should STRONGLY SUGGEST refactoring it.**
>
> An unclear interface that causes bugs once will cause bugs again. The fix should address the root cause (confusing API design), not just the symptom (wrong value passed).

**Signs of an unclear interface:**
- Parameter named `offset` but receives an index
- Caller needs to know implementation details to use correctly
- FIXME/TODO comments about "breaks some scenes"
- Same conversion logic duplicated in multiple places

**Refactoring checklist:**
1. Rename parameters to match their actual meaning
2. Add explicit helper methods with clear names
3. Document the contract with `@note` or `/* NOTE: */`
4. Add assertions or validation when possible

### Documentation for Future AI

When fixing bugs or adding features, document the **why** not just the **what**:

```cpp
/* NOTE: Use LOCAL offset within the specific UBO, not global offset.
 * Example: Element 300 with maxElementCountPerUBO=256 is at local index 44 in UBO #1.
 * The byte offset is 44 * blockAlignedSize, not 300 * blockAlignedSize! */
const auto byteOffset = this->getByteOffsetForElement(index);
```

This helps future AI sessions understand the reasoning behind the code.

## Generated Headers — Include Policy

The engine has two top-level headers that look similar but behave very differently
with respect to the compilation cache:

| Header | File | Regenerated by CMake | When to use |
|---|---|---|---|
| `Libs/emeraude_platform.hpp` | Hand-written, in `src/Libs/` | **Never** | When you only need arch / OS detection (`IS_LINUX`, `IS_WINDOWS`, `IS_X86_ARCH`, `OS_NAME`, `IsLinux`, `IsWindows`, `IsMacOS`, `PLATFORM_TARGETED`, `__PRETTY_FUNCTION__` MSVC fallback). Lives under `Libs/` because the directory is meant to be extracted to its own repository eventually. |
| `emeraude_config.hpp` | Generated from `src/emeraude_config.hpp.in` | On every CMake reconfigure | When you need version constants (`EngineName`, `VersionMajor/Minor/Patch`), build-time debug flags (`IsDebug`, `ObserverDebugEnabled`, `WindowEventsDebugEnabled`, …), or `#cmakedefine` toggles (`IMGUI_ENABLED`, `LIBSNDFILE_ENABLED`, …). |

`emeraude_config.hpp` transitively includes `Libs/emeraude_platform.hpp`, so including
the former gives you everything. **Don't include `emeraude_config.hpp` just for a
platform macro** — every header that does so cascades a recompile through its TUs
whenever any CMake-driven value changes. Use the lighter `Libs/emeraude_platform.hpp`.

### Rules

1. **Include what you use.** Don't rely on a transitive include — if your file
   names `IsDebug` or `IS_WINDOWS`, include the corresponding header directly.
2. **Prefer `Libs/emeraude_platform.hpp` in headers.** Headers fan out widely; a stable
   include keeps the cache warm. Reserve `emeraude_config.hpp` for files that
   genuinely need the volatile content (versions, debug flags, cmakedefines).
3. **`GLFW_INCLUDE_VULKAN` is a CMake compile definition** (`SetupGLFW.cmake`), not
   a header macro. You do not need to include either of these headers to make
   `<GLFW/glfw3.h>` pull in Vulkan extensions.
4. **OS-suffixed source files** (`Foo.linux.cpp`, `Foo.windows.cpp`, `Foo.mac.cpp`,
   `Foo.mac.mm`) are compiled only on the matching OS via CMake selection. The
   internal `#if IS_LINUX` / `#if IS_WINDOWS` guards in those files are redundant.
   Non-suffixed `.cpp` files that contain `#if IS_*` blocks DO need
   `#include "emeraude_platform.hpp"` placed **before** the first `#if`.

---

## See Also

- [AGENTS.md](../AGENTS.md) - Main project documentation
- [Tracer System Usage](../AGENTS.md#tracer-system-usage) - Logging conventions
- [Coordinate System](coordinate-system.md) - Y-up convention
