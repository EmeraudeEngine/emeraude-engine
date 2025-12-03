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
| `ResourceTrait.hpp` | `AbstractServiceProvider` | Container access interface (merged) |
| `Container.hpp` | `Container<resource_t>` | Template store per resource type |
| `Manager.hpp` | `Manager` | Central coordinator, access to all containers |
| `BaseInformation.hpp` | `BaseInformation` | Resource metadata (store index) |

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
- **MANDATORY**: Implement `load(AbstractServiceProvider&)` without parameters
- Neutral resource must ALWAYS succeed (no I/O)
- Be immediately usable and visually identifiable
- No external dependencies

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
2. **MANDATORY**: Implement neutral resource `load(AbstractServiceProvider&)`
3. Implement file/data loading with failure possibility
4. `onDependenciesLoaded()` for finalization
5. Register in `Manager`

### Loading with Dependencies
```cpp
bool load(AbstractServiceProvider& provider, const Json::Value& data) override {
    // 1. Initialize enqueuing
    if (!this->initializeEnqueuing(false)) return false;

    // 2. Load immediate data
    loadImmediateData(data);

    // 3. Declare dependencies (automatic cycle detection)
    auto dep = provider.container<OtherResource>()->getResource(data["dep"]);
    if (!addDependency(dep)) return false;  // Cycle detected = failure

    // 4. Finalize enqueuing
    return this->setLoadSuccess(true); // Resource transitions to Loading
}

bool onDependenciesLoaded() override {
    // 5. Finalization when ALL dependencies are ready
    // NOTE: Called OUTSIDE mutex to avoid deadlocks
    uploadToGPU();
    return true; // Resource transitions to Loaded
}
```

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

## CRITICAL Attention Points

| Point | Importance | Description |
|-------|------------|-------------|
| **Thread safety** | CRITICAL | Atomic status + mutex on lists |
| **Deadlock prevention** | CRITICAL | Virtual calls OUTSIDE lock |
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
