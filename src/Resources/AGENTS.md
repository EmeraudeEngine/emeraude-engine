# Resource Management - Development Context

Context spécifique pour le développement du système de gestion des ressources d'Emeraude Engine.

## 🎯 Vue d'ensemble du module

Système de ressources fail-safe qui garantit de JAMAIS retourner nullptr et de toujours fournir une ressource valide, même en cas d'échec de chargement.

## 📋 Règles spécifiques à Resources/

### Philosophie Fail-Safe OBLIGATOIRE
- **JAMAIS** de retour nullptr depuis les Containers
- **TOUJOURS** fournir une ressource valide (vraie ou neutral)
- **JAMAIS** de vérification d'erreur côté client
- Les erreurs sont loggées mais ne cassent jamais l'application

### Pattern Neutral Resource
- **OBLIGATOIRE** : Implémenter `load(ServiceProvider&)` sans paramètres
- La ressource neutral doit TOUJOURS réussir (pas d'I/O)
- Être immédiatement utilisable et visuellement identifiable
- Aucune dépendance externe

### Gestion des dépendances
- Utiliser `addDependency()` pour déclarer les dépendances
- `onDependenciesLoaded()` pour la finalisation (upload GPU, etc.)
- Propagation automatique des événements parent-enfant
- Reference counting avec `std::shared_ptr`

## 🛠️ Commandes de développement

```bash
# Tests resources
ctest -R Resources
./test --filter="*Resource*"

# Debug chargement
./Emeraude --debug-resources
./Emeraude --log-loading
./Emeraude --show-defaults  # Affiche les ressources neutres
```

## 🔗 Fichiers importants

- `Manager.cpp/.hpp` - Coordinateur central, accès aux containers
- `Container.hpp` - Template store par type de ressource
- `ResourceTrait.cpp/.hpp` - Interface de base pour toutes les ressources
- `LoadingRequest.hpp` - Wrapper pour chargement asynchrone
- `@docs/resource-management.md` - Architecture détaillée

## ⚡ Patterns de développement

### Création d'un nouveau type de ressource
1. Hériter de `ResourceTrait`
2. **OBLIGATOIRE** : Implémenter la neutral resource `load(ServiceProvider&)`
3. Implémenter le chargement fichier/données avec possibilité d'échec
4. `onDependenciesLoaded()` pour finalisation
5. Enregistrer dans `Manager`

### Chargement avec dépendances
```cpp
bool load(ServiceProvider& provider, const Json::Value& data) override {
    // 1. Charger données immédiates
    loadImmediateData(data);
    
    // 2. Déclarer dépendances
    auto dep = provider.container<OtherResource>()->getResource(data["dep"]);
    addDependency(dep);
    
    return true; // Resource reste en Loading
}

bool onDependenciesLoaded() override {
    // 3. Finalisation quand TOUTES les dépendances sont prêtes
    uploadToGPU();
    return true; // Resource passe à Loaded
}
```

### Garbage Collection
- `use_count() == 1` → seul le Container détient la ressource
- `unloadUnusedResources()` pour libérer mémoire
- Garder les Default resources en cache permanent

## 🚨 Points d'attention

- **Thread safety** : Mutex sur les maps de ressources
- **Dependency cycles** : Éviter les cycles dans les dépendances
- **Memory management** : `shared_ptr` pour reference counting automatique
- **Status tracking** : Unloaded → Loading → Loaded/Failed
- **Cache efficiency** : Clé par nom de ressource pour réutilisation