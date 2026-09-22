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

### Procedural CDLOD terrain with DiamondSquare

A `Graphics::Renderable::TerrainResource` owns the height grid (physics, spawn) and draws it through
`Graphics::Geometry::CDLODTerrainResource` — a shared patch displaced by a height clipmap
(`src/Graphics/AGENTS.md` § "Adaptive geometries"). The grid must be a power of two of cells and a
power-of-two multiple of the patch (64 quads by default); `loadDiamondSquare()` snaps the division up.

```cpp
#include "Graphics/Renderable/TerrainResource.hpp"

auto terrain = std::make_shared< EmEn::Graphics::Renderable::TerrainResource >(resources, "MyTerrain");

/* Optional, BEFORE loading: the CDLOD knobs (defaults = the measured ones of the `terrain` demo). */
EmEn::Graphics::Geometry::CDLODTerrainParameters parameters;
parameters.detailDistance = 384.0F; /* 1 m quads up to 384 m, 2 m up to 768 m, ... */
terrain->setCDLODParameters(parameters);

/* 16 km at 1 m, relief ±2000 m, Hurst 1.25 (the measured `terrain` exponent), one tile per 8 m. */
terrain->loadDiamondSquare(16384.0F, 16384, material, {.factor = 2000.0F, .roughness = 1.0F, .seed = 0, .hurst = 1.25F}, {}, 16384.0F * 0.125F);
```

- JSON keys (`TerrainResource::load(const Json::Value &)`): `GridSize`, `GridDivision`, `GridDetailDistance`,
  `GridPatchQuads`, `GridClipTexels`, plus the material and displacement keys.
- A ground of a few hundred metres takes `BasicGroundResource`: nothing streams at that size.
- Bench a terrain with its sun frozen (`terrain --demo-options 100000,25`); the wireframe option 2 shows
  the level rings, not a crack — judge seams solid.

## Material Patterns

### Glass Material (Reflection + Refraction with Fresnel)

Create a realistic glass material with both reflection and refraction, using Fresnel effect to blend between them based on viewing angle:

```cpp
auto materialResource = resources.container<EmEn::Graphics::Material::StandardResource>()
    ->getOrCreateResourceWithCallback("GlassMaterial", [&cubemapTexture](auto & newMaterial)
{
    // Base surface: slight tint, near-mirror dielectric
    newMaterial.setAlbedoComponent(Color< float >{0.1F, 0.1F, 0.12F, 1.0F});
    newMaterial.setRoughnessComponent(0.05F);
    newMaterial.setMetalnessComponent(0.0F);

    // Reflection: cubemap
    if ( !newMaterial.setReflectionComponent(cubemapTexture) )
    {
        return newMaterial.setManualLoadSuccess(false);
    }

    // Refraction: cubemap + IOR
    // IOR: 1.0 = air, 1.33 = water, 1.5 = glass, 2.42 = diamond
    if ( !newMaterial.setRefractionComponent(cubemapTexture, 1.5F) )
    {
        return newMaterial.setManualLoadSuccess(false);
    }

    // Optional artistic overrides of the physical blend (neutral = 1.0)
    // 0 = component disabled, 1 = fully BRDF/Fresnel-controlled
    newMaterial.setReflectionAmount(0.8F);
    newMaterial.setRefractionAmount(0.95F);

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
- Both components must use a cubemap texture (explicit, render target, or the scene environment cubemap)
- Amount=0 effectively disables the component; the neutral default is 1.0 (fully physical blend)

### Reflection-Only Material (Chrome/Mirror)

```cpp
newMaterial.setAlbedoComponent(Color< float >{0.8F, 0.8F, 0.8F, 1.0F});
newMaterial.setRoughnessComponent(0.02F);
newMaterial.setMetalnessComponent(1.0F);

// Cubemap reflection, slightly damped for a mirror effect
newMaterial.setReflectionComponent(cubemapTexture);
newMaterial.setReflectionAmount(0.95F);
```

### Refraction-Only Material (Water surface)

```cpp
newMaterial.setAlbedoComponent(Color< float >{0.1F, 0.3F, 0.4F, 0.9F});  // Slight transparency
newMaterial.setRoughnessComponent(0.1F);
newMaterial.setMetalnessComponent(0.0F);

// Water IOR (1.33) with moderate refraction
newMaterial.setRefractionComponent(cubemapTexture, 1.33F);
newMaterial.setRefractionAmount(0.7F);
```

### Debugging Material UBO Values

To debug material property values at runtime:

```cpp
// In StandardResource.cpp, add trace in updateVideoMemory():
TraceInfo{ClassId} <<
    "Material '" << this->name() << "':" "\n"
    "  UBO Index = " << m_sharedUBOIndex << "\n"
    "  reflectionAmount[52] = " << m_materialProperties[ReflectionAmountOffset] << "\n"
    "  refractionAmount[53] = " << m_materialProperties[RefractionAmountOffset] << "\n"
    "  ior[8] = " << m_materialProperties[IOROffset];
```

### Material Property Offsets (Quick Reference)

| Offset | Property | Setter Method |
|--------|----------|---------------|
| 8 | ior | `setIOR(ior)` or via `setRefractionComponent` |
| 38 | heightScale | `setHeightScale(scale)` or via `setHeightComponent` |
| 52 | reflectionAmount | `setReflectionAmount(amount)` (artistic override, neutral 1.0) |
| 53 | refractionAmount | `setRefractionAmount(amount)` (artistic override, neutral 1.0) |

### PBR Material (StandardResource)

Create physically-based materials with metallic-roughness workflow:

```cpp
auto pbrMaterial = resources.container<EmEn::Graphics::Material::StandardResource>()
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
    mat.setReflectionComponent(cubemap);

    // IBL intensity control (0.0-1.0)
    mat.setIBLIntensity(1.0F);

    return mat.setManualLoadSuccess(true);
});
```

### Material JSON Format (Unified)

Materials can be loaded from JSON files with a unified format supporting Basic and Standard:

```json
{
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

**Legacy keys (still accepted as fallbacks):**

| Legacy key | Mapped to | Note |
|------------|-----------|------|
| `"Diffuse"` | `"Albedo"` | Tried only when `"Albedo"` is absent |
| `"Specular"` | `"Roughness"` | Tried only when `"Roughness"` is absent; a `Value`/`Texture` is INVERTED (high specular = low roughness) |
| `"Shininess"` (on `"Specular"`) | `roughness = 1 - glossiness` | ⚠️ `"Shininess"` is an authored **GLOSSINESS in [0,1]**, never a Phong exponent |

> **Note:** There is no `"Ambient"` component any more — ambient occlusion (`"AmbientOcclusion"`)
> plus IBL replace it. The scene manifest material `"Type"` accepts `"Standard"` and `"PBR"`
> as synonyms for the same, single lit material.

**Height component with Parallax Occlusion Mapping:**
```json
{
    "Height": { "Type": "Texture", "Data": { "Name": "Walls/Bricks001-height" }, "Scale": 0.02 }
}
```
- `Scale`: Maximum parallax depth **in UV units** (default 0.02). Values above 0.05 tend to look exaggerated.
  ⚠️ ~90 store materials carry `1.0` (authored before the PBR material): a relief one repeat deep.
- The layer count comes from `Core/Graphics/Texture/POMIterations` (default 0 = off) unless the
  material calls `setParallaxIterations()` (see `src/Graphics/AGENTS.md` § Parallax Occlusion Mapping).

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

**UBO Layout (StandardResource, 80 floats = 320 bytes):**

| Offset | Property | Range |
|--------|----------|-------|
| 0-3 | albedoColor | vec4 |
| 4 | roughness | 0-1 |
| 5 | metalness | 0-1 |
| 6 | normalScale | 0-1 |
| 7 | specularFactor | 0-1 (1.0) |
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
| 50 | opacity | 0-1 (1.0) |
| 51 | alphaThreshold | 0-1 (alpha-test cutoff) |
| 52 | reflectionAmount | 0-1 (1.0 = BRDF-controlled) |
| 53 | refractionAmount | 0-1 (1.0 = Fresnel-controlled) |
| 54-55 | padding | STD140 padding |
| 56-79 | per-component UVW transforms | vec4 × 6 (scale.xy, offset.zw), neutral (1,1,0,0) |

**Multi-pass rendering:**
- IBL contribution is computed in **ambient pass only**
- Light passes use Cook-Torrance BRDF (no IBL accumulation)
- F0 for metals: `mix(vec3(0.04), albedo, metalness)`

**Code references:**
- `StandardResource.hpp:IBLIntensityOffset` - UBO offset constant
- `StandardResource.cpp:setIBLIntensity()` - Dynamic IBL control
- `LightGenerator.cpp:generateAmbientFragmentShader()` - IBL in ambient pass

### PBR Clear Coat Material (Car Paint, Varnished Wood)

```cpp
auto material = resources.container<EmEn::Graphics::Material::StandardResource>()
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
auto material = resources.container<EmEn::Graphics::Material::StandardResource>()
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
- `StandardResource.hpp:setClearCoatNormalComponent()` — Setter
- `StandardResource.hpp:ClearCoatNormalScaleOffset` — UBO offset 49
- `LightGenerator.PBR.cpp` — Ncc computation using fragment-local TBN

### PBR Subsurface Scattering Material (Skin, Wax, Marble)

```cpp
auto material = resources.container<EmEn::Graphics::Material::StandardResource>()
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
auto material = resources.container<EmEn::Graphics::Material::StandardResource>()
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
auto material = resources.container<EmEn::Graphics::Material::StandardResource>()
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
    mat.setAlbedoComponent("Walls/Bricks001-color");
    mat.setRoughnessComponent(0.8F);
    mat.setMetalnessComponent(0.0F);
    mat.setNormalComponent("Walls/Bricks001-normal");
    // POM: height map + scale (max depth of white pixels)
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
- A layer count: `mat.setParallaxIterations(32)`, or `Core/Graphics/Texture/POMIterations` > 0 for the
  materials that set none (default 0 = the height map is ignored)
- Height map texture (grayscale: white = high, black = low) that is a real HEIGHT — ⚠️ a map derived from
  the photo's luminance extrudes its grain into spikes (`src/Graphics/AGENTS.md` § Parallax Occlusion Mapping)
- Normal map recommended (POM displaces UVs, normal map provides surface detail)

**Runtime control:**
```cpp
material->setHeightScale(0.03F); // Change depth dynamically
material->setParallaxIterations(32); // Layers at a grazing view (UBO value, 0..64)
material->setParallaxFadeDistances(8.0F, 18.0F); // Full relief closer, no march beyond
```

---

## Related Documentation

- [`docs/graphics-system.md`](graphics-system.md) - Graphics system overview
- [`docs/render-targets.md`](render-targets.md) - Render target architecture
- [`docs/saphir-shader-system.md`](saphir-shader-system.md) - Shader generation
- [`docs/caution-points.md`](caution-points.md) - Critical warnings
- [`src/Graphics/AGENTS.md`](../src/Graphics/AGENTS.md) - Graphics subsystem
- [`src/Saphir/AGENTS.md`](../src/Saphir/AGENTS.md) - Shader system
