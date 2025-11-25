# Graphics System

Context spécifique pour le développement du système graphique haut niveau d'Emeraude Engine.

## Vue d'ensemble du module

Couche d'abstraction haut niveau au-dessus de Vulkan pour les concepts graphiques (style OpenGL). Gère les resources graphiques chargeables (Geometry, Material, Renderable) et l'assemblage complet pour le rendu via un système d'instancing.

## Règles spécifiques à Graphics/

### Philosophie d'abstraction
- **Haut niveau** : Concepts graphiques abstraits vs API Vulkan bas niveau
- **Resources chargeables** : Geometry, Material, Texture via système Resources (fail-safe)
- **Facilité de déclaration** : Interface simplifiée style OpenGL pour déclarer éléments graphiques
- **Options configurables** : Paramètres de rendu accessibles et pilotables

### Architecture: Système d'instancing

**Geometry** : Description de la géométrie au GPU (vertices, indices, formats)
**Material** : Assemblage de couleurs, textures, propriétés pour habiller la géométrie
**Renderable** : Géométrie + Material = objet complet unique prêt à rendre
**RenderableInstance** : Instance d'un Renderable (transformation, paramètres spécifiques)

### Exemple d'instancing
```
Renderable "wooden_crate":
  - Geometry: cube
  - Material: wood texture + normal map

RenderableInstance #1: position (10, 0, 5), scale 1.0
RenderableInstance #2: position (15, 0, 8), scale 1.2
RenderableInstance #100: position (50, 2, 30), scale 0.8

→ 100 caisses en bois rendues sans dupliquer géométrie/material
```

### Types de RenderableInstance
- **Unique** : Rendu classique (1 instance, push constants) → utilisé par `Visual`
- **Multiple** : GPU instancing (N instances, VBO matrices) → utilisé par `MultipleVisuals`
- **Cubemap support** : Les deux types adaptent leur stratégie push constants selon `isCubemap`
- Voir @docs/renderable-instance-system.md pour architecture complète

### Système de contexte de rendu
- **RenderPassContext** : commandBuffer, viewMatrices, readStateIndex, isCubemap
- **PushConstantContext** : pipelineLayout, stageFlags, useAdvancedMatrices, useBillboarding
- Permet de réduire les paramètres de fonction et supporter le rendu cubemap multiview
- Défini dans `RenderableInstance/RenderContext.hpp`

### Integration avec Components
- **Visual** : Utilise un RenderableInstance::Unique
- **MultipleVisuals** : Utilise un RenderableInstance::Multiple
- Registration automatique au Renderer via observateurs de Scene

### Renderer: Point central du système

Le **Renderer** est le gestionnaire principal qui coordonne:
- **TransferManager** : Upload/download GPU (CPU ↔ GPU transfers)
- **LayoutManager** : Gestion centralisée des layouts de pipelines Vulkan
- **ShaderManager** : Saphir pour génération automatique GLSL
- **SharedUBOManager** : Partage d'UBO entre multiples resources
- **VertexBufferFormatManager** : Centralisation des formats géométriques

### RenderTarget
- **Abstraction haut niveau** : Déclaration simplifiée style OpenGL
- **Utilise Vulkan en interne** : Classes Vulkan pour implémentation
- **Types** : Shadow maps, render-to-texture, vues off-screen
- **Render passes/Framebuffers** : Gérés par Vulkan, pas Graphics

### Integration avec Saphir
- Material déclare ses **requirements** (normals, tangents, UVs, colors)
- Geometry fournit ses **attributes** (vertex format)
- Saphir fait le **strict checking** + génération shader
- Voir @docs/saphir-shader-system.md pour détails complets

### Integration avec Resources
- **OBLIGATOIRE** : Toutes les resources Graphics héritent de ResourceTrait
- **Fail-safe** : Neutral resources pour Geometry, Material, Texture
- **Chargement asynchrone** : Via système Resources avec dépendances
- Voir @docs/resource-management.md pour architecture complète

### Convention de coordonnées
- **Y-DOWN obligatoire** dans toutes les transformations
- Matrices de projection configurées pour Vulkan Y-down
- Cohérence avec Physics, Scenes, Audio

## Commandes de développement

```bash
# Tests graphics
ctest -R Graphics
./test --filter="*Graphics*"
```

## Fichiers importants

### Structure par concept
- `Geometry/` - Descriptions géométriques GPU (vertices, indices, formats)
- `Material/` - Matériaux (textures, couleurs, propriétés)
- `Renderable/` - Objets complets (Geometry + Material)
- `RenderableInstance/` - Instances de Renderables (transformations, contexte de rendu)
  - `RenderContext.hpp` - Structures POD pour contexte de rendu (RenderPassContext, PushConstantContext)
- `RenderTarget/` - Abstractions pour cibles de rendu (2D, cubemap multiview)
- `TextureResource/` - Textures Vulkan (1D, 2D, 3D, Cubemap)

### Resources d'images
- **ImageResource** : Wrapper autour de `Pixmap<uint8_t>` pour images 2D. Utilisé par `Texture2D`.
- **VolumetricImageResource** : Données volumétriques 3D (`std::vector<uint8_t>` + dimensions). Utilisé par `Texture3D`.
  - Stocke width, height, depth, colorCount explicitement
  - Méthodes : `data()`, `width()`, `height()`, `depth()`, `colorCount()`, `bytes()`, `isValid()`

### Hiérarchie des textures
```
ImageResource (2D, Pixmap)          VolumetricImageResource (3D, raw bytes)
        ↓                                       ↓
   Texture1D                                Texture3D
   Texture2D
   TextureCubemap
```
- **Texture1D** : Image 1D (VK_IMAGE_TYPE_1D), utilise `ImageResource`
- **Texture2D** : Image 2D (VK_IMAGE_TYPE_2D), utilise `ImageResource`
- **Texture3D** : Volume 3D (VK_IMAGE_TYPE_3D), utilise `VolumetricImageResource`
- **TextureCubemap** : 6 faces (VK_IMAGE_VIEW_TYPE_CUBE), utilise `CubemapImageResource`

### Gestionnaire principal
- `Renderer.hpp/.cpp` - Coordinateur central du système graphique

### Managers intégrés
- TransferManager - Transferts CPU ↔ GPU
- LayoutManager - Layouts pipelines Vulkan
- ShaderManager (Saphir) - Génération GLSL automatique
- SharedUBOManager - Partage UBO entre resources
- VertexBufferFormatManager - Formats géométriques centralisés

### Documentation complémentaire
- `@docs/saphir-shader-system.md` - Génération automatique de shaders
- `@docs/resource-management.md` - Système de chargement fail-safe
- `@docs/graphics-system.md` - Architecture détaillée Graphics (instancing, Renderer, RenderTargets)
- `@docs/renderable-instance-system.md` - Système RenderableInstance (Unique, Multiple, flags, layers)
- `@docs/coordinate-system.md` - Convention Y-down (CRITIQUE)

## Patterns de développement

### Création d'une Geometry
1. Définir le vertex format (positions, normals, UVs, etc.)
2. Enregistrer le format avec VertexBufferFormatManager
3. Implémenter neutral resource (fail-safe)
4. Charger via Resources avec données externes

### Création d'un Material
1. Déclarer requirements (normals, tangent space, UVs, colors)
2. Définir textures et propriétés
3. Implémenter neutral material (default appearance)
4. Saphir générera shaders compatibles automatiquement

### Création d'un Renderable
1. Combiner Geometry + Material
2. Vérification Saphir (Material requirements vs Geometry attributes)
3. Si incompatible → échec chargement → neutral renderable
4. Si compatible → génération shader + pipeline Vulkan

### Utilisation via Components
```cpp
// Dans Scene
auto node = scene->root()->createChild("crate", position);

// Visual utilise un RenderableInstance
auto renderable = resources.container<RenderableResource>()->getResource("wooden_crate");
node->newVisual(renderable, castShadows, receiveShadows, "main_visual");

// Registration automatique au Renderer (observateurs)
```

## Points d'attention

- **Point critique** : Graphics/Renderer est le cœur du framework
- **Développement actif** : Système en évolution constante
- **Strict checking Saphir** : Material requirements DOIT matcher Geometry attributes
- **Fail-safe obligatoire** : Toutes resources Graphics doivent avoir neutral version
- **Y-down convention** : Jamais de flip de coordonnées
- **Abstraction Vulkan** : Ne jamais appeler Vulkan directement depuis Graphics
- **Thread safety** : TransferManager gère synchronisation CPU-GPU
- **Instancing** : Utiliser RenderableInstance pour objets multiples identiques
- **Dynamic states** : Viewport/scissor dynamiques pour éviter recréation pipelines au resize

## Dynamic Viewport/Scissor (Resize Optimization)

Les pipelines graphiques 3D utilisent `VK_DYNAMIC_STATE_VIEWPORT` et `VK_DYNAMIC_STATE_SCISSOR` pour éviter leur recréation lors du redimensionnement de la fenêtre.

**Fichiers clés:**
- `RenderTarget/Abstract.cpp:setViewport()` - Configure viewport ET scissor dynamiquement
- `RenderableInstance/Abstract.cpp` - Appelle `setViewport()` à chaque bind de pipeline
- `Saphir/Generator/*.cpp` - Tous déclarent les dynamic states

**Résultat:** Resize fluide même en Debug, pas de stutter ni de recréation de pipelines.

Voir @docs/graphics-system.md section "Dynamic Viewport and Scissor" pour détails complets.

## Documentation détaillée

Pour l'architecture complète du système Graphics:
- @docs/graphics-system.md - Architecture instancing, Renderer, subsystems

Systèmes liés:
- @docs/saphir-shader-system.md - Génération automatique shaders
- @docs/resource-management.md - Chargement fail-safe
- @src/Vulkan/AGENTS.md - Abstraction Vulkan bas niveau
