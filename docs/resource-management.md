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
    Status m_status;  // Unloaded, Loading, Loaded, Failed

    // Loading lifecycle
    virtual bool load(ServiceProvider&) = 0;  // Neutral resource (no params)
    virtual bool load(ServiceProvider&, const std::filesystem::path&) = 0;
    virtual bool load(ServiceProvider&, const Json::Value&) = 0;

    // Dependency management
    bool addDependency(const std::shared_ptr<ResourceTrait>& dependency);
    virtual bool onDependenciesLoaded();  // Finalization hook

    // State queries
    bool isLoaded() const;
    bool isLoading() const;
    Status status() const;
};
```

**Responsibilities:**
- **Dependency tracking**: Maintains parent-child relationships
- **Loading states**: Tracks Unloaded → Loading → Loaded/Failed transitions
- **Event propagation**: Notifies parents when dependencies complete
- **Finalization**: Provides hook (`onDependenciesLoaded`) for GPU upload, etc.

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

## Resource Lifecycle and Dependency Chain

### Complete Resource Lifecycle

```
┌─────────────┐
│  Unloaded   │  Initial state
└──────┬──────┘
       │ getResource() or addDependency()
       ▼
┌─────────────┐
│   Loading   │  load() called, dependencies declared
└──────┬──────┘
       │ All sub-resources finish
       ▼
┌─────────────┐
│onDependencies│  Finalization hook (GPU upload, etc.)
│   Loaded()  │
└──────┬──────┘
       │ Finalization succeeds
       ▼
┌─────────────┐
│   Loaded    │  Resource ready for use
└─────────────┘

       OR (if fails)
       ▼
┌─────────────┐
│   Failed    │  Container returns Default instead
└─────────────┘
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

- All Container methods are protected by `std::mutex m_resourcesAccess`
- Async loading happens in thread pool, results synchronized via mutex
- ResourceTrait has `std::mutex m_dependenciesAccess` for dependency list
- Client code doesn't need to worry about threading

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
| **Observable** | Event notifications for load state | Engine can react, client doesn't need to |
| **Cached** | `shared_ptr` in Container for fast reuse | Performance optimization |
| **Self-Cleaning** | Garbage collection based on `use_count()` | Automatic memory management |
| **Identifiable** | Visual/audio placeholders for missing resources | Developer-friendly debugging |

### Core Philosophy

> "The resource manager's job is to provide a resource, no matter what. The client's job is to use it. That's it."