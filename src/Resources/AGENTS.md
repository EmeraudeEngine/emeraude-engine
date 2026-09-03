# Resource Management

Context for developing the Emeraude Engine resource management system.

## Module Overview

Fail-safe resource system that guarantees NEVER returning nullptr and always providing a valid resource, even on loading failure.

## Architecture (v0.8.35+)

### Main Classes

| File | Class | Role |
|------|-------|------|
| `Types.hpp` | Enums + functions | `SourceType`, `Status`, `DepComplexity` + string conversions |
| `ResourceTrait.hpp` | `ResourceTrait` | Base interface for all resources |
| `ResourceTrait.hpp` | `AbstractServiceProvider` | Service access interface (merged) |
| `Container.hpp` | `Container<resource_t>` | Template store per resource type |
| `Container.cpp` | `ServiceAccess::*` | Non-template service facade — see [§ Include discipline](#include-discipline-container-is-a-compile-firewall) |
| `LoadingRequest.hpp/.cpp` | `LoadingRequest` | **Non-template** loading request (file / download / direct data) |
| `Manager.hpp` | `Manager` | Central coordinator, access to all containers |
| `BaseInformation.hpp` | `BaseInformation` | Resource metadata (store index) |

### Include discipline: Container is a compile firewall

> [!IMPORTANT]
> `Container.hpp` is reached by ~70 translation units. **Never include a service header in it.**
> It is a header-only template, so any expression in a method body that does *not* depend on
> `resource_t` is analysed at template **definition** time — pulling `Net/Manager.hpp`,
> `FileSystem.hpp` or `ThreadPool.hpp` into every consumer. `Net::Manager` is the worst offender:
> it pulls `FileSystem`, `ThreadPool`, `Network/URI`, `Console/ControllableTrait` (it is console-driven
> since 2026-08-27) and the JSON layer behind its cache index.
>
> Two mechanisms keep the header thin (2026-07, measured: 42 headers / 12 826 LOC → **20 / 6 834**):
>
> 1. **`ServiceAccess` free functions** (declared in `Container.hpp`, implemented in `Container.cpp`)
>    wrap every touch of a heavy service: `netManagerObservable()`, `isNetManagerObservable()`,
>    `fileDownloadedNotificationCode()`, `downloadStatus()`, `downloadedFilepath()`, `enqueueTask()`,
>    `startDownload()`. **Need a new service call? Add a function there** — do not include
>    the service header.
> 2. **`LoadingRequest` is deliberately not a template.** It only ever needs the polymorphic
>    `ResourceTrait` (`load()` is virtual). That lets every heavy body (`url`, `setDownloadProcessed`,
>    `setDownloadFailed`) live in `LoadingRequest.cpp`, keeping `Network/URL` out of the header. It
>    computes no cache path any more: the downloaded file's location comes from `Net::Manager`
>    (`ServiceAccess::downloadedFilepath(ticket)`), which owns the download cache. Consequence: it stores a `shared_ptr< ResourceTrait >`, so `Container::loadingTask()`
>    downcasts with `static_cast< resource_t * >` before publishing the `ResourceLoaded`
>    notification (safe — no virtual inheritance of `ResourceTrait` anywhere).
>
> Consumers that genuinely instantiate a container (`container< T >()` performs a `static_cast`
> downcast, which needs the complete type) must include `Resources/Container.hpp` **explicitly**.
> Resource headers only need a forward declaration — see [§ Resource headers must not include Container.hpp](#resource-headers-must-not-include-containerhpp).

### Resource headers must not include Container.hpp

The 37 `*Resource.hpp` headers use `Container` in exactly two ways, and **both are satisfied by a
forward declaration** — a `friend` of a specialization and an alias never instantiate the template:

```cpp
/* Forward declarations. */
namespace EmEn::Resources
{
	template< typename resource_t >
	class Container;
}
/* ... */
friend class Resources::Container< Texture2D >;             // declaration is enough
using Texture2Ds = Container< …::Texture2D >;               // alias, no instantiation
```

Restoring the `#include` in any of them re-inflates the whole cascade: it took **133 → 67** the
number of TU parsing `Container.hpp` and removed ~415 000 cumulated lines of project headers
(2026-07). `ResourceTrait.hpp` also carries this forward declaration, so the local block is
redundant in principle — it is kept per-header for locality.

### AbstractServiceProvider Interface

Services available to resources via `this->serviceProvider()` (injected at construction):

| Method | Returns | Purpose |
|--------|---------|---------|
| `primaryServices()` | `PrimaryServices&` | Engine primary services (ThreadPool, FileSystem, Settings) |
| `graphicsRenderer()` | `Graphics::Renderer&` | GPU resource creation |
| `audioManager()` | `Audio::Manager&` | Audio system access |
| `container<T>()` | `Container<T>*` | Access to other resource containers |

**Accessing core services:** `fileSystem()` and `settings()` are accessed through `primaryServices()`:
```cpp
this->serviceProvider().primaryServices().settings()    // Configuration
this->serviceProvider().primaryServices().fileSystem()   // File path resolution
this->serviceProvider().primaryServices().threadPool()   // Background task execution
```

**ThreadPool access:** The engine's `ThreadPool` is accessed via `primaryServices().threadPool()`.
Resources can submit background tasks (e.g., LOD generation) without spawning ad-hoc threads:
```cpp
this->serviceProvider().primaryServices().threadPool()->enqueue([...] { /* background work */ });
```

> [!WARNING]
> **Do NOT use `std::async` for background tasks in resources.** Use the engine ThreadPool via
> `primaryServices().threadPool()->enqueue()`. Unbounded `std::async` spawns one thread per task,
> causing CPU contention on heavy scenes (e.g., Sponza with 50+ meshes).

**Constructor injection:** ServiceProvider is passed as the first constructor argument to every resource. The `load()` methods no longer receive it — resources access it via `this->serviceProvider()`.

```cpp
// Constructor: ResourceTrait(AbstractServiceProvider & serviceProvider, name, flags)
// Storage: AbstractServiceProvider & m_serviceProvider (non-nullable reference)
// Access: this->serviceProvider() returns the reference
```

**Code reference:** `ResourceTrait.hpp:AbstractServiceProvider`, `ResourceTrait.hpp:ResourceTrait()`

### Resource Lifecycle

```
Unloaded → Enqueuing/ManualEnqueuing → Loading → Loaded/Failed
```

**Status enum:**
- `Unloaded` (0): Initial state
- `Enqueuing` (1): Auto mode, dependencies being added
- `ManualEnqueuing` (2): Manual mode, user controls dependencies
- `Loading` (3): No more dependencies allowed, waiting for completion
- `Loaded` (4): Ready for use
- `Failed` (5): Loading failed

## Resources-Specific Rules

### MANDATORY Fail-Safe Philosophy
- **NEVER** return nullptr from Containers
- **ALWAYS** provide a valid resource (real or neutral)
- **NEVER** require error checking on client side
- Errors are logged but never break the application

### Neutral Resource Pattern
- **MANDATORY**: Implement `load()` (no parameters) for neutral/default resources
- Neutral resource must ALWAYS succeed (no I/O)
- Be immediately usable and visually identifiable
- No external dependencies
- ServiceProvider is available via `this->serviceProvider()` if needed

### Thread Safety (CRITICAL)

**Atomic status:**
```cpp
std::atomic<Status> m_status{Status::Unloaded};  // Lock-free queries
```

**Mutex for lists:**
```cpp
std::mutex m_dependenciesAccess;  // Protects m_parentsToNotify and m_dependenciesToWaitFor
```

**Two-phase pattern (avoids deadlocks):**
```cpp
void checkDependencies() noexcept {
    Action action = Action::None;
    {
        std::lock_guard lock{m_dependenciesAccess};
        // Phase 1: Determine action under lock
        if (allDependenciesLoaded()) action = Action::CallOnDependenciesLoaded;
    }
    // Phase 2: Execute OUTSIDE lock (virtual calls + notifications)
    if (action == Action::CallOnDependenciesLoaded) {
        this->onDependenciesLoaded();  // Virtual call OUTSIDE lock!
    }
}
```

### Cycle Detection (NEW v0.8.35)

**Automatic in addDependency():**
```cpp
if (this->wouldCreateCycle(dependency)) [[unlikely]] {
    m_status = Status::Failed;
    return false;
}
```

**Recursive DFS algorithm:**
```cpp
bool wouldCreateCycle(const shared_ptr<ResourceTrait>& dep) const noexcept {
    if (dep.get() == this) return true;  // Self-reference
    for (const auto& sub : dep->m_dependenciesToWaitFor) {
        if (sub.get() == this || this->wouldCreateCycle(sub)) return true;
    }
    return false;
}
```

### Dependency Management
- Use `addDependency()` to declare dependencies
- `onDependenciesLoaded()` for finalization (GPU upload, etc.)
- Automatic parent-child event propagation
- Reference counting with `std::shared_ptr`

## Development Patterns

### Creating a New Resource Type
1. Inherit from `ResourceTrait`
2. Constructor must accept `AbstractServiceProvider &` as first parameter, forwarded to `ResourceTrait`
3. **MANDATORY**: Implement neutral resource `load()` (no parameters)
4. Implement `load(filepath)` and `load(Json::Value)` with failure possibility
5. `onDependenciesLoaded()` for finalization
6. Register in `Manager`

### Loading with Dependencies
```cpp
// Constructor: ServiceProvider injected at construction
MyResource(AbstractServiceProvider & serviceProvider, const std::string & name, uint32_t flags)
    : ResourceTrait{serviceProvider, name, flags} {}

// load() no longer receives ServiceProvider — use this->serviceProvider()
bool load(const Json::Value& data) noexcept override {
    // 1. Initialize enqueuing
    if (!this->initializeEnqueuing(false)) return false;

    // 2. Load immediate data
    loadImmediateData(data);

    // 3. Declare dependencies (automatic cycle detection)
    auto dep = this->serviceProvider().container<OtherResource>()->getResource(data["dep"]);
    if (!addDependency(dep)) return false;  // Cycle detected = failure

    // 4. Finalize enqueuing
    return this->setLoadSuccess(true); // Resource transitions to Loading
}

bool onDependenciesLoaded() noexcept override {
    // 5. Finalization when ALL dependencies are ready
    // NOTE: Called OUTSIDE mutex to avoid deadlocks
    uploadToGPU();
    return true; // Resource transitions to Loaded
}
```

### Container API Methods (v0.8+)

| Method | Purpose |
|--------|---------|
| `getResource(name)` | Get existing or load from store |
| `getDefaultResource()` | Get neutral/fallback resource |
| `getOrCreateResource(name, fn)` | Get existing or create+initialize via function |
| `getOrCreateUnloadedResource(name)` | Get existing or create empty shell (unloaded state) |
| `getRandomResource()` | Get random loaded resource |
| `preloadResource(name)` | Trigger async preload |

**Note:** `getOrCreateUnloadedResource()` creates a resource in unloaded state, useful when you need to manually load the resource later via custom initialization.

### Garbage Collection
- `use_count() == 1` → only Container holds the resource
- `unloadUnusedResources()` to free memory
- Keep Default resources in permanent cache

## Development Commands

> [!WARNING]
> **There is no test target in the engine** (`ctest -R Resources` and `./test --filter=...`,
> advertised here until 2026-08-27, never existed). The only unit suite of the cascade is
> emeraude-base's `EmeraudeBaseUnitTests`. Resources behaviour is verified at runtime through the
> console commands below, with the Vulkan validation layers on.

### Console (`Core.ResourcesManagerService`)

| Command | Role |
|---|---|
| `listContainers()` | Containers as JSON (class id, name, resource count). |
| `listResources(containerName)` | Names available in a container (`ImageResource`, `MeshResource`, …). |
| `loadResource(containerName, resourceName)` | Requests the asynchronous loading of a store resource — an `ExternalData` one is downloaded first by `Net::Manager`. Fails when the name is not in the store. |
| `resourceStatus(containerName, resourceName)` | `Unloaded` / `Enqueuing` / `ManualEnqueuing` / `Loading` / `Loaded` / `Failed`; error when the name is unknown. Poll it after `loadResource()`. |

Both new commands (2026-08-27) go through two `ContainerInterface` virtuals, `requestResource(name)`
and `resourceStatus(name)`, so the console never needs the container type — code keeps using the
typed `getResource()`.

### Loading queue — three rules the 2026-08-27 audit had to restore

1. **`asyncLoad` means asynchronous, everywhere.** `checkLoadedResource()` passed `!asyncLoad` to
   `pushInLoadingQueue()` while the two other call sites passed it unchanged, so
   `getOrCreateResource()` (the async variant) loaded **synchronously** and
   `getOrCreateResourceSync()` asynchronously. Fixed; never "compensate" a flag at one call site.
2. **Never run a load — and never notify — under `m_resourcesAccess`.** The synchronous branch
   called `loadingTask()` with the container mutex held, and `loadingTask()` emits
   `LoadingProcessStarted` / `ResourceLoaded` / `LoadingProcessFinished`. An observer reacting to
   `ResourceLoaded` by asking the same container for another resource (a Material fetching its
   Texture) re-entered `getResource()` on a **non-recursive** mutex — undefined behaviour, and by
   defect 1 that was the default path. The load is now carried by `Container::DeferredSyncLoad`,
   declared **before** the `scoped_lock` in every public entry point: destruction order (reverse of
   declaration) unlocks first, then loads.
3. **An `ExternalData` resource cannot be loaded synchronously.** There is no synchronous download
   path: the request used to reach `loadingTask()`'s "should be downloaded first" branch, which
   only traced — the resource stayed non-terminal forever. It is now failed explicitly, with a
   message telling the caller to ask asynchronously.

### ⚠️⚠️ A container binds to its store ONCE, so `getLocalStore()` must never return null

`onInitialize()` runs the boot-time discovery (dynamic scan, or the JSON indexes) and **then**
registers every container with `this->getLocalStore("<StoreName>")`. That `shared_ptr` is captured
in `Container::m_localStore` and kept for the container's whole life — there is no rebinding, ever.

Until 2026-08-28 `getLocalStore()` returned `nullptr` for a store the discovery had not produced,
and the consequence was silent and total:

- Every container born with a null store answers `availableResourceNames()` with `{}` and
  `isResourceAvailable()` with `false`, permanently.
- A later `Manager::update(root)` — which is exactly what `Core::openFiles()` does with a resource
  index — hits `if ( !m_localStores.contains(storeName) )`, creates a **brand new** map under that
  name, and registers the resources into it. The manager sees them. No container ever will.
- Nothing logs anything. `Core.openFiles` reports success, `listResources` returns `[]`, and the
  natural conclusion is that the loader or the network is broken.

**This is not an edge case for an embedding application.** It bites any host whose data directories
contain no store sub-directory — app_system has none, so on it **all 34 containers were sterile and
the whole runtime `update()` path was dead**. Found while running the `ExternalData` fixture on
macOS (`app_system/tools/external-data-check/`), where it is platform-independent: Linux and Windows
were equally affected and simply had not run that path yet.

`getLocalStore()` now creates the store when absent and is documented as never returning null. The
cost is one empty map per store name; the benefit is that `update()` works at runtime, which is what
it exists for.

## Async Lambda Capture Safety (CRITICAL)

> [!CRITICAL]
> Lambdas passed to `getOrCreateResource()` execute on the **thread pool**, potentially after
> the calling object is destroyed. All captured data must be **self-contained**.

### Rules

1. **NEVER capture `this`** or local references to stack-allocated objects
2. **Pre-resolve** all `shared_ptr` dependencies before the lambda
3. **Copy scalars** (float, int, Color) by value
4. **Move-capture** containers of `shared_ptr` to avoid atomic refcount overhead

### Pattern: Pre-Resolve + Value Capture

```cpp
// Pre-resolve outside the lambda (on the calling thread)
auto texture = (texIdx < m_textures.size()) ? m_textures[texIdx] : nullptr;
float roughness = pbr.roughnessFactor;
Color< float > color{pbr.baseColorFactor[0], pbr.baseColorFactor[1], ...};

// Lambda captures only values and shared_ptrs — fully self-contained
auto material = container->getOrCreateResource(name, [
    texture = std::move(texture), roughness, color
] (auto & res) {
    if ( texture != nullptr ) res.setAlbedoComponent(texture);
    else res.setAlbedoComponent(color);
    res.setRoughnessComponent(roughness);
    return res.setManualLoadSuccess(true);
});
```

### Why This Matters

`getOrCreateResource()` (Container.hpp:1208) enqueues the lambda to the thread pool and returns immediately. If the caller is stack-allocated (e.g., `GLTFLoader loader{...}`), `this` becomes dangling before the lambda executes → **use-after-free crash**.

**Code references:**
- `Container.hpp:getOrCreateResource()` — Async path (thread pool)
- `Container.hpp:getOrCreateResourceSync()` — Sync path (blocks calling thread)
- `Scenes/GLTFLoader.cpp` — Reference implementation of safe async captures

## CRITICAL Attention Points

| Point | Importance | Description |
|-------|------------|-------------|
| **Thread safety** | CRITICAL | Atomic status + mutex on lists |
| **Deadlock prevention** | CRITICAL | Virtual calls OUTSIDE lock |
| **Async capture safety** | CRITICAL | No `this` capture in getOrCreateResource lambdas |
| **Cycle detection** | HIGH | Automatic DFS in addDependency() |
| **Memory management** | HIGH | `shared_ptr` for reference counting |
| **Status tracking** | MEDIUM | State machine: Unloaded → Loading → Loaded/Failed |
| **Cache efficiency** | MEDIUM | Key by resource name for reuse |

## Removed Files (v0.8.35)

- `AbstractServiceProvider.hpp` → Merged into `ResourceTrait.hpp`
- `LoadingRequest.hpp` → had been folded into `Container.hpp` — **re-extracted in v0.8.40**, this
  time as a **non-template** class with its own `.cpp`, deliberately, to keep `FileSystem`,
  `Network/URL`, `String` and `IO` out of a header parsed by ~70 TU. Do not fold it back in:
  see [§ Include discipline](#include-discipline-container-is-a-compile-firewall).
- `Randomizer.hpp` → Removed

## Future Improvements (suggestions)

| Suggestion | Complexity | Impact | Priority |
|------------|------------|--------|----------|
| **Optimize cycle detection** | Low | Medium | High |
| DFS algorithm with `visited set` → O(n) instead of O(n²) | | | |
| **Priority system** | Medium | High | High |
| `LoadPriority::Critical/High/Normal/Low/Deferred` | | | |
| **Advanced metrics** | Low | Medium | Medium |
| Cache hits/misses, load times, slow resources | | | |
| **Resource bundles** | Medium | High | Medium |
| Resource groups for scene transitions | | | |
| **Progressive loading** | High | High | Low |
| LOD for textures, streaming for open-world | | | |

See [`../../docs/resource-management.md`](../../docs/resource-management.md) section "Future Improvements" for implementation details.

## Detailed Documentation

For complete resources system architecture:
- [`../../docs/resource-management.md`](../../docs/resource-management.md) - Fail-safe, dependencies, detailed lifecycle, thread safety, future suggestions

Related systems:
- [`../Net/AGENTS.md`](../Net/AGENTS.md) - Resource download from URLs (`"Source": "ExternalData"`): the manager's contract, the `FileDownloaded`-on-main-thread rule, the console check of the whole chain
- [`../Graphics/AGENTS.md`](../Graphics/AGENTS.md) - Geometry, Material, Texture as resources
- [`../Audio/AGENTS.md`](../Audio/AGENTS.md) - SoundResource, MusicResource
- [`../../dependencies/emeraude-base/src/AGENTS.md`](../../dependencies/emeraude-base/src/AGENTS.md) - Observer/Observable pattern
