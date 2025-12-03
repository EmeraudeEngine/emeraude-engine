# Saphir 2.0 - Reflective Architecture Proposal

**Status:** Proposal
**Date:** 2025-11-26
**Author:** LondNoir + Claude

## Objectives

1. Make Saphir more **reflective** - able to query Materials and Geometries rather than receiving generated code
2. Allow injection of **custom shaders** that can be analyzed
3. Simplify creation of new Material types
4. Improve validation and debugging

---

## Problems with Current Architecture

### 1. Material as Code Generator

Currently, each Material directly implements generation:

```cpp
class Interface {
    virtual bool generateVertexShaderCode(...) = 0;
    virtual bool generateFragmentShaderCode(...) = 0;
    virtual bool setupLightGenerator(...) = 0;
};
```

**Problems:**
- Saphir cannot "see" what a Material does
- Logic duplication between BasicResource and StandardResource
- Difficult to optimize or validate generated code
- Impossible to inject custom GLSL

### 2. Current Flow

```
Material::Interface ──────────────┐
  • generateVertexShaderCode()    │
  • generateFragmentShaderCode()  │
  • setupLightGenerator()         ├──► SceneRendering ──► Program ──► GLSL
  • fragmentColor()               │        Generator
                                  │
Geometry::Interface ──────────────┤
  • vertex attributes             │
                                  │
LightGenerator ───────────────────┘
  • declareSurface*()
  • generate*ShaderCode()
```

Material is a **black box** that produces code. Saphir assembles but does not understand.

---

## Proposal: Reflective Architecture

### Central Concept: MaterialDescriptor

A Material no longer generates code - it **declares** its properties via a semantic descriptor:

```cpp
struct TextureSlot
{
    std::string samplerName;           // GLSL sampler name
    uint32_t uvChannel = 0;            // UV channel (0 = primary, 1 = secondary)
    bool uses3DCoordinates = false;    // Cubemap or 3D texture
    std::optional<std::string> scale;  // Optional scale variable
};

struct MaterialDescriptor
{
    // === Surface properties (optional textures) ===
    std::optional<TextureSlot> albedo;          // diffuse/base color
    std::optional<TextureSlot> normal;          // normal map
    std::optional<TextureSlot> roughness;       // roughness (PBR)
    std::optional<TextureSlot> metallic;        // metallic (PBR)
    std::optional<TextureSlot> ao;              // ambient occlusion
    std::optional<TextureSlot> emissive;        // self-illumination
    std::optional<TextureSlot> opacity;         // alpha/transparency
    std::optional<TextureSlot> displacement;    // height/displacement map

    // === Constant colors/values (if no texture) ===
    std::optional<Color> albedoColor;
    std::optional<Color> emissiveColor;
    float roughnessValue = 0.5f;
    float metallicValue = 0.0f;
    float opacityValue = 1.0f;
    float normalScale = 1.0f;

    // === Behavior ===
    ShadingModel shadingModel = ShadingModel::PBR;  // PBR, Phong, Gouraud, Unlit
    BlendingMode blending = BlendingMode::None;
    bool doubleSided = false;
    bool alphaTest = false;
    float alphaCutoff = 0.5f;
    bool receivesShadows = true;

    // === Requirements inferred automatically ===
    [[nodiscard]] GeometryRequirements inferRequirements() const noexcept;
};
```

### New Generation Flow

```
Material ──► MaterialDescriptor ──┐
                                  │
Geometry ──► GeometryCapabilities ├──► Saphir::Analyzer ──► GLSL
                                  │         │
CustomShader ──► ShaderFragment ──┘         ├── Infer geometry requirements
                   (new)                    ├── Validate compatibility
                                            ├── Select shading model
                                            └── Generate optimal code
```

**Saphir becomes intelligent:**
- It **reads** the MaterialDescriptor
- It **infers** requirements (normals, tangents, UVs...)
- It **validates** against GeometryCapabilities
- It **generates** optimal code

---

## Custom Shader Support

### Concept: ShaderFragment

An analyzable GLSL fragment with metadata:

```cpp
class ShaderFragment
{
public:
    // === Parsing ===
    [[nodiscard]] static
    std::optional<ShaderFragment> parse(const std::string& glslCode) noexcept;

    [[nodiscard]] static
    std::optional<ShaderFragment> loadFromFile(const std::string& path) noexcept;

    // === Introspection ===

    // What the fragment REQUIRES (inputs)
    [[nodiscard]] const std::vector<ShaderInput>& inputs() const noexcept;

    // What the fragment PRODUCES (outputs)
    [[nodiscard]] const std::vector<ShaderOutput>& outputs() const noexcept;

    // Uniforms/samplers used
    [[nodiscard]] const std::vector<UniformRequirement>& uniforms() const noexcept;

    // Functions defined
    [[nodiscard]] const std::vector<FunctionSignature>& functions() const noexcept;

    // === Code ===
    [[nodiscard]] const std::string& code() const noexcept;

    // === Validation ===
    [[nodiscard]] bool isCompatibleWith(const GeometryCapabilities& geo) const noexcept;
    [[nodiscard]] std::vector<std::string> getMissingRequirements(const GeometryCapabilities& geo) const noexcept;
};
```

### GLSL Syntax with Saphir Pragmas

```glsl
// water_surface.glsl
#pragma saphir version 1
#pragma saphir type fragment

// Required inputs declaration
#pragma saphir input vec3 worldPosition
#pragma saphir input vec3 worldNormal
#pragma saphir input vec2 texCoord0

// Produced outputs declaration
#pragma saphir output vec4 finalColor

// Custom uniforms declaration
#pragma saphir uniform sampler2D waterNormalMap
#pragma saphir uniform sampler2D foamTexture
#pragma saphir uniform float time
#pragma saphir uniform float waveScale
#pragma saphir uniform float waveSpeed
#pragma saphir uniform vec3 waterColor
#pragma saphir uniform float opacity

// Custom fragment entry point
void saphir_fragment()
{
    // UV animation
    vec2 animatedUV = texCoord0 + vec2(time * waveSpeed);
    vec2 distortedUV = animatedUV + sin(worldPosition.xz * waveScale) * 0.02;

    // Normal mapping with two layers
    vec3 normal1 = texture(waterNormalMap, distortedUV).xyz * 2.0 - 1.0;
    vec3 normal2 = texture(waterNormalMap, distortedUV * 0.5 + 0.3).xyz * 2.0 - 1.0;
    vec3 combinedNormal = normalize(normal1 + normal2);

    // Fresnel effect
    vec3 viewDir = normalize(cameraPosition - worldPosition);
    float fresnel = pow(1.0 - max(dot(viewDir, worldNormal), 0.0), 3.0);

    // Foam at edges (example)
    float foam = texture(foamTexture, texCoord0 * 4.0).r;

    // Final color
    vec3 color = mix(waterColor, vec3(1.0), foam * 0.3);
    color = mix(color, vec3(0.8, 0.9, 1.0), fresnel * 0.5);

    finalColor = vec4(color, opacity);
}
```

### Saphir Analysis

When Saphir parses this file:

1. **Metadata extraction:**
   - Required inputs: `worldPosition`, `worldNormal`, `texCoord0`
   - Produced outputs: `finalColor`
   - Uniforms: 7 declared

2. **Validation against Geometry:**
   ```
   Geometry provides: [position, normal, uv0, tangent]
   Fragment requires: [worldPosition, worldNormal, texCoord0]
   OK Compatible
   ```

3. **Pipeline integration:**
   - Generates standard vertex shader (transformations)
   - Injects uniforms into descriptor set
   - Assembles `saphir_fragment()` into `main()`

---

## Revised Material Interface

```cpp
class Interface : public Resources::ResourceTrait
{
public:
    // === NEW: Reflective API ===

    /**
     * @brief Returns the semantic descriptor of the material.
     * @note Saphir will use this descriptor to generate code.
     */
    [[nodiscard]]
    virtual MaterialDescriptor descriptor() const noexcept = 0;

    /**
     * @brief Returns an optional custom fragment shader.
     * @note If present, Saphir integrates it instead of generating.
     */
    [[nodiscard]]
    virtual std::optional<ShaderFragment> customFragment() const noexcept
    {
        return std::nullopt;  // By default, Saphir generates everything
    }

    /**
     * @brief Returns an optional custom vertex shader.
     * @note Rare, but useful for special effects (vertex displacement).
     */
    [[nodiscard]]
    virtual std::optional<ShaderFragment> customVertex() const noexcept
    {
        return std::nullopt;
    }

    // === DEPRECATED: Old API (backward compatibility) ===

    [[deprecated("Use descriptor() instead")]]
    [[nodiscard]]
    virtual bool generateVertexShaderCode(
        Saphir::Generator::Abstract & generator,
        Saphir::VertexShader & vertexShader
    ) const noexcept;

    [[deprecated("Use descriptor() instead")]]
    [[nodiscard]]
    virtual bool generateFragmentShaderCode(
        Saphir::Generator::Abstract & generator,
        Saphir::LightGenerator & lightGenerator,
        Saphir::FragmentShader & fragmentShader
    ) const noexcept;

    [[deprecated("Use descriptor() instead")]]
    [[nodiscard]]
    virtual bool setupLightGenerator(Saphir::LightGenerator & lightGenerator) const noexcept;
};
```

---

## Saphir::Analyzer - The New Core

```cpp
namespace Saphir
{
    class Analyzer
    {
    public:
        /**
         * @brief Analyzes a MaterialDescriptor and generates shader code.
         */
        [[nodiscard]]
        GenerationResult analyze(
            const MaterialDescriptor& material,
            const GeometryCapabilities& geometry,
            const SceneContext& scene
        ) noexcept;

        /**
         * @brief Analyzes a custom ShaderFragment.
         */
        [[nodiscard]]
        ValidationResult validate(
            const ShaderFragment& fragment,
            const GeometryCapabilities& geometry
        ) noexcept;

        /**
         * @brief Generates final code by combining descriptor + custom fragment.
         */
        [[nodiscard]]
        GenerationResult generate(
            const MaterialDescriptor& material,
            const std::optional<ShaderFragment>& customFragment,
            const GeometryCapabilities& geometry,
            const SceneContext& scene
        ) noexcept;
    };

    struct GenerationResult
    {
        bool success;
        std::string vertexShaderCode;
        std::string fragmentShaderCode;
        std::vector<std::string> warnings;
        std::vector<std::string> errors;

        // Statistics for debugging
        struct Stats {
            uint32_t textureCount;
            uint32_t uniformCount;
            uint32_t inputCount;
            bool usesNormalMapping;
            bool usesPBR;
            bool usesCustomFragment;
        } stats;
    };

    struct ValidationResult
    {
        bool compatible;
        std::vector<std::string> missingInputs;
        std::vector<std::string> warnings;
    };
}
```

---

## Before/After Comparison

| Aspect | Saphir 1.x | Saphir 2.0 |
|--------|------------|------------|
| **Reflection** | Material = black box | Introspectable descriptor |
| **Custom shaders** | Impossible | Injectable GLSL fragments |
| **Validation** | Runtime (potential crash) | Compile-time + clear logs |
| **New Material** | Implement 4 methods | Fill a descriptor |
| **Debugging** | Difficult | Readable descriptor + stats |
| **Optimization** | Manual | Auto dead code elimination |
| **Learning curve** | Understand entire architecture | Fill a struct |

---

## Implementation Plan

### Phase 1: MaterialDescriptor (Foundation)

- [ ] Create `Saphir/MaterialDescriptor.hpp`
- [ ] Define `TextureSlot`, `ShadingModel`, etc. structures
- [ ] Implement `inferRequirements()`
- [ ] Unit tests

### Phase 2: Saphir::Analyzer (Core)

- [ ] Create `Saphir/Analyzer.hpp/.cpp`
- [ ] Implement generation from MaterialDescriptor
- [ ] Support models: Unlit, Phong, PBR
- [ ] Integrate with existing LightGenerator
- [ ] Tests with simple Materials

### Phase 3: ShaderFragment (Custom Shaders)

- [ ] Define `#pragma saphir` pragma syntax
- [ ] Implement lightweight GLSL parser
- [ ] Create `Saphir/ShaderFragment.hpp/.cpp`
- [ ] Input/output validation
- [ ] Pipeline injection
- [ ] Tests with custom shaders

### Phase 4: BasicResource/StandardResource Migration

- [ ] Add `descriptor()` to BasicResource
- [ ] Add `descriptor()` to StandardResource
- [ ] Route SceneRendering to Analyzer if `descriptor()` available
- [ ] Maintain backward compatibility with old methods
- [ ] Non-regression tests

### Phase 5: Deprecation and Cleanup

- [ ] Mark old methods `[[deprecated]]`
- [ ] Migrate all internal code to descriptors
- [ ] Document new API
- [ ] Update `docs/saphir-shader-system.md`
- [ ] Remove legacy code (future version)

---

## Open Questions

1. **Pragma syntax** - `#pragma saphir` or different format (JSON header, GLSL attributes)?

2. **Custom level** - Allow custom vertex or only fragment?

3. **Hot-reload** - Reload ShaderFragments on the fly during development?

4. **Material inheritance** - Can a Material inherit from another descriptor?

5. **Compute shaders** - Integrate in same system or separate?

---

## References

- Current architecture: `docs/saphir-shader-system.md`
- Material interface: `src/Graphics/Material/Interface.hpp`
- Generators: `src/Saphir/Generator/`
- LightGenerator: `src/Saphir/LightGenerator.hpp`
