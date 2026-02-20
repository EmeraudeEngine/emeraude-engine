# Shadow Mapping System

This document details the shadow mapping architecture in Emeraude Engine, including PCF soft shadows, global controls, and per-light configuration.

## Overview

The engine supports shadow mapping for all three light types:
- **Directional lights**: 2D shadow maps (standard or Cascaded Shadow Maps)
- **Spot lights**: 2D shadow maps
- **Point lights**: Cubic shadow maps (6-face cubemap)

## Global Shadow Mapping Control

Shadow mapping can be globally enabled/disabled via the settings system.

**Setting key:** `GraphicsShadowMappingEnabledKey` (`Core/Graphics/Renderer/ShadowMappingEnabled`)

| Value | Behavior |
|-------|----------|
| `true` (default) | Shadow maps rendered, shadow-enabled passes used |
| `false` | Shadow maps skipped, base pass types used (no shadow sampling) |

### Implementation Details

When the global setting is disabled:

1. **Shadow map rendering skipped**: `Scene::renderShadowMaps()` checks `Renderer::isShadowMapsEnabled()` and returns early
2. **Base pass types forced**: `Scene::renderLightedSelection()` selects base or `*ColorMap` pass types (no shadow sampling)
3. **Shadow samplers unused**: Descriptor sets still contain dummy shadow textures at binding 1, but the shader never samples them

**Code references:**
- `Scenes/Scene.rendering.cpp:978` - Global setting check in `renderLightedSelection()`
- `Scenes/Scene.rendering.cpp:1003` - Directional light pass selection
- `Scenes/Scene.rendering.cpp:1034` - Point light pass selection
- `Scenes/Scene.rendering.cpp:1064` - Spotlight pass selection
- `Graphics/Renderer.cpp:isShadowMapsEnabled()` - Setting accessor

### Why Global Control Matters

Without the global check, disabling shadow mapping via settings caused Vulkan validation errors:

```
VK_IMAGE_LAYOUT_UNDEFINED (0) but expected VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL
```

**Root cause:** Shadow maps are created but never rendered (layout stays UNDEFINED). However, lighting passes still attempted to bind shadow-enabled descriptor sets containing these images.

**Solution:** The rendering code now checks the global setting and selects base pass types (e.g., `SpotLightPass` or `SpotLightPassColorMap`) when shadows are disabled, which generate shaders without shadow sampling code.

## PCF Soft Shadows

Percentage-Closer Filtering (PCF) provides soft shadow edges by sampling multiple points in the shadow map.

### PCF Methods

The engine supports four PCF methods, configured via `GraphicsShadowMappingPCFMethodKey`:

| Setting Value | Internal Method | Description | Sample Pattern |
|---------------|-----------------|-------------|----------------|
| `"Performance"` | Grid | Max FPS, basic quality | Square pattern, (2n+1)² samples |
| `"Balanced"` | VogelDisk | Recommended sweet spot | Circular, evenly distributed |
| `"Quality"` | PoissonDisk | Better visuals | Circular, random-looking distribution |
| `"Ultra"` | OptimizedGather | Best quality, optimized | Uses `textureGather` for efficiency |

**Default:** `"Balanced"` (VogelDisk - best quality/performance balance)

### PCF Radius Per Light Type

Each light type has different default PCF radius values:

| Light Type | Default Radius | Reason |
|------------|----------------|--------|
| Directional | 1.0 | Large coverage, subtle softness |
| Spot | 4.0 | Medium coverage, visible softness |
| Point | Auto (*100) | Depth-based contact hardening |

**Point light auto-calculation:**
```cpp
m_PCFRadius = (1.0F / static_cast<float>(resolution)) * 100.0F;
```

This creates a "contact hardening" effect where shadows are sharper near contact points and softer further away.

### Per-Vertex Lighting (Low Quality) Shadow Constraint

> [!WARNING]
> **GLSL shader inputs are READ-ONLY!**

In per-vertex (Gouraud) lighting mode, `diffuseFactor` and `specularFactor` are computed in the vertex shader and passed to the fragment shader as interface block members. These are shader inputs and cannot be modified.

When shadow mapping is enabled in low quality mode, the fragment shader creates local copies:

```glsl
// Create local copies of read-only shader inputs
float diffuseFactor = svLight.diffuseFactor;
float specularFactor = svLight.specularFactor;

// Now safe to apply shadow factor
diffuseFactor *= shadowFactor;
specularFactor *= shadowFactor;
```

**Code reference:** `Saphir/LightGenerator.PerVertex.cpp:generateGouraudFragmentShader()`

### PCF Code Generation

Shadow map filtering code is generated in `LightGenerator.ShadowMap.cpp`:

| Function | Purpose |
|----------|---------|
| `generate2DShadowMapCode()` | Non-PCF 2D shadow sampling |
| `generate2DShadowMapPCFCode()` | PCF-enabled 2D shadows |
| `generate3DShadowMapCode()` | Non-PCF cubemap shadow sampling |
| `generate3DShadowMapPCFCode()` | PCF-enabled cubemap shadows |
| `generateCSMShadowMapCode()` | Cascaded Shadow Maps |

**Code references:**
- `Saphir/LightGenerator.ShadowMap.cpp` - All shadow map code generation
- `Saphir/LightGenerator.hpp:PCFMethod` - PCF method enum

## Per-Light Shadow Configuration

Each light component can independently configure shadow mapping:

### Shadow Map Resolution

```cpp
light->setShadowMapResolution(1024);  // Power of 2 recommended
```

Resolution of 0 disables shadow mapping for that light.

### Shadow Bias

```cpp
light->setShadowBias(0.005F);  // Prevent shadow acne
```

Bias offsets depth comparison to prevent self-shadowing artifacts.

### PCF Radius

```cpp
light->setPCFRadius(2.0F);  // Filter radius in texels
```

Larger radius = softer shadows but more blurring.

## Light Descriptor Sets

Each light uses one of two descriptor set configurations:

**Without shadow map** — shared UBO-only descriptor set (from `SharedUniformBuffer`):

| Binding | Content |
|---------|---------|
| 0 | Light UBO (dynamic offset) |

**With shadow map** — dedicated per-light descriptor set:

| Binding | Content |
|---------|---------|
| 0 | Light UBO (dynamic offset) |
| 1 | Shadow map sampler (2D, Cube, or 2DArrayShadow) |

**Color projection** is handled via the global `BindlessTextureManager` descriptor set, not via per-light descriptor sets. The light UBO carries a `ColorProjectionIndex` field (`uint` encoded as `bit_cast<float>`) that indexes into the bindless 2D or Cube texture array. When no texture is assigned, the sentinel value `0xFFFFFFFF` causes the shader to skip sampling (`projectionColor = vec3(1.0)`).

**Why bindless for color projection?** Per-light descriptor sets use `UNIFORM_BUFFER_DYNAMIC` at binding 0, which does not support `UPDATE_AFTER_BIND_BIT`. This makes deferred texture writes unsafe with frames-in-flight. The bindless set uses `UPDATE_AFTER_BIND_BIT` + `PARTIALLY_BOUND_BIT`, allowing textures to be registered asynchronously after resource loading completes via `ObserverTrait` notification.

**Code references:**
- `Scenes/Component/SpotLight.cpp:createShadowDescriptorSet()` - 2-binding shadow descriptor
- `Scenes/Component/PointLight.cpp:createShadowDescriptorSet()` - 2-binding shadow descriptor
- `Scenes/Component/DirectionalLight.cpp:createShadowDescriptorSet()` - 2-binding shadow descriptor
- `Scenes/Component/AbstractLightEmitter.cpp:registerColorProjectionInBindless()` - Bindless registration
- `Graphics/BindlessTextureManager.hpp` - Global bindless descriptor set

## Render Pass Types

The shader system generates different programs per pass type. Each `RenderPassType` is a combinatorial variant encoding light type + shadow mode + color projection:

### Directional Light

| Pass Type | Shadow | Color Projection | Use Case |
|-----------|--------|-------------------|----------|
| `DirectionalLightPass` | No | No | Base directional, no extras |
| `DirectionalLightPassShadowMap` | 2D | No | Standard shadow map |
| `DirectionalLightPassCSM` | CSM | No | Cascaded Shadow Maps |
| `DirectionalLightPassColorMap` | No | Yes | Color projection only |
| `DirectionalLightPassFull` | 2D | Yes | Shadow + color projection |
| `DirectionalLightPassFullCSM` | CSM | Yes | CSM + color projection |

### Point Light

| Pass Type | Shadow | Color Projection | Use Case |
|-----------|--------|-------------------|----------|
| `PointLightPass` | No | No | Base point light |
| `PointLightPassShadowMap` | Cube | No | Cubemap shadow |
| `PointLightPassColorMap` | No | Yes | Color projection only |
| `PointLightPassFull` | Cube | Yes | Shadow + color projection |

### Spot Light

| Pass Type | Shadow | Color Projection | Use Case |
|-----------|--------|-------------------|----------|
| `SpotLightPass` | No | No | Base spotlight |
| `SpotLightPassShadowMap` | 2D | No | Standard shadow map |
| `SpotLightPassColorMap` | No | Yes | Color projection only |
| `SpotLightPassFull` | 2D | Yes | Shadow + color projection |

### Helper Functions

| Function | Purpose |
|----------|---------|
| `renderPassUsesShadowMap(type)` | Returns true for `*ShadowMap`, `*CSM`, `*Full`, `*FullCSM` |
| `renderPassUsesCSM(type)` | Returns true for `*CSM`, `*FullCSM` |
| `renderPassUsesColorProjection(type)` | Returns true for `*ColorMap`, `*Full`, `*FullCSM` |

**Code reference:** `Graphics/Types.hpp` — Enum definition and helper functions

## Color Projection

Color projection allows a light to project a texture onto surfaces (like a gobo/light mask). It works **independently** of shadow maps — a light can project colors without the overhead of rendering a shadow map.

### How It Works

1. **Texture assignment:** `light->setColorProjectionTexture(texture)` assigns a 2D or cubemap texture
2. **Pass type selection:** `Scene.rendering.cpp` selects `*ColorMap` or `*Full` pass type based on `hasColorProjectionTexture()`
3. **Shader generation:** The `LightGenerator` generates bindless sampling code using the UBO's `ColorProjectionIndex`
4. **Projection coordinates:** Uses the light's `ViewProjectionMatrix` (from UBO) to project fragment position into light space

### UV Coordinate Convention

> [!WARNING]
> **ScaleBiasMatrix is already baked into ViewProjectionMatrix!**
>
> The `RenderTarget::ScaleBiasMatrix` transforms clip-space [-1,1] to UV [0,1]. It is pre-multiplied into the light's `ViewProjectionMatrix` stored in the UBO.
>
> Shadow maps use `textureProj()` which handles perspective divide + bias automatically.
> Color projection does the perspective divide manually (`projCoords = .xyz / .w`), so UVs are already in [0,1].
>
> **Do NOT apply additional `* 0.5 + 0.5` to color projection UVs** — this causes a double-bias offset.

```glsl
// CORRECT — ScaleBiasMatrix already in ViewProjectionMatrix
const vec3 projCoords = svPositionLightSpace.xyz / svPositionLightSpace.w;
uint cpIdx = floatBitsToUint(uLight.ColorProjectionIndex);
if ( cpIdx != 0xFFFFFFFFu ) {
    projectionColor = texture(uBindlessTextures2D[nonuniformEXT(cpIdx)], projCoords.xy).rgb;
}

// WRONG — double bias, pattern is offset
projectionColor = texture(uBindlessTextures2D[nonuniformEXT(cpIdx)], projCoords.xy * 0.5 + 0.5).rgb;
```

### Light Type Specifics

| Light Type | Projection Method | Bindless Array |
|------------|-------------------|----------------|
| Spot | 2D projection via ViewProjectionMatrix | `sampler2D[]` (binding 1) |
| Directional | 2D projection via ViewProjectionMatrix (non-CSM only) | `sampler2D[]` (binding 1) |
| Point | Cubemap lookup via DirectionWorldSpace | `samplerCube[]` (binding 3) |

**Note:** CSM directional lights cannot use color projection (CSM computes light-space position per-cascade in the fragment shader, which is incompatible with a single projection texture).

### When No Texture Assigned

The `ColorProjectionIndex` in the UBO is set to `0xFFFFFFFF` (sentinel). The shader checks `cpIdx != 0xFFFFFFFFu` before sampling — when no texture is assigned, `projectionColor` remains `vec3(1.0)` (hardcoded default). No bindless texture access occurs, and the SPIR-V compiler may optimize out the multiplication entirely.

**Code references:**
- `Saphir/LightGenerator.PerFragment.cpp` — Color projection sampling (all 4 shading variants)
- `Scenes/Component/AbstractLightEmitter.hpp:setColorProjectionTexture()` — Texture assignment
- `Scenes/Scene.rendering.cpp:renderLightedSelection()` — Pass type selection logic
- `Graphics/Types.hpp:renderPassUsesColorProjection()` — Helper function

## Image Layout Lifecycle

Shadow map images follow this layout progression:

1. **Creation:** `VK_IMAGE_LAYOUT_UNDEFINED`
2. **Before shadow pass:** Transition to `VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL`
3. **During shadow pass:** Depth written
4. **After shadow pass:** Transition to `VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL`
5. **During lighting pass:** Sampled as texture

**Critical:** If shadow rendering is skipped (global setting disabled), images remain in `UNDEFINED` layout and must not be bound to descriptors.

## Settings Summary

| Setting Key | Type | Default | Description |
|-------------|------|---------|-------------|
| `GraphicsShadowMappingEnabledKey` | bool | true | Global shadow mapping enable |
| `GraphicsShadowMappingPCFEnabledKey` | bool | true | PCF soft shadows enable |
| `GraphicsShadowMappingPCFMethodKey` | string | "Balanced" | PCF sampling method ("Performance", "Balanced", "Quality", "Ultra") |
| `GraphicsShadowMappingPCFSampleKey` | int | 2 | PCF sample count (for Grid) |

**Code reference:** `SettingKeys.hpp` - All shadow mapping setting keys
