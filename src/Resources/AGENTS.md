# Resource Management

Context spécifique pour le développement du système de gestion des ressources d'Emeraude Engine.

## Vue d'ensemble du module

Système de ressources fail-safe qui garantit de JAMAIS retourner nullptr et de toujours fournir une ressource valide, même en cas d'échec de chargement.

## Architecture (v0.8.35+)

### Classes principales

| Fichier | Classe | Rôle |
|---------|--------|------|
| `Types.hpp` | Enums + fonctions | `SourceType`, `Status`, `DepComplexity` + conversions string |
| `ResourceTrait.hpp` | `ResourceTrait` | Interface de base pour toutes les ressources |
| `ResourceTrait.hpp` | `AbstractServiceProvider` | Interface d'accès aux containers (fusionnée) |
| `Container.hpp` | `Container<resource_t>` | Store template par type de ressource |
| `Manager.hpp` | `Manager` | Coordinateur central, accès à tous les containers |
| `BaseInformation.hpp` | `BaseInformation` | Métadonnées de ressource (store index) |

### Cycle de vie des ressources

```
Unloaded → Enqueuing/ManualEnqueuing → Loading → Loaded/Failed
```

**Status enum:**
- `Unloaded` (0) : État initial
- `Enqueuing` (1) : Mode auto, dépendances en cours d'ajout
- `ManualEnqueuing` (2) : Mode manuel, utilisateur contrôle les dépendances
- `Loading` (3) : Plus de dépendances autorisées, attente de complétion
- `Loaded` (4) : Prêt à l'utilisation
- `Failed` (5) : Échec de chargement

## Règles spécifiques à Resources/

### Philosophie Fail-Safe OBLIGATOIRE
- **JAMAIS** de retour nullptr depuis les Containers
- **TOUJOURS** fournir une ressource valide (vraie ou neutral)
- **JAMAIS** de vérification d'erreur côté client
- Les erreurs sont loggées mais ne cassent jamais l'application

### Pattern Neutral Resource
- **OBLIGATOIRE** : Implémenter `load(AbstractServiceProvider&)` sans paramètres
- La ressource neutral doit TOUJOURS réussir (pas d'I/O)
- Être immédiatement utilisable et visuellement identifiable
- Aucune dépendance externe

### Thread Safety (CRITIQUE)

**Atomic status:**
```cpp
std::atomic<Status> m_status{Status::Unloaded};  // Lock-free queries
```

**Mutex pour listes:**
```cpp
std::mutex m_dependenciesAccess;  // Protège m_parentsToNotify et m_dependenciesToWaitFor
```

**Pattern deux phases (évite deadlocks):**
```cpp
void checkDependencies() noexcept {
    Action action = Action::None;
    {
        std::lock_guard lock{m_dependenciesAccess};
        // Phase 1: Déterminer l'action sous verrou
        if (allDependenciesLoaded()) action = Action::CallOnDependenciesLoaded;
    }
    // Phase 2: Exécuter HORS verrou (appels virtuels + notifications)
    if (action == Action::CallOnDependenciesLoaded) {
        this->onDependenciesLoaded();  // Virtual call OUTSIDE lock!
    }
}
```

### Détection de cycles (NOUVEAU v0.8.35)

**Automatique dans addDependency():**
```cpp
if (this->wouldCreateCycle(dependency)) [[unlikely]] {
    m_status = Status::Failed;
    return false;
}
```

**Algorithme DFS récursif:**
```cpp
bool wouldCreateCycle(const shared_ptr<ResourceTrait>& dep) const noexcept {
    if (dep.get() == this) return true;  // Auto-référence
    for (const auto& sub : dep->m_dependenciesToWaitFor) {
        if (sub.get() == this || this->wouldCreateCycle(sub)) return true;
    }
    return false;
}
```

### Gestion des dépendances
- Utiliser `addDependency()` pour déclarer les dépendances
- `onDependenciesLoaded()` pour la finalisation (upload GPU, etc.)
- Propagation automatique des événements parent-enfant
- Reference counting avec `std::shared_ptr`

## Patterns de développement

### Création d'un nouveau type de ressource
1. Hériter de `ResourceTrait`
2. **OBLIGATOIRE** : Implémenter la neutral resource `load(AbstractServiceProvider&)`
3. Implémenter le chargement fichier/données avec possibilité d'échec
4. `onDependenciesLoaded()` pour finalisation
5. Enregistrer dans `Manager`

### Chargement avec dépendances
```cpp
bool load(AbstractServiceProvider& provider, const Json::Value& data) override {
    // 1. Initialiser l'enqueuing
    if (!this->initializeEnqueuing(false)) return false;

    // 2. Charger données immédiates
    loadImmediateData(data);

    // 3. Déclarer dépendances (détection de cycle automatique)
    auto dep = provider.container<OtherResource>()->getResource(data["dep"]);
    if (!addDependency(dep)) return false;  // Cycle détecté = échec

    // 4. Finaliser l'enqueuing
    return this->setLoadSuccess(true); // Resource passe à Loading
}

bool onDependenciesLoaded() override {
    // 5. Finalisation quand TOUTES les dépendances sont prêtes
    // NOTE: Appelé HORS mutex pour éviter deadlocks
    uploadToGPU();
    return true; // Resource passe à Loaded
}
```

### Garbage Collection
- `use_count() == 1` → seul le Container détient la ressource
- `unloadUnusedResources()` pour libérer mémoire
- Garder les Default resources en cache permanent

## Commandes de développement

```bash
# Tests resources
ctest -R Resources
./test --filter="*Resource*"
```

## Points d'attention CRITIQUES

| Point | Importance | Description |
|-------|------------|-------------|
| **Thread safety** | CRITIQUE | Status atomique + mutex sur listes |
| **Deadlock prevention** | CRITIQUE | Appels virtuels HORS verrou |
| **Cycle detection** | HAUTE | DFS automatique dans addDependency() |
| **Memory management** | HAUTE | `shared_ptr` pour reference counting |
| **Status tracking** | MOYENNE | Machine à états : Unloaded → Loading → Loaded/Failed |
| **Cache efficiency** | MOYENNE | Clé par nom de ressource pour réutilisation |

## Fichiers supprimés (v0.8.35)

- `AbstractServiceProvider.hpp` → Fusionné dans `ResourceTrait.hpp`
- `LoadingRequest.hpp` → Supprimé (fonctionnalité intégrée ailleurs)
- `Randomizer.hpp` → Supprimé

## Améliorations futures (suggestions)

| Suggestion | Complexité | Impact | Priorité |
|------------|------------|--------|----------|
| **Optimiser détection cycles** | Basse | Moyen | Haute |
| Algorithme DFS avec `visited set` → O(n) au lieu de O(n²) | | | |
| **Système de priorités** | Moyenne | Haut | Haute |
| `LoadPriority::Critical/High/Normal/Low/Deferred` | | | |
| **Métriques avancées** | Basse | Moyen | Moyenne |
| Cache hits/misses, temps de chargement, ressources lentes | | | |
| **Resource bundles** | Moyenne | Haut | Moyenne |
| Groupes de ressources pour transitions de scènes | | | |
| **Chargement progressif** | Haute | Haut | Basse |
| LOD pour textures, streaming pour open-world | | | |

Voir @docs/resource-management.md section "Future Improvements" pour les détails d'implémentation.

## Documentation détaillée

Pour l'architecture complète du système de resources:
- @docs/resource-management.md - Fail-safe, dépendances, lifecycle détaillé, thread safety, suggestions futures

Systèmes liés:
- @src/Net/AGENTS.md - Téléchargement resources depuis URLs
- @src/Graphics/AGENTS.md - Geometry, Material, Texture comme resources
- @src/Audio/AGENTS.md - SoundResource, MusicResource
- @src/Libs/AGENTS.md - Observer/Observable pattern
