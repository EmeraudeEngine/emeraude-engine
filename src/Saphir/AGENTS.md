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
4. **GizmoRendering**: Editor gizmos (vertex color, no lighting, depth test OFF). See `Scenes/Editor/AGENTS.md`

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
- `LightGenerator.cpp/.hpp` - Lighting code generation (PerFragment, PerVertex, PBR, NormalMap, color projection)
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
- Parallax Occlusion Mapping (if material has Height component and POM iterations > 0)

When disabled:
- Per-vertex lighting (Gouraud shading)
- No normal mapping
- Simplified reflection/refraction
- POM completely disabled (forced to 0 iterations at source)

### POM Iterations Setting

`POMIterationsKey` (`Core/Graphics/Shader/POMIterations`, default: 16) controls POM ray-marching quality.

**Quality cascade** (centralized in `SceneRendering` constructor):
```cpp
this->setPOMIterations(this->highQualityEnabled()
    ? settings.getOrSetDefault<int>(POMIterationsKey, DefaultPOMIterations)
    : 0);
```

| Value | Effect |
|-------|--------|
| `0` | POM completely disabled — no POM code in shaders, no extra vertex outputs |
| `4-8` | Low quality (fast, visible stepping artifacts) |
| `16` | Default (good balance of quality/performance) |
| `32-64` | High quality (smooth, more GPU load per fragment) |

**Key design**: When `pomIterations() == 0`, materials behave identically to having no Height component — `textCoords()` returns original UVs, no POM GLSL is generated, no extra vertex shader outputs.

**Code references:**
- `SettingKeys.hpp:POMIterationsKey` — Setting key definition
- `Generator/Abstract.hpp:setPOMIterations()` — Clamps to [4, 64] or 0 (special disable value)
- `Generator/SceneRendering.hpp` constructor — Quality cascade logic
- `StandardResource.cpp:m_pomGenerationActive` — Fragment shader conditional
- `PBRResource.cpp:m_pomGenerationActive` — Fragment shader conditional

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

## PBR Advanced Material Features

The PBR Cook-Torrance BRDF supports several advanced material layers. Each feature is **compile-time conditional** — when a parameter is at its default (off) value, no extra shader code is generated.

### Clear Coat

Adds a second specular lobe on top of the base material (car paint, varnished wood).

| Parameter | UBO Offset | Range | Effect |
|-----------|-----------|-------|--------|
| `clearCoatFactor` | 16 | 0-1 | Coat intensity (0 = none) |
| `clearCoatRoughness` | 17 | 0-1 | Coat roughness (0 = mirror) |
| `clearCoatNormalScale` | 49 | 0+ (1.0) | Clear coat normal map intensity |

- Uses a separate GGX NDF + Smith G with its own roughness
- Energy conservation: base specular is scaled by `(1 - clearCoatFactor)`
- **Clear coat normal map** (KHR_materials_clearcoat): Optional dedicated normal map for the clear coat layer, simulating micro-imperfections (orange peel, swirl marks) independent of the base surface. When no clear coat normal is provided, the coat uses the base surface normal (`Ncc = N`).
- **Fragment-local TBN**: The clear coat normal is transformed using a tangent frame derived from the fragment normal N (`cross(N, up)`), NOT from the vertex TBN matrix. This avoids dependency on base normal mapping being active.
- **Files**: `LightGenerator.PBR.cpp` (per-light), `LightGenerator.cpp` (ambient IBL), `PBRResource.cpp` (texture sampling + UBO)

### Subsurface Scattering (SSS)

Simulates light scattering beneath the surface (skin, wax, leaves, marble).

| Parameter | UBO Offset | Range | Effect |
|-----------|-----------|-------|--------|
| `subsurfaceIntensity` | 18 | 0-1 | Master SSS weight + wrap amount |
| `subsurfaceRadius` | 19 | 0+ | Scatter distance for thickness falloff |
| `subsurfaceColor` | 20-23 | vec4 | Color of scattered light |
| Thickness map | texture | 0-1 | Optional per-pixel thickness |

**Three techniques combined:**
1. **Wrap diffuse** — Softens shadow terminator: `NdotLWrap = (NdotL + wrap) / (1 + wrap)`
2. **Back-lit transmittance** — Light through thin areas: `exp(-thickness / radius) * NdotLBack`
3. **Ambient SSS** — Tinted ambient in shadow areas

> [!WARNING]
> **SSS wrap value clamped to 0.99**: `sssWrap = min(sssIntensity, 0.99)`. When `sssIntensity = 1.0`, `smoothstep(sssWrap, 1.0, x)` requires `edge0 < edge1`. With `sssWrap = 1.0`, this becomes `smoothstep(1.0, 1.0, x)` — **undefined behavior in GLSL** (produces NaN on some GPUs, causing flickering/darkening). See: `LightGenerator.PBR.cpp` lines 702, 716.

- **Files**: `LightGenerator.PBR.cpp` (per-light wrap + transmittance), `LightGenerator.cpp` (ambient SSS)

### Sheen

Adds a soft edge highlight for fabric-like materials (velvet, silk, wool).

| Parameter | UBO Offset | Range | Effect |
|-----------|-----------|-------|--------|
| `sheenColor` | 24-27 | vec4 | Sheen color tint (black = off) |
| `sheenRoughness` | 28 | 0-1 | 0 = satin, 1 = wool |

- Uses Charlie distribution (sin²-based NDF) for soft retroreflection
- Energy conservation via DFG approximation: `sheenScaling = 1 - max(sheenColor) * (0.157 * sheenRoughness + 0.04)`
- Applied to both per-light and ambient passes
- **Files**: `LightGenerator.PBR.cpp` (per-light), `LightGenerator.cpp` (ambient)

### Anisotropic Specular

Stretches specular highlights along a direction (brushed metal, hair, vinyl records).

| Parameter | UBO Offset | Range | Effect |
|-----------|-----------|-------|--------|
| `anisotropy` | 29 | -1 to 1 | Stretch strength (0 = isotropic) |
| `anisotropyRotation` | 30 | 0-1 | Direction rotation (maps to 0-2π) |

**BRDF functions (compile-time conditional):**
- `distributionGGXAniso(T, B, N, H, at, ab)` — Anisotropic GGX NDF
- `visibilityAniso(T, B, N, V, L, at, ab)` — Smith height-correlated anisotropic visibility (Heitz 2014)

**Key implementation details:**
- **Roughness squaring**: `alphaRoughness = roughness²`, then `at = alpha * (1 + aniso)`, `ab = alpha * (1 - aniso)`. Must match engine's standard GGX convention.
- **Procedural tangent frame**: T/B derived from N in fragment shader (`cross(N, up)`), NOT from mesh TBN. This avoids triangle-seam artifacts at UV discontinuities.
- **Normal mapping compatible**: Procedural frame is rebuilt from the perturbed N, so anisotropy correctly follows normal-mapped surfaces.
- **Files**: `LightGenerator.PBR.cpp` (BRDF functions + per-light), vertex shader TBN only for normal mapping

> [!WARNING]
> **Anisotropy tangent frame**: Do NOT use `ViewTBNMatrix` for anisotropy direction. Per-vertex tangent vectors from UV-mapped meshes have discontinuities at UV seams, causing visible triangle edges in specular highlights. Always compute T/B procedurally from the fragment normal.

### Feature Combinations

Features can be combined freely. The shader generator handles all combinations:
- Clear Coat + Subsurface, Clear Coat + Anisotropy, etc.
- Sheen is typically used alone (fabric materials are rarely metallic/clear-coated)
- When `subsurfaceIntensity = 0`, `sheenColor = black`, `anisotropy = 0`, `clearCoatFactor = 0`: no extra code generated

### Clear Coat Normal — Fragment-Local TBN Pattern

> [!WARNING]
> **Do NOT use `ViewTBNMatrix` for clear coat normal transformation.** The clear coat normal map must use a fragment-local tangent frame derived from the surface normal N, identical to the anisotropy pattern. Using the vertex TBN matrix (`ViewTBNMatrix`) causes GPU hangs when normal mapping is not active on the base material, and GLSL compilation errors (`svViewTBNMatrix` undeclared) when the TBN synthesis is conditional on `m_useNormalMapping`.
>
> **Pattern:**
> ```glsl
> const vec3 ccT = abs(N.y) < 0.999 ? normalize(cross(N, vec3(0.0, 1.0, 0.0))) : normalize(cross(N, vec3(1.0, 0.0, 0.0)));
> const vec3 ccB = cross(N, ccT);
> const vec3 Ncc = normalize(ccT * SurfaceClearCoatNormal.x + ccB * SurfaceClearCoatNormal.y + N * SurfaceClearCoatNormal.z);
> ```
>
> **Code reference:** `LightGenerator.PBR.cpp` — Ncc blocks in both CC+SSS and CC-only paths

## Color Projection Code Generation

Color projection allows lights to project a texture onto surfaces (gobo/light mask effect). It is **independent** of shadow mapping — a light can project colors without a shadow map.

### RenderPassType Drives Generation

The `RenderPassType` enum encodes shadow + color projection combinations. The `LightGenerator` switch statements use fallthrough logic to set `enableShadowMap` and `enableColorProjection` flags:

```cpp
case DirectionalLightPassFullCSM: enableColorProjection = true; [[fallthrough]];
case DirectionalLightPassCSM: enableShadowMap = true; lightType = Directional; break;

case DirectionalLightPassFull: enableColorProjection = true; [[fallthrough]];
case DirectionalLightPassShadowMap: enableShadowMap = true; [[fallthrough]];
case DirectionalLightPassColorMap: if (!enableShadowMap) { enableColorProjection = true; } [[fallthrough]];
case DirectionalLightPass: lightType = Directional; break;
```

### Shader Program Variants

Each `RenderPassType` generates a **distinct shader program** with only the needed code:

| Pass Suffix | Per-Light Samplers | Bindless Arrays | Texture Samples |
|-------------|-------------------|-----------------|-----------------|
| (base) | 0 | 0 | 0 |
| `ShadowMap`/`CSM` | 1 (shadow) | 0 | 1 |
| `ColorMap` | 0 | 1 (2D or Cube) | 1 |
| `Full`/`FullCSM` | 1 (shadow) | 1 (2D or Cube) | 2 |

When color projection is not active, `projectionColor = vec3(1.0)` is hardcoded — no texture sample, and the SPIR-V compiler optimizes out the multiply.

### Bindless Color Projection Sampling

Color projection textures are accessed via the global `BindlessTextureManager` descriptor set, **not** via per-light descriptor sets. The light UBO carries a `uint` bindless index (`ColorProjectionIndex`) encoded as `std::bit_cast<float>(uint32_t)`:

```glsl
// In fragment shader (generated by LightGenerator):
uint cpIdx = floatBitsToUint(uLight.ColorProjectionIndex);
if ( cpIdx != 0xFFFFFFFFu ) {
    // 2D lights (directional, spot):
    projectionColor = texture(uBindlessTextures2D[nonuniformEXT(cpIdx)], projCoords.xy).rgb;
    // Point lights (cubemap):
    projectionColor = texture(uBindlessTexturesCube[nonuniformEXT(cpIdx)], direction.xyz).rgb;
}
```

The bindless array declarations use `Declaration::Sampler::UnboundedArray` and are placed on the `PerBindless` set. The `GL_EXT_nonuniform_qualifier` extension is required.

**Why bindless?** Per-light descriptor sets use `UNIFORM_BUFFER_DYNAMIC` (binding 0), which does not support `UPDATE_AFTER_BIND_BIT`. This makes deferred texture writes unsafe with frames-in-flight. The bindless set uses `UPDATE_AFTER_BIND_BIT` + `PARTIALLY_BOUND_BIT`, allowing textures to be registered asynchronously after resource loading completes.

### ScaleBiasMatrix UV Caveat

> [!WARNING]
> **Do NOT apply `* 0.5 + 0.5` to color projection UVs!**
>
> `ScaleBiasMatrix` is pre-multiplied into `ViewProjectionMatrix` in the UBO. Shadow maps use `textureProj()` which handles this automatically. Color projection does manual perspective divide (`projCoords = .xyz / .w`), and the UVs are already in [0,1] range.
>
> Adding `* 0.5 + 0.5` causes double-bias (pattern offset). See: `docs/shadow-mapping.md` Color Projection section.

### Helper Functions

| Function | Location | Purpose |
|----------|----------|---------|
| `renderPassUsesColorProjection()` | `Graphics/Types.hpp` | True for `*ColorMap`, `*Full`, `*FullCSM` |
| `renderPassUsesShadowMap()` | `Graphics/Types.hpp` | True for `*ShadowMap`, `*CSM`, `*Full`, `*FullCSM` |
| `renderPassUsesCSM()` | `Graphics/Types.hpp` | True for `*CSM`, `*FullCSM` |

**Code references:**
- `LightGenerator.cpp:generateVertexShaderCode()` — Switch with fallthrough for enableColorProjection
- `LightGenerator.cpp:generateFragmentShaderCode()` — Switch with fallthrough for enableColorProjection
- `LightGenerator.PerFragment.cpp` — Bindless color projection sampling (all 4 shading variants)
- `Graphics/BindlessTextureManager.hpp` — `Texture2DBinding` (1), `TextureCubeBinding` (3) constants
- `Graphics/Types.hpp:RenderPassType` — Enum definition (16 values)
- `Scenes/Component/AbstractLightEmitter.cpp:registerColorProjectionInBindless()` — Async texture registration

## SSBO Memory Qualifiers

`ShaderStorageBlock` supports GLSL memory access qualifiers via `setAccessQualifier()`:

| Qualifier | GLSL Output | Use Case |
|-----------|-------------|----------|
| `AccessQualifier::None` | `buffer Name { ... }` | Read-write access (default) |
| `AccessQualifier::ReadOnly` | `readonly buffer Name { ... }` | GPU reads only (e.g., bone matrices, per-draw data) |
| `AccessQualifier::WriteOnly` | `writeonly buffer Name { ... }` | GPU writes only (e.g., compute output buffers) |

> [!CRITICAL]
> **SSBOs in vertex/geometry/tessellation shaders MUST be `readonly` unless `vertexPipelineStoresAndAtomics`
> is enabled.** Without the `readonly` qualifier, Vulkan requires this device feature for any storage buffer
> accessed in these stages. Omitting it causes `VUID-RuntimeSpirv-NonWritable-06341` validation errors and
> pipeline creation failure on many GPUs.
>
> **Rule of thumb:** If the shader only reads from an SSBO, always mark it `ReadOnly`. This is both
> semantically correct and avoids unnecessary feature requirements.

```cpp
Declaration::ShaderStorageBlock ssbo{setIndex, 0, Declaration::MemoryLayout::Std430, "MyData", "ubMyData"};
ssbo.setAccessQualifier(Declaration::AccessQualifier::ReadOnly);
ssbo.addMember(Declaration::VariableType::Matrix4, "matrices[]");
vertexShader->declare(ssbo);
```

**Code references:**
- `Declaration/ShaderStorageBlock.hpp` — `setAccessQualifier()` / `accessQualifier()`
- `Declaration/Types.hpp` — `AccessQualifier` enum
- `Generator/SceneRendering.cpp` — Skinning SSBO (ReadOnly)
- `Generator/ShadowCasting.cpp` — Skinning SSBO (ReadOnly)

## MDI Shader Generation

When `IsMultiDrawIndirectEnabled` is set on the generator, the shader system produces MDI-specific vertex shader variants:

### Push Constant Layout Change

MDI mode replaces the model matrix push constant with a BDA address pair:
```glsl
layout(push_constant) uniform Matrices {
    uint perDrawAddrLo;   // Low 32 bits of SSBO device address
    uint perDrawAddrHi;   // High 32 bits
    mat4 viewProjectionMatrix;
    float frameIndex;
} pcMatrices;
```

### BDA Buffer Reference Declaration

The vertex shader declares a `buffer_reference` struct matching `PerDrawData` (std430):
```glsl
layout(buffer_reference, std430) readonly buffer PerDrawDataRef {
    mat4 modelMatrix;
    uint frameIndex;
    uint _padding[3];
};
```

### Model Matrix Access via gl_DrawID

All synthesis methods (`synthesizeVertexPositionInWorldSpace`, `prepareModelViewMatrix`, `prepareModelViewProjectionMatrix`, `synthesizeVertexVectorInWorldSpace`) check `isMDIEnabled()` first:
```glsl
const uint64_t addr = packUint2x32(uvec2(pcMatrices.perDrawAddrLo, pcMatrices.perDrawAddrHi));
const mat4 svMDIModelMatrix = mat4(PerDrawDataRef(addr)[gl_DrawID].modelMatrix);
```

### Extension Registration Order (CRITICAL)

> [!WARNING]
> **Extensions MUST be registered in `VertexShader::enableMDI()`, NOT in `prepareMDIModelMatrix()` or
> `onSourceCodeGeneration()`.** The code generation flow is: `generateHeaders()` (emits `#extension`)
> → `onSourceCodeGeneration()` (emits BDA struct). Extensions registered after `generateHeaders()`
> appear AFTER the struct usage → `GL_EXT_buffer_reference: required extension not requested` error.

Required extensions:
- `GL_EXT_buffer_reference` — BDA buffer references
- `GL_EXT_buffer_reference2` — Array indexing on buffer references (`[gl_DrawID]`)
- `GL_ARB_gpu_shader_int64` — `uint64_t` type + `packUint2x32()`
- `GL_ARB_shader_draw_parameters` — `gl_DrawID` built-in

### Objects Excluded from MDI Shader Generation

MDI shaders are NOT generated for objects with special rendering requirements:
- Sprites (`isSprite()`) — need billboard model matrix (`getSpriteModelMatrix()`)
- InfinityView — need rotation-only view matrix
- Depth-test/write-disabled — order-dependent rendering
- The check happens in `Scene.rendering.cpp` before `getReadyForMDI()` is called.

**Code references:**
- `Generator/Abstract.hpp:IsMultiDrawIndirectEnabled` — Generator flag
- `Generator/Abstract.cpp:declareMatrixPushConstantBlock()` — MDI push constant block
- `VertexShader.hpp:enableMDI()` — Extension registration + `m_MDIEnabled` flag
- `VertexShader.cpp:prepareMDIModelMatrix()` — BDA reconstruction + SSBO access
- `VertexShader.cpp:onSourceCodeGeneration()` — `PerDrawDataRef` struct declaration
- `Program.hpp:wasMDIEnabled()` — Query MDI state from program
- `RenderableInstance/Abstract.cpp:getReadyForMDI()` — MDI program generation
- `Renderable/ProgramCacheKey.hpp:isMDIEnabled` — Cache key discrimination

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

### Vertex Shader Projection Output

`generateVertexShaderShadowMapCode()` outputs position data for fragment shadow sampling AND color projection:

- **2D (directional/spot):** `PositionLightSpace` (vec4) - fragment position in light clip space (used by shadow maps AND color projection)
- **Cubemap (point):** `DirectionWorldSpace` (vec4) - direction from light to fragment (used by cubemap shadow AND cubemap color projection)

These outputs are generated whenever shadow maps OR color projection is enabled (not only for shadow maps).

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
- @docs/shadow-mapping.md - Shadow mapping, PCF methods, global controls, color projection

Related systems:
- @src/Graphics/AGENTS.md - Material and Geometry for 3D generation
- @src/Overlay/AGENTS.md - 2D pipeline via OverlayGenerator
- @src/Resources/AGENTS.md - Generation during onDependenciesLoaded()
- @src/Vulkan/AGENTS.md - SPIR-V compilation and pipelines
