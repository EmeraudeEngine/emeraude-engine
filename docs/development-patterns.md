# Development Patterns

Detailed code examples and patterns for Emeraude Engine usage.

> **Application-level patterns** (Actors, Weapons, UI, Logic) should be in the application's own `docs/development-patterns.md`.

## Table of Contents

- [Graphics Patterns](#graphics-patterns)
- [Terrain Patterns](#terrain-patterns)
- [Material Patterns](#material-patterns)

---

## Graphics Patterns

### Fixing Z-Fighting (Depth Bias)

If you experience Z-fighting (flickering surfaces) during multi-pass rendering or decal rendering, you can enable `DepthBias` on your `Renderable`'s `RasterizationOptions`.

```cpp
// In your MeshResource creation or loading
EmEn::Graphics::RasterizationOptions options;
// Enable bias: factor (slope), units (constant), clamp
// Recommended start: Slope -1.0 adapts to angle, Constant 0.0 avoids floating
options.setDepthBias(-1.0F, 0.0F);

// Load mesh with these options to be used by the pipeline
myMesh->load(geometry, material, options);
```

**Note:** Slope-based bias (`factor`) is often sufficient and cleaner than constant offset for preventing Z-Fighting on overlay passes.

---

## Terrain Patterns

### Procedural Terrain with DiamondSquare

Generate terrain with procedural heightmaps using the DiamondSquare algorithm:

```cpp
#include "Graphics/Geometry/AdaptiveVertexGridResource.hpp"

// Create adaptive LOD terrain
auto terrain = resources.container<EmEn::Graphics::Geometry::AdaptiveVertexGridResource>()
    ->getOrCreateResource("MyTerrain", true, {
        8192.0F,   // 8km total size
        256,       // grid subdivisions
        true       // tileable (same corner values)
    });

// Apply procedural heightmap
// factor = max height displacement in meters (normalized output)
// roughness = 0.0-1.0, controls terrain detail
// seed = for reproducible results
terrain->grid().applyDiamondSquare({
    100.0F,  // ±100 meters max height
    0.5F,    // moderate roughness
    42       // seed
});
```

**Note:** The DiamondSquare algorithm outputs normalized values in [-1, 1]. The `factor` parameter directly represents the maximum height in world units (meters).

### Tuning LOD Distance Selection

Adjust when LOD transitions occur based on distance:

```cpp
// Default values: baseMultiplier=0.125, thresholdGrowth=2.0
// - baseMultiplier: Initial threshold as fraction of sector size
// - thresholdGrowth: How fast thresholds grow for lower LODs

// For higher quality at distance (more GPU load)
terrain->setLODDistanceParameters(0.2F, 1.5F);

// For more aggressive LOD reduction (better performance)
terrain->setLODDistanceParameters(0.08F, 2.5F);
```

| baseMultiplier | Effect |
|----------------|--------|
| Lower (0.08) | Earlier LOD transitions, better performance |
| Higher (0.2) | Later transitions, higher quality at distance |

| thresholdGrowth | Effect |
|-----------------|--------|
| Lower (1.5) | More gradual LOD falloff |
| Higher (2.5) | Faster falloff to low LODs |

### Complete Terrain Setup Example

```cpp
void setupTerrain(EmEn::Resources::Manager & resources)
{
    // Create terrain resource with LOD support
    auto terrain = resources.container<EmEn::Graphics::Geometry::AdaptiveVertexGridResource>()
        ->getOrCreateResource("GiantTerrain", true, {
            16384.0F,  // 16km terrain
            512,       // high resolution base
            true       // tileable edges
        });

    // Apply procedural heightmap
    terrain->grid().applyDiamondSquare({
        150.0F,  // ±150m mountains
        0.6F,    // rougher terrain
        7        // consistent seed
    });

    // Tune LOD for large terrain
    terrain->setLODDistanceParameters(0.1F, 2.0F);

    // Optional: Custom rasterization
    EmEn::Graphics::RasterizationOptions rasterOpts;
    rasterOpts.setPolygonMode(VK_POLYGON_MODE_FILL);
    rasterOpts.setCullMode(VK_CULL_MODE_BACK_BIT);

    // Create renderable with material and options
    auto renderable = terrain->createRenderable(terrainMaterial, rasterOpts);
}
```

---

## Material Patterns

### Glass Material (Reflection + Refraction with Fresnel)

Create a realistic glass material with both reflection and refraction, using Fresnel effect to blend between them based on viewing angle:

```cpp
auto materialResource = resources.container<EmEn::Graphics::Material::StandardResource>()
    ->getOrCreateResourceWithCallback("GlassMaterial", [&cubemapTexture](auto & newMaterial)
{
    // Base colors: slight tint, high specular
    newMaterial.setAmbientComponent(Color< float >{0.02F, 0.02F, 0.02F, 1.0F});
    newMaterial.setDiffuseComponent(Color< float >{0.1F, 0.1F, 0.12F, 1.0F});
    newMaterial.setSpecularComponent(White, 128.0F);

    // Reflection: cubemap + amount (0-1)
    // Amount controls blend: 0 = no reflection, 1 = full reflection
    if ( !newMaterial.setReflectionComponent(cubemapTexture, 0.8F) )
    {
        return newMaterial.setManualLoadSuccess(false);
    }

    // Refraction: cubemap + IOR + amount
    // IOR: 1.0 = air, 1.33 = water, 1.5 = glass, 2.42 = diamond
    // Amount controls blend: 0 = no refraction, 1 = full refraction
    if ( !newMaterial.setRefractionComponent(cubemapTexture, 1.5F, 0.95F) )
    {
        return newMaterial.setManualLoadSuccess(false);
    }

    return newMaterial.setManualLoadSuccess(true);
});
```

**How Fresnel blending works:**

When BOTH reflection AND refraction are present:
1. `fresnelFactor` is auto-generated using Schlick approximation
2. At **grazing angles** (looking at edge): More reflection (fresnelFactor -> 1)
3. At **perpendicular angles** (looking straight): More refraction (fresnelFactor -> 0)
4. Final color = `mix(refracted, reflected, fresnelFactor) * amounts`

**Important constraints:**
- IOR is clamped to [1.0, 3.0] - values below 1.0 become 1.0
- Both components must use a cubemap texture
- Amount=0 effectively disables the component

### Reflection-Only Material (Chrome/Mirror)

```cpp
newMaterial.setAmbientComponent(Color< float >{0.02F, 0.02F, 0.02F, 1.0F});
newMaterial.setDiffuseComponent(Color< float >{0.8F, 0.8F, 0.8F, 1.0F});
newMaterial.setSpecularComponent(White, 256.0F);

// High reflection amount for mirror effect
newMaterial.setReflectionComponent(cubemapTexture, 0.95F);
```

### Refraction-Only Material (Water surface)

```cpp
newMaterial.setAmbientComponent(Color< float >{0.0F, 0.02F, 0.04F, 1.0F});
newMaterial.setDiffuseComponent(Color< float >{0.1F, 0.3F, 0.4F, 0.9F});  // Slight transparency
newMaterial.setSpecularComponent(White, 64.0F);

// Water IOR (1.33) with moderate refraction
newMaterial.setRefractionComponent(cubemapTexture, 1.33F, 0.7F);
```

### Debugging Material UBO Values

To debug material property values at runtime:

```cpp
// In StandardResource.cpp, add trace in updateVideoMemory():
TraceInfo{ClassId} <<
    "Material '" << this->name() << "':" "\n"
    "  UBO Index = " << m_sharedUBOIndex << "\n"
    "  reflectionAmount[20] = " << m_materialProperties[ReflectionAmountOffset] << "\n"
    "  refractionAmount[21] = " << m_materialProperties[RefractionAmountOffset] << "\n"
    "  refractionIOR[22] = " << m_materialProperties[RefractionIOROffset];
```

### Material Property Offsets (Quick Reference)

| Offset | Property | Setter Method |
|--------|----------|---------------|
| 20 | reflectionAmount | `setReflectionComponent(texture, amount)` |
| 21 | refractionAmount | `setRefractionComponent(texture, ior, amount)` |
| 22 | refractionIOR | `setRefractionIOR(ior)` or via `setRefractionComponent` |
| 23 | heightScale | `setHeightScale(scale)` or via `setHeightComponent` |

### PBR Material (PBRResource)

Create physically-based materials with metallic-roughness workflow:

```cpp
auto pbrMaterial = resources.container<EmEn::Graphics::Material::PBRResource>()
    ->getOrCreateResourceWithCallback("GoldPBR", [&cubemap](auto & mat)
{
    // Albedo: base color (gold tint)
    mat.setAlbedoComponent(Color<float>{1.0F, 0.843F, 0.0F, 1.0F});

    // Roughness: 0.0 = mirror, 1.0 = rough
    mat.setRoughnessComponent(0.3F);

    // Metalness: 0.0 = dielectric, 1.0 = metal
    mat.setMetalnessComponent(1.0F);

    // Optional: Normal map for surface detail
    mat.setNormalComponent(normalTexture);

    // IBL (Image-Based Lighting) from cubemap
    mat.setReflectionComponent(cubemap, 1.0F);

    // IBL intensity control (0.0-1.0)
    mat.setIBLIntensity(1.0F);

    return mat.setManualLoadSuccess(true);
});
```

### Material JSON Format (Unified)

Materials can be loaded from JSON files with a unified format supporting Basic, Standard, and PBR:

```json
{
    "Ambient": { "Type": "Texture", "Data": { "Name": "Category/TextureName" } },
    "Diffuse": { "Type": "Texture", "Data": { "Name": "Category/TextureName" } },
    "Specular": { "Type": "Color", "Data": [0.5, 0.5, 0.5], "Shininess": 0.5 },
    "Albedo": { "Type": "Texture", "Data": { "Name": "Category/TextureName" } },
    "Roughness": { "Type": "Value", "Data": 0.5 },
    "Metalness": { "Type": "Value", "Data": 0.0 },
    "Normal": { "Type": "Texture", "Data": { "Name": "Category/TextureName-normal" }, "Scale": 1.0 },
    "AmbientOcclusion": { "Type": "None" },
    "Reflection": { "Type": "Automatic", "Amount": 0.1 },
    "Opacity": { "Type": "None" },
    "AutoIllumination": { "Type": "None" },
    "Height": { "Type": "None" }
}
```

**Height component with Parallax Occlusion Mapping:**
```json
{
    "Height": { "Type": "Texture", "Data": { "Name": "Walls/Bricks001-height" }, "Scale": 0.02 }
}
```
- `Scale`: Maximum parallax depth (default 0.02). Values above 0.05 tend to look exaggerated.
- Requires `EnableHighQuality = true` and `POMIterations > 0` (see Saphir AGENTS.md).

**FillingType values:**
| Type | Data Format | Description |
|------|-------------|-------------|
| `Value` | `float` | Single numeric value |
| `Color` | `[r, g, b]` or `[r, g, b, a]` | Color array |
| `Texture` | `{ "Name": "path" }` | 2D texture reference |
| `Cubemap` | `{ "Name": "path" }` | Cubemap texture |
| `Automatic` | (optional params) | Auto-configure (e.g., use scene environment cubemap) |
| `None` | (none) | Component disabled |

**Code references:**
- `Graphics/Types.hpp:FillingType` - Enum definition
- `Graphics/Material/Helpers.cpp:parseComponentBase()` - JSON parsing logic

**PBR UBO Layout (PBRResource):**

| Offset | Property | Range |
|--------|----------|-------|
| 0-3 | albedoColor | vec4 |
| 4 | roughness | 0-1 |
| 5 | metalness | 0-1 |
| 6 | normalScale | 0-1 |
| 7 | f0 | ~0.04 |
| 8 | ior | 1.0-3.0 |
| 9 | iblIntensity | 0-1 |
| 10 | autoIlluminationAmount | 0+ |
| 11 | aoIntensity | 0-1 |
| 12-15 | autoIlluminationColor | vec4 |
| 16 | clearCoatFactor | 0-1 |
| 17 | clearCoatRoughness | 0-1 |
| 18 | subsurfaceIntensity | 0-1 |
| 19 | subsurfaceRadius | 0+ |
| 20-23 | subsurfaceColor | vec4 |
| 24-27 | sheenColor | vec4 |
| 28 | sheenRoughness | 0-1 |
| 29 | anisotropy | -1 to 1 |
| 30 | anisotropyRotation | 0-1 |
| 31 | transmissionFactor | 0-1 |
| 32-35 | attenuationColor | vec4 |
| 36 | attenuationDistance | 0+ |
| 37 | thicknessFactor | 0+ |
| 38 | heightScale | 0+ (0.02) |
| 39 | iridescenceFactor | 0-1 (0.0) |
| 40 | iridescenceIOR | 1.0+ (1.3) |
| 41 | iridescenceThicknessMin | nm (100.0) |
| 42 | iridescenceThicknessMax | nm (400.0) |
| 43 | dispersion | 0+ (0.0) |
| 44-47 | specularColorFactor | vec4 (white) |
| 48 | emissiveStrength | 0+ (1.0) |
| 49 | clearCoatNormalScale | 0+ (1.0) |
| 50-51 | padding | reserved |

**Multi-pass rendering:**
- IBL contribution is computed in **ambient pass only**
- Light passes use Cook-Torrance BRDF (no IBL accumulation)
- F0 for metals: `mix(vec3(0.04), albedo, metalness)`

**Code references:**
- `PBRResource.hpp:IBLIntensityOffset` - UBO offset constant
- `PBRResource.cpp:setIBLIntensity()` - Dynamic IBL control
- `LightGenerator.cpp:generateAmbientFragmentShader()` - IBL in ambient pass

### PBR Clear Coat Material (Car Paint, Varnished Wood)

```cpp
auto material = resources.container<EmEn::Graphics::Material::PBRResource>()
    ->getOrCreateResourceWithCallback("CarPaint", [](auto & mat)
{
    mat.setAlbedoComponent(Color<float>{0.8F, 0.1F, 0.1F, 1.0F}); // Red paint
    mat.setRoughnessComponent(0.4F);
    mat.setMetalnessComponent(0.0F);
    mat.setClearCoatComponent(1.0F, 0.05F); // Full coat, near-mirror finish
    mat.setReflectionComponentFromEnvironmentCubemap();
    return mat.setManualLoadSuccess(true);
});
```

### PBR Clear Coat with Normal Map (Orange Peel, Swirl Marks)

```cpp
auto material = resources.container<EmEn::Graphics::Material::PBRResource>()
    ->getOrCreateResourceWithCallback("CarPaintDetailled", [&ccNormalTexture](auto & mat)
{
    mat.setAlbedoComponent(Color<float>{0.8F, 0.1F, 0.1F, 1.0F}); // Red paint
    mat.setRoughnessComponent(0.4F);
    mat.setMetalnessComponent(0.0F);
    mat.setClearCoatComponent(1.0F, 0.05F); // Full coat, near-mirror finish
    // Clear coat normal: dedicated normal map + scale (0.1-1.0 typical)
    mat.setClearCoatNormalComponent(ccNormalTexture, 0.3F);
    mat.setReflectionComponentFromEnvironmentCubemap();
    return mat.setManualLoadSuccess(true);
});
```

**Clear coat normal scale guide:**

| Value | Appearance |
|-------|-----------|
| 0.0 | No effect (same as no CC normal) |
| 0.1 | Very subtle micro-imperfections |
| 0.3 | Visible orange peel / swirl marks |
| 0.5 | Pronounced perturbation |
| 1.0 | Full normal map effect |

**Important:** The clear coat normal map should use high-frequency patterns (micro-bumps, noise) rather than large-scale features. Base normal mapping is independent and not required for clear coat normal to work.

**Code references:**
- `PBRResource.hpp:setClearCoatNormalComponent()` — Setter
- `PBRResource.hpp:ClearCoatNormalScaleOffset` — UBO offset 49
- `LightGenerator.PBR.cpp` — Ncc computation using fragment-local TBN

### PBR Subsurface Scattering Material (Skin, Wax, Marble)

```cpp
auto material = resources.container<EmEn::Graphics::Material::PBRResource>()
    ->getOrCreateResourceWithCallback("HumanSkin", [](auto & mat)
{
    mat.setAlbedoComponent(Color<float>{0.8F, 0.6F, 0.5F, 1.0F}); // Skin tone
    mat.setRoughnessComponent(0.5F);
    mat.setMetalnessComponent(0.0F);
    // SSS: intensity, radius, color tint
    mat.setSubsurfaceComponent(0.5F, 1.0F, Color<float>{1.0F, 0.2F, 0.1F, 1.0F});
    // Optional: thickness map for transmittance (ears, fingers)
    // mat.setSubsurfaceThicknessComponent(thicknessTexture);
    return mat.setManualLoadSuccess(true);
});
```

**SSS color presets:**
| Material | Color (R, G, B) |
|----------|----------------|
| Skin | (1.0, 0.2, 0.1) |
| Jade | (0.0, 0.8, 0.2) |
| Wax | (1.0, 0.8, 0.4) |
| Marble | (0.9, 0.9, 1.0) |
| Leaf | (0.2, 0.9, 0.1) |

### PBR Sheen Material (Fabric, Velvet, Wool)

```cpp
auto material = resources.container<EmEn::Graphics::Material::PBRResource>()
    ->getOrCreateResourceWithCallback("VelvetFabric", [](auto & mat)
{
    mat.setAlbedoComponent(Color<float>{0.15F, 0.02F, 0.02F, 1.0F}); // Deep red velvet
    mat.setRoughnessComponent(0.9F);
    mat.setMetalnessComponent(0.0F);
    // Sheen: color tint, roughness (0=satin, 1=wool)
    mat.setSheenComponent(Color<float>{0.8F, 0.3F, 0.3F, 1.0F}, 0.3F);
    return mat.setManualLoadSuccess(true);
});
```

**Sheen roughness guide:**
| Value | Appearance |
|-------|-----------|
| 0.0-0.2 | Satin, silk (tight highlights) |
| 0.3-0.5 | Velvet (soft retroreflection) |
| 0.6-0.8 | Denim, cotton |
| 0.9-1.0 | Wool, felt (very broad sheen) |

### PBR Anisotropic Material (Brushed Metal, Hair, Vinyl)

```cpp
auto material = resources.container<EmEn::Graphics::Material::PBRResource>()
    ->getOrCreateResourceWithCallback("BrushedSteel", [](auto & mat)
{
    mat.setAlbedoComponent(Color<float>{0.9F, 0.9F, 0.92F, 1.0F}); // Steel
    mat.setRoughnessComponent(0.3F);
    mat.setMetalnessComponent(1.0F);
    // Anisotropy: strength (-1..1), rotation (0..1)
    mat.setAnisotropyComponent(0.7F, 0.0F);
    // IMPORTANT: Metallic anisotropic materials need IBL for visible reflections
    mat.setReflectionComponentFromEnvironmentCubemap();
    return mat.setManualLoadSuccess(true);
});
```

**Anisotropy values guide:**
| Value | Effect |
|-------|--------|
| -1.0 | Maximum stretch perpendicular to tangent |
| 0.0 | Isotropic (standard GGX) |
| 0.5-0.7 | Brushed metal look |
| 1.0 | Maximum stretch along tangent |

> **Important:** Metallic materials (metalness=1.0) have zero diffuse (kD=0). Without environment cubemap reflections, only direct specular highlights are visible. Always call `setReflectionComponentFromEnvironmentCubemap()` for metallic anisotropic materials.

### Parallax Occlusion Mapping (Standard Material)

Add depth/relief illusion to flat surfaces using a height map:

```cpp
auto material = resources.container<EmEn::Graphics::Material::StandardResource>()
    ->getOrCreateResourceWithCallback("BrickWall", [](auto & mat)
{
    mat.setAmbientComponent(Color<float>{0.1F, 0.1F, 0.1F, 1.0F});
    mat.setDiffuseComponent("Walls/Bricks001-color");
    mat.setSpecularComponent(White, 32.0F);
    mat.setNormalComponent("Walls/Bricks001-normal");
    // POM: height map + scale (max depth of white pixels)
    mat.setHeightComponent("Walls/Bricks001-height", 0.02F);
    return mat.setManualLoadSuccess(true);
});
```

### Parallax Occlusion Mapping (PBR Material)

```cpp
auto material = resources.container<EmEn::Graphics::Material::PBRResource>()
    ->getOrCreateResourceWithCallback("BrickWallPBR", [](auto & mat)
{
    mat.setAlbedoComponent("Walls/Bricks001-color");
    mat.setRoughnessComponent(0.8F);
    mat.setMetalnessComponent(0.0F);
    mat.setNormalComponent("Walls/Bricks001-normal");
    // POM: height map + scale
    mat.setHeightComponent("Walls/Bricks001-height", 0.02F);
    mat.setReflectionComponentFromEnvironmentCubemap();
    return mat.setManualLoadSuccess(true);
});
```

**Height scale guide:**

| Value | Appearance |
|-------|-----------|
| 0.005 | Very subtle, barely noticeable |
| 0.01 | Subtle depth |
| 0.02 | Default — natural-looking relief |
| 0.03 | Pronounced depth |
| 0.05+ | Exaggerated — good for demos, too much for realism |

**Requirements:**
- `EnableHighQuality` must be `true` (per-fragment lighting needed)
- `POMIterations` must be > 0 (default: 16, set to 0 to disable globally)
- Height map texture (grayscale: white = high, black = low)
- Normal map recommended (POM displaces UVs, normal map provides surface detail)

**Runtime control:**
```cpp
material->setHeightScale(0.03F); // Change depth dynamically
```

---

## Related Documentation

- [`docs/graphics-system.md`](graphics-system.md) - Graphics system overview
- [`docs/render-targets.md`](render-targets.md) - Render target architecture
- [`docs/saphir-shader-system.md`](saphir-shader-system.md) - Shader generation
- [`docs/caution-points.md`](caution-points.md) - Critical warnings
- [`src/Graphics/AGENTS.md`](../src/Graphics/AGENTS.md) - Graphics subsystem
- [`src/Saphir/AGENTS.md`](../src/Saphir/AGENTS.md) - Shader system
