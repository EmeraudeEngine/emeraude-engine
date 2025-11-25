# Saphir Shader System

Context spécifique pour le développement du système de génération automatique de shaders d'Emeraude Engine.

## Vue d'ensemble du module

Saphir génère automatiquement du code GLSL à partir des propriétés de matériaux, des attributs géométriques et du contexte de scène. Il élimine le besoin de centaines de variantes de shaders écrites manuellement.

## Règles spécifiques à Saphir/

### Philosophie de génération
- **Génération paramétrique** : Shaders créés à partir d'inconnues (material + geometry + scene)
- **Vérification de compatibilité STRICTE** : Material requirements DOIT matcher geometry attributes
- **Échec gracieux** : Si incompatible → resource loading fails → application continue
- **Cache agressif** : Évite génération et compilation redondantes

### Types de générateurs
1. **SceneGenerator** : Objets 3D avec éclairage complet, matériaux, effets
2. **OverlayGenerator** : Éléments 2D (UI, HUD, texte, sprites)
3. **ShadowManager** : Shaders minimaux pour génération shadow maps

### Vérification de compatibilité
```cpp
Material requirements: [Normals, TangentSpace, TextureCoordinates2D]
Geometry attributes:   [positions, normals, uvs]  // PAS de tangents!
→ ÉCHEC avec log détaillé
```

## Commandes de développement

```bash
# Tests spécifiques
ctest -R Saphir
./test --filter="*Shader*"
```

## Fichiers importants

- `CodeGeneratorInterface.cpp/.hpp` - Interface base pour tous les générateurs
- `LightGenerator.cpp/.hpp` - Génération éclairage (PerFragment, PerVertex)
- `Program.cpp/.hpp` - Gestion programmes Vulkan et cache SPIR-V
- `ShaderManager.cpp/.hpp` - Coordinateur principal système Saphir
- `@docs/saphir-shader-system.md` - Architecture complète du système

## Patterns de développement

### Ajout d'un nouveau générateur
1. Hériter de `CodeGeneratorInterface`
2. Implémenter `check(material, geometry)` pour compatibilité
3. Implémenter `generate()` pour production GLSL
4. Enregistrer avec le ShaderManager

### Extension d'un générateur existant
1. Identifier le type de generator (Scene/Overlay/Shadow)
2. Ajouter conditions dans les méthodes generate
3. Tester toutes les combinaisons material/geometry
4. Vérifier performance cache (hash des inputs)

### Debug échecs de génération
1. Examiner logs détaillés (matériel vs géométrie)
2. Vérifier export des tangents (Blender/Maya)
3. Simplifier matériel OU enrichir géométrie
4. Tester avec matériel par défaut d'abord

## Points d'attention

- **Strict checking** : Material requirements DOIT être satisfait par geometry
- **Cache par hash** : Inputs identiques → même shader (performance)
- **Fail-safe integration** : Échecs logés mais app continue (pas de crash)
- **Y-down convention** : Matrices projection configurées pour Vulkan
- **Thread safety** : Cache protégé, génération peut être parallèle
- **Utilisé par Graphics et Overlay** : Graphics (3D), Overlay (2D) utilisent Saphir
- **Génération runtime** : Shaders générés à la demande pendant chargement resources

## Documentation détaillée

Pour l'architecture complète du système Saphir:
- @docs/saphir-shader-system.md** - Génération paramétrique, compatibilité, cache

Systèmes liés:
- @src/Graphics/AGENTS.md** - Material et Geometry pour génération 3D
- @src/Overlay/AGENTS.md** - Pipeline 2D via OverlayGenerator
- @src/Resources/AGENTS.md** - Génération pendant onDependenciesLoaded()
- @src/Vulkan/AGENTS.md** - Compilation SPIR-V et pipelines