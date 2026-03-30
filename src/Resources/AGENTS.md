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
| `Manager.hpp` | `Manager` | Central coordinator, access to all containers |
| `BaseInformation.hpp` | `BaseInformation` | Resource metadata (store index) |

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

```bash
# Resources tests
ctest -R Resources
./test --filter="*Resource*"
```

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
- `LoadingRequest.hpp` → Removed (functionality integrated elsewhere)
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

See @docs/resource-management.md section "Future Improvements" for implementation details.

## Detailed Documentation

For complete resources system architecture:
- @docs/resource-management.md - Fail-safe, dependencies, detailed lifecycle, thread safety, future suggestions

Related systems:
- @src/Net/AGENTS.md - Resource download from URLs
- @src/Graphics/AGENTS.md - Geometry, Material, Texture as resources
- @src/Audio/AGENTS.md - SoundResource, MusicResource
- @src/Libs/AGENTS.md - Observer/Observable pattern
