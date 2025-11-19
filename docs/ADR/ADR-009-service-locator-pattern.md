# ADR-009: Service Locator Pattern

## Status
**Accepted** - Core dependency injection mechanism throughout engine

## Context

Game engines have complex subsystem dependencies:
- Graphics system needs Resource system for loading textures/meshes
- Resource system needs FileSystem for loading from disk
- Audio system needs Resource system for loading sounds
- Physics system needs Scene graph for entity positions
- Scene graph needs Graphics for rendering, Physics for simulation

**Traditional Dependency Management:**
1. **Global singletons**: Static instances accessible anywhere
2. **Constructor injection**: Pass dependencies through constructors
3. **Factory patterns**: Centralized object creation with dependency resolution
4. **Service locator**: Central registry providing access to services

**Problems with Alternatives:**
- **Singletons**: Global state, testing difficulties, hidden dependencies
- **Constructor injection**: Deep parameter passing, circular dependencies
- **Factories**: Centralized creation logic, complex factory hierarchies

**Service Locator Benefits:**
- Clear dependency requests (explicit service lookup)
- Flexible service implementation swapping
- Centralized service management
- Testable (can provide mock services)

**Service Locator Drawbacks:**
- Runtime dependency resolution (vs compile-time)
- Hidden dependencies (services not visible in signatures)
- Service location overhead (lookup cost)

## Decision

**Emeraude Engine implements a Service Locator pattern with ServiceInterface and ServiceProvider for dependency injection throughout the engine.**

**Core Architecture:**
```cpp
ServiceInterface    // Base for all engine services (singleton-like)
ServiceProvider     // Provides access to services for resources/components
```

**Design Principles:**
- **Services as singletons**: Each service type has single instance per engine
- **Explicit lookup**: Services request dependencies via ServiceProvider
- **Type-safe access**: Template-based service retrieval
- **Centralized management**: Single point for service registration and access

## Architecture Details

**ServiceInterface (Service Base):**
```cpp
class ServiceInterface : public Libs::NameableTrait {
public:
    // Each service has unique name and acts as singleton
    // Cannot be duplicated within engine instance
};
```

**Example Services:**
- Graphics::Renderer (graphics operations)
- Resources::Manager (resource loading and caching)
- Physics::Manager (physics simulation)
- Scenes::Manager (scene graph management)
- Audio::Manager (sound processing)
- Window (platform window management)
- FileSystem (file I/O operations)

**ServiceProvider (Dependency Injection):**
```cpp
class ServiceProvider {
public:
    // Access to core services
    const FileSystem& fileSystem() const noexcept;
    Graphics::Renderer& graphicsRenderer() const noexcept;
    
    // Type-safe container access for resources
    template<typename resource_t>
    Container<resource_t>* container() noexcept;
    
private:
    FileSystem& m_fileSystem;
    Graphics::Renderer& m_graphicsRenderer;
    // References to other services...
};
```

**Usage Patterns:**
```cpp
// In ResourceTrait::load()
bool TextureResource::load(ServiceProvider& provider, const std::filesystem::path& path) {
    // Get file system service
    const auto& fs = provider.fileSystem();
    auto data = fs.readFile(path);
    
    // Get graphics service for GPU upload
    auto& renderer = provider.graphicsRenderer();
    m_gpuTexture = renderer.createTexture(data);
    
    return true;
}

// In Component or System
class Visual : public Component {
    void initialize(ServiceProvider& provider) {
        // Access resource system via provider
        auto textures = provider.container<TextureResource>();
        m_texture = textures->getResource("default.png");
    }
};
```

## Implementation Strategy

**Service Registration:**
```cpp
// Engine initialization (approximate)
class Engine {
    FileSystem m_fileSystem;
    Graphics::Renderer m_renderer;
    Resources::Manager m_resourceManager;
    
    void initialize() {
        // Create services
        m_renderer.initialize(vulkan);
        m_resourceManager.initialize(m_fileSystem, m_renderer);
        
        // Services reference each other as needed
    }
};
```

**Service Access Patterns:**
```cpp
// Resource loading context
class ResourceTrait {
    virtual bool load(ServiceProvider& provider) = 0;  // Neutral resource
    virtual bool load(ServiceProvider& provider, const filesystem::path& path) = 0;  // From file
    virtual bool load(ServiceProvider& provider, const Json::Value& data) = 0;  // From data
};

// All resource loading methods receive ServiceProvider for dependency access
```

## Consequences

### Positive
- **Explicit dependencies**: Clear what services each component requires
- **Testable**: Can provide mock ServiceProvider for unit tests
- **Flexible**: Can swap service implementations without changing clients
- **Centralized**: Single point to manage all service relationships
- **Type-safe**: Template-based access prevents wrong service types
- **Singleton management**: Ensures single instance per service type

### Negative
- **Runtime resolution**: Service lookup happens at runtime vs compile-time
- **Hidden dependencies**: Services not visible in function signatures
- **Lookup overhead**: Small performance cost for service resolution
- **Service location coupling**: Components coupled to ServiceProvider interface

### Neutral
- **Complexity trade-off**: More complex than singletons, simpler than full DI frameworks
- **Performance**: Lookup cost vs construction injection cost

## Service Categories

**Core Engine Services:**
- **FileSystem**: Disk I/O, path resolution, file existence checking
- **Window**: Platform window management, input handling
- **Graphics::Renderer**: High-level rendering coordination
- **Vulkan services**: Low-level graphics API management

**Resource Services:**
- **Resources::Manager**: Central resource coordinator
- **Container\<T\>**: Type-specific resource caching and loading
- **Net services**: Network downloading for remote resources

**Simulation Services:**
- **Physics::Manager**: Physics world management and simulation
- **Scenes::Manager**: Scene graph management and coordination  
- **Audio::Manager**: 3D audio processing and device management

**Utility Services:**
- **Settings**: Configuration and preferences management
- **Tracer**: Logging and debugging output

## Service Lifecycle

**Initialization Order:**
```
1. Core services (FileSystem, Settings, Window)
2. Platform services (Vulkan, Graphics renderer)  
3. Resource services (Resource manager, containers)
4. Simulation services (Physics, Scenes, Audio)
5. Application services (User-specific services)
```

**Dependency Resolution:**
```cpp
// Services can access other services via their own ServiceProvider
class ResourceManager : public ServiceInterface, public ServiceProvider {
    // As ServiceInterface: provides resource management to others
    // As ServiceProvider: can access FileSystem, Graphics, etc.
    
    void loadTexture(const string& name) {
        auto data = fileSystem().readFile(name);  // Use FileSystem service
        auto texture = graphicsRenderer().createTexture(data);  // Use Graphics service
    }
};
```

## Alternative Patterns Considered

**Global Singletons:**
```cpp
// REJECTED
TextureResource::load() {
    auto data = FileSystem::instance().readFile(path);  // Global access
    auto texture = Renderer::instance().createTexture(data);
}
```
- ❌ Testing difficulties (global state)
- ❌ Hidden dependencies
- ❌ Initialization order problems

**Constructor Dependency Injection:**
```cpp
// REJECTED 
class TextureResource {
    TextureResource(FileSystem& fs, Renderer& renderer);  // Explicit deps
};
```
- ❌ Deep parameter passing through call chains
- ❌ Circular dependency problems
- ❌ Complex factory hierarchies

**Pure Factory Pattern:**
```cpp
// REJECTED
class ResourceFactory {
    TextureResource* createTexture(deps...);  // Central creation
};
```
- ❌ Centralized creation logic
- ❌ Factory becomes god object
- ❌ Limited flexibility for resource types

## Integration Points

**Resource System Integration:**
- All resource loading methods receive ServiceProvider
- Resources can access any engine service during loading
- Fail-safe pattern works with service dependencies

**Component Integration:**
- Components receive ServiceProvider during initialization
- Can access services for functionality (graphics, audio, etc.)
- Service access typically cached in component members

**Cross-System Communication:**
- Systems locate each other via ServiceProvider
- Clear service boundaries prevent tight coupling
- Observer pattern used for cross-system events

## Related ADRs
- ADR-003: Fail-Safe Resource Management (resources use ServiceProvider for dependencies)
- ADR-006: Component Composition Over Inheritance (components access services via ServiceProvider)

## References
- `src/ServiceInterface.hpp` - Base service interface definition
- `src/Resources/ServiceProvider.hpp` - Service provider interface
- `src/Resources/Manager.hpp` - Example of ServiceInterface + ServiceProvider implementation