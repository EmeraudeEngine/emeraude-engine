# Net System - Development Context

Context spécifique pour le développement du système de téléchargement réseau d'Emeraude Engine.

## 🎯 Vue d'ensemble du module

Système de téléchargement de ressources via réseau basé sur ASIO. Intégration transparente avec le système Resources pour chargement d'assets depuis URLs.

## 📋 Règles spécifiques à Net/

### Objectif principal
- **Téléchargement de ressources** : Download d'assets/fichiers depuis URLs
- **PAS de multijoueur** : Net n'est pas pour networking gameplay
- **Integration Resources** : Workflow transparent avec système de chargement

### Architecture ASIO
- **Basé sur ASIO** : Boost.Asio ou standalone pour gestion réseau
- **Protocoles supportés** : HTTP/HTTPS et autres protocoles ASIO
- **Asynchrone** : Téléchargements non-bloquants
- **Gestion interne** : ASIO gère timeouts, retries, erreurs réseau

### Integration avec Resources

**Workflow automatique** :
```
1. Resource load() détecte une URL au lieu d'un chemin fichier
2. Resources délègue à Net pour téléchargement
3. Net télécharge de façon asynchrone
4. Net retourne le fichier téléchargé à Resources
5. Resources finalise le chargement normalement
```

**Transparent pour le client** :
```cpp
// Client code identique, que ce soit fichier local ou URL
auto texture = resources.container<TextureResource>()->getResource("logo.png");
// ou
auto texture = resources.container<TextureResource>()->getResource("https://example.com/logo.png");

// Net gère automatiquement le download si URL détectée
```

### Système de cache local
- **Cache automatique** : Ressources téléchargées sauvegardées localement
- **Évite re-téléchargements** : Vérification cache avant download
- **Gestion transparente** : Cache géré automatiquement par Net

### Téléchargements asynchrones
- **Non-bloquant** : Pas de freeze pendant downloads
- **Integration async Resources** : Compatible avec chargement asynchrone Resources
- **Status tracking** : Resources peut suivre progression via observables

## 🛠️ Commandes de développement

```bash
# Tests net
ctest -R Net
./test --filter="*Net*"

# Debug téléchargements
./Emeraude --debug-net
./Emeraude --show-downloads
./Emeraude --clear-net-cache  # Vider cache local
```

## 🔗 Fichiers importants

- `Manager.cpp/.hpp` - Gestionnaire principal, requêtes de téléchargement
- Cache local (emplacement à documenter)
- `@docs/resource-management.md` - Integration avec Resources

## ⚡ Patterns de développement

### Utilisation via Resources (automatique)
```cpp
// Resources détecte URL et utilise Net automatiquement
auto mesh = resources.container<MeshResource>()->getResource(
    "https://cdn.example.com/assets/character.obj"
);

// Mesh commence Loading (téléchargement en cours)
// Quand download terminé → parsing → Loaded
// Si échec download → Default mesh (fail-safe)
```

### Requête explicite (rare, usage avancé)
```cpp
// Si besoin de contrôle direct sur téléchargement
netManager.requestDownload(
    "https://example.com/file.dat",
    "/local/cache/path",
    [](bool success, const std::string& localPath) {
        if (success) {
            // Fichier disponible à localPath
        } else {
            // Échec téléchargement
        }
    }
);
```

### Gestion du cache
```cpp
// Vérifier si ressource en cache
bool cached = netManager.isCached("https://example.com/texture.png");

// Vider cache (maintenance)
netManager.clearCache();

// Forcer re-téléchargement (ignore cache)
netManager.forceDownload(url, callback);
```

## 🚨 Points d'attention

- **ASIO gère complexité** : Timeouts, retries, erreurs réseau gérés par ASIO
- **Thread safety** : ASIO gère threading, Net thread-safe par design
- **Cache local** : Vérifier espace disque disponible pour cache
- **URLs dans stores** : Resources stores peuvent contenir URLs au lieu de chemins
- **Fail-safe integration** : Échec download → Resources retourne neutral resource
- **Pas de multijoueur** : Net est pour assets, pas gameplay networking

## 📚 Documentation détaillée

Systèmes liés:
→ **@docs/resource-management.md** - Integration automatique avec Resources
→ **@src/Resources/AGENTS.md** - Système de chargement fail-safe
→ **ASIO documentation** - Détails sur protocoles et gestion réseau
