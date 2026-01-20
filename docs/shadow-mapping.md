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
| `false` | Shadow maps skipped, NoShadow passes used for all lights |

### Implementation Details

When the global setting is disabled:

1. **Shadow map rendering skipped**: `Scene::renderShadowMaps()` checks `Renderer::isShadowMapsEnabled()` and returns early
2. **NoShadow pass types forced**: `Scene::renderLightedSelection()` checks the global setting before selecting pass types
3. **No descriptor binding**: Shadow-enabled descriptor sets (with shadow map samplers) are never bound

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

**Solution:** The rendering code now checks the global setting and forces `NoShadow` pass types when disabled, which use descriptors without shadow map bindings.

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

## Shadow Descriptor Sets

Each shadow-casting light creates its own descriptor set with:
- **Binding 0:** Light UBO (uniform buffer with light properties)
- **Binding 1:** Shadow map sampler (2D or Cube)

**Code references:**
- `Scenes/Component/SpotLight.cpp:createShadowDescriptorSet()` - 2D shadow descriptor
- `Scenes/Component/PointLight.cpp:createShadowDescriptorSet()` - Cubemap shadow descriptor
- `Scenes/Component/DirectionalLight.cpp:createShadowDescriptorSet()` - 2D/CSM shadow descriptor

## Render Pass Types

The shader system generates different programs for shadow and non-shadow passes:

| Pass Type | Shadow Map | Use Case |
|-----------|------------|----------|
| `DirectionalLightPass` | Yes | Directional with shadow |
| `DirectionalLightPassNoShadow` | No | Directional without shadow |
| `DirectionalLightPassCSM` | Yes (CSM) | Cascaded shadow maps |
| `PointLightPass` | Yes (Cube) | Point light with shadow |
| `PointLightPassNoShadow` | No | Point light without shadow |
| `SpotLightPass` | Yes | Spotlight with shadow |
| `SpotLightPassNoShadow` | No | Spotlight without shadow |

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
