# AVConsole (Audio Video Console) - Development Context

Context spécifique pour le développement du système AVConsole d'Emeraude Engine.

## 🎯 Vue d'ensemble du module

Console de mixage audio-vidéo pour gérer les connexions entre caméras, microphones, écouteurs (speakers) et render-targets. Chaque Scene possède son propre AVConsole.

## 📋 Règles spécifiques à AVConsole/

### Concept: Console de mixage
- **Audio Video Console** : Abstraction d'une console de mixage AV
- **Par Scene** : Chaque Scene a son AVConsole dédié
- **Gestion connexions** : Lie cameras, micros, speakers entre eux et aux outputs

### Fonctionnalités principales

**Gestion des caméras** :
- Enregistrement automatique des Camera components
- Liaison camera → render-target
- Switch entre caméras multiples
- Définition de la caméra active pour affichage utilisateur

**Gestion audio** :
- Enregistrement automatique des Microphone components
- Liaison micro → sortie audio
- Configuration des listeners (points d'écoute)
- Mix audio de la scène

**Render-to-texture** :
- Camera → render-target custom (pas juste écran principal)
- Multiples cameras pour multiples render-targets simultanés
- Utilisation: mirrors, security cameras, portals, mini-maps, etc.

### Integration avec Scene
- **Observateurs** : AVConsole observe ajout de Camera/Microphone components
- **Registration automatique** : Components s'enregistrent automatiquement
- **Lifecycle** : AVConsole suit le lifecycle de la Scene

### Switch de caméra
```cpp
// Scene avec plusieurs caméras
auto cam1 = node1->newCamera(..., "main_camera");
auto cam2 = node2->newCamera(..., "security_camera");

// AVConsole détecte automatiquement les deux

// Switch caméra active
scene->avConsole().setActiveCamera("main_camera");  // Vue joueur
// ou
scene->avConsole().setActiveCamera("security_camera");  // Vue sécurité
```

### Render-to-texture
```cpp
// Créer render-target custom
auto renderTarget = graphics.createRenderTarget("mirror_view", 512, 512);

// Lier caméra au render-target
auto mirrorCam = mirrorNode->newCamera(..., "mirror_camera");
scene->avConsole().bindCameraToTarget("mirror_camera", renderTarget);

// mirrorCam rend dans renderTarget au lieu de l'écran principal
// renderTarget peut être utilisé comme texture sur un mesh (miroir)
```

## 🛠️ Commandes de développement

```bash
# Tests AVConsole
ctest -R AVConsole
./test --filter="*AVConsole*"

# Debug AVConsole
./Emeraude --debug-avconsole
./Emeraude --list-cameras
./Emeraude --list-microphones
```

## 🔗 Fichiers importants

- `Manager.cpp/.hpp` - Gestionnaire AVConsole (un par Scene)
- Intégré dans Scene lifecycle
- `@src/Scenes/AGENTS.md` - Context général du scene graph
- `@src/Scenes/Component/Camera.hpp` - Camera component
- `@src/Scenes/Component/Microphone.hpp` - Microphone component

## ⚡ Patterns de développement

### Multi-caméras avec switch
```cpp
// Setup plusieurs caméras dans la scène
auto playerNode = scene->root()->createChild("player", playerPos);
auto playerCam = playerNode->newCamera(90.0f, 16.0f/9.0f, 0.1f, 1000.0f, "player_view");

auto droneNode = scene->root()->createChild("drone", dronePos);
auto droneCam = droneNode->newCamera(120.0f, 16.0f/9.0f, 0.1f, 5000.0f, "drone_view");

// Switch dynamique
if (userPressedKey(Key::V)) {
    scene->avConsole().setActiveCamera("drone_view");  // Vue drone
} else {
    scene->avConsole().setActiveCamera("player_view");  // Vue joueur
}
```

### Mini-map avec render-to-texture
```cpp
// Caméra top-down pour mini-map
auto minimapNode = scene->root()->createChild("minimap_cam", Vec3(0, -100, 0));
minimapNode->lookDown();  // Vue du dessus
auto minimapCam = minimapNode->newCamera(60.0f, 1.0f, 0.1f, 200.0f, "minimap");

// Render-target pour mini-map
auto minimapTarget = graphics.createRenderTarget("minimap_rt", 256, 256);
scene->avConsole().bindCameraToTarget("minimap", minimapTarget);

// Utiliser minimapTarget comme texture dans l'overlay
overlayManager.displayTexture(minimapTarget, screenWidth - 266, 10, 256, 256);
```

### Configuration audio 3D
```cpp
// Microphone attaché à la caméra active (listener suit caméra)
auto listener = playerNode->newMicrophone("player_ears");

// AVConsole configure automatiquement OpenAL listener
// Position et orientation du listener suivent le Node
```

## 🚨 Points d'attention

- **Un AVConsole par Scene** : Pas global, lié à la Scene
- **Registration automatique** : Ne pas enregistrer manuellement Camera/Microphone
- **Active camera required** : Au moins une caméra doit être active pour rendu
- **Render-target lifetime** : Gérer durée de vie des render-targets custom
- **Performance** : Multiples render-targets = multiples render passes (coût GPU)
- **Audio listener unique** : Un seul listener actif (lié à caméra active généralement)

## 📚 Documentation détaillée

Systèmes liés:
→ **@src/Scenes/AGENTS.md** - Scene graph et components
→ **@src/Audio/AGENTS.md** - Système audio 3D
→ **@src/Graphics/AGENTS.md** - Render-targets et rendering
→ **@docs/scene-graph-architecture.md** - Architecture complète Scenes
