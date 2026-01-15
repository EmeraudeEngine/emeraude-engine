# Saphir Shader System

> [!CRITICAL]
> **Before modifying ANY generator or cache code, READ [`docs/pipeline-caching-system.md`](../../docs/pipeline-caching-system.md) FIRST!**
>
> Key rule: `computeProgramCacheKey()` MUST include `renderPassHandle` as its first hash component.
> Forgetting this causes Vulkan validation errors: "sample count mismatch", "format mismatch".

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
| 3 | `GraphicsPipeline` | `Renderer::m_graphicsPipelines` | `getHash(renderPass)` | Reuse Vulkan pipeline objects |

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

> [!CRITICAL]
> **computeProgramCacheKey() MUST include renderPassHandle!**
>
> Each generator's `computeProgramCacheKey()` MUST include the render pass handle as the FIRST
> hash component. This is required because Vulkan pipelines are tied to specific render passes.
>
> Without renderPassHandle, pipelines created for offscreen rendering (1 sample) would be
> incorrectly reused for main view rendering (4 samples), causing validation errors.

### computeProgramCacheKey() Requirements

Every generator must implement `computeProgramCacheKey()` with these MANDATORY components:

```cpp
size_t MyGenerator::computeProgramCacheKey () const noexcept
{
    size_t hash = Hash::FNV1a(ClassId);  // Generator type identifier

    // 1. MANDATORY: Render pass handle (pipeline compatibility)
    if ( const auto * framebuffer = this->renderTarget()->framebuffer(); framebuffer != nullptr )
    {
        hashCombine(hash, reinterpret_cast< size_t >(framebuffer->renderPass()->handle()));
    }

    // 2. MANDATORY: Render target type (cubemap vs single layer)
    hashCombine(hash, static_cast< size_t >(this->renderTarget()->isCubemap()));

    // 3. Generator-specific parameters...
    // (renderable name, layer index, flags, etc.)

    return hash;
}
```

**Required includes** for accessing render pass handle:
```cpp
#include "Vulkan/Framebuffer.hpp"
#include "Vulkan/RenderPass.hpp"
```

### Generator Types
1. **SceneGenerator**: 3D objects with full lighting, materials, effects
2. **OverlayRendering**: 2D elements (UI, HUD, text, sprites) - generates 4 program variants
3. **ShadowManager**: Minimal shaders for shadow map generation

### OverlayRendering Generator

Generates 4 shader program variants based on:
- **Premultiplied alpha**: Affects blend state configuration
- **BGRA source**: Affects fragment shader (`.bgra` swizzle or not)

Cache key includes: render target type + premultiplied alpha + BGRA source format.
See: `OverlayRendering.cpp:computeProgramCacheKey()`

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

## Fresnel Effect Generation (Reflection + Refraction)

When a material has BOTH reflection AND refraction components, Saphir generates Fresnel blending code.

### Generation Flow

1. **`StandardResource::generateFragmentShaderCode()`** detects both components present
2. Generates `fresnelFactor` variable using Schlick approximation:
   ```glsl
   const float F0 = pow((1.0 - ubMaterial.refractionIOR) / (1.0 + ubMaterial.refractionIOR), 2.0);
   const float cosTheta = max(dot(-refractionI, refractionNormal), 0.0);
   const float fresnelFactor = F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
   ```
3. **`LightGenerator::generateAmbientFragmentShader()`** uses fresnelFactor for ambient pass
4. **`LightGenerator::generateFinalFragmentOutput()`** uses fresnelFactor for light passes

### Lighting Code Pattern

```cpp
// In LightGenerator.cpp - when m_useReflection && m_useRefraction
const auto code = (std::stringstream{} <<
    "const vec3 reflected = mix(" << m_surfaceDiffuseColor << ", " << m_surfaceReflectionColor << ", " << m_surfaceReflectionAmount << ").rgb;" "\n"
    "const vec3 refracted = mix(" << m_surfaceDiffuseColor << ", " << m_surfaceRefractionColor << ", " << m_surfaceRefractionAmount << ").rgb;" "\n\n" <<
    m_fragmentColor << ".rgb += mix(refracted, reflected, fresnelFactor) * lighting;").str();
```

### Important Notes

- `fresnelFactor` is **only generated when BOTH** reflection AND refraction are present
- Using `fresnelFactor` without both components causes "undefined variable" shader error
- The `amount` parameters control the blend between base color and cubemap sample
- Fresnel determines the blend between reflected and refracted result
- Files: `StandardResource.cpp:1449-1472`, `LightGenerator.cpp:601-672`

## Critical Points

- **Strict checking**: Material requirements MUST be satisfied by geometry
- **Hash-based cache**: Identical inputs → same shader (performance)
- **Fail-safe integration**: Failures logged but app continues (no crash)
- **Y-down convention**: Projection matrices configured for Vulkan
- **Thread safety**: Cache protected, generation can be parallel
- **Used by Graphics and Overlay**: Graphics (3D), Overlay (2D) use Saphir
- **Runtime generation**: Shaders generated on demand during resource loading
- **Alpha preservation**: Lighting calculations use `.rgb` only, never modify alpha channel. See: `LightGenerator.cpp:603-661`
- **Color space conversion** (3D only): sRGB↔Linear functions apply gamma only to RGB, alpha passes through unchanged. See: `FragmentShader.cpp:generateToSRGBColorFunction()`, `generateToLinearColorFunction()`. Note: Overlay system does NOT use color space conversion - swap-chain format (UNORM vs SRGB) determines final handling.

## Detailed Documentation

For complete Saphir system architecture:
- @docs/saphir-shader-system.md - Parametric generation, compatibility, cache

Related systems:
- @src/Graphics/AGENTS.md - Material and Geometry for 3D generation
- @src/Overlay/AGENTS.md - 2D pipeline via OverlayGenerator
- @src/Resources/AGENTS.md - Generation during onDependenciesLoaded()
- @src/Vulkan/AGENTS.md - SPIR-V compilation and pipelines
