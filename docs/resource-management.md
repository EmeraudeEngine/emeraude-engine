# Resource Management System

This document provides detailed architecture for the resource management system, including the fail-safe design philosophy, dependency tracking, and asynchronous loading mechanisms.

## Quick Reference: Key Terminology

- **Manager**: Central service that provides access to all resource Containers (one per resource type). Handles global operations like garbage collection.
- **Container\<resource_t\>**: Template class serving as a "Store" for one specific resource type. Manages caching, loading, and lifecycle of resources. Also called "Store" in context.
- **ResourceTrait**: Base interface for all loadable resources. Provides dependency tracking, loading lifecycle states, and observable notifications.
- **Top-Resource**: A resource at the top of a dependency chain (e.g., MeshResource that depends on MaterialResource, which depends on TextureResources). Has no parent resources waiting for it.
- **Dependency Chain**: Hierarchical relationship where resources depend on sub-resources. Loading propagates from leaves to root.
- **Neutral Resource**: Default/fallback version of a resource created without loading external data. Always functional and identifiable (e.g., checkerboard texture, gray cube).
- **LoadingRequest**: Internal wrapper for asynchronous loading operations. Handles file loading, network downloads, and caching.
- **Reference Counting**: Automatic memory management using `std::shared_ptr`. Resources are kept in memory as long as they're referenced.
- **Garbage Collection**: Process of unloading resources when `use_count() == 1` (only Container holds reference).

## Design Philosophy: Fail-Safe Architecture

The resource management system's **fundamental responsibility** is to **always provide a valid, usable resource**, regardless of any failure condition.

### Core Principle: Never Crash, Always Provide

The system is designed so that:
- Client code **never receives nullptr**
- Client code **never checks for loading errors**
- Client code **never handles failure modes**
- Application **never crashes** due to missing/corrupt resources
- Errors are **logged** but **don't propagate** as exceptions
- Default/neutral resources serve as **fail-safe fallbacks**

### Why This Design?

In production game engines and real-time applications:
- Better to show a placeholder (pink checkerboard texture) than crash
- Gameplay code should focus on gameplay, not resource error handling
- Developers identify issues through logs and visual placeholders
- "The show must go on" - application continues running even with broken assets

### Design Tradeoffs

- **Robustness** over correctness: Graceful degradation preferred
- **Simplicity** for client code: Zero error handling required
- **Transparency**: Loading is asynchronous and automatic
- **Identifiability**: Placeholders are obvious to developers

## Architecture: Three-Layer System

### Layer 1: Manager (Central Coordinator)

```cpp
// src/Resources/Manager.hpp
class Manager : public ServiceInterface, public ServiceProvider {
    std::map<std::type_index, std::unique_ptr<ContainerInterface>> m_containers;
    std::unordered_map<std::string, shared_ptr<...>> m_localStores;

    // Access a specific resource type's Container
    template<typename T>
    Container<T>* container();

    // Global operations
    size_t memoryOccupied() const;
    size_t unusedMemoryOccupied() const;
    size_t unloadUnusedResources();
};
```

**Responsibilities:**
- Provides access to type-specific Containers
- Coordinates garbage collection across all resource types
- Manages resource index files (stores)
- Reports global memory usage

### Layer 2: Container\<resource_t\> (Type-Specific Store)

```cpp
// src/Resources/Container.hpp
template<typename resource_t>
class Container : public ContainerInterface, public ObserverTrait {
    std::unordered_map<std::string, std::shared_ptr<resource_t>> m_resources;
    std::shared_ptr<std::unordered_map<std::string, BaseInformation>> m_localStore;

    // Core API
    std::shared_ptr<resource_t> getResource(const std::string& name, bool asyncLoad = true);
    std::shared_ptr<resource_t> getOrCreateResource(const std::string& name, ...);
    std::shared_ptr<resource_t> getDefaultResource();

    // Management
    bool preloadResource(const std::string& name);
    size_t unloadUnusedResources();
};
```

**Responsibilities:**
- **Caching**: Keeps `shared_ptr` to all loaded resources (fast reuse)
- **Loading**: Triggers async or sync loading when resources are requested
- **Default fallback**: Always returns valid resource (real or default)
- **Garbage collection**: Unloads resources with `use_count() == 1`
- **Thread safety**: Protects resource map with mutex

### Layer 3: ResourceTrait (Base Interface)

```cpp
// src/Resources/ResourceTrait.hpp
class ResourceTrait : public std::enable_shared_from_this<ResourceTrait>,
                      public NameableTrait, public FlagTrait, public ObservableTrait {
    std::vector<std::shared_ptr<ResourceTrait>> m_parentsToNotify;
    std::vector<std::shared_ptr<ResourceTrait>> m_dependenciesToWaitFor;
    std::atomic<Status> m_status{Status::Unloaded};  // Thread-safe status
    std::mutex m_dependenciesAccess;  // Protects dependency lists

    // Loading lifecycle
    virtual bool load(AbstractServiceProvider&) = 0;  // Neutral resource (no params)
    virtual bool load(AbstractServiceProvider&, const std::filesystem::path&) = 0;
    virtual bool load(AbstractServiceProvider&, const Json::Value&) = 0;

    // Dependency management
    bool addDependency(const std::shared_ptr<ResourceTrait>& dependency);
    virtual bool onDependenciesLoaded();  // Finalization hook

    // State queries
    bool isLoaded() const;
    bool isLoading() const;
    Status status() const;

    // Circular dependency detection (internal)
    bool wouldCreateCycle(const std::shared_ptr<ResourceTrait>& dependency) const noexcept;
};
```

**Responsibilities:**
- **Dependency tracking**: Maintains parent-child relationships with circular dependency detection
- **Loading states**: Tracks Unloaded → Enqueuing → Loading → Loaded/Failed transitions
- **Event propagation**: Notifies parents when dependencies complete (thread-safe)
- **Finalization**: Provides hook (`onDependenciesLoaded`) for GPU upload, etc.
- **Thread safety**: Atomic status + mutex-protected dependency lists

## The Neutral Resource Pattern (Critical!)

**Every resource type MUST implement `load(ServiceProvider&)` without parameters.**

This method creates a **neutral/default** version of the resource that:
- **Always succeeds** (no file I/O, no network, no dependencies)
- **Is immediately usable** (valid GPU resources, valid data structures)
- **Is easily identifiable** by developers (visual/audio placeholders)

### Examples

```cpp
// TextureResource - Procedural checkerboard
class TextureResource : public ResourceTrait {
    bool load(ServiceProvider& provider) override {
        // Generate 64×64 checkerboard pattern (pink/black)
        m_pixels = generateCheckerboard(64, 64, Color::Pink, Color::Black);
        m_width = 64;
        m_height = 64;
        return true;  // Always succeeds
    }

    bool load(ServiceProvider& provider, const std::filesystem::path& path) override {
        // Load real texture from file
        if (!loadImageFile(path, m_pixels, m_width, m_height)) {
            return false;  // Can fail
        }
        return true;
    }

    bool onDependenciesLoaded() override {
        // Upload to GPU (same for neutral or real)
        m_vkImage = vulkan.createImage(m_pixels, m_width, m_height);
        return true;
    }
};

// MeshResource - Procedural gray cube
class MeshResource : public ResourceTrait {
    bool load(ServiceProvider& provider) override {
        // Generate unit cube geometry
        m_vertices = generateCubeVertices(1.0F);
        m_indices = generateCubeIndices();

        // Neutral gray material (no dependencies)
        m_material = provider.container<MaterialResource>()->getDefaultResource();

        return true;  // Always succeeds
    }

    bool load(ServiceProvider& provider, const Json::Value& data) override {
        // Load real mesh from data
        m_vertices = loadVertices(data);
        m_indices = loadIndices(data);

        // Add material dependency
        auto material = provider.container<MaterialResource>()->getResource(data["material"]);
        addDependency(material);  // ← Keeps resource in Loading state

        return true;
    }

    bool onDependenciesLoaded() override {
        // ← ALL dependencies loaded! Material + textures ready
        // Upload complete data to GPU
        m_vertexBuffer = vulkan.createBuffer(m_vertices);
        m_indexBuffer = vulkan.createBuffer(m_indices);
        return true;
    }
};
```

### Neutral Resource Guarantees

- Created once per Container, cached as "Default"
- Zero dependencies (self-contained)
- No external data required
- Never fails to load
- Immediately usable after creation

### Image and Texture Resource Types

The engine provides specialized image resources for different dimensions:

| Resource Type | Data Storage | Dimensions | Used By |
|--------------|--------------|------------|---------|
| `ImageResource` | `Pixmap<uint8_t>` | 2D (width × height) | Texture1D, Texture2D, TextureCubemap |
| `VolumetricImageResource` | `std::vector<uint8_t>` | 3D (width × height × depth) | Texture3D |
| `CubemapImageResource` | 6 × `Pixmap<uint8_t>` | 6 faces | TextureCubemap |

**VolumetricImageResource** (`Graphics/VolumetricImageResource.hpp`):
```cpp
class VolumetricImageResource : public ResourceTrait {
    std::vector<uint8_t> m_data;
    uint32_t m_width{0}, m_height{0}, m_depth{0};
    uint32_t m_colorCount{4};  // RGBA

    // Neutral resource: 32×32×32 RGB gradient cube
    bool load(ServiceProvider&) override;

    // File loading: TODO
    bool load(ServiceProvider&, const std::filesystem::path&) override;
};
```

Key methods: `data()`, `width()`, `height()`, `depth()`, `colorCount()`, `bytes()`, `isValid()`, `isGrayScale()`, `averageColor()`

## Resource Lifecycle and Dependency Chain

### Complete Resource Lifecycle

```
┌─────────────┐
│  Unloaded   │  Initial state (Status::Unloaded)
└──────┬──────┘
       │ initializeEnqueuing() called
       ▼
┌─────────────┐
│  Enqueuing  │  Dependencies are being declared (Status::Enqueuing)
│  (or Manual)│  For manual mode: Status::ManualEnqueuing
└──────┬──────┘
       │ setLoadSuccess(true) called
       ▼
┌─────────────┐
│   Loading   │  Waiting for dependencies to complete (Status::Loading)
└──────┬──────┘
       │ All dependencies loaded (m_dependenciesToWaitFor empty)
       ▼
┌─────────────────┐
│onDependencies   │  Virtual finalization hook (GPU upload, etc.)
│   Loaded()      │  Called OUTSIDE mutex to prevent deadlocks
└──────┬──────────┘
       │ Returns true
       ▼
┌─────────────┐
│   Loaded    │  Resource ready for use (Status::Loaded)
└─────────────┘

       OR (if any step fails)
       ▼
┌─────────────┐
│   Failed    │  Container returns Default instead (Status::Failed)
└─────────────┘
```

### Loading Status Enum

```cpp
enum class Status : uint8_t {
    Unloaded = 0,        // Initial state
    Enqueuing = 1,       // Auto mode: dependencies being added
    ManualEnqueuing = 2, // Manual mode: user controls dependency addition
    Loading = 3,         // No more deps can be added, waiting for completion
    Loaded = 4,          // Ready for use
    Failed = 5           // Load failed, resource unusable
};
```

### Key Implementation Details

```cpp
// Inside resource load()
bool MeshResource::load(ServiceProvider& provider, const Json::Value& data) {
    // 1. Load immediate data
    m_vertices = loadVertices(data);
    m_indices = loadIndices(data);

    // 2. Declare dependencies
    auto material = provider.container<MaterialResource>()->getResource(data["material"]);
    addDependency(material);  // ← Resource stays in Loading state

    return true;  // load() succeeded, but resource not Loaded yet
}

// Called automatically when all dependencies finish
bool MeshResource::onDependenciesLoaded() {
    // 3. Finalize with complete data
    // At this point: geometry + material + textures ALL ready
    m_vertexBuffer = vulkan.createBuffer(m_vertices);
    m_indexBuffer = vulkan.createBuffer(m_indices);

    // 4. Resource transitions to Loaded state
    return true;
}
```

## Return Scenarios: Real, Default (Not Found), Default (Failed)

The Container **always** returns a valid `std::shared_ptr<resource_t>`. Three scenarios:

### Scenario 1: Resource Found and Loaded Successfully

```cpp
auto tex = container->getResource("logo.png");
// → Container finds "logo.png" in store
// → Creates TextureResource, loads from file
// → Returns shared_ptr (Status: Loading → Loaded)
// Result: Real texture displayed
```

### Scenario 2: Resource Not Found in Store

```cpp
auto tex = container->getResource("missing.png");
// → Container checks store, not found
// → Returns container->getDefaultResource()
// Result: Pink checkerboard displayed, warning logged
```

### Scenario 3: Resource Loading Fails

```cpp
auto tex = container->getResource("corrupt.png");
// → Container finds "corrupt.png" in store
// → Creates TextureResource, attempts load
// → load() returns false (file corrupt, parse error, etc.)
// → Container returns getDefaultResource() instead
// Result: Pink checkerboard displayed, error logged
```

## Client Code Usage: Zero Error Handling

### Complete Example: Loading and Using a Mesh

```cpp
// In projet-alpha (client application)

// 1. Request resource - ALWAYS succeeds, NEVER nullptr
auto mesh = resources.container<MeshResource>()->getResource("character");

// 2. Attach to scene node - ALWAYS safe
auto node = scene->createChild("player");
node->attachMesh(mesh);  // No checks needed!

// 3. Rendering - Engine handles state
// Frame 1-50: mesh is Loading → Renderer skips (not displayed yet)
// Frame 51+: mesh is Loaded → Renderer draws
// If load failed: Default mesh displayed (gray cube)

// Client code never checks anything:
// - No if (mesh != nullptr)
// - No if (mesh->isLoaded())
// - No try/catch
// - Just use it!
```

## Thread Safety and Garbage Collection

### Thread Safety

The resource system is designed for concurrent access from multiple threads:

**Container Level:**
- All Container methods protected by `std::mutex m_resourcesAccess`
- Async loading happens in thread pool, synchronized via mutex

**ResourceTrait Level:**
- `std::atomic<Status> m_status` - Lock-free status queries from any thread
- `std::mutex m_dependenciesAccess` - Protects parent/dependency lists
- **Critical Pattern**: Virtual methods (`onDependenciesLoaded()`) and observer notifications are called **outside** the mutex lock to prevent deadlocks

**Two-Phase Pattern in checkDependencies():**
```cpp
void ResourceTrait::checkDependencies() noexcept {
    Action pendingAction = Action::None;

    // Phase 1: Determine action under lock
    {
        const std::lock_guard<std::mutex> lock{m_dependenciesAccess};
        if (allDependenciesLoaded()) {
            pendingAction = Action::CallOnDependenciesLoaded;
        }
    }

    // Phase 2: Execute action OUTSIDE lock (prevents deadlocks)
    if (pendingAction == Action::CallOnDependenciesLoaded) {
        const bool success = this->onDependenciesLoaded();  // Virtual call
        // Re-acquire lock only for status update...
    }
}
```

**Client code doesn't need to worry about threading** - all synchronization is internal.

### Circular Dependency Detection

The system automatically detects and prevents circular dependencies:

```cpp
bool ResourceTrait::addDependency(const shared_ptr<ResourceTrait>& dependency) noexcept {
    // ...
    if (this->wouldCreateCycle(dependency)) [[unlikely]] {
        TraceError{TracerTag} << "Circular dependency detected!";
        m_status = Status::Failed;
        return false;
    }
    // ...
}
```

**Detection Algorithm (DFS):**
```cpp
bool ResourceTrait::wouldCreateCycle(const shared_ptr<ResourceTrait>& dep) const noexcept {
    // Direct self-reference
    if (dep.get() == this) return true;

    // Recursive check through dependency's sub-dependencies
    for (const auto& subDep : dep->m_dependenciesToWaitFor) {
        if (subDep.get() == this) return true;
        if (this->wouldCreateCycle(subDep)) return true;  // DFS
    }
    return false;
}
```

This prevents deadlocks where A waits for B, and B waits for A.

### Garbage Collection

```cpp
// Each Container tracks usage
size_t Container<resource_t>::unusedMemoryOccupied() const {
    size_t bytes = 0;
    for (const auto& resource : m_resources | std::views::values) {
        if (resource.use_count() == 1) {  // Only Container holds it
            bytes += resource->memoryOccupied();
        }
    }
    return bytes;
}

// Unload unused resources
size_t Container<resource_t>::unloadUnusedResources() {
    size_t unloaded = 0;
    for (auto it = m_resources.begin(); it != m_resources.end(); ) {
        if (it->second.use_count() == 1) {  // Only we hold it
            it = m_resources.erase(it);  // Destroy resource
            unloaded++;
        } else {
            ++it;
        }
    }
    return unloaded;
}

// Manager can trigger global GC
size_t Manager::unloadUnusedResources() {
    size_t total = 0;
    for (auto& [type, container] : m_containers) {
        total += container->unloadUnusedResources();
    }
    return total;
}
```

### When to Trigger GC

- After scene transitions (old scene resources released)
- When memory pressure is detected
- Manual trigger via console command
- Periodic cleanup (e.g., every 60 seconds)

## Design Principles Summary

| Principle | Description | Benefit |
|-----------|-------------|---------|
| **Always Valid** | Never return nullptr, always provide usable resource | Client code simplicity, zero crashes |
| **Fail-Safe** | Default resources as fallbacks for all failures | Application continues running |
| **Async by Design** | Immediate return, background loading | No blocking, smooth experience |
| **Dependency Aware** | Automatic tracking and propagation | Correct load order guaranteed |
| **Cycle Detection** | Automatic circular dependency detection via DFS | Prevents deadlocks at runtime |
| **Thread-Safe** | Atomic status + mutex-protected lists | Safe concurrent access |
| **Deadlock-Free** | Virtual calls outside mutex locks | No lock ordering issues |
| **Observable** | Event notifications for load state | Engine can react, client doesn't need to |
| **Cached** | `shared_ptr` in Container for fast reuse | Performance optimization |
| **Self-Cleaning** | Garbage collection based on `use_count()` | Automatic memory management |
| **Identifiable** | Visual/audio placeholders for missing resources | Developer-friendly debugging |

### Core Philosophy

> "The resource manager's job is to provide a resource, no matter what. The client's job is to use it. That's it."

## File Structure (v0.8.35+)

```
src/Resources/
├── Types.hpp              # Enums (SourceType, Status, DepComplexity) + conversion functions
├── ResourceTrait.hpp      # Base class + AbstractServiceProvider (merged)
├── ResourceTrait.cpp      # Implementation (dependency management, cycle detection)
├── Container.hpp          # Template Container<resource_t> (type-specific store)
├── Manager.hpp            # Central Manager (access to all containers)
├── Manager.cpp            # Manager implementation
├── BaseInformation.hpp    # Resource metadata from store index files
└── BaseInformation.cpp    # Parsing implementation
```

**Note:** `AbstractServiceProvider` has been merged into `ResourceTrait.hpp` as these classes are tightly coupled. The `requires std::is_base_of_v<ResourceTrait, resource_t>` constraint ensures type safety on container access.

## Future Improvements (Suggestions)

This section documents potential enhancements for future development.

### 1. Optimize Circular Dependency Detection

**Current:** Recursive DFS algorithm is O(n²) worst case for deep dependency graphs.

**Suggestion:** Use visited node marking to avoid re-traversing:
```cpp
bool wouldCreateCycle(const shared_ptr<ResourceTrait>& dep,
                      std::unordered_set<const ResourceTrait*>& visited) const noexcept {
    if (dep.get() == this) return true;
    if (visited.contains(dep.get())) return false;  // Already checked
    visited.insert(dep.get());

    for (const auto& sub : dep->m_dependenciesToWaitFor) {
        if (this->wouldCreateCycle(sub, visited)) return true;
    }
    return false;
}
```

**Benefit:** O(n) complexity, significant improvement for complex resource graphs.

### 2. Loading Priority System

**Current:** All resources are treated equally in the loading queue.

**Suggestion:** Add priority levels to control loading order:
```cpp
enum class LoadPriority : uint8_t {
    Critical = 0,   // UI, player model, essential audio
    High = 1,       // Visible scene elements
    Normal = 2,     // Standard resources
    Low = 3,        // Background, preloading
    Deferred = 4    // Load only when explicitly needed
};

class ResourceTrait {
    LoadPriority m_priority{LoadPriority::Normal};
    // ...
};
```

**Benefit:** Faster perceived loading times by prioritizing visible/critical content.

### 3. Progressive/Streaming Loading

**Current:** Resources are either fully loaded or not loaded.

**Suggestion:** Support progressive loading for large resources:
```cpp
enum class LoadLevel : uint8_t {
    Metadata = 0,   // Size, format info only
    Preview = 1,    // Low-res version (mipmap level 4+)
    Standard = 2,   // Normal quality
    Full = 3        // Maximum quality with all mipmaps
};

class TextureResource : public ResourceTrait {
    LoadLevel m_currentLevel{LoadLevel::Metadata};
    bool upgradeToLevel(LoadLevel target);
    // ...
};
```

**Benefit:** Faster initial scene display, smoother streaming for open-world scenarios.

### 4. Enhanced Metrics and Profiling

**Current:** Only `memoryOccupied()` is available.

**Suggestion:** Add comprehensive statistics:
```cpp
struct ResourceMetrics {
    size_t totalResources;
    size_t loadedResources;
    size_t failedResources;
    size_t memoryUsedBytes;
    size_t cacheHits;
    size_t cacheMisses;
    std::chrono::milliseconds totalLoadTime;
    std::chrono::milliseconds averageLoadTime;
    std::vector<std::pair<std::string, std::chrono::milliseconds>> slowestResources;
};

class Manager {
    ResourceMetrics getMetrics() const;
    void resetMetrics();
    // ...
};
```

**Benefit:** Better profiling, identification of bottlenecks, optimization guidance.

### 5. Resource Groups / Bundles

**Current:** Resources are loaded individually.

**Suggestion:** Support grouped loading for scene transitions:
```cpp
class ResourceBundle {
    std::string m_name;
    std::vector<std::string> m_resourceNames;
    LoadPriority m_priority;

    void preloadAll();
    void unloadAll();
    bool isFullyLoaded() const;
    float loadProgress() const;  // 0.0 to 1.0
};
```

**Benefit:** Simplified scene management, better memory control, loading screen progress bars.

### Implementation Priority

| Suggestion | Complexity | Impact | Priority |
|------------|------------|--------|----------|
| Optimize cycle detection | Low | Medium | High |
| Loading priorities | Medium | High | High |
| Enhanced metrics | Low | Medium | Medium |
| Resource bundles | Medium | High | Medium |
| Progressive loading | High | High | Low |