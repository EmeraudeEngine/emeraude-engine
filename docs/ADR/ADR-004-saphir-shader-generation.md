# ADR-004: Saphir Automatic Shader Generation System

## Status
**Accepted** - Implemented as the primary shader management system

## Context

Traditional game engines require manual creation and maintenance of shader variants:

**Manual Shader Approach:**
- shader_diffuse.glsl
- shader_diffuse_normal.glsl  
- shader_diffuse_normal_specular.glsl
- shader_pbr_simple.glsl
- shader_pbr_full.glsl
- ... (potentially hundreds of combinations)

**Problems with Manual Shaders:**
- **Combinatorial explosion**: N features = 2^N potential shader variants
- **Maintenance nightmare**: Code duplication across similar shaders
- **Error prone**: Easy to forget variants or introduce inconsistencies
- **Inflexible**: Adding new material feature requires modifying many files
- **Storage cost**: Disk space, memory, and build time overhead
- **Developer burden**: Writing GLSL for every possible combination

**Alternative Approaches:**
1. **Uber-shaders**: Single shader with all features, runtime branching
2. **Shader includes/macros**: Preprocessor-based variant generation  
3. **Node-based editors**: Visual shader creation tools
4. **Runtime compilation**: Generate shaders on-demand from parameters

## Decision

**Emeraude Engine implements "Saphir" - an automatic GLSL generation system that creates optimal shaders from Material + Geometry + Scene parameters.**

**Core Concept:**
> "Generators are shaders awaiting their unknowns"

**Architecture:**
```
Material (requirements, textures, properties)
    +
Geometry (vertex attributes available)
    + 
SceneContext (lighting, shadows, effects)
    ↓
Generator.generate() 
    ↓
GLSL source (perfectly adapted)
    ↓
GLSLang compile → SPIR-V
    ↓
Vulkan pipeline
```

**Three Generator Types:**
1. **SceneGenerator**: 3D objects with full lighting, materials, effects
2. **OverlayGenerator**: 2D UI elements (no lighting, screen-space)
3. **ShadowManager**: Minimal shaders for shadow map generation

## Implementation Strategy

**Strict Compatibility Checking:**
- Material declares requirements (normals, tangent space, UVs, vertex colors)
- Geometry provides attributes (vertex format)
- Generator verifies: Requirements ⊆ Available Attributes
- If incompatible → resource loading fails → neutral resource used

**Code Generation Process:**
```cpp
// Example: PBR material with normal mapping
Material requirements: [Normals, TangentSpace, TextureCoordinates2D]
Geometry attributes:   [Positions, Normals, Tangents, UVs]
Scene context:        [DirectionalLight, Shadows=true]

→ SceneGenerator produces:
   - Vertex shader: Transform vertices, pass tangent space
   - Fragment shader: Sample textures, compute PBR lighting, shadow mapping
   - Only includes needed features (optimal performance)
```

**Cache System:**
```cpp
// Hash inputs before generation
string cacheKey = hash(material, geometry, sceneContext);
if (cache.contains(cacheKey)) {
    return cache[cacheKey];  // Skip generation + compilation
}

// Cache miss: generate, compile, store
string glsl = generator.generate(material, geometry, sceneContext);
spirv = compile(glsl);
cache[cacheKey] = spirv;
```

## Consequences

### Positive
- **Zero maintenance**: Shaders exist once in generator logic
- **Always optimal**: Only includes features actually needed
- **No forgotten variants**: Generated on-demand for any combination
- **Flexible**: New features added in one place affect all compatible combinations
- **Efficient storage**: No pre-generated shader files
- **Cache optimization**: Avoids redundant generation and compilation
- **Strict validation**: Impossible to use incompatible Material+Geometry combinations

### Negative
- **Runtime generation cost**: First-time shader generation takes time
- **Complex generator logic**: Generator code is more complex than individual shaders
- **Limited GLSL control**: Less fine-tuned control than hand-written shaders
- **Debug complexity**: Generated GLSL harder to debug than static files
- **Dependency on generators**: All shaders depend on generator system functioning

### Neutral
- **Industry trend**: Many modern engines use similar approaches (Unity ShaderGraph, Unreal Material Editor)
- **Cache effectiveness**: Performance depends on cache hit rate

## Compatibility Rules

**Material Requirements (what material needs):**
- `Normals` → For lighting calculations
- `TangentSpace` → For normal mapping (requires tangents + bitangents)  
- `TextureCoordinates` → Primary UVs (2D or 3D)
- `VertexColors` → Per-vertex color attributes

**Failure Example:**
```
Material "PBR_Metal":
    requires: [Normals, TangentSpace, TextureCoordinates2D]
    textures: [diffuse, normal, roughness]

Geometry "SimpleBox": 
    attributes: [positions, normals, uvs]  // NO TANGENTS!

Result: FAILURE
Log: "Material 'PBR_Metal' requires tangent space but geometry 'SimpleBox' only provides [positions, normals, uvs]. Add tangents to geometry or use simpler material."
→ onDependenciesLoaded() returns false
→ Resource loading fails → neutral renderable used
```

## Generator Specialization

**SceneGenerator (3D Objects):**
- Full lighting (directional, point, spot)
- Normal/tangent space support
- Shadow receiving
- PBR and Phong material models
- Texture sampling (diffuse, normal, roughness, metallic, emissive)

**OverlayGenerator (2D UI):**
- No lighting (screen-space)
- Alpha blending support
- Texture sampling for UI elements
- Vertex colors for tinting
- Clipping regions

**ShadowManager (Depth-only):**
- Minimal vertex shader (position transform only)
- Empty/minimal fragment shader
- No materials, textures, or lighting
- Optimized for shadow map generation

## Integration Points

**Resource Loading Integration:**
```cpp
// Inside onDependenciesLoaded()
bool onDependenciesLoaded() override {
    // 1. Check compatibility
    if (!saphir.checkCompatibility(m_material, m_geometry)) {
        Log::error("Incompatible Material+Geometry combination");
        return false;  // Resource fails → neutral resource used
    }
    
    // 2. Generate shader
    auto glsl = saphir.generate(m_material, m_geometry, sceneContext);
    auto spirv = glslang.compile(glsl);
    
    // 3. Create Vulkan pipeline  
    m_pipeline = vulkan.createPipeline(spirv);
    
    return true;  // Resource ready for rendering
}
```

## Related ADRs
- ADR-002: Vulkan-Only Graphics API (generates Vulkan-compatible GLSL/SPIR-V)
- ADR-003: Fail-Safe Resource Management (shader generation failures handled gracefully)
- ADR-005: Graphics Instancing System (shaders shared across instances)

## References
- `docs/saphir-shader-system.md` - Complete Saphir architecture and usage
- `src/Saphir/AGENTS.md` - Saphir implementation context  
- `src/Graphics/AGENTS.md` - Integration with Graphics system