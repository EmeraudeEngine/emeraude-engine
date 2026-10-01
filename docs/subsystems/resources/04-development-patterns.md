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

### When a dependency FAILS (2026-10-01)

A failure always propagates and releases every link (owner ruling). The child and its parents hold each other
through strong pointers (`m_dependenciesToWaitFor` / `m_parentsToNotify`): a failed child that kept them leaked the
whole chain with its GPU objects, and the parents waited in `Loading` forever — a glTF with ONE unreadable image
rendered nothing and leaked to shutdown. Every failure path calls `releaseLinksAfterFailure()`: `setLoadSuccess(false)`,
a failed `onDependenciesLoaded()`, a failed file parse, an `addDependency()` refusal that fails the resource, and the
container's manual creation (`getOrCreateResource[Sync]()`: a function that returns false or leaves the resource
unfinished now `failLoading()`s it). `addDependency()` of a child that ALREADY failed is handled the same way.

Each parent is asked **`onDependencyFailed(const ResourceTrait & dependency)`** (protected virtual, default
`false`): `true` means "I replaced what it provided, I go on" (logged as a warning), `false` means the parent fails in
turn, up the chain (logged as an error). The six texture types answer through `TextureResource::Abstract::
takeDefaultData()`: they take their type's DEFAULT data (image, volume, cubemap, movie) — a broken texture shows the
default one and the asset still renders. A new resource type with a sensible fallback overrides the hook; the default
is the safe one.

### Container API Methods (v0.8+)

| Method | Purpose |
|--------|---------|
| `getResource(name)` | Get existing or load from store |
| `getDefaultResource()` | Get neutral/fallback resource |
| `getOrCreateResource(name, fn)` | Get existing or create+initialize via function |
| `getOrCreateUnloadedResource(name)` | Get existing or create empty shell (unloaded state) |
| `getRandomResource()` | Get random loaded resource |
| `preloadResource(name)` | Trigger async preload |
| `localFilepath(name)` | The local file a store entry points to, or `std::nullopt` (direct data, URL, absent) — Sep 2026 |

**Deriving a VARIANT of a store resource** (Sep 2026): read the entry's file through `localFilepath()`
(`FastJSON::getRootFromFile`), change what differs, and load the result under ANOTHER name with
`getOrCreateResource(variantName, [data = std::move(json)] (auto & r) { return r.load(data); })` — the JSON
captured by value, the store entry untouched. Never rebuild the path from the store's directory layout.
First user: `forest`'s chick, an `Emerald` whose volume thickness is measured on its mesh.

**Note:** `getOrCreateUnloadedResource()` creates a resource in unloaded state, useful when you need to manually load the resource later via custom initialization.

### Garbage Collection
- `use_count() == 1` → only Container holds the resource
- `unloadUnusedResources()` to free memory
- Keep Default resources in permanent cache
