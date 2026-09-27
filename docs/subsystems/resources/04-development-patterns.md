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
