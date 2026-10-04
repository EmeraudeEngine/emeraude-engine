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
| `releaseLocalData()` | Releases now, whatever `Core/Resources/ReleaseLocalData` says and without the grace delay, every releasable lease-free CPU copy (diagnostic). |
| `acquireLocalData(container, resource)` | Leases the CPU copy of a held resource, reloading it when released, then drops it: `{residentBefore, bytesBefore, leased, bytesLeased, residentAfter}` (diagnostic; blocks the console thread for the reload). |
| `requestLocalData(container, resource)` | The non-blocking ask: `{leased, bytes}`; not resident = the reload is scheduled on the pool, ask again. |
| `memoryCensus()` | The CPU memory the LOADED resources hold (their local data — pixmaps, shapes, decoded audio / movie frames — never their GPU memory), per container, largest first: `{totalBytes, totalUnusedBytes, containers: [{id, loaded, bytes, unusedBytes}]}`. `unusedBytes` = held by the store alone. A resource still loading is skipped; a resource outside every container (LOD levels, a ground's generated grid) is not counted. Measurements: `docs/todo/cpu-copies-retained-after-upload.md`. |
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
