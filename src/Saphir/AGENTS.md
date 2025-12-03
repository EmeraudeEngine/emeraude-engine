# Saphir Shader System

Context for developing the Emeraude Engine automatic shader generation system.

## Module Overview

Saphir automatically generates GLSL code from material properties, geometry attributes, and scene context. It eliminates the need for hundreds of manually written shader variants.

## Saphir-Specific Rules

### Generation Philosophy
- **Parametric generation**: Shaders created from unknowns (material + geometry + scene)
- **STRICT compatibility checking**: Material requirements MUST match geometry attributes
- **Graceful failure**: If incompatible → resource loading fails → application continues
- **Aggressive caching**: 3-level cache avoids redundant generation and compilation

### 3-Level Cache System

| Level | Object | Location | Cache Key | Benefit |
|-------|--------|----------|-----------|---------|
| 1 | `ShaderModule` | `ShaderManager` | Source code hash | Reuse compiled SPIR-V between programs |
| 2 | `Program` | `Renderer::m_programs` | `computeProgramCacheKey()` | Skip entire shader generation |
| 3 | `GraphicsPipeline` | `Renderer::m_graphicsPipelines` | `getHash()` | Reuse Vulkan pipeline objects |

**Program cache** (Level 2) provides the biggest gain - it short-circuits before any shader code generation when an identical configuration exists. See: `Generator::Abstract::generateShaderProgram()`

```
RenderableInstance created
    │
    ▼
computeProgramCacheKey() ──► Cache hit? ──► Return cached Program (skip all generation)
    │                              │
    │ miss                         │ hit (1482 reuses typical)
    ▼                              │
Generate GLSL code                 │
Compile ShaderModules              │
Create Program                     │
Cache Program ◄────────────────────┘
```

### Generator Types
1. **SceneGenerator**: 3D objects with full lighting, materials, effects
2. **OverlayGenerator**: 2D elements (UI, HUD, text, sprites)
3. **ShadowManager**: Minimal shaders for shadow map generation

### Compatibility Checking
```cpp
Material requirements: [Normals, TangentSpace, TextureCoordinates2D]
Geometry attributes:   [positions, normals, uvs]  // NO tangents!
→ FAILURE with detailed log
```

## Development Commands

```bash
# Specific tests
ctest -R Saphir
./test --filter="*Shader*"
```

## Important Files

- `Generator/Abstract.cpp/.hpp` - Base class for all generators, implements cache lookup
- `Generator/SceneRendering.cpp/.hpp` - 3D scene rendering generator
- `Generator/ShadowCasting.cpp/.hpp` - Shadow map generator
- `Generator/OverlayRendering.cpp/.hpp` - 2D overlay generator
- `LightGenerator.cpp/.hpp` - Lighting generation (PerFragment, PerVertex)
- `Program.cpp/.hpp` - Shader program (shaders + pipeline layout)
- `ShaderManager.cpp/.hpp` - ShaderModule cache and compilation

## Development Patterns

### Adding a New Generator
1. Inherit from `Generator::Abstract`
2. Implement `computeProgramCacheKey()` - **REQUIRED** for cache system
3. Implement `prepareUniformSets()`, `onGenerateShadersCode()`, `onCreateDataLayouts()`, `onGraphicsPipelineConfiguration()`
4. Cache key must include all parameters that affect shader output

### Extending an Existing Generator
1. Identify generator type (Scene/Overlay/Shadow)
2. Add conditions in generate methods
3. Test all material/geometry combinations
4. Verify cache performance (input hash)

### Debugging Generation Failures
1. Examine detailed logs (material vs geometry)
2. Verify tangent export (Blender/Maya)
3. Simplify material OR enrich geometry
4. Test with default material first

## Critical Points

- **Strict checking**: Material requirements MUST be satisfied by geometry
- **Hash-based cache**: Identical inputs → same shader (performance)
- **Fail-safe integration**: Failures logged but app continues (no crash)
- **Y-down convention**: Projection matrices configured for Vulkan
- **Thread safety**: Cache protected, generation can be parallel
- **Used by Graphics and Overlay**: Graphics (3D), Overlay (2D) use Saphir
- **Runtime generation**: Shaders generated on demand during resource loading

## Detailed Documentation

For complete Saphir system architecture:
- @docs/saphir-shader-system.md - Parametric generation, compatibility, cache

Related systems:
- @src/Graphics/AGENTS.md - Material and Geometry for 3D generation
- @src/Overlay/AGENTS.md - 2D pipeline via OverlayGenerator
- @src/Resources/AGENTS.md - Generation during onDependenciesLoaded()
- @src/Vulkan/AGENTS.md - SPIR-V compilation and pipelines
