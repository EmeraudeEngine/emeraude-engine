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
