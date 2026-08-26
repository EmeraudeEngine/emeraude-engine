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

### 3-Level Cache System (IN-MEMORY, one process run)

⚠️ These three levels are process-lifetime hash maps. They are **not** the three ON-DISK cache
stages (source dump / SPIR-V binary cache / `VkPipelineCache`) — see
"The three shader-cache stages" at the end of this file.

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

## ShaderManager Include Contract (compile-time)

> [!IMPORTANT]
> **`ShaderManager.hpp` must never expose glslang or `Program`.** Both leaks were
> removed on 2026-07-27; re-introducing either one silently multiplies compile time
> across the whole engine.

`ShaderManager.hpp` is included by 21 translation units, one of which is
`Graphics/Renderer.hpp` — itself included by 81 more (7 of them headers that
repropagate it). Anything reachable from `ShaderManager.hpp` is therefore paid for
by most of the engine. Two rules keep it cheap:

1. **`Saphir::Program` is forward-declared, never included.** `getShaderModules()`
   only takes `const std::shared_ptr< Program > &`, which needs a declaration, not a
   definition. `Program.hpp` transitively drags in every shader class, `SetIndexes`,
   `Graphics/VertexBufferFormatManager.hpp` and `Vulkan/GraphicsPipeline.hpp`
   (~70 headers / 25.7k lines). Consumers that need the complete type include
   `Saphir/Program.hpp` themselves.
2. **The glslang state lives behind a pimpl.** `struct ShaderManager::GLSLangContext`
   (declared in the header, defined in `ShaderManager.cpp`) holds `TBuiltInResource`,
   `DirStackFileIncluder`, `EProfile`, `EShMessages` and the target version. Keeping
   them as by-value members forced `<glslang/Public/ShaderLang.h>` into every
   consumer. The Saphir→GLSLang shader-type conversion is a **file-local free
   function** in the `.cpp` for the same reason: its `EShLanguage` return type alone
   pulled the glslang public header in.

Consequences to respect when editing this class:

- The constructor and destructor are **defined out-of-line** in `ShaderManager.cpp`.
  This is mandatory: `std::unique_ptr< GLSLangContext >` needs the complete type for
  its deleter. Same pattern as `Graphics::Renderer` — see
  [`docs/windows-export-api.md`](../../docs/windows-export-api.md) § "exported pimpl".
- Declaring the destructor suppresses the implicit move constructor; `ShaderManager`
  is a by-value member of `Renderer` and is direct-initialized, so this is fine — do
  not add a move that would require the complete type in the header.
- Measured effect: `ShaderManager.hpp` went from 75 project headers / 26 570 lines to
  **10 / 2 506**; `Graphics/Renderer.hpp` from 127 / 47 064 to **87 / 38 624**.
- The PCH masks missing includes. After touching this header, verify a TU that used
  to rely on the transitive `Program.hpp` still compiles with
  `-DEMERAUDE_ENABLE_PCH=Off`.

## Quality Setting Architecture

**There is no shader-quality SETTING any more** (Aug 2026). The engine has ONE lighting model — Cook-Torrance, shaded per fragment — and the quality tier is a RENDERING decision carried by `Saphir::Generator::Abstract`'s `HighQualityEnabled` flag. It is meant to be driven by rendering DISTANCE (a distant surface takes the cheap branches: simpler transmission, no Fresnel-gated reflection, no parallax). ⚠️ Nothing drives it down yet — every program is generated at full quality; the flag is the hook, and since it is part of the program cache key a future distance switch produces its own variants for free.

### Single Read Pattern
The setting is read **once** in `SceneRendering` constructor and passed to `LightGenerator`:

```cpp
// SceneRendering.hpp:68 - Single read point
m_lightGenerator{settings, renderPassType}   // quality is decided by the renderer, not by a setting

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
When the renderer asks for the high tier:
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

1. **`StandardResource::generateFragmentShaderCode()`** declares the reflection frame
   (`reflectionNormal`, `reflectionI`, at `Location::Top`, reused between the reflection and
   refraction blocks) and samples the reflection/refraction colours — **high quality only**.
2. **`LightGenerator::generateAmbientFragmentShader()`** detects both components
   (`m_usePBRMode && m_useReflection && m_useRefraction && highQualityEnabled()`) and generates
   the `fresnelFactor` itself, with the Schlick approximation:
   ```glsl
   const float NdotV = max(dot(reflectionNormal, -reflectionI), 0.0);
   const float fresnelFactor = 0.04 + (1.0 - 0.04) * pow(1.0 - NdotV, 5.0);
   ```
3. The blend `mix(refractedColor, reflectedColor, fresnelFactor)` is added in that **same
   ambient pass** — IBL is the whole contribution of glass. The light passes do NOT re-mix
   reflection and refraction.

### Important Notes

- `fresnelFactor` is **only generated when BOTH** reflection AND refraction are present, in
  high quality; using it anywhere else causes an "undefined variable" shader error
- **F0 is the fixed 0.04 dielectric value**, NOT derived from `ubMaterial.refractionIOR` — the
  material IOR drives the refraction vector (`eta = 1.0 / IOR`), not this Fresnel term
- The `amount` parameters are artistic weights on each leg (neutral `1.0` = fully
  Fresnel-controlled), not a blend against the base colour
- ~~`LightGenerator::generateFinalFragmentOutput()`~~ — DELETED (Aug 2026) along with the whole Blinn-Phong machinery: it had no caller left once the Gouraud and Phong generators were removed.

- ⚠️ (historical) `generateFinalFragmentOutput()` used to carry `!m_usePBRMode`
  reflection/refraction branches that CONSUME a `fresnelFactor` declared by the material. Since
  the material merge, the only material declaring reflection/refraction is `StandardResource`,
  which always calls `enablePBRMode()` — those branches are **unreachable legacy**. Do not
  revive them expecting a material to publish `fresnelFactor`.
- Files: `Graphics/Material/StandardResource.cpp:generateFragmentShaderCode()`,
  `LightGenerator.cpp:generateAmbientFragmentShader()`

## IBL Ambient Pass (Jul 2026, IBL lot 3)

`LightGenerator::generateAmbientFragmentShader()` is where ALL image-based lighting lands
(contract: IBL only in the ambient pass, never in light passes). When the program carries
the bindless set (`generator.bindlessTexturesEnabled()` — enabled for every LIT instance
since lot 3, see `RenderableInstance/Abstract.cpp` gating), the pass reads the reserved
slots: cube 1 = baked irradiance (E/π), cube 2 = GGX-prefiltered environment, 2D 3 =
split-sum BRDF LUT.

- **Diffuse irradiance** (every branch): `baseColor × texture(irradiance, N) × envLuminance`
  with the **RAW** world normal `N` (Y-up cubemap convention — the former `(N.x, -N.y, N.z)`
  compensation belonged to the Y-down era and is gone) — the GEOMETRIC world normal
  (`NormalWorldSpace`, synthesized in the ambient-pass vertex branch when bindless is on);
  a 32² cosine-convolved cubemap carries no frequency a normal map could reveal.
  ⚠️ The tint is ALWAYS the raw base color (albedo/diffuse), never the 1/π photometric
  `surfaceColor` (the cubemap already stores E/π) and never the Phong ambient component
  (an artistic constant-ambient hack — a light-grey ka under a 17k lx sky washes
  everything out; it stays on the legacy scalar path).
- **PBR HQ specular** (reflective branch): split-sum reconstruction
  `FssEss = F0·lut.x + lut.y` + **Fdez-Agüera multi-scatter** (`FmsEms`, no extra
  resource) + energy-conserving `kD` on the irradiance.
- **Scalar coexistence**: the legacy `ambientLightColor × intensity` term REMAINS in the
  shader — the scene zeroes the pushed intensity when the sky drives the ambient
  (`Scene::refreshAmbientLightProperties`), so scalar-lit scenes (RTGI demos, manual
  ambient) are untouched. The irradiance SLOT publication itself is gated by the
  `applyAmbient` contract (`Scene::updateEnvironmentIBL`), parking on the default BLACK
  cubemap otherwise — the IBL term then contributes exactly nothing.
- **Baked AO** now modulates ONLY the diffuse ambient terms (`aoFactor` at each addition
  site) — the old global multiply darkened the emissive and the specular IBL.
- The reflection sample itself (`SurfaceReflectionColor`) is roughness-driven since lot 3:
  `textureLod(prefiltered, R, roughness × (mips-1))` in the `StandardResource` bindless
  generator, mip 0 being an exact environment copy for mirrors. The transmission reads the
  prefiltered slot too. (`LightGenerator::roughnessShaderExpression()` still falls back to
  `sqrt(2/(shininess+2))` for a material that only declared a shininess — `BasicResource`,
  which carries no reflection, so that branch never drives a prefiltered fetch.)

## Legacy (Blinn-Phong) Specular — Energy Normalisation (Jul 2026)

The non-PBR specular is a **normalised BRDF times an irradiance**, exactly like its diffuse sibling.
It is now the `BasicResource` tier alone: since the material merge, `declareSurfaceSpecular()` has a
single caller, and the legacy `StandardResource` this was written for no longer exists.
Emitted at three sites, all computing the same expression in their own space:

| Generator | Space | Normal used |
|---|---|---|
| `LightGenerator.PerVertex.cpp` | view (vertex shader, interpolated result) | `NormalViewSpace` |
| `LightGenerator.PerFragment.cpp` | view | `twoSidedN` |
| `LightGenerator.PerFragment.NormalMap.cpp` | **tangent** | `m_surfaceNormalVector` |

```glsl
const float specularExponent = max(<Material.Shininess>, 1.0);
SpecularFactor = pow(max(dot(N, H), 0.0), specularExponent)
               * ((specularExponent + 2.0) / 25.132741228718345)   /* (n + 2) / (8*pi) */
               * DiffuseFactor;                                     /* carries N.L * LightFactor */
```

Three things to keep straight:

1. **`(n + 2) / (8*pi)`, not `(n + 2) / (2*pi)`.** The latter belongs to the modified-Phong BRDF or to
   the Blinn NDF taken alone; using it is a factor of 4.
2. **The `N.L` arrives via `DiffuseFactor`, not `LightFactor`.** `DiffuseFactor` is already
   `N.L * LightFactor`, so the shadow/attenuation factor is applied exactly once. Multiplying by both
   would square it.
3. **`max(..., 1.0)` on the exponent** is a real floor, C++ API and manifest alike:
   `BasicResource` takes the manifest `Shininess` RAW as a Blinn-Phong exponent (default 200),
   with nothing keeping an authored value above 1. Without the floor, an exponent under 1 gives a
   lobe that never decays.

Why it matters photometrically: unnormalised, the term was `specularColor * illuminance * pow(...)`,
a raw multiple of the illuminance with no cosine — a 0.5 grey specular under a 50000 lx sun returned
22350 nits, five times the luminance of the sky above it. Now the diffuse and the specular of one
material are on the same scale, and both are commensurable with lights authored in lux/candela.
Full diagnosis and measurements: `docs/caution-points.md`, "The legacy specular was not
energy-normalised".

> [!WARNING]
> **The manifest key `Shininess` means two different things depending on the container.**
> `BasicResource` reads it as the raw exponent above; the merged `StandardResource` reads it as an
> authored **glossiness** `[0,1]` and converts it as `roughness = 1 - glossiness`. The old
> `StandardResource::specularExponentFromGlossiness()` bridge died with the legacy material — do
> not look for it, and never apply a conversion twice.

> [!NOTE]
> The **PBR low-quality** specular approximation in `LightGenerator.cpp` (`lqSpecPower`) is still
> unnormalised and still multiplies the raw illuminance. ⚠️ VOID since Aug 2026: `lqSpecPower`
> lived in `generateFinalFragmentOutput()`, written for the Gouraud path, and was deleted with it.

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
- **Files**: `LightGenerator.PBR.cpp` (per-light), `LightGenerator.cpp` (ambient IBL), `StandardResource.cpp` (texture sampling + UBO)

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

### Emission on the UNLIT path — `emissionMultiplier()` MULTIPLIES, it does not ADD

Emission is normally applied by `LightGenerator`, which **never runs for an unlit material**.
So `SimplePass` (the skybox, unlit sprites) used to emit a bare `svOutputFragment = SurfaceColor;`
— the material-properties block carried `emissiveStrength` but nothing consumed it, which is why a
skybox authored at 8000 nits rendered at its raw texel value. Found only by dumping the generated
GLSL; the C++ setters all looked correct.

`Material::Interface::emissionMultiplier()` closes it, and the asymmetry is deliberate:

- **UNLIT**: there is no lighting to add emission on top of, so the surface colour **IS** the
  emitted radiance — the emission MULTIPLIES it (`SceneRendering.cpp`, `SimplePass` branch).
- **LIT**: `LightGenerator` keeps ADDING the emission to the shaded result.

Applying it in both would double-count. ⚠️ And it is deliberately **NOT** applied to the albedo
G-buffer attachment written from the same `fragmentColor()` expression: albedo is a reflectance in
[0,1], and pushing 8000 nits into it poisons every consumer that reads it — RTGI first.

### MRT normal output — the `N` declaration contract

`SceneRendering` writes the view-space perturbed normal to the G-buffer normal
attachment (`svOutputNormal`) for the **`AmbientPass`** (and the `SimplePass` output slot, which
is unlit and never reaches the light generator), using
`LightGenerator::finalNormalViewSpaceExpression()`. That helper returns the bare identifier
**`N`** whenever normal mapping is active — so `N` **must be declared** for the ambient pass:

- **`AmbientPass`** never reaches a light-pass generator (it returns right after
  `generateAmbientFragmentShader()`, which does *not* declare `N`). So `generateFragmentShaderCode()`
  declares `N` up-front, at the top of the function, for the ambient pass.
- Light passes either self-declare their own `N` (high-quality PBR, two-sided-flipped), shade in
  tangent space without a view-space `N` (Blinn-Phong with normal map), or compute lighting
  per-vertex (low-quality Gouraud).

> **History (Jul 2026)**: the `SimplePass` used to be REMAPPED by `checkRenderPassType()` to a
> light-pass type when the scene used the *static lighting* mode (a single light baked as GLSL
> literals). That whole mode was REMOVED — `SimplePass` is now strictly unlit (light set
> disabled or instance lighting disabled), `checkRenderPassType()` is gone, and the `N`
> declaration guard covers the `AmbientPass` only.

> [!CAUTION]
> **When you touch a lighting shader, always check BOTH quality levels
> (`Core/Graphics/Shader/EnableHighQuality`, default `false`).** High and low quality route
> through *different* generators (per-fragment PBR/Blinn-Phong vs per-vertex Gouraud), so a
> change that compiles in one can break the other — a variable declared by the high-quality
> fragment path (e.g. `N`, `ViewTBNMatrix`) may be entirely absent in the low-quality Gouraud
> path. Test with `EnableHighQuality` both `true` and `false`.
>
> This particular bug is also **latent until a post-process effect enables the normal G-buffer
> attachment**. A `SimplePass` material (static single light) with a normal map compiles fine
> with no post-process, then fails to compile (`'N' : undeclared identifier`, `redefinition`
> for high-quality PBR, or `'ViewTBNMatrix' : undeclared identifier` in low-quality Gouraud)
> the moment SSAO/SSR/RTR/RTGI turns the attachment on. If you add a shading family or a pass
> that writes `svOutputNormal`, keep the `N`-declaration guard in `generateFragmentShaderCode()`
> and the `ViewTBNMatrix` request in `generateVertexShaderCode()` in sync. See
> `docs/caution-points.md`.

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
    // Point lights (cubemap) — NEGATED, see the CAUTION below:
    projectionColor = texture(uBindlessTexturesCube[nonuniformEXT(cpIdx)], -DirectionWorldSpace.xyz).rgb;
}
```

> [!CAUTION]
> **A point-light gobo is sampled along `-DirectionWorldSpace`, never the raw vector.**
> `DirectionWorldSpace` is the **FRAGMENT → LIGHT** vector; a projection texture must be sampled
> along the direction the light **EMITS**, light → fragment. The two are antipodal, so the raw
> vector returns the **opposite cubemap face** — and it did, from the day the gobo was written
> (0.8.6, Feb 2026) until Aug 2026.
>
> Measured with `global-illumination --demo-options 0,1`, which projects the `AxisDebug` cubemap
> from the room's only omni light: the ceiling above the light read **MAGENTA (-Y)** and the floor
> **GREEN (+Y)** — exactly swapped. After negating, at the same two camera poses: ceiling green,
> floor magenta.
>
> ⚠️ **This is NOT a Y-up residual**, and it would have been misfiled as one. `git log -S` puts the
> line at 0.8.6 (2026-02-20) with no Y negation ever present, six months before the flip. Check the
> history before filing a sign error under the migration that happens to be nearby — the same
> mistake was made on SSR's camera-ward ray rejection the same week.
>
> ⚠️ The point-light SHADOW lookup takes the SAME vector and negates it inside
> `generate3DShadowMapCode()`, for the same reason. When that one was fixed, this site — six lines
> away in the same generator, fed by the same variable — was not checked. **A sign fix on a
> direction is a cue to audit every other consumer of that variable**, not just the one that
> produced the visible symptom.

The bindless array declarations use `Declaration::Sampler::UnboundedArray` and are placed on the `PerBindless` set. The `GL_EXT_nonuniform_qualifier` extension is required.

**Why bindless?** Per-light descriptor sets use `UNIFORM_BUFFER_DYNAMIC` (binding 0), which does not support `UPDATE_AFTER_BIND_BIT`. This makes deferred texture writes unsafe with frames-in-flight. The bindless set uses `UPDATE_AFTER_BIND_BIT` + `PARTIALLY_BOUND_BIT`, allowing textures to be registered asynchronously after resource loading completes.

### Declaration de-duplication contract

Several declaration kinds are **global, fixed-identity resources** that multiple
composable generators legitimately declare into the *same* shader without
coordinating. For these, `declare()` is **silently idempotent** — a re-declaration
of the same name is byte-identical, returns `true`, and emits **no warning**:

- **Vertex input attributes** (`VertexShader::declare(const Declaration::InputAttribute &)`).
  Name, location and GLSL type are all derived from the `VertexAttributeType`, so a
  duplicate cannot conflict. The `synthesize*` / TBN helpers in `VertexShader.cpp`
  and the `Generator/*` passes each declare what they consume.
- **Unbounded bindless arrays** (`AbstractShader::declare(const Declaration::Sampler &)`
  when `declaration.isUnbounded()`). A fixed name maps to a fixed set/binding/type
  (e.g. `uBindlessTexturesCube` → cube binding on the `PerBindless` set). They are
  declared **independently** by the material (`StandardResource`, at each of its feature
  sites) **and** the `LightGenerator` variants (cube shadows, color projection) into one fragment
  shader — there is **no single coordinator** across those subsystems, so a localized
  "declare once" cannot cover it. Silent de-dup is the mechanism.

**Do not re-introduce a warning or a `quiet`/once-guard for these** — it only
produced log spam (hundreds of lines per program build) with zero actionable
signal. **Bounded / named samplers still warn** on duplicates: there a
same-name / different-binding clash is a real bug worth catching.

> [!NOTE]
> Every subsystem declares the bindless arrays **on use** (each `StandardResource` feature
> site, each `LightGenerator` variant), unconditionally — no up-front "declare once"
> coordinator and no guards. There is no shared owner across material ↔ LightGenerator,
> so the silent de-dup above is what keeps a single declaration in the shader and the log
> clean. Do not add a localized once-guard back; it cannot cover the cross-subsystem case
> and only fragments the pattern.

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
- **Y-up convention**: the world is Y-up; the projection carries the single Y flip that reconciles it with Vulkan's Y-down NDC
- **Thread safety**: Cache protected, generation can be parallel
- **Used by Graphics and Overlay**: Graphics (3D), Overlay (2D) use Saphir
- **Runtime generation**: Shaders generated on demand during resource loading
- **Alpha preservation**: Lighting calculations use `.rgb` only, never modify alpha channel. See: `LightGenerator.cpp:603-661`
- **Color space conversion** (3D only): sRGB↔Linear functions apply gamma only to RGB, alpha passes through unchanged. See: `FragmentShader.cpp:generateToSRGBColorFunction()`, `generateToLinearColorFunction()`. Note: Overlay system does NOT use color space conversion - swap-chain format (UNORM vs SRGB) determines final handling.

## Descriptor set binding contract (Aug 2026)

**The sealed pipeline layout is the ONLY authority on set indexes.** `prepareUniformSets()`
enables set types in a fixed order (PerView, PerSceneTransforms, PerLight, PerModel,
PerModelLayer, PerBindless); `Generator::Abstract::createDataLayout()` builds the
`VkPipelineLayout` in that same order and `SetIndexes::set(SetType)` returns each set's
final index. Every CPU binding site MUST read that index.

Rules for `RenderableInstance::Abstract` (render tracked, render simple, castShadows):

- Bind **iff** `program->setIndexes().isSetEnabled(SetType::X)`, at index
  `program->setIndexes().set(SetType::X)`. NEVER a running `setOffset++` counter: one
  skipped set shifts every following set one slot down, and Vulkan then rejects the whole
  binding (`VUID-vkCmdBindDescriptorSets-pDescriptorSets-00358`, then
  `VUID-vkCmdDrawIndexed-None-08600` for the sets the shader statically uses).
- A set declared by the layout but unavailable at recording time is a **contract
  violation**, not a case to skip: the draw call is dropped and
  `traceMissingDescriptorSet()` reports it once per instance. Fix the generation/binding
  pair, never the binding alone.
- Every set has TWO conditions — one at generation, one at binding — and they must be
  equivalent by construction. Known pairs:

| Set | Generation condition | Binding condition |
|-----|----------------------|-------------------|
| `PerView` | `SceneRendering`: always · `ShadowCasting`: instancing, cubemap or CSM | render target view UBO (always exists) |
| `PerSceneTransforms` | `useInstanceTransformsSet()` | `sceneTransformsDS != nullptr` |
| `PerLight` | lighting enabled + a light render pass type | `lightEmitter != nullptr && isCreated()` |
| `PerModel` | `SkeletalDataTrait::hasSkeletalData()` (RENDERABLE) | `hasSkinningResources()` (INSTANCE) |
| `PerModelLayer` | `materialEnabled()` / alpha-tested shadows | material `!= nullptr` |
| `PerBindless` | `bindlessTexturesEnabled()` | manager + descriptor set available |

⚠️ **The `PerModel` row is the one that bit us** (Aug 2026, `reflexion-debug` options 3 and 4):
the layout condition is a property of the RENDERABLE, the binding condition a property of the
INSTANCE, and the instance's skinning descriptor sets were created lazily from the LOGIC
thread on the first `processLogics()`. A probe cubemap renders at scene build time, before
that first logic tick: the sealed layout declared `PerModel`, the binding skipped it, the
material landed in the skinning slot and the bindless array in the material slot. They are
now created by `RenderableInstance::Abstract::prepareSkinningResources()` at the top of
`getReadyForRender()` **and** `getReadyForShadowCasting()`, and an instance missing them is
not "ready" even when the renderable-level program cache holds its program. See
[`docs/renderable-instance-system.md`](../../docs/renderable-instance-system.md).

## Cubemap Rendering Mode (Multiview)

When rendering to a cubemap (e.g., environment probes, reflection captures), the shader system operates differently:

> [!CRITICAL]
> **Rendering INTO a cubemap takes THREE coupled pieces. They are one mechanism — never touch one alone.**
>
> | # | Where | What |
> |---|---|---|
> | 1 | `ViewMatrices3DUBO::CubemapOrientation` | each face looks along ITS OWN axis, **up vectors NEGATED** (sides `-Y`, `+Y` pole `+Z`, `-Y` pole `-Z`) |
> | 2 | `ViewMatrices3DUBO::updatePerspectiveViewProperties()` | the Y-up projection flip is **UNDONE** for cube faces |
> | 3 | `GraphicsPipeline::configureRasterizationState(..., mirroredViewport = renderTarget()->isCubemap())` | the **front face is inverted** |
>
> **Why it cannot be done in coordinates alone.** The cube-face convention is LEFT-handed — a face
> wants `right × up = +look` (the `(dx,dy,dz)` tables in `CubemapResource` / `IBLBaker`) — while a
> camera is right-handed and gives `right × up = -look`. **Measured: with honest look/up vectors all
> six faces come out with exactly the OPPOSITE right vector.** Changing `up` only ROTATES a face
> (`up → -up` gives `right → -right` *and* `up → -up`, a 180° turn), it never MIRRORS it, so no
> re-parameterisation can repair the handedness. Piece 2 supplies the second mirror
> (mirror + mirror = a 180° rotation, orientation-preserving), piece 1's negated ups cancel that
> rotation, and piece 3 pays for the winding the reflection reverses.
>
> **The three symptoms, in the order they peel off** — reproduce with
> `reflexion-debug --demo-options 0,3,0` (mode 3 = `CameraOnce`). ⚠️ **It is the only usable mode**:
> modes 1/2/5 reflect a SKY, which has no left/right landmark, so a mirrored reflection is
> invisible there. Do not "verify" a cubemap orientation against a sky.
>
> | Missing piece | What you see |
> |---|---|
> | face axes wrong (pre-Aug-2026 table) | GROUND at the top of the sphere, SKY at the bottom, faces not joining |
> | 1 + 2 | palm TRUNK left, its CROWN crossed to the RIGHT — the crown lands on the `+Y` pole face whose mirror axis is X. ⚠️ A GLOBAL mirror moves the whole tree; a PER-FACE one cuts it in two. That distinction is what identifies the defect. |
> | 3 | geometry correctly PLACED but the cubemap mostly **BLACK** (culling eats it) |
>
> Correct: palm whole on the left, dragon on the right, continuous horizon, no black.
>
> ⚠️⚠️ **This mechanism feeds EVERY cubemap render target — reflection probes AND point-light shadow
> cubemaps.** Measured across the change on `light-and-shadow-debug`: 3329 differing pixels out of
> 4 665 600, i.e. shadows unaffected.
>
> **What is NOT this defect** (measured, do not re-chase): the material reflection path, the IBL
> split-sum, SSR and RTR are healthy — modes 1, 2 and 5 (both the SSR and the RTR branch) reflect
> correctly, including after a sky change with and without lighting derivation. All three sampling
> paths take the raw world-space `reflect(I, N)` with no negation, which is the correct contract.

### Matrix Sources by Render Mode

| Matrix | Standard Mode | Cubemap Mode |
|--------|---------------|--------------|
| Projection | Push constant (MVP) or UBO | UBO (shared for all faces) |
| View | Push constant (MVP) or UBO | UBO indexed by `gl_ViewIndex` |
| Model | Push constant (MVP) | Push constant (Model only) |

### Push Constant Declaration

The push constant structure changes based on render target type. See: `Generator/Abstract.cpp:declareMatrixPushConstantBlock()`

```cpp
// Standard mode (non-cubemap, non-instanced, no advanced matrices) —
// ONLY when the InstanceTransforms SSBO path is unavailable (fallback):
layout(push_constant) uniform Matrices {
    mat4 modelViewProjectionMatrix;  // Pre-combined MVP
} pcMatrices;

// Standard mode on the InstanceTransforms SSBO (the DEFAULT scene path since B1):
layout(push_constant) uniform Matrices {
    mat4 viewProjectionMatrix;  // VP only — model matrix from the SSBO
} pcMatrices;

// Cubemap mode (non-instanced)
layout(push_constant) uniform Matrices {
    mat4 modelMatrix;  // Model only - View/Projection from UBO
} pcMatrices;
```

### InstanceTransforms SSBO Path (motion vectors B1)

The classic non-instanced scene path reads its model matrix from the per-scene
`InstanceTransforms` SSBO instead of push constants:

- **SetType::PerSceneTransforms** — dedicated set (dynamic index, enabled right after
  PerView by `SceneRendering::prepareUniformSets()` via `useInstanceTransformsSet()`:
  non-instanced, non-MDI, non-cubemap, scene transforms initialized — classic AND advanced).
- **GLSL**: `readonly buffer InstanceTransforms { mat4 viewProjection; mat4
  previousViewProjection; mat4 instanceMatrices[]; } ubInstanceTransforms;` — entries
  interleave `{model, previousModel}` (stride 2). The header is reserved for the
  motion-vector pass.
- **Indexing**: `gl_InstanceIndex * 2` — the slot is encoded in the `firstInstance`
  draw parameter (`CommandBuffer::drawWithFirstInstance()`); with `instanceCount == 1`,
  `gl_InstanceIndex == firstInstance` and NO `shaderDrawParameters` feature is required
  (contrary to `gl_BaseInstance`). Min-spec safe.
- **Push blocks**: classic = VP + frameIndex, advanced = V + frameIndex (projection from
  the view UBO) — both 68 B, declared by `declareMatrixPushConstantBlock()`. The advanced
  V+M+frameIndex fallback (132 B, min-spec VIOLATION) only survives for scenes whose
  instance transforms failed to initialize.
- ⚠️ **Two-condition contract**: the descriptor SET (pipeline layout) follows
  `setIndexes.isSetEnabled(PerSceneTransforms)` — sealed at `prepareUniformSets()` time —
  while the MATRIX SOURCE follows `Program::wasInstanceTransformsEnabled()` (the vertex
  shader flag). The CPU binding in `RenderableInstance::Abstract::render()` follows
  setIndexes; the push constants and `firstInstance` follow the shader flag. NEVER mix
  the two conditions (a bound-but-unreferenced set is legal; a missing set in the sealed
  layout order corrupts every subsequent set index).
- **VertexShader preparations**: `prepareInstanceModelMatrix()` (the template), with
  branches in `prepareModelViewMatrix()`, `prepareModelViewProjectionMatrix()`,
  `synthesizeVertexPositionInWorldSpace()`, world-space normal and world TBN fallbacks,
  plus the non-instanced shadow-receiving reads in `LightGenerator.ShadowMap.cpp`.
- **Assumed limit (owner decision)**: cubemap scene, shadow 2D, CSM and shadow-cubemap
  paths STAY on push constants (64-68 B, min-spec clean — no motion data needed there).
- **Velocity clip positions** — `VertexShader::synthesizeVelocityClipPositions()` emits
  `svClipPositionCurrent` (recomputed from the MVP, deliberately **independent** of the
  `gl_Position` instruction so output ordering cannot break it) and `svClipPositionPrevious`
  (`previousViewProjection` × the odd-slot previous model, or the previous skinned pose).
  The fragment side outputs the plain NDC delta. Both endpoints are expressed in the
  **same, jitter-free** projection by construction (see the jitter contract below) — no
  subtraction is performed, and reintroducing one would be a regression.
- **⚠️ Infinity view**: renderables drawn with the translation-free view
  (`isUsingInfinityView()`, the sky background) take their CURRENT clip position from the pushed
  infinity view, so their PREVIOUS one MUST come from the header's
  `previousViewProjectionInfinity`, never from `previousViewProjection`. The generator flag
  `IsUsingInfinityView` selects it at generation time and reaches the program cache key through
  `flags()` — two instances of the same renderable, one infinity-view and one not, must never
  share a program. Mixing the two forms is a **STRUCTURAL** mismatch: it differs by the whole
  camera translation and therefore does NOT cancel on a static camera (lived: a smooth
  NDC-position-like velocity gradient over the entire sky, blamed on the translucent glass in
  front of it for two sessions before anyone visualised the buffer).
- **⚠️ Infinity view also PINS THE CLIP DEPTH** (added Aug 2026):
  `synthesizeVertexPositionInScreenSpace()` emits `gl_Position.z = gl_Position.w` right after the
  MVP multiply whenever `isInfinityViewEnabled()`. The projection maps near to 0 and far to 1
  (Vulkan range, NOT reversed), so `z = w` lands exactly on the far plane — the standard skybox
  trick (`clipPosition.xyww` in the Khronos glTF Sample Viewer's `skybox.vert`). **The geometric
  size of the sky therefore stops mattering, and no camera far distance can ever clip it.**
  Before the pin, a short far distance silently deleted the whole sky (see
  `Scenes/AGENTS.md` § "The background is drawn FIRST"). Safe by construction: the background is
  the ONLY user of the infinity view in the whole cascade, and it is drawn with the depth test AND
  the depth write disabled, so pinning its depth can neither fail a comparison nor pollute the
  depth buffer. ⚠️ It must stay OUT of the velocity path: the velocity clip positions are
  synthesized independently of `gl_Position` (`synthesizeVelocityClipPositions()`), which is
  precisely what keeps the motion vectors correct — never "simplify" that by reusing
  `gl_Position`.

### TAA Sub-Pixel Jitter — The Per-Draw Push Constant Contract (fixed 2026-07-25)

**Invariant: NO matrix ever carries the sub-pixel jitter.** It is applied to the clip position
and nowhere else, from a per-draw push constant:

```glsl
gl_Position = svModelViewProjectionMatrix * vec4(vaVertex, 1.0);
gl_Position.xy += pcMatrices.projectionJitter * gl_Position.w;   /* NDC translation */
```

Three sites MUST stay in lockstep — the layout is declared, read and written in different
files:

| role | location | rule |
|---|---|---|
| declares the `vec2` member | `Generator/Abstract.cpp::declareMatrixPushConstantBlock()` | after the pushed V/VP matrix; `FrameIndex` stays appended last, so the vec2 lands at offset 64 with no padding (76 B total) |
| emits the read | `VertexShader::isProjectionJitterPushed()` → `synthesizeVertexPositionInScreenSpace()` | mirrors the predicate above |
| writes the value | `RenderableInstance::Unique`/`Multiple` (rendering **and** shadow casting) | floats 16-17, push size 76 B |

Blocks that carry the jitter: instanced (advanced/billboard → V, classic → VP) and the
non-instanced InstanceTransforms paths (advanced → V, classic → VP). Blocks that do NOT:
MDI, cubemap/CSM (nothing is pushed for them), the MVP fallback and the 132 B advanced
fallback. Those last three bake the jitter into their CPU-computed matrix instead — legal
**only** because none of them outputs a velocity (the 132 B fallback simply rasterizes
unjittered: no room for the member, assumed limit).

> [!CAUTION]
> Adding the member to one branch and forgetting another fails in two different ways:
> a shader reading a member the generator did not declare is a **hard glslang error**
> (`'projectionJitter' : no such field in structure 'pcMatrices'` — this is how the classic
> `RenderableInstanceSimplePassVertexShader` path was caught), while a CPU push that is
> shorter than the declared block is **silent** — the shader offsets `gl_Position` by
> uninitialized memory with no validation warning. Cheap exhaustive check: enable
> `Core/Graphics/Shader/ShowSourceCode` and grep the generated GLSL for
> declaration-versus-use across every program.

> [!CAUTION]
> **Sub-pixel jitter must NEVER travel through the view UBO.** The first TAA implementation
> wrote the jitter into `ubView.projectionMatrix`, which the advanced path multiplies by the
> pushed view matrix to rebuild its MVP. That UBO is single-buffered, so the raster could read
> frame N±1's jitter while the velocity subtracted frame N's → motion vectors polluted by an
> NDC-constant offset on a perfectly static camera. Mechanism in `docs/caution-points.md` §
> "Sub-pixel projection jitter raced the single-buffered view UBO" and
> `src/Graphics/AGENTS.md` § 16 Rule 4.

### Push Constant Min-Spec (128 B) — Engine-Wide Rules

- `Vulkan::PipelineLayout::createOnHardware()` VALIDATES every push constant range:
  hard error above the device `maxPushConstantsSize`, warning above the 128-byte Vulkan
  minimum guarantee. A min-spec warning in the logs is a portability defect to fix.
- The INSTANCED advanced/billboard block pushes **V only** (+ frameIndex); the shader
  recomposes VP from the view UBO projection × V (`prepareModelViewProjectionMatrix()`
  instanced branches). The former V + VP + frameIndex block was 132 B. This applies to
  shadow casting too: the ShadowCasting vertex shader declares the view uniform block for
  EVERY instanced program (the PerView set was already enabled/bound for instancing).
- Plain (non-advanced, non-billboard) instanced still pushes VP; MDI pushes BDA + VP.

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

## clang-tidy — the six warnings that are LEFT ON PURPOSE

Saphir is scanned with the project `.clang-tidy` (`run-clang-tidy -p .claude-build-release
"dependencies/emeraude-engine/src/Saphir/.*\.cpp"`). It went from 12 warnings to 6 in Aug 2026;
the remaining six are deliberate, so **do not "fix" them**:

- **`Declaration/Types.cpp` × 2 — `bugprone-branch-clone`.** The std140 alignment switch returns
  the same number for several type families, but each family carries its own comment stating the
  rule that justifies it (`vec2` → 8, `dvec2` → 16, `vec3`/`vec4` → 16…). Merging the branches
  would delete the explanation, which is the only reason the table is readable. The duplication
  is didactic.
⚠️ Everything else is fixed, never silenced — no NOLINT, no check disabled.

### The shader includer: there is none, on purpose

Saphir **generates** its GLSL: no source handed to glslang ever carries an `#include`, and no
include directory was ever registered (the single `pushExternalLocalDirectory()` call had been
commented out for as long as it existed). glslang nevertheless requires an `Includer` argument
for `preprocess()` and `parse()`, so `ShaderManager` passes glslang's own no-op
`glslang::TShader::ForbidIncluder`.

That replaced a copied `DirStackFileIncluder` (deleted Aug 2026) whose class name and comment
came verbatim from glslang's `StandAlone/` sample while the file carried only the Emeraude
header — a licensing loose end, four unfixable `cppcoreguidelines-owning-memory` warnings (the
Includer contract is raw-pointer based: glslang frees the `IncludeResult *` itself), and code
that was never once executed.

`ForbidIncluder` is also the safer behaviour: an `#include` appearing by accident now FAILS
instead of being silently resolved against an empty search stack.

⚠️ **The day hand-written GLSL sources become a thing** (see [`docs/todo/manual-glsl-sources.md`](../../docs/todo/manual-glsl-sources.md), "Prepare a way to use
manual GLSL sources"), a real includer plugs in exactly there — written against an actual
specification (which directories, which search policy, what caching), not copied from a sample.

## The three shader-cache stages — what each one actually caches (audited Aug 2026)

Three DIFFERENT things are persisted to disk, under three separate setting keys. They are not
tiers of one mechanism, and only two of them are caches at all:

| Stage | Setting key | Default | Stores | Read back? | Measured gain |
|---|---|---|---|---|---|
| 1. Source dump | `Core/Graphics/Shader/EnableSourceCodeDump` | `false` | the generated GLSL, one sub-directory per generator | **NEVER** | none — it is an inspection tool |
| 2. SPIR-V binary cache | `Core/Graphics/Shader/EnableBinaryCache` | **`true`** since Aug 2026 | glslang's SPIR-V output, one blob per shader | yes | 393 ms → 10.3 ms on 232 modules (**38x**) |
| 3. `VkPipelineCache` | `Core/Graphics/Shader/EnablePipelineCache` | `true` | the driver's SPIR-V→ISA result, one blob | yes | 5702 ms → 31 ms on 294 pipelines (**182x**) |

The defaults live in `SettingKeys.hpp` (`DefaultSourceCodeDumpEnabled`,
`DefaultBinaryCacheEnabled`, `DefaultPipelineCacheEnabled`).

`--clear-renderer-cache` wipes the shader caches, the pipeline cache included.

### Stage 1 — the generated-GLSL DUMP, not a cache (renamed Aug 2026)

`Core/Graphics/Shader/EnableSourceCodeDump` writes every generated GLSL to
`~/.cache/<app>/generated-shaders/`. **Nothing ever reads it back**:
`AbstractShader::loadSourceCode()` — the only function able to re-inject a file into a shader —
has zero callers in the whole cascade. It structurally cannot be a cache either: its key is
`std::hash` of the source it stores, so you must have generated the source already to know which
file to read. That is why it was renamed away from "cache" — the old name described something
this facility never was.

> [!WARNING]
> **The Aug 2026 rename ships with NO migration, deliberately.** No alias, no fallback, no
> warning at startup — `Settings` has no key-migration mechanism at all. Consequence: an
> existing `settings.json` keeps `Core/Graphics/Shader/EnableSourceCodeCache` as **dead JSON**
> that is silently ignored, and anyone who had the dump enabled finds it **OFF** until they set
> `Core/Graphics/Shader/EnableSourceCodeDump`. The owner accepted the break because this is a
> debug facility defaulting to `false`. Do NOT add a compatibility alias to "fix" a report of
> the dump having stopped — point at the new key instead.
>
> The private `ShaderManager` symbols moved with it (no public API touched):
> `SourceCodeCacheEnabledKey`→`SourceCodeDumpEnabledKey`,
> `DefaultSourceCodeCacheEnabled`→`DefaultSourceCodeDumpEnabled`,
> `m_sourceCodeCacheEnabled`→`m_sourceCodeDumpEnabled`,
> `ShaderSourcesDirectoryName`→`GeneratedShadersDirectoryName`,
> `m_shadersSourcesDirectory`→`m_generatedShadersDirectory`,
> `cacheShaderSourceCode()`→`dumpShaderSourceCode()`,
> `generateShaderSourceCacheFilepath()`→`generateShaderDumpFilepath()`,
> `readCache()`→`readBinaryCache()` (it only ever reads binaries now). The two sibling keys
> `EnableBinaryCache` and `EnablePipelineCache` are untouched, and so is `ShowSourceCode` —
> that one logs, it does not write files.

It exists to let a human inspect what the generators produced, and it is now shaped for that:

- **one sub-directory per generator** (`SceneRendering/`, `ShadowCasting/`, `PostProcessing/`,
  `OverlayRendering/`, `GizmoRendering/`, `TBNSpaceRendering/`), created lazily. The generator
  identity is carried by `Generator::Abstract::generatorClassId()` and threaded through
  `ShaderManager::getShaderModules()`;
- the dump now happens **before** the binary-cache check. It used to hang off the compile path,
  so a binary cache hit silently stopped producing it.

Re-verified at runtime after the rename (`material-debug`, all 10 options): the dump lands in
`generated-shaders/` — 232 files, SceneRendering 226, OverlayRendering 3, PostProcessing 2,
ShadowCasting 1 — and the run logs no error. ⚠️ That file count and the 336 below are **different
measurements**, not a contradiction: 336 counts distinct generated SceneRendering sources, not
files left on disk. Do not try to reconcile them.

⚠️ Measured on one `material-debug` load: **336 distinct sources for SceneRendering alone**
(265 fragment, 71 vertex), against 3 for OverlayRendering, 2 for PostProcessing and 1 for
ShadowCasting. That number is the program-variant count, and it is the real load-time driver —
worth understanding before optimising any cache.

⚠️ **`readBinaryCache()` no longer touches this directory at all** — hence its name: it only ever
reads binaries now. It used to index the dumped sources into `m_cachedShaderSourceCodes` — a
member that was written and never once read, not even by `clearCache()`, which walks the
directory itself. Since `readBinaryCache()` only runs when the *binary* cache is on, and the dump
is off by default, that loop was scanning an **empty path** and logging an
`IO::directoryEntries()` error on every startup as soon as the binary cache became the default.
The member is gone and the function returns early on an empty binaries directory. `clearCache()`
guards both of its loops for the same reason: a disabled facility leaves its path empty, and
`--clear-renderer-cache` runs whatever the settings say. See
[`docs/caution-points.md`](../../docs/caution-points.md) § "Flipping a default to ON runs a path
nobody had ever run".

### Stage 2 — the binary (SPIR-V) cache — hardened, and ON BY DEFAULT since Aug 2026

`Core/Graphics/Shader/EnableBinaryCache` skips glslang on a hit. `DefaultBinaryCacheEnabled`
flipped from `false` to **`true`**. What paid for the flip, measured 2026-08-13 on
`material-debug` with all 10 options (RTX 3070 Ti, Release). The instrumented envelope is
source dump + cache lookup + glslang compile + `vkCreateShaderModule`, placed **after** the
in-memory ShaderModule hash lookup, so it counts cache MISSES only — **232 shader modules**:

| Run | Total | Per module |
|---|---|---|
| cache OFF | 393 ms | 1.69 ms |
| cache ON, cold (writes the 232 blobs) | 391 ms | 1.68 ms |
| cache ON, warm (reads) | **10.3 ms** | **0.044 ms** |

**38x faster, 383 ms saved — and writing the cache on a cold run is FREE** (391 vs 393 ms is
noise). That absence of a first-launch penalty is the whole argument for the default: there is
nothing to trade away. 0 residual `.tmp` files.

What made it safe enough to enable (engine commit `56fabc9a`): the filename says WHICH shader
(`<name>_<source hash>.bin`); an application header now says
whether the blob is still VALID, and **every field is checked before a byte reaches
`vkCreateShaderModule`**: magic, format version, source hash, shader stage, blob size, FNV-1a
content hash, and a **toolchain identity hash** — glslang's version string, its SPIR-V generator
version, the client/target environment pair (⚠️ macOS targets Vulkan 1.2 / SPIR-V 1.5, everything
else 1.3 / 1.6) and the engine version. A rejected file is deleted and the shader recompiled.

⚠️ That toolchain hash is the whole point: without it a glslang upgrade left stale SPIR-V on disk
and it was fed to the driver unchecked. Plus two structural checks the blob must pass anyway —
size a multiple of 4, and the SPIR-V magic word `0x07230203` as its first word.

Writes go to a `.tmp` file and are renamed, so a `SIGKILL` cannot leave a truncated blob for the
next launch.

Verified live during that hardening pass — a **separate run** from the timings above, hence a
different blob count: 342 blobs written on the first run and all 342 reloaded on the second, with
no stray `.tmp`; a blob corrupted in its data and another with a falsified toolchain hash were
both rejected and recompiled — 2 rejected, 340 reused, no crash.

### Stage 3 — the `VkPipelineCache` — it EXISTS now (engine commit `e583df40`)

⚠️ This file used to state "There is NO VkPipelineCache". **That is obsolete** — do not act on
that claim if you find it echoed elsewhere.

`Core/Graphics/Shader/EnablePipelineCache`, default `true` (already `true` before the Aug 2026
binary-cache pass, unchanged by it). This is the stage that matters most, because it skips the
DRIVER's SPIR-V→ISA compilation. Measured on the same demo, 294 graphics pipelines:

| Run | Total |
|---|---|
| driver cache active | 33 ms |
| driver cache OFF | 5702 ms |
| driver cache OFF, engine cache restored from disk | 31 ms |

**182x.** The serialised blob is 7.4 MB.

Ownership split, do not move it: `Vulkan::Device` owns the `VkPipelineCache` object,
`Graphics::Renderer` does the disk I/O (`loadPipelineCache()` / `savePipelineCache()`).

### glslang's optimizer is COMPILED OUT of this build

`ShaderManager.cpp` sets `SpvOptions::disableOptimizer = true` before `GlslangToSpv()`.
**That flag is SILENTLY IGNORED.** glslang is built here with `ENABLE_OPT=OFF`: `libSPIRV.a`
contains ZERO SPIRV-Tools symbols (verified 2026-08-13), so there is no optimizer to disable in
the first place — and nothing to enable either.

Do not chase that flag for compile time or SPIR-V quality. Turning a real optimizer on would
mean adding SPIRV-Tools to the dependency cascade for no gain: desktop NVIDIA/AMD drivers fully
re-optimize whatever SPIR-V they receive. The levers that actually move load time are the two
caches above and the number of program VARIANTS (336 distinct SceneRendering sources on a single
`material-debug` load).
