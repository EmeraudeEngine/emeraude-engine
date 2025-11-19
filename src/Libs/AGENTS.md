# Libs (Libraries) - Development Context

Context spécifique pour le développement des bibliothèques utilitaires d'Emeraude Engine.

## 🎯 Vue d'ensemble du module

**Fondation du moteur** - Bibliothèques utilitaires agnostiques fournissant concepts de base, mathématiques, manipulation de données, et intégrations externes. Tout le moteur repose sur Libs pour uniformisation.

## 📋 Règles spécifiques à Libs/

### Philosophie: Fondation agnostique CRITIQUE
- **Fondation du moteur** : Tous les systèmes (Graphics, Physics, Audio, Scenes) utilisent Libs
- **Agnostique** : AUCUNE dépendance vers systèmes haut niveau (Scenes, Physics, Graphics, etc.)
- **Uniformisation** : Fournir types et concepts communs à tout le moteur
- **Réutilisable** : Code générique, pas spécifique à un cas d'usage

### Architecture par domaine

**Algorithms/** - Algorithmes utiles multimédia
- Implémentations simples d'algos classiques
- Optimisés pour usage temps réel

**Compression/** - Abstraction compression/décompression
- Standardisation logique compression de données
- Wrappers: zlib, lzma
- Interface commune pour tous algorithmes

**Debug/** - Outils stats et développement
- Profiling, timings, statistiques
- Helpers de débogage

**GamesTools/** - Classes utilitaires jeux vidéo
- Helpers spécifiques gameplay et jeux
- Concepts génériques jeux

**Hash/** - Algorithmes de hashage
- MD5, SHA, et autres algorithmes classiques
- Pour checksums, identifiants, caching

**IO/** - Lecture/écriture fichiers génériques
- Abstractions I/O cross-platform
- Support archives (ZIP via lib externe)
- Manipulation fichiers/dossiers

**Math/** - Bibliothèque mathématique 2D/3D complète
- **Vector** : Vecteurs 2D/3D/4D
- **Matrix** : Matrices transformations
- **Quaternion** : Rotations 3D
- **CartesianFrame** : Système coordonnées (position + base orthonormale)
- **Collision/Intersection** : Détection géométrique
- **Courbes de Bézier** : Interpolation smooth
- Toute logique mathématique 2D/3D présente et future

**Network/** - Généralisation logiques web
- Helpers protocoles réseau
- Abstractions communication

**PixelFactory/** - Manipulation d'images
- Chargement/sauvegarde formats image
- Transformations pixel (resize, crop, filters)
- Génération procédurale images

**VertexFactory/** - Manipulation géométrie 3D
- Génération meshes procéduraux
- Transformations géométriques
- Calculs normales, tangentes, UVs

**WaveFactory/** - Manipulation audio
- Chargement formats audio (via libsndfile)
- Transformations samples audio
- Génération procédurale sons

**Time/** - Gestion temporelle
- Chronomètres (chronos)
- Timers
- Helpers timing via objets et interfaces

**Concepts généraux (racine Libs/)** :
- **Observer/Observable** : Pattern événementiel
- **Versioning** : Gestion versions
- **JSON** : Parsing/écriture JSON rapide
- **Flags** : Gestion drapeaux binaires
- **Traits** : Helpers (NamingTrait, etc.)
- **Strings** : Manipulation chaînes caractères
- **ThreadPool** : Pool de threads performant (classe excellente)

### Dépendances externes intégrées
- **libsndfile** : Chargement formats audio (WaveFactory)
- **zlib** : Compression (Compression)
- **lzma** : Compression (Compression)
- **ZIP library** : Archives (IO)
- Autres selon besoins

## 🛠️ Commandes de développement

```bash
# Tests Libs
ctest -R Libs
./test --filter="*Libs*"

# Tests par catégorie
./test --filter="*Math*"
./test --filter="*IO*"
./test --filter="*ThreadPool*"
```

## 🔗 Fichiers importants

### Math (critique)
- `Vector.hpp` - Vecteurs 2D/3D/4D
- `Matrix.hpp` - Matrices transformations
- `Quaternion.hpp` - Rotations 3D
- `CartesianFrame.hpp` - Système coordonnées avec base orthonormale
- `Collision.hpp` - Détection collisions
- `Bezier.hpp` - Courbes de Bézier

### Concepts généraux
- `Observer.hpp` / `Observable.hpp` - Pattern événementiel
- `ThreadPool.hpp` - Pool threads performant
- `JSON.hpp` - Manipulation JSON
- `FlagTrait.hpp` - Gestion flags
- `NamableTrait.hpp` - Trait pour noms

### Factories
- `PixelFactory/` - Manipulation images
- `VertexFactory/` - Génération/manipulation géométrie
- `WaveFactory/` - Manipulation audio

## ⚡ Patterns de développement

### Utilisation Math dans tout le moteur
```cpp
// Graphics utilise Math
Matrix4 projectionMatrix = Math::perspective(fov, aspect, near, far);
Vector3 cameraPos = camera.cartesianFrame().position();

// Physics utilise Math
Vector3 force = mass * acceleration;
Quaternion rotation = Quaternion::fromEuler(pitch, yaw, roll);

// Audio utilise Math
Vector3 soundPos = emitter.cartesianFrame().position();
float distance = (listenerPos - soundPos).length();

// Tout le moteur uniformisé via Libs/Math
```

### Pattern Observer/Observable
```cpp
// Observable dans Resources
class TextureResource : public ResourceTrait, public Observable {
    void finishLoading() {
        // ...
        notifyObservers(Event::Loaded);
    }
};

// Observer dans Scene
class Scene : public Observer {
    void onNotify(Observable* source, Event event) override {
        if (event == Event::Loaded) {
            // Réagir au chargement
        }
    }
};
```

### ThreadPool pour tâches async
```cpp
// Pool global ou local
ThreadPool pool(std::thread::hardware_concurrency());

// Soumettre tâches
auto future1 = pool.enqueue([](int x) { return x * 2; }, 42);
auto future2 = pool.enqueue([](){ loadHeavyResource(); });

// Attendre résultats
int result = future1.get();  // 84
future2.wait();  // Attendre fin chargement
```

### Ajout d'une nouvelle lib (règles)
```cpp
// ❌ INTERDIT: Dépendre de systèmes haut niveau
#include "Scenes/Node.hpp"  // NON! Libs ne dépend pas de Scenes

// ✅ CORRECT: Agnostique et générique
class MyUtility {
    // Fonctionne sans connaître Scenes/Physics/Graphics
    static Vector3 interpolate(const Vector3& a, const Vector3& b, float t);
};

// ✅ Les systèmes haut niveau utilisent Libs
#include "Libs/MyUtility.hpp"  // Graphics/Scenes/Physics peuvent inclure Libs
```

## 🚨 Points d'attention CRITIQUES

- **Fondation du moteur** : Tout repose sur Libs, stabilité critique
- **Zéro dépendance haut niveau** : Libs ne doit JAMAIS inclure Scenes/Physics/Graphics/etc.
- **Agnostique** : Code générique, pas spécifique à un cas d'usage
- **Uniformisation** : Types Math utilisés PARTOUT (Vector, Matrix, CartesianFrame)
- **Thread-safe** : Considérer thread-safety pour utilitaires partagés
- **Performance** : Code critique (utilisé partout), optimiser si nécessaire
- **Documentation** : Bien documenter, beaucoup de systèmes dépendent de Libs
- **Tests exhaustifs** : Bug dans Libs affecte tout le moteur

## 📚 Documentation détaillée

Libs est référencé par tous les systèmes:
→ **@src/Math/** - Documentation mathématique détaillée (à créer si besoin)
→ **@src/Scenes/AGENTS.md** - Utilise CartesianFrame
→ **@src/Physics/AGENTS.md** - Utilise Vector, Matrix, collision detection
→ **@src/Graphics/AGENTS.md** - Utilise Math pour transformations
→ **@src/Audio/AGENTS.md** - Utilise Math pour positionnement 3D
