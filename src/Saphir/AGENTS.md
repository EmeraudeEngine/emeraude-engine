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
- `LightGenerator.cpp/.hpp` - Lighting code generation (PerFragment, PerVertex)
- `Program.cpp/.hpp` - Shader program (shaders + pipeline layout)
- `ShaderManager.cpp/.hpp` - ShaderModule cache and compilation

## Quality Setting Architecture

The `EnableHighQualityKey` setting controls shader quality (per-fragment vs per-vertex lighting, normal mapping, etc.).

### Single Read Pattern
The setting is read **once** in `SceneRendering` constructor and passed to `LightGenerator`:

```cpp
// SceneRendering.hpp:68 - Single read point
m_lightGenerator{settings, renderPassType, settings.getOrSetDefault<bool>(EnableHighQualityKey, DefaultEnableHighQuality)}

// SceneRendering.hpp:71 - Reuses value from LightGenerator
if ( m_lightGenerator.highQualityEnabled() ) {
    this->enableFlag(HighQualityEnabled);
}
```

**Why this pattern:**
- Avoids double reading of the same setting
- `LightGenerator` stores the value for use in `generateAmbientFragmentShader()` (which doesn't have access to the generator)
- `Generator::Abstract::highQualityEnabled()` is used elsewhere in shader generation code

### High Quality Effects
When enabled (`EnableHighQualityKey = true`):
- Per-fragment lighting (Phong-Blinn or PBR Cook-Torrance)
- Normal mapping support (if geometry provides tangent space)
- Per-fragment reflection/refraction with Fresnel

When disabled:
- Per-vertex lighting (Gouraud shading)
- No normal mapping
- Simplified reflection/refraction

### Per-Vertex Lighting Shader Input Constraint

> [!CRITICAL]
> **GLSL shader inputs are READ-ONLY in fragment shaders!**
>
> In per-vertex (Gouraud) lighting, `diffuseFactor` and `specularFactor` are computed in the vertex shader and passed to the fragment shader via an interface block (`LightBlock`). These are shader inputs and CANNOT be modified.

**Problem scenario (caused shader compilation error):**
```glsl
// WRONG - trying to modify shader input
svLight.diffuseFactor *= shadowFactor;  // ERROR: l-value required
```

**Solution:**
Create local copies of the shader inputs before modification:
```glsl
// CORRECT - create local copies
float diffuseFactor = svLight.diffuseFactor;
float specularFactor = svLight.specularFactor;

// Now safe to modify
diffuseFactor *= shadowFactor;
specularFactor *= shadowFactor;
```

**Code reference:** `LightGenerator.PerVertex.cpp:generateGouraudFragmentShader()` - Local copies created before shadow factor multiplication

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

## Cubemap Rendering Mode (Multiview)

When rendering to a cubemap (e.g., environment probes, reflection captures), the shader system operates differently:

### Matrix Sources by Render Mode

| Matrix | Standard Mode | Cubemap Mode |
|--------|---------------|--------------|
| Projection | Push constant (MVP) or UBO | UBO (shared for all faces) |
| View | Push constant (MVP) or UBO | UBO indexed by `gl_ViewIndex` |
| Model | Push constant (MVP) | Push constant (Model only) |

### Push Constant Declaration

The push constant structure changes based on render target type. See: `Generator/Abstract.cpp:declareMatrixPushConstantBlock()`

```cpp
// Standard mode (non-cubemap, non-instanced, no advanced matrices)
layout(push_constant) uniform Matrices {
    mat4 modelViewProjectionMatrix;  // Pre-combined MVP
} pcMatrices;

// Cubemap mode (non-instanced)
layout(push_constant) uniform Matrices {
    mat4 modelMatrix;  // Model only - View/Projection from UBO
} pcMatrices;
```

### View Matrix Access Pattern

Code that needs the view matrix must check the render mode:

```cpp
// In generator code
const auto viewMatrixSource = vertexShader.isCubemapModeEnabled() ?
    ViewUB(UniformBlock::Component::ViewMatrix, true) :    // UBO: ubView.instance[gl_ViewIndex].viewMatrix
    MatrixPC(PushConstant::Component::ViewMatrix);         // Push constant: pcMatrices.viewMatrix
```

### Files Implementing Cubemap Support

- `Generator/Abstract.cpp:declareMatrixPushConstantBlock()` - Push constant declaration
- `VertexShader.cpp:prepareModelViewMatrix()` - ModelView matrix computation
- `VertexShader.cpp:prepareModelViewProjectionMatrix()` - MVP computation
- `VertexShader.cpp:prepareSpriteModelMatrix()` - Billboard sprite support
- `LightGenerator.PerFragment.cpp` - Light direction/position in view space
- `LightGenerator.PerFragment.NormalMap.cpp` - Normal mapping light calculations
- `LightGenerator.PerVertex.cpp` - Per-vertex (Gouraud) lighting
- `LightGenerator.PBR.cpp` - PBR lighting calculations

### CPU-Side Matrix Push (Graphics Layer)

The CPU code must match the shader expectations. See: `Graphics/RenderableInstance/Unique.cpp:pushMatricesForRendering()`

```cpp
if ( passContext.isCubemap ) {
    // Push only model matrix (View/Projection in UBO)
    vkCmdPushConstants(..., MatrixBytes, modelMatrix.data());
} else if ( pushContext.useAdvancedMatrices ) {
    // Push View + Model separately
    vkCmdPushConstants(..., MatrixBytes * 2, buffer.data());
} else {
    // Push combined MVP
    vkCmdPushConstants(..., MatrixBytes, modelViewProjectionMatrix.data());
}
```

### Common Pitfall

> [!WARNING]
> When adding code that uses `MatrixPC(ViewMatrix)`, always check if cubemap mode requires using `ViewUB(ViewMatrix, true)` instead. Failure to do so causes shader compilation errors: `'viewMatrix' : no such field in structure 'pcMatrices'`

## Shadow Map Code Generation

Shadow map sampling code is generated in `LightGenerator.ShadowMap.cpp`. The code is separated by light type and PCF mode.

### Generation Functions

| Function | Purpose |
|----------|---------|
| `generate2DShadowMapCode()` | Non-PCF 2D shadow (directional, spot) |
| `generate2DShadowMapPCFCode()` | PCF-enabled 2D shadows |
| `generate3DShadowMapCode()` | Non-PCF cubemap shadow (point light) |
| `generate3DShadowMapPCFCode()` | PCF-enabled cubemap shadows |
| `generateCSMShadowMapCode()` | Cascaded Shadow Maps |

### PCF Methods

The `PCFMethod` enum controls soft shadow sampling:

| Method | Description |
|--------|-------------|
| `Grid` | Regular grid pattern, (2n+1)² samples |
| `VogelDisk` | Vogel spiral disk, even distribution |
| `PoissonDisk` | Pre-computed Poisson disk |
| `OptimizedGather` | 4-tap `textureGather` optimization |

**Code references:**
- `LightGenerator.ShadowMap.cpp` - All shadow map code generation
- `LightGenerator.hpp:PCFMethod` - PCF method enum
- `LightGenerator.hpp:m_PCFMethod` - Active PCF method

### Vertex Shader Shadow Output

`generateVertexShaderShadowMapCode()` outputs position data for fragment shadow sampling:

- **2D shadows (directional/spot):** `PositionLightSpace` (vec4) - fragment position in light clip space
- **Cubemap shadows (point):** `DirectionWorldSpace` (vec4) - direction from light to fragment

### Settings Integration

Shadow mapping settings are read during generator construction:

| Setting | Member | Effect |
|---------|--------|--------|
| `GraphicsShadowMappingPCFEnabledKey` | `m_usePCF` | Enable/disable PCF |
| `GraphicsShadowMappingPCFMethodKey` | `m_PCFMethod` | PCF sampling method |
| `GraphicsShadowMappingPCFSampleKey` | `m_PCFSample` | Grid sample count |

See [`docs/shadow-mapping.md`](../../docs/shadow-mapping.md) for complete shadow mapping architecture.

## Detailed Documentation

For complete Saphir system architecture:
- @docs/saphir-shader-system.md - Parametric generation, compatibility, cache
- @docs/shadow-mapping.md - Shadow mapping, PCF methods, global controls

Related systems:
- @src/Graphics/AGENTS.md - Material and Geometry for 3D generation
- @src/Overlay/AGENTS.md - 2D pipeline via OverlayGenerator
- @src/Resources/AGENTS.md - Generation during onDependenciesLoaded()
- @src/Vulkan/AGENTS.md - SPIR-V compilation and pipelines
