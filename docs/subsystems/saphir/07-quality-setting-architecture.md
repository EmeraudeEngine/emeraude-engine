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
- Parallax Occlusion Mapping (if material has a Height component; its layer count is a UBO value)

When disabled:
- Per-vertex lighting (Gouraud shading)
- No normal mapping
- Simplified reflection/refraction

### POM Iterations Setting

`GraphicsTexturePOMIterationsKey` (`Core/Graphics/Texture/POMIterations`, **default: 0**) is the POM
layer count of every material with a height map that does not set its own
(`StandardResource::setParallaxIterations()`). It is read **by the material, at its creation**, and
written to its UBO (`parallaxParameters.x`) — it is **not a generator input any more** (2026-09-22):
`Generator::Abstract::pomIterations()`/`setPOMIterations()` are deleted, and the POM code is generated
for every material with a Height component. Changing the setting needs a relaunch (materials are
created once), never a shader-cache purge.

| Value | Effect |
|-------|--------|
| `0` | The march is skipped: `pomTexCoords` = the mesh UVs, bit-exact (plain normal mapping) |
| `8-16` | Visible layers at grazing angles (the bisection refinement hides most of it) |
| `32` | The `relief` demo's default |
| `64` | The compile-time bound of the loop (`StandardResource::MaxParallaxIterations`) |

> [!WARNING]
> Until 2026-09-22 the count was a GLSL literal baked by the generator and gated the codegen, and it did
> **not** enter `computeProgramCacheKey()`: a program built under one count served the next materials and,
> through the on-disk SPIR-V cache, the next launches. A per-material value belongs in the material UBO,
> never in a literal — the program cache keys on layouts and flags, not values.

**Code references:**
- `SettingKeys.hpp:GraphicsTexturePOMIterationsKey` — Setting key definition
- `StandardResource.cpp:create()` — resolution into the UBO
- `StandardResource.cpp:generateFragmentShaderCode()` — the march (details: `src/Graphics/AGENTS.md` § Parallax Occlusion Mapping)

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
