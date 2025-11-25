# Scene Graph System

Context spécifique pour le développement du système de scene graph hiérarchique d'Emeraude Engine.

## Vue d'ensemble du module

Système de scene graph basé sur une architecture de composition (Entity-Component) avec deux types d'entités: **Nodes** dynamiques hiérarchiques et **StaticEntities** plates optimisées. Double buffering pour thread-safety entre simulation et rendu.

## Règles spécifiques à Scenes/

### Philosophie: Composition Over Inheritance
- **Entités génériques** : Node et StaticEntity sont des conteneurs de position
- **Components donnent le sens** : Visual, Light, Camera, SoundEmitter, etc.
- **JAMAIS de sous-classes** : Utiliser composition de Components au lieu de Player extends Node
- **Flexibilité maximale** : Ajouter/retirer comportements dynamiquement

### Architecture: Deux types d'entités

**Node (Dynamique)** : Arbre hiérarchique avec physique, transformations relatives au parent
**StaticEntity (Statique)** : Flat map optimisée, pas de physique, transformations absolues

Voir @docs/scene-graph-architecture.md pour détails complets.

### Convention de coordonnées
- **Y-DOWN obligatoire** dans CartesianFrame
- Transformations locales pour Nodes (relatives au parent)
- World space recalculé à la demande (pas de cache actuellement)

### Components disponibles
**Rendu:** Visual, MultipleVisuals
**Lumières:** DirectionalLight, PointLight, SpotLight
**Audio:** SoundEmitter, Microphone
**Physics:** DirectionalPushModifier, SphericalPushModifier, Weight
**Utilitaires:** Camera, ParticlesEmitter

### Système d'observateurs
- **Registration automatique** : Scene observe ajout de Components
- Visual → enregistrement au rendu
- Camera/Microphone → enregistrement à AVConsole
- Lights → enregistrement à LightSet
- **JAMAIS de registration manuelle**

### Optimisation spatiale
- **Octrees par Scene** : Un pour physique, un pour rendu
- **Frustum culling** : Actif sur parcours de l'arbre
- Optimisation future : Culling par secteur d'Octree

## Commandes de développement

```bash
# Tests scene graph
ctest -R Scenes
./test --filter="*Scene*"
```

## Fichiers importants

- `Manager.cpp/.hpp` - SceneManager, gestion de multiples Scenes + ActiveScene
- `Scene.cpp/.hpp` - Une scène avec son Root Node, Octrees, observateurs
- `Node.cpp/.hpp` - Entité dynamique hiérarchique (arbre)
- `StaticEntity.cpp/.hpp` - Entité statique optimisée (flat map)
- `AbstractEntity.cpp/.hpp` - Base commune pour gestion des Components
- `LocatableInterface.cpp/.hpp` - Interface pour coordonnées/déplacements
- `ToolKit.cpp/.hpp` - Helpers pour construction d'entités complexes
- `Component/Abstract.hpp` - Classe de base pour tous les Components (pure virtual onSuspend/onWakeup)
- `Component/SoundEmitter.cpp/.hpp` - Émetteur audio avec gestion suspend/wakeup des sources
- `@docs/scene-graph-architecture.md` - **Architecture complète et détaillée**
- `@docs/coordinate-system.md` - Convention Y-down (CRITIQUE)

## Patterns de développement

### Création d'un objet dynamique (Node)
```cpp
// Créer comme enfant d'un Node existant
auto player = scene->root()->createChild("player", initialPos);

// Ajouter des Components
player->newVisual(meshResource, castShadows, receiveShadows, "body");
player->newCamera(90.0f, 16.0f/9.0f, 0.1f, 1000.0f, "player_cam");

// Configurer physique
player->bodyPhysicalProperties().setMass(80.0f);
player->enableSphereCollision(true);
```

### Création d'une géométrie statique (StaticEntity)
```cpp
// Créer via Scene
auto building = scene->createStaticEntity("building_01");
building->setPosition(worldPos);

// Ajouter Visual et Light
building->newVisual(buildingMesh, true, true, "main");
building->newPointLight(Color::Warm, 100.0f, 20.0f, "lamp");
```

### Hiérarchie (véhicule avec roues)
```cpp
// Véhicule parent
auto vehicle = scene->root()->createChild("vehicle", vehiclePos);
vehicle->newVisual(carBodyMesh, true, true, "body");

// Roues enfants (suivent automatiquement le parent)
auto wheelFL = vehicle->createChild("wheel_FL", localPos_FL);
wheelFL->newVisual(wheelMesh, true, true, "wheel");

// Bouger le véhicule → roues suivent automatiquement
vehicle->applyForce(forwardVector * thrust);
```

### Création d'un nouveau Component
1. Hériter de `Component::Abstract` (Abstract.hpp)
2. Implémenter `processLogics()` si logique per-frame nécessaire
3. Implémenter `move()` si réaction au déplacement de l'entité nécessaire
4. Implémenter `onSuspend()`/`onWakeup()` (pure virtual, obligatoire)
5. Enregistrer avec Scene si observation automatique nécessaire

### Système Suspend/Wakeup (Scene Manager Level)
Quand le Scene Manager change de scène active, les entités et leurs components sont suspendus/réveillés pour libérer les ressources poolées (ex: sources audio OpenAL).

**Architecture (Template Method Pattern):**

1. **AbstractEntity** (`AbstractEntity.hpp/.cpp`):
   - `suspend()` / `wakeup()` - Méthodes publiques non-virtuelles
   - Appellent `onSuspend()`/`onWakeup()` de l'entité puis itèrent les components
   - `onSuspend()`/`onWakeup()` - Hooks protégés virtuels (défaut vide)

2. **Component::Abstract** (`Component/Abstract.hpp`):
   - `onSuspend()` / `onWakeup()` - Pure virtual protégées (contrat obligatoire)
   - Appelées par `AbstractEntity` (friend class)
   - Chaque component doit implémenter (même si vide)

**Flux d'appel:**
```
Scene::disable() → entity->suspend() → entity->onSuspend()
                                     → component->onSuspend() (for each)

Scene::enable()  → entity->wakeup()  → entity->onWakeup()
                                     → component->onWakeup() (for each)
```

**Implémentations existantes:**
- `SoundEmitter`: Libère/réacquiert source audio, mémorise état playing
- Autres components: Implémentation vide (pas de ressources poolées)

Voir `Scene.cpp:enable()`, `Scene.cpp:disable()`, `AbstractEntity.cpp:suspend()`, `AbstractEntity.cpp:wakeup()`

## Points d'attention

- **Double buffering** : Logic thread écrit activeFrame, Render thread lit renderFrame
- **Swap atomique** : m_renderFrame = m_activeFrame à la fin de chaque frame logique
- **Smart pointers** : shared_ptr et weak_ptr pour gestion automatique de la hiérarchie
- **Manager et Scene** : Gèrent construction/destruction fail-safe (en développement)
- **Root Node** : Immutable, ne peut ni bouger ni recevoir de Components
- **Y-down convention** : CartesianFrame utilise Y-down partout
- **Pas de cache world** : Recalcul à la demande (optimisation future prévue)
- **Observateurs** : Registration automatique, ne pas enregistrer manuellement
- **Suspend/Wakeup** : Tout nouveau Component DOIT implémenter `onSuspend()`/`onWakeup()` (pure virtual)
- **Friend class** : `AbstractEntity` est friend de `Component::Abstract` pour accéder aux hooks protégés

## Documentation détaillée

Pour l'architecture complète, les diagrammes, et les patterns avancés:
- @docs/scene-graph-architecture.md
