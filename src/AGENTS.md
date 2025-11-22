# Core Framework Components - Development Context

Context spécifique pour les composants de base à la racine de src/ d'Emeraude Engine.

## 🎯 Vue d'ensemble

Composants fondamentaux du framework situés directement dans `src/`. Ces fichiers constituent le cœur du moteur et orchestrent tous les sous-systèmes (Graphics, Audio, Physics, Scenes, etc.).

## 📋 Composants principaux

### Core - Cœur du framework
**Fichiers**: `Core.cpp/.hpp` (26KB header, 37KB cpp)

**Rôle central**: Point central de tout le framework, classe principale dont on hérite pour produire une application.

**Responsabilités**:
- **Orchestration**: Coordonne tous les sous-systèmes du moteur
- **Boucles principales**: Gère trois boucles d'exécution
  - Boucle principale
  - Boucle logique (thread séparé)
  - Boucle rendu (thread séparé)
- **Cycle de vie**: Point d'entrée pour surcharger le comportement applicatif

**Pattern d'utilisation**:
```cpp
class MyApplication : public Core {
    void onInit() override { /* Initialisation */ }
    void onLogicUpdate(float deltaTime) override { /* Logique */ }
    void onRenderUpdate() override { /* Rendu */ }
    void onShutdown() override { /* Nettoyage */ }
};
```

### Tracer - Système de logging
**Fichiers**: `Tracer.cpp/.hpp` (35KB header, 12KB cpp)

**Rôle**: Système de logging runtime pour tracer l'exécution du programme.

**Types de messages**:
- `info` - Informations générales
- `warning` - Avertissements
- `error` - Erreurs récupérables
- `fatal` - Erreurs critiques
- `success` - Opérations réussies
- `debug` - Debug (retiré via constexpr en Release)

**Fonctionnalités**:
- **Output flexible**: Terminal avec couleurs automatiques + fichiers de log
- **Métadonnées**: Fichier, ligne, tag, timestamp automatiques
- **Couleurs**: Coloration automatique selon le type de message
- **Performance**: Messages debug éliminés à la compilation en Release

**Usage typique**:
```cpp
TRACE_INFO("System initialized");
TRACE_WARNING("Low memory", "Memory");
TRACE_ERROR("Failed to load texture", "Graphics");
TRACE_DEBUG("Variable value: {}", value);  // Retiré en Release
```

### Window - Gestion de fenêtre OS
**Fichiers**: `Window.cpp/.hpp` + `Window.{linux,mac,windows}.cpp`

**Rôle**: Abstraction cross-platform de la fenêtre physique via GLFW.

**Responsabilités**:
- **Fenêtre OS**: Création, resize, déplacement, fullscreen
- **Événements**: Gestion événements OS (close, focus, minimize, etc.)
- **Vulkan Surface**: Création de la SwapChain Vulkan pour le rendu
- **Platform-specific**: Code spécialisé par OS (Linux/macOS/Windows)

**Intégration**:
- Utilisé par Core pour créer la fenêtre principale
- Fournit la surface Vulkan au Renderer
- Pas de lien direct avec Input ou Overlay

### Settings - Configuration applicative
**Fichiers**: `Settings.cpp/.hpp` (23KB header), `SettingKeys.hpp` (13KB)

**Rôle**: Système de configuration global du framework avec persistance JSON.

**Fonctionnalités**:
- **Format**: Stockage JSON dans dossiers config utilisateur par OS
- **Persistance**: Sauvegarde automatique à la fermeture de l'application
- **Édition live**: SHIFT+F5 ouvre le fichier dans l'éditeur de texte par défaut
- **SettingKeys**: Définit toutes les clés de configuration disponibles

**Types de paramètres**:
- Résolution, mode fenêtre
- Qualité graphique, vsync
- Volumes audio
- Chemins de resources
- Options de debug

**Hiérarchie**:
Arguments > Settings > Valeurs par défaut

### Arguments - Parsing ligne de commande
**Fichiers**: `Arguments.cpp/.hpp` (8KB header)

**Rôle**: Gestionnaire des arguments passés au programme (argc/argv).

**Fonctionnalités**:
- **Parsing**: Analyse des arguments en ligne de commande
- **Ajout dynamique**: Possibilité d'ajouter des arguments à la volée
- **Override Settings**: Arguments plus forts que Settings
- **Distribution**: Arguments passés aux sous-systèmes concernés

**Usage par Core**: Utilisé au démarrage pour configurer l'initialisation

**Exemples**:
```bash
./MyApp --fullscreen --resolution 1920x1080 --debug-renderer
```

### PrimaryServices - Conteneur de services
**Fichiers**: `PrimaryServices.cpp/.hpp` (7KB chacun)

**Rôle**: Conteneur pour transporter facilement les services principaux du moteur.

**Services inclus**:
- **Arguments**: Parser d'arguments
- **FileSystem**: Système de fichiers
- **Settings**: Configuration
- **Net::Manager**: Gestionnaire réseau
- **ThreadPool**: Pool de workers pour travail asynchrone

**Usage**: Simplifie le passage de dépendances entre composants

### FileSystem - Abstraction système de fichiers
**Fichiers**: `FileSystem.cpp/.hpp` (11KB header, 13KB cpp)

**Rôle**: Abstraction cross-platform pour accéder aux répertoires système.

**Répertoires gérés**:
- **Config**: Dossier configuration utilisateur par OS
- **Cache**: Dossier cache application
- **Data**: Dossier données utilisateur
- **Temp**: Dossier temporaire

**Cross-platform**:
- Windows: AppData
- Linux: ~/.config, ~/.cache
- macOS: ~/Library/Application Support

**Intégration**: Utilisé par Settings et Resources pour localiser fichiers

### ServiceInterface - Interface commune services
**Fichiers**: `ServiceInterface.hpp` (4KB)

**Rôle**: Interface de base pour tous les services du framework.

**Contrat commun**:
- **Initialisation**: Protocol d'init standardisé
- **Shutdown**: Nettoyage propre des resources
- **Cycle de vie**: Gestion uniforme des services

**Services héritant**: Renderer, Physics, Audio, Resources, Net::Manager, etc.

### PlatformManager - Détection plateforme
**Fichiers**: `PlatformManager.cpp/.hpp` (2KB header, 5KB cpp)

**Rôle**: Gestionnaire runtime de la plateforme courante.

**Responsabilités**:
- **Détection OS**: Identifie Linux/Windows/macOS/Web dynamiquement
- **Initialisation GLFW**: Bootstrap du framework GLFW
- **Coordination**: Coordonne avec le code du dossier PlatformSpecific/

**Usage**: Permet d'adapter le comportement selon l'OS au runtime

### Notifier - Notifications onscreen
**Fichiers**: `Notifier.cpp/.hpp` (5KB chacun)

**Rôle**: Système de notifications visuelles pour l'utilisateur final.

**Fonctionnalités**:
- **Messages onscreen**: Affichage de messages à l'écran (toasts/popups)
- **Types**: Info, warning, error, success
- **Non-bloquant**: N'interrompt pas l'exécution

**Usage**: Feedback utilisateur pour événements importants

### CursorAtlas - Gestion curseurs
**Fichiers**: `CursorAtlas.cpp/.hpp` (4KB chacun)

**Rôle**: Gestionnaire des curseurs système et personnalisés.

**Fonctionnalités**:
- **Curseurs standards**: Arrow, hand, crosshair, text, etc. (via OS)
- **Curseurs custom**: Chargement depuis images
- **GLFW wrapper**: Abstraction GLFW pour curseurs
- **CEF integration**: Transmission curseurs à librairies externes (CEF)

**Intégration**: Utilisé avec Window pour changer curseur selon contexte

### Help - Aide ligne de commande
**Fichiers**: `Help.cpp/.hpp` (10KB header)

**Rôle**: Système d'aide en ligne de commande (--help).

**Fonctionnalités**:
- **Arguments**: Affiche liste des arguments disponibles avec descriptions
- **Formatage**: Aide formatée et colorée dans le terminal
- **Info framework**: Version, build info, dépendances

**Déclenchement**: `./MyApp -h` ou `./MyApp --help`

### User - Représentation utilisateur
**Fichiers**: `User.cpp/.hpp` (2KB header)

**Rôle**: Généralisation optionnelle de l'utilisateur final du moteur.

**Usage**: Personnalisation Settings/chemins par utilisateur (optionnel)

### Fichiers utilitaires

**Types.hpp** (4KB):
- Types communs utilisés au premier niveau du moteur
- Enums et typedefs globaux
- Types de messages du framework

**Constants.hpp** (6KB):
- Constantes du moteur
- Fréquence de mise à jour logique
- Limites et valeurs par défaut

**Identification.hpp** (10KB):
- Identification de l'application finale
- Nom, version, auteur, copyright
- Métadonnées applicatives

## 🛠️ Patterns de développement

### Création d'une application
```cpp
#include "Core.hpp"

class MyGame : public Core {
    void onInit() override {
        // Initialiser scènes, resources, etc.
        TRACE_INFO("Game initialized");
    }

    void onLogicUpdate(float deltaTime) override {
        // Logique jeu (thread séparé)
        updateGameState(deltaTime);
    }

    void onRenderUpdate() override {
        // Rendu (thread séparé)
        renderScene();
    }

    void onShutdown() override {
        TRACE_INFO("Game shutdown");
    }
};

int main(int argc, char* argv[]) {
    MyGame game;
    game.run(argc, argv);  // Parse Arguments, init, boucles, shutdown
    return 0;
}
```

### Utilisation de Settings
```cpp
// Lecture
int width = settings.get<int>("window.width");
bool vsync = settings.get<bool>("graphics.vsync");

// Écriture (sauvegardé à la fermeture)
settings.set("window.width", 1920);
settings.set("audio.master_volume", 0.8f);

// Édition manuelle: SHIFT+F5 pendant exécution
```

### Utilisation de Tracer
```cpp
// Messages simples
TRACE_INFO("Loading level");
TRACE_WARNING("Low FPS detected");
TRACE_ERROR("Failed to load texture: {}", path);
TRACE_FATAL("Vulkan device lost");

// Avec tags
TRACE_INFO("Shader compiled", "Graphics");
TRACE_DEBUG("Position: ({}, {}, {})", x, y, z);  // Retiré en Release

// Fichier de log
tracer.setLogFile("game.log");
tracer.enableFileLogging(true);
```

### Utilisation de FileSystem
```cpp
// Répertoires cross-platform
auto configPath = fileSystem.getConfigDirectory();  // ~/.config/MyApp/
auto cachePath = fileSystem.getCacheDirectory();    // ~/.cache/MyApp/
auto dataPath = fileSystem.getDataDirectory();      // ~/.local/share/MyApp/
auto tempPath = fileSystem.getTempDirectory();      // /tmp/MyApp/

// Settings utilise automatiquement configPath
// Resources peut utiliser dataPath pour assets utilisateur
```

### Override avec Arguments
```bash
# Arguments overrident Settings
./MyGame --window.width=2560 --window.height=1440 --graphics.vsync=false

# Arguments personnalisés
./MyGame --custom-flag --my-value=42
```

```cpp
// Dans le code
if (arguments.has("custom-flag")) {
    int value = arguments.get<int>("my-value");
}
```

## 🚨 Points d'attention

- **Core est le point central**: Tout passe par Core, ne pas bypasser
- **Trois threads**: Main, Logic, Render - attention à la thread safety
- **Arguments > Settings**: Hiérarchie de priorité stricte
- **Tracer debug gratuit**: Messages debug éliminés en Release (zero cost)
- **Settings auto-save**: Sauvegarde automatique, pas besoin d'appeler save()
- **SHIFT+F5**: Édition Settings live, attention aux valeurs invalides
- **FileSystem cross-platform**: Jamais de chemins hardcodés OS-specific
- **ServiceInterface**: Tous les services doivent respecter le protocol init/shutdown
- **Window possède Surface**: Ne pas créer de Vulkan Surface manuellement
- **PlatformManager init GLFW**: Doit être initialisé avant Window

## 🔗 Intégration avec les sous-systèmes

### Core orchestre tout
```
Core
 ├─ Window (fenêtre + Vulkan Surface)
 ├─ PrimaryServices
 │   ├─ Arguments
 │   ├─ FileSystem
 │   ├─ Settings
 │   ├─ Net::Manager
 │   └─ ThreadPool
 ├─ Renderer (Graphics)
 ├─ Physics
 ├─ Audio
 ├─ Scenes
 ├─ Resources
 └─ Input
```

### Flux d'initialisation
```
1. main(argc, argv)
2. PlatformManager init GLFW
3. Arguments parse (argc, argv)
4. Settings load (FileSystem config dir)
5. Arguments override Settings
6. Window create (+ Vulkan Surface)
7. Core init subsystems (ServiceInterface)
8. Core start threads (Logic, Render)
9. Main loop
10. Core shutdown subsystems
11. Settings save (auto)
```

## 📚 Documentation complémentaire

Sous-systèmes orchestrés par Core:
→ **@src/Graphics/AGENTS.md** - Système graphique haut niveau
→ **@src/Vulkan/AGENTS.md** - Backend Vulkan bas niveau
→ **@src/Physics/AGENTS.md** - Moteur physique Jolt
→ **@src/Audio/AGENTS.md** - Audio 3D spatial OpenAL
→ **@src/Scenes/AGENTS.md** - Scene graph hiérarchique
→ **@src/Resources/AGENTS.md** - Chargement resources fail-safe
→ **@src/Input/AGENTS.md** - Gestion input GLFW
→ **@src/Overlay/AGENTS.md** - Interface 2D overlay

Concepts liés:
→ **@docs/coordinate-system.md** - Convention Y-down (CRITIQUE)
→ **@docs/resource-management.md** - Chargement fail-safe

Platform-specific:
→ **@src/PlatformSpecific/AGENTS.md** - Code spécialisé par OS
