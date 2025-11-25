# Saphir 2.0 - Proposition d'Architecture Réflexive

**Statut:** Proposition
**Date:** 2025-11-26
**Auteur:** LondNoir + Claude

## Objectifs

1. Rendre Saphir plus **réflexif** — capable d'interroger les Materials et Geometries plutôt que de recevoir du code généré
2. Permettre l'injection de **shaders custom** analysables
3. Simplifier la création de nouveaux types de Materials
4. Améliorer la validation et le debugging

---

## Problèmes de l'Architecture Actuelle

### 1. Material comme générateur de code

Actuellement, chaque Material implémente directement la génération:

```cpp
class Interface {
    virtual bool generateVertexShaderCode(...) = 0;
    virtual bool generateFragmentShaderCode(...) = 0;
    virtual bool setupLightGenerator(...) = 0;
};
```

**Problèmes:**
- Saphir ne peut pas "voir" ce que fait un Material
- Duplication de logique entre BasicResource et StandardResource
- Difficile d'optimiser ou valider le code généré
- Impossible d'injecter du GLSL custom

### 2. Flux actuel

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

Le Material est une **boîte noire** qui produit du code. Saphir assemble mais ne comprend pas.

---

## Proposition: Architecture Réflexive

### Concept Central: MaterialDescriptor

Un Material ne génère plus de code — il **déclare** ses propriétés via un descripteur sémantique:

```cpp
struct TextureSlot
{
    std::string samplerName;           // Nom GLSL du sampler
    uint32_t uvChannel = 0;            // Canal UV (0 = primaire, 1 = secondaire)
    bool uses3DCoordinates = false;    // Cubemap ou 3D texture
    std::optional<std::string> scale;  // Variable de scale optionnelle
};

struct MaterialDescriptor
{
    // === Propriétés de surface (textures optionnelles) ===
    std::optional<TextureSlot> albedo;          // diffuse/base color
    std::optional<TextureSlot> normal;          // normal map
    std::optional<TextureSlot> roughness;       // roughness (PBR)
    std::optional<TextureSlot> metallic;        // metallic (PBR)
    std::optional<TextureSlot> ao;              // ambient occlusion
    std::optional<TextureSlot> emissive;        // self-illumination
    std::optional<TextureSlot> opacity;         // alpha/transparency
    std::optional<TextureSlot> displacement;    // height/displacement map

    // === Couleurs/valeurs constantes (si pas de texture) ===
    std::optional<Color> albedoColor;
    std::optional<Color> emissiveColor;
    float roughnessValue = 0.5f;
    float metallicValue = 0.0f;
    float opacityValue = 1.0f;
    float normalScale = 1.0f;

    // === Comportement ===
    ShadingModel shadingModel = ShadingModel::PBR;  // PBR, Phong, Gouraud, Unlit
    BlendingMode blending = BlendingMode::None;
    bool doubleSided = false;
    bool alphaTest = false;
    float alphaCutoff = 0.5f;
    bool receivesShadows = true;

    // === Requirements inférés automatiquement ===
    [[nodiscard]] GeometryRequirements inferRequirements() const noexcept;
};
```

### Nouveau Flux de Génération

```
Material ──► MaterialDescriptor ──┐
                                  │
Geometry ──► GeometryCapabilities ├──► Saphir::Analyzer ──► GLSL
                                  │         │
CustomShader ──► ShaderFragment ──┘         ├── Infer geometry requirements
                   (nouveau)                ├── Validate compatibility
                                            ├── Select shading model
                                            └── Generate optimal code
```

**Saphir devient intelligent:**
- Il **lit** le MaterialDescriptor
- Il **infère** les requirements (normals, tangents, UVs...)
- Il **valide** contre GeometryCapabilities
- Il **génère** le code optimal

---

## Support des Custom Shaders

### Concept: ShaderFragment

Un morceau de GLSL analysable avec métadonnées:

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

    // Ce que le fragment REQUIERT (inputs)
    [[nodiscard]] const std::vector<ShaderInput>& inputs() const noexcept;

    // Ce que le fragment PRODUIT (outputs)
    [[nodiscard]] const std::vector<ShaderOutput>& outputs() const noexcept;

    // Uniforms/samplers utilisés
    [[nodiscard]] const std::vector<UniformRequirement>& uniforms() const noexcept;

    // Fonctions définies
    [[nodiscard]] const std::vector<FunctionSignature>& functions() const noexcept;

    // === Code ===
    [[nodiscard]] const std::string& code() const noexcept;

    // === Validation ===
    [[nodiscard]] bool isCompatibleWith(const GeometryCapabilities& geo) const noexcept;
    [[nodiscard]] std::vector<std::string> getMissingRequirements(const GeometryCapabilities& geo) const noexcept;
};
```

### Syntaxe GLSL avec Pragmas Saphir

```glsl
// water_surface.glsl
#pragma saphir version 1
#pragma saphir type fragment

// Déclaration des inputs requis
#pragma saphir input vec3 worldPosition
#pragma saphir input vec3 worldNormal
#pragma saphir input vec2 texCoord0

// Déclaration des outputs produits
#pragma saphir output vec4 finalColor

// Déclaration des uniforms custom
#pragma saphir uniform sampler2D waterNormalMap
#pragma saphir uniform sampler2D foamTexture
#pragma saphir uniform float time
#pragma saphir uniform float waveScale
#pragma saphir uniform float waveSpeed
#pragma saphir uniform vec3 waterColor
#pragma saphir uniform float opacity

// Point d'entrée du fragment custom
void saphir_fragment()
{
    // Animation des UVs
    vec2 animatedUV = texCoord0 + vec2(time * waveSpeed);
    vec2 distortedUV = animatedUV + sin(worldPosition.xz * waveScale) * 0.02;

    // Normal mapping avec deux couches
    vec3 normal1 = texture(waterNormalMap, distortedUV).xyz * 2.0 - 1.0;
    vec3 normal2 = texture(waterNormalMap, distortedUV * 0.5 + 0.3).xyz * 2.0 - 1.0;
    vec3 combinedNormal = normalize(normal1 + normal2);

    // Fresnel effect
    vec3 viewDir = normalize(cameraPosition - worldPosition);
    float fresnel = pow(1.0 - max(dot(viewDir, worldNormal), 0.0), 3.0);

    // Foam at edges (exemple)
    float foam = texture(foamTexture, texCoord0 * 4.0).r;

    // Couleur finale
    vec3 color = mix(waterColor, vec3(1.0), foam * 0.3);
    color = mix(color, vec3(0.8, 0.9, 1.0), fresnel * 0.5);

    finalColor = vec4(color, opacity);
}
```

### Analyse par Saphir

Quand Saphir parse ce fichier:

1. **Extraction des métadonnées:**
   - Inputs requis: `worldPosition`, `worldNormal`, `texCoord0`
   - Outputs produits: `finalColor`
   - Uniforms: 7 déclarés

2. **Validation contre Geometry:**
   ```
   Geometry provides: [position, normal, uv0, tangent]
   Fragment requires: [worldPosition, worldNormal, texCoord0]
   ✓ Compatible
   ```

3. **Intégration dans le pipeline:**
   - Génère le vertex shader standard (transformations)
   - Injecte les uniforms dans le descriptor set
   - Assemble `saphir_fragment()` dans `main()`

---

## Interface Material Révisée

```cpp
class Interface : public Resources::ResourceTrait
{
public:
    // === NOUVEAU: API Réflexive ===

    /**
     * @brief Retourne le descripteur sémantique du material.
     * @note Saphir utilisera ce descripteur pour générer le code.
     */
    [[nodiscard]]
    virtual MaterialDescriptor descriptor() const noexcept = 0;

    /**
     * @brief Retourne un fragment shader custom optionnel.
     * @note Si présent, Saphir l'intègre au lieu de générer.
     */
    [[nodiscard]]
    virtual std::optional<ShaderFragment> customFragment() const noexcept
    {
        return std::nullopt;  // Par défaut, Saphir génère tout
    }

    /**
     * @brief Retourne un vertex shader custom optionnel.
     * @note Rare, mais utile pour effets spéciaux (vertex displacement).
     */
    [[nodiscard]]
    virtual std::optional<ShaderFragment> customVertex() const noexcept
    {
        return std::nullopt;
    }

    // === DEPRECATED: Ancienne API (rétro-compatibilité) ===

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

## Saphir::Analyzer - Le Nouveau Coeur

```cpp
namespace Saphir
{
    class Analyzer
    {
    public:
        /**
         * @brief Analyse un MaterialDescriptor et génère le code shader.
         */
        [[nodiscard]]
        GenerationResult analyze(
            const MaterialDescriptor& material,
            const GeometryCapabilities& geometry,
            const SceneContext& scene
        ) noexcept;

        /**
         * @brief Analyse un ShaderFragment custom.
         */
        [[nodiscard]]
        ValidationResult validate(
            const ShaderFragment& fragment,
            const GeometryCapabilities& geometry
        ) noexcept;

        /**
         * @brief Génère le code final en combinant descriptor + fragment custom.
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

        // Statistiques pour debugging
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

## Comparaison Avant/Après

| Aspect | Saphir 1.x | Saphir 2.0 |
|--------|------------|------------|
| **Réflexion** | Material = boîte noire | Descripteur introspectable |
| **Custom shaders** | Impossible | Fragments GLSL injectables |
| **Validation** | Runtime (crash potentiel) | Compile-time + logs clairs |
| **Nouveau Material** | Implémenter 4 méthodes | Remplir un descripteur |
| **Debugging** | Difficile | Descripteur lisible + stats |
| **Optimisation** | Manuelle | Dead code elimination auto |
| **Courbe d'apprentissage** | Comprendre toute l'archi | Remplir une struct |

---

## Plan d'Implémentation

### Phase 1: MaterialDescriptor (Foundation)

- [ ] Créer `Saphir/MaterialDescriptor.hpp`
- [ ] Définir les structures `TextureSlot`, `ShadingModel`, etc.
- [ ] Implémenter `inferRequirements()`
- [ ] Tests unitaires

### Phase 2: Saphir::Analyzer (Core)

- [ ] Créer `Saphir/Analyzer.hpp/.cpp`
- [ ] Implémenter génération depuis MaterialDescriptor
- [ ] Supporter les modèles: Unlit, Phong, PBR
- [ ] Intégrer avec LightGenerator existant
- [ ] Tests avec Materials simples

### Phase 3: ShaderFragment (Custom Shaders)

- [ ] Définir syntaxe des pragmas `#pragma saphir`
- [ ] Implémenter parser GLSL léger
- [ ] Créer `Saphir/ShaderFragment.hpp/.cpp`
- [ ] Validation des inputs/outputs
- [ ] Injection dans le pipeline
- [ ] Tests avec shaders custom

### Phase 4: Migration BasicResource/StandardResource

- [ ] Ajouter `descriptor()` à BasicResource
- [ ] Ajouter `descriptor()` à StandardResource
- [ ] Router SceneRendering vers Analyzer si `descriptor()` disponible
- [ ] Maintenir rétro-compatibilité avec anciennes méthodes
- [ ] Tests de non-régression

### Phase 5: Deprecation et Cleanup

- [ ] Marquer anciennes méthodes `[[deprecated]]`
- [ ] Migrer tout le code interne vers descripteurs
- [ ] Documenter la nouvelle API
- [ ] Mettre à jour `docs/saphir-shader-system.md`
- [ ] Supprimer code legacy (version future)

---

## Questions Ouvertes

1. **Syntaxe des pragmas** — `#pragma saphir` ou format différent (JSON header, attributs GLSL)?

2. **Niveau de custom** — Autoriser vertex custom ou seulement fragment?

3. **Hot-reload** — Recharger les ShaderFragments à chaud pendant le développement?

4. **Héritage de Materials** — Un Material peut-il hériter d'un autre descripteur?

5. **Compute shaders** — Intégrer dans le même système ou séparé?

---

## Références

- Architecture actuelle: `docs/saphir-shader-system.md`
- Material interface: `src/Graphics/Material/Interface.hpp`
- Generators: `src/Saphir/Generator/`
- LightGenerator: `src/Saphir/LightGenerator.hpp`
