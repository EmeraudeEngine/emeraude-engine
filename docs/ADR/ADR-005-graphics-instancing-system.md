# ADR-005: Graphics Instancing System Architecture

## Status
**Accepted** - Core architecture for all rendering operations

## Context

Game engines need to render multiple similar objects efficiently:
- Forests with hundreds of identical trees
- Cities with many similar buildings  
- Particle systems with thousands of particles
- UI elements with repeated textures

**Traditional Approaches:**
1. **Individual draw calls**: One draw call per object (inefficient)
2. **Manual batching**: Developer manually groups similar objects
3. **Automatic batching**: Engine automatically groups objects by material/geometry
4. **GPU instancing**: Send multiple transforms in single draw call

**Performance Considerations:**
- Draw call overhead dominates performance for many small objects
- GPU can efficiently process thousands of similar objects in parallel
- CPU cost of preparing draw calls often exceeds GPU rendering cost
- Memory bandwidth for uploading per-object data

**Design Complexity:**
- How to separate object definition from object instances
- How to share resources (geometry, materials, shaders) efficiently
- How to handle per-instance data (transforms, colors, etc.)
- How to integrate with scene graph and component systems

## Decision

**Emeraude Engine implements an instancing hierarchy that separates object definition from object usage, enabling efficient rendering of duplicates.**

**Architecture:**
```
Geometry (shape definition)
    +
Material (appearance definition)  
    =
Renderable (complete object definition)
    ↓
RenderableInstance (positioned usage with unique transform)
    ↓
Visual Component (scene graph integration)
```

**Key Principles:**
- **Separation of concerns**: Definition (Renderable) vs Usage (RenderableInstance)
- **Resource sharing**: Many instances share single Renderable
- **Automatic optimization**: Renderer automatically batches identical Renderables
- **Transparent to user**: Developer creates instances, engine handles optimization

## Architecture Details

**Layer 1: Geometry (Shape Definition)**
- GPU-side vertex and index data
- Vertex format definition (queryable by Saphir)
- Immutable once uploaded to GPU
- Shared by multiple Renderables

**Layer 2: Material (Appearance Definition)**  
- Textures, colors, and material properties
- Requirements declaration (needs normals, tangents, UVs, etc.)
- Loaded via Resource system with fail-safe fallbacks
- Shared by multiple Renderables

**Layer 3: Renderable (Complete Object)**
- Geometry + Material combination
- Single Vulkan pipeline (optimized for this exact combination)
- Saphir compatibility validation (Material requirements ↔ Geometry attributes)
- Immutable definition shared by all instances

**Layer 4: RenderableInstance (Positioned Object)**
- Reference to parent Renderable (shared)
- Unique transformation matrix (position, rotation, scale)
- Per-instance parameters (optional material overrides)
- Rendering flags (shadows, visibility, etc.)

## Implementation Strategy

**Resource Creation:**
```cpp
// 1. Load resources (cached, reused)
auto geometry = resources.get<GeometryResource>("cube");
auto material = resources.get<MaterialResource>("wood");

// 2. Create renderable (definition) - happens once
auto renderable = Renderable::create(geometry, material);
// → Saphir generates shader for this Material+Geometry combination
// → Single Vulkan pipeline created

// 3. Create many instances (usage) - happens many times  
for (int i = 0; i < 1000; ++i) {
    auto node = scene->root()->createChild("crate_" + std::to_string(i), position[i]);
    node->newVisual(renderable);  // RenderableInstance created
}
```

**Automatic Batching:**
```cpp
// Renderer analysis each frame:
// 1000 RenderableInstances all reference same Renderable
// → Single draw call with 1000 transforms uploaded to GPU
// → Massive performance improvement (1000x fewer draw calls)
```

**Integration with Scene Graph:**
```cpp
// Visual Component bridges Scene Graph ↔ Graphics
class Visual : public Component {
    RenderableInstance m_instance;
    
    void move(const CartesianFrame& parentFrame) override {
        // Node moved → update RenderableInstance transform
        m_instance.setTransform(parentFrame);
    }
};
```

## Consequences

### Positive
- **Performance**: Massive reduction in draw calls for duplicate objects
- **Memory efficiency**: Geometry, materials, and shaders shared across instances
- **Automatic optimization**: Developer creates instances, renderer optimizes automatically
- **Scalability**: Can efficiently render thousands of similar objects
- **Resource reuse**: Single Renderable definition supports unlimited instances
- **GPU instancing support**: Automatic use of hardware instancing where beneficial

### Negative
- **Complexity overhead**: More layers than simple "one mesh = one draw call"
- **Memory indirection**: Additional pointer lookups for shared resources
- **Flexibility limitations**: Harder to optimize for completely unique objects
- **CPU overhead**: Transform management and instance tracking costs

### Neutral
- **Industry standard**: Similar to approaches used in modern game engines
- **Vulkan alignment**: Fits well with Vulkan's explicit resource management

## Performance Characteristics

**Scalability Benefits:**
```
Traditional:  1000 objects = 1000 draw calls
Instanced:    1000 objects = 1-10 draw calls (grouped by Renderable)

Memory usage:
Traditional:  1000 × (Geometry + Material + Shader) = massive duplication
Instanced:    1 × (Geometry + Material + Shader) + 1000 × Transform = minimal overhead
```

**CPU Performance:**
- Setup cost: O(1) per unique Renderable
- Instance cost: O(1) per RenderableInstance  
- Rendering cost: O(unique Renderables) instead of O(total instances)

**GPU Performance:**
- Reduced draw call overhead
- Better GPU utilization (larger batches)
- Efficient transform uploads
- Hardware instancing support

## Integration Points

**Scene Graph Integration:**
- Visual Components automatically create RenderableInstances
- Node transforms automatically update instance transforms
- Hierarchical transforms handled by Scene Graph

**Resource System Integration:**
- Geometry and Material loaded via fail-safe Resource system
- Renderable creation triggers Saphir shader generation
- Resource dependencies managed automatically

**Physics Integration:**
- RenderableInstance transforms read from physics Node positions
- No direct coupling (Scene Graph mediates)

## Related ADRs
- ADR-002: Vulkan-Only Graphics API (instancing implemented via Vulkan)
- ADR-003: Fail-Safe Resource Management (resources shared by instances)
- ADR-004: Saphir Shader Generation (shaders shared across instances)
- ADR-006: Component Composition Over Inheritance (Visual component pattern)

## References
- `docs/graphics-system.md` - Complete graphics architecture with instancing details
- `src/Graphics/AGENTS.md` - Graphics system implementation context
- `docs/scene-graph-architecture.md` - Scene graph integration patterns