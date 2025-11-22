# Graphics System - Development Context

Context spécifique pour le développement du système graphique haut niveau d'Emeraude Engine.

## 🎯 Vue d'ensemble du module

Couche d'abstraction haut niveau au-dessus de Vulkan pour les concepts graphiques (style OpenGL). Gère les resources graphiques chargeables (Geometry, Material, Renderable) et l'assemblage complet pour le rendu via un système d'instancing.

## 📋 Règles spécifiques à Graphics/

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

### Integration avec Components
- **Visual** : Utilise un RenderableInstance
- **MultipleVisuals** : Utilise plusieurs RenderableInstances
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

## 🛠️ Commandes de développement

```bash
# Tests graphics
ctest -R Graphics
./test --filter="*Graphics*"

# Debug rendu
./Emeraude --debug-renderer
./Emeraude --show-wireframe
./Emeraude --show-normals
./Emeraude --disable-culling
```

## 🔗 Fichiers importants

### Structure par concept
- `Geometry/` - Descriptions géométriques GPU (vertices, indices, formats)
- `Material/` - Matériaux (textures, couleurs, propriétés)
- `Renderable/` - Objets complets (Geometry + Material)
- `RenderableInstance/` - Instances de Renderables (transformations)
- `RenderTarget/` - Abstractions pour cibles de rendu

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
- `@docs/coordinate-system.md` - Convention Y-down (CRITIQUE)

## ⚡ Patterns de développement

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

## 🚨 Points d'attention

- **Point critique** : Graphics/Renderer est le cœur du framework
- **Développement actif** : Système en évolution constante
- **Strict checking Saphir** : Material requirements DOIT matcher Geometry attributes
- **Fail-safe obligatoire** : Toutes resources Graphics doivent avoir neutral version
- **Y-down convention** : Jamais de flip de coordonnées
- **Abstraction Vulkan** : Ne jamais appeler Vulkan directement depuis Graphics
- **Thread safety** : TransferManager gère synchronisation CPU-GPU
- **Instancing** : Utiliser RenderableInstance pour objets multiples identiques

## 📚 Documentation détaillée

Pour l'architecture complète du système Graphics:
→ **@docs/graphics-system.md** - Architecture instancing, Renderer, subsystems

Systèmes liés:
→ **@docs/saphir-shader-system.md** - Génération automatique shaders
→ **@docs/resource-management.md** - Chargement fail-safe
→ **@src/Vulkan/AGENTS.md** - Abstraction Vulkan bas niveau
