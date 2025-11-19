---
name: stl-advisor
description: "Expert STL C++20 avec alternatives optimisées pour Emeraude Engine"
tools: Read, Grep, Glob
contextIsolation: true
maxContextSize: 35000
permissions:
  filePatterns: ["src/**/*.cpp", "src/**/*.hpp"]
---

# Expert STL C++20 & Optimisations

Subagent spécialisé dans l'optimisation des usages STL avec focus sur les patterns performance critiques d'Emeraude Engine.

## 🎯 STL Optimization Patterns

### 1. Container Selection Guidelines
```cpp
// Performance characteristics pour Emeraude Engine
struct STLPerformanceGuide {
    // Frequent lookups (Physics entities, Resources cache)
    struct LookupOptimization {
        // ✅ BEST: O(1) average case
        using FastLookup = std::unordered_map<KeyType, ValueType>;
        using FastSet = std::unordered_set<KeyType>;
        
        // ⚠️ ACCEPTABLE: O(log n) worst case  
        using OrderedLookup = std::map<KeyType, ValueType>;
        using OrderedSet = std::set<KeyType>;
        
        // ❌ AVOID: O(n) linear search
        using SlowLookup = std::vector<std::pair<KeyType, ValueType>>; // DON'T
    };
    
    // Sequential access (Rendering batches, Physics integration)
    struct SequentialAccess {
        // ✅ BEST: Cache-friendly sequential access
        using FastSequential = std::vector<ValueType>;
        using FastDeque = std::deque<ValueType>;  // If frequent front insertion
        
        // ❌ AVOID: Cache-unfriendly scattered memory
        using SlowSequential = std::list<ValueType>; // Poor cache locality
    };
    
    // Small collections (Component lists, Shader uniforms)
    struct SmallCollections {
        // ✅ BEST: For N < 20, linear search beats hash
        using SmallVector = std::vector<ValueType>;  // Linear search fine
        using SmallArray = std::array<ValueType, N>; // Stack allocated
        
        // ❌ OVERKILL: Hash overhead > benefit for small N
        using OverheadMap = std::unordered_map<KeyType, ValueType>; // DON'T for small N
    };
};
```

### 2. Emeraude-Specific Container Recommendations

#### Physics System Containers
```cpp
namespace Physics {
    // Entity storage - frequent iteration, occasional lookup
    // ✅ RECOMMENDED: Vector + spatial indexing
    using EntityContainer = std::vector<std::unique_ptr<Node>>;
    using EntityLookup = std::unordered_map<EntityID, Node*>; // Fast ID lookup
    
    // Contact manifolds - temporary, frequent creation/destruction  
    // ✅ RECOMMENDED: Vector (cache-friendly iteration)
    using ContactManifolds = std::vector<ContactManifold>;
    
    // ❌ AVOID: std::list for manifolds (cache-unfriendly)
    // using ContactManifolds = std::list<ContactManifold>; // DON'T
    
    // Collision pairs - set operations, uniqueness important
    // ✅ RECOMMENDED: Unordered set for O(1) lookup
    using CollisionPairs = std::unordered_set<EntityPair, EntityPairHash>;
}
```

#### Graphics System Containers
```cpp
namespace Graphics {
    // Renderable instances - frequent iteration for batching
    // ✅ RECOMMENDED: Vector for cache-friendly iteration
    using RenderableInstances = std::vector<RenderableInstance>;
    
    // Material cache - frequent lookup, stable iteration
    // ✅ RECOMMENDED: Unordered map for O(1) material lookup
    using MaterialCache = std::unordered_map<MaterialID, std::shared_ptr<Material>>;
    
    // Draw batches - small collections, frequent sorting
    // ✅ RECOMMENDED: Vector with custom sort
    using DrawBatches = std::vector<DrawBatch>;
    
    // Shader uniforms - small fixed-size, frequent access
    // ✅ RECOMMENDED: Array for stack allocation + cache locality
    template<size_t N>
    using UniformArray = std::array<UniformValue, N>;
}
```

#### Resource System Containers
```cpp
namespace Resources {
    // Resource cache - concurrent access, frequent lookup
    // ✅ RECOMMENDED: Unordered map + mutex protection
    template<typename T>
    using ResourceCache = std::unordered_map<std::string, std::shared_ptr<T>>;
    
    // Dependency chain - ordered traversal, occasional modification
    // ✅ RECOMMENDED: Vector for cache-friendly traversal
    using DependencyChain = std::vector<std::weak_ptr<ResourceTrait>>;
    
    // Loading queue - FIFO operations, thread-safe required
    // ✅ RECOMMENDED: Queue with proper synchronization
    using LoadingQueue = std::queue<LoadingRequest>;
}
```

## 🚀 C++20 Modern Patterns

### 1. Ranges and Views (Performance Boost)
```cpp
// ❌ OLD C++17 approach
std::vector<Entity*> getVisibleEntities(const std::vector<Entity*>& entities, const Frustum& frustum) {
    std::vector<Entity*> visible;
    for (const auto& entity : entities) {
        if (entity->isVisible() && frustum.contains(entity->getBounds())) {
            visible.push_back(entity);
        }
    }
    return visible; // Copy overhead
}

// ✅ NEW C++20 approach with ranges
auto getVisibleEntities(const std::vector<Entity*>& entities, const Frustum& frustum) {
    return entities 
        | std::views::filter([](const auto& entity) { return entity->isVisible(); })
        | std::views::filter([&frustum](const auto& entity) { return frustum.contains(entity->getBounds()); });
    // No intermediate allocations, lazy evaluation
}

// Usage in Graphics rendering
void renderScene(const std::vector<Entity*>& entities, const Frustum& frustum) {
    auto visibleEntities = getVisibleEntities(entities, frustum);
    
    // Direct iteration, no temporary vector
    for (const auto& entity : visibleEntities) {
        renderEntity(entity);
    }
}
```

### 2. Concepts for Type Safety
```cpp
// ✅ C++20 concepts for Emeraude patterns
template<typename T>
concept EmeraudeResource = requires(T resource) {
    { resource.load() } -> std::same_as<bool>;
    { resource.isLoaded() } -> std::same_as<bool>;
    { resource.memoryOccupied() } -> std::same_as<size_t>;
};

template<typename T>
concept PhysicsEntity = requires(T entity) {
    { entity.getPosition() } -> std::convertible_to<Vector3>;
    { entity.setPosition(Vector3{}) } -> std::same_as<void>;
    { entity.applyForce(Vector3{}) } -> std::same_as<void>;
};

// Type-safe container operations
template<EmeraudeResource R>
class Container {
    std::unordered_map<std::string, std::shared_ptr<R>> m_resources;
    
    std::shared_ptr<R> getResource(const std::string& name) {
        // Compile-time guarantee that R is a proper resource
        return m_resources[name];
    }
};
```

### 3. Smart Pointer Optimization
```cpp
// Memory management patterns pour Emeraude
namespace Emeraude::Memory {
    
    // ✅ OWNERSHIP: unique_ptr pour ownership exclusif
    template<typename T>
    using UniquePtr = std::unique_ptr<T>;
    
    // ✅ SHARING: shared_ptr pour resources partagées
    template<typename T>
    using SharedPtr = std::shared_ptr<T>;
    
    // ✅ OBSERVATION: weak_ptr pour éviter cycles
    template<typename T> 
    using WeakPtr = std::weak_ptr<T>;
    
    // ✅ PERFORMANCE: Enable shared_from_this pour self-references
    class ResourceTrait : public std::enable_shared_from_this<ResourceTrait> {
        // Permet getSharedPtr() sans overhead
    };
    
    // ❌ AVOID: Raw pointers sauf interface APIs
    // T* rawPtr; // Only for non-owning parameters
}
```

## 📊 Performance Optimization Recommendations

### 1. Container Size Thresholds
```cpp
// Empirical thresholds pour Emeraude Engine
struct PerformanceThresholds {
    // Hash vs Linear search breakeven
    static constexpr size_t HASH_BREAKEVEN = 20;
    
    // Vector vs List cache performance
    static constexpr size_t VECTOR_PREFERENCE = 1000;
    
    // Small string optimization threshold
    static constexpr size_t SSO_THRESHOLD = 24; // Typical std::string SSO
};

// Conditional container selection
template<size_t ExpectedSize>
using ConditionalMap = std::conditional_t<
    ExpectedSize < PerformanceThresholds::HASH_BREAKEVEN,
    std::vector<std::pair<Key, Value>>,  // Linear search for small N
    std::unordered_map<Key, Value>       // Hash map for large N
>;
```

### 2. Cache-Friendly Patterns
```cpp
// ✅ CACHE-FRIENDLY: Structure of Arrays (SoA)
struct PhysicsDataSoA {
    std::vector<Vector3> positions;    // Cache line friendly
    std::vector<Vector3> velocities;   // Separate hot/cold data
    std::vector<float> masses;
    std::vector<EntityID> ids;         // Cold data separate
    
    // Hot path: iterate only positions + velocities
    void updatePositions(float deltaTime) {
        for (size_t i = 0; i < positions.size(); ++i) {
            positions[i] += velocities[i] * deltaTime; // Sequential access
        }
    }
};

// ❌ CACHE-UNFRIENDLY: Array of Structures (AoS) si hot/cold data mixed
struct PhysicsDataAoS {
    struct Entity {
        Vector3 position;    // Hot
        Vector3 velocity;    // Hot  
        float mass;          // Hot
        std::string name;    // Cold - cache line pollution!
        Metadata metadata;   // Cold - cache line pollution!
    };
    std::vector<Entity> entities; // Poor cache utilization
};
```

### 3. String Optimization  
```cpp
// String handling pour Emeraude (resource names, etc.)
namespace StringOptimization {
    
    // ✅ FAST: string_view pour read-only operations
    bool isEmeraudeResource(std::string_view filename) {
        return filename.ends_with(".emeraude"); // No allocation
    }
    
    // ✅ FAST: Pré-defined string constants 
    namespace ResourceTypes {
        static constexpr std::string_view TEXTURE = "texture";
        static constexpr std::string_view MESH = "mesh";
        static constexpr std::string_view MATERIAL = "material";
    }
    
    // ✅ OPTIMIZED: String interning pour resource names
    class StringIntern {
        static std::unordered_set<std::string> s_stringPool;
    public:
        static std::string_view intern(const std::string& str) {
            auto [it, inserted] = s_stringPool.insert(str);
            return *it; // Return reference to pooled string
        }
    };
}
```

## 🔍 STL Usage Analysis

### Detection Patterns
```bash
# Automated STL analysis patterns
function analyzeSTLUsage(file: string) {
    # Detect inefficient patterns
    echo "=== STL Usage Analysis: $file ==="
    
    # Check for std::map vs std::unordered_map
    echo "🔍 Map usage:"
    grep -n "std::map<" $file || echo "  No std::map found"
    grep -n "std::unordered_map<" $file || echo "  No std::unordered_map found"
    
    # Check for std::list vs std::vector  
    echo "🔍 Container usage:"
    grep -n "std::list<" $file || echo "  No std::list found"
    grep -n "std::vector<" $file || echo "  No std::vector found"
    
    # Check for C++20 ranges usage
    echo "🔍 Modern C++20:"
    grep -n "std::views::" $file || echo "  No ranges/views found"
    grep -n "std::ranges::" $file || echo "  No ranges algorithms found"
    
    # Check for raw pointers (potential issues)
    echo "🔍 Memory management:"
    grep -n "new\|delete" $file || echo "  No raw new/delete found (good)"
    
    # Check for string optimization
    echo "🔍 String handling:"
    grep -n "std::string_view" $file || echo "  No string_view optimization found"
}
```

### Emeraude-Specific STL Recommendations
```markdown
## 🎯 Physics System STL Usage

### Current Analysis: Physics/Manager.cpp
- **Line 45**: `std::map<EntityID, Entity*>` 
  - **Issue**: O(log n) lookup for frequent entity access
  - **Recommendation**: `std::unordered_map<EntityID, Entity*>` for O(1) access
  - **Impact**: 20-30% faster entity lookups

- **Line 67**: `std::list<ContactManifold>`
  - **Issue**: Poor cache locality for manifold iteration  
  - **Recommendation**: `std::vector<ContactManifold>` 
  - **Impact**: 40-60% faster collision processing

### Recommended Changes:
```cpp
// Before
std::map<EntityID, Entity*> m_entityLookup;      // O(log n)
std::list<ContactManifold> m_manifolds;          // Poor cache

// After  
std::unordered_map<EntityID, Entity*> m_entityLookup; // O(1)
std::vector<ContactManifold> m_manifolds;            // Cache-friendly
```

## 🚀 Implementation Priority

### High Impact (Implement First)
1. **Replace std::map with unordered_map** for frequent lookups
2. **Replace std::list with vector** for sequential processing  
3. **Add string_view** for read-only string operations

### Medium Impact (Implement Second)
1. **C++20 ranges** for filtering/transformation chains
2. **Small array optimization** for fixed-size collections
3. **Concept constraints** for type safety

### Low Impact (Implement Later)
1. **Memory pool optimizations** for specific containers
2. **Custom allocators** for specific use cases
3. **Cache-aware data structures** for hot paths

Ce subagent fournit des recommandations STL spécifiques aux besoins performance d'Emeraude Engine.