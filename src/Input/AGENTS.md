# Input System - Development Context

Context spécifique pour le développement du système de gestion des entrées d'Emeraude Engine.

## 🎯 Vue d'ensemble du module

Système de gestion des entrées basé sur GLFW, offrant à la fois consultation directe de l'état des périphériques et système d'événements. Supporte clavier, souris, gamepads, joysticks.

## 📋 Règles spécifiques à Input/

### Architecture double approche

**État direct (Polling)** : Consultation de l'état actuel via Controllers
- Clavier : Touches pressées/relâchées
- Souris : Position, boutons, scroll
- Gamepads/Joysticks : Axes, boutons

**Système d'événements** : Callbacks pour réactions temps réel
- Tous les événements GLFW implémentés
- Dispatch via interfaces à hériter
- Enregistrement dans InputManager

### Intégration GLFW
- **Support natif** : Tout ce que GLFW supporte est disponible
- **Événements complets** : Key press/release, mouse move/click, gamepad, etc.
- **Cross-platform** : Gestion automatique des différences OS via GLFW

### Système d'enregistrement

**Interfaces à hériter** :
- Interface clavier (key events)
- Interface souris (mouse events)
- Autres interfaces selon périphériques

**Enregistrement dans Manager** :
```cpp
// Objet hérite de KeyboardInterface
class MyController : public KeyboardInterface {
    void onKeyPress(Key key) override { ... }
    void onKeyRelease(Key key) override { ... }
};

// Enregistrement
MyController controller;
inputManager.registerKeyboardListener(&controller);
```

**Exemple d'usage** :
- OverlayManager hérite des interfaces clavier/souris
- Enregistré automatiquement au démarrage du moteur
- Reçoit les événements et les dispatch aux Screen/Surface

### Consultation directe (Controllers)

Alternative au système d'événements pour polling d'état :
```cpp
// Consulter état clavier
if (inputManager.keyboard().isKeyPressed(Key::W)) {
    moveForward();
}

// Consulter état souris
auto mousePos = inputManager.mouse().position();
bool leftClick = inputManager.mouse().isButtonPressed(MouseButton::Left);
```

### Mapping d'actions (responsabilité application)
- **Input fournit** : Inputs bruts GLFW
- **Application gère** : Mapping "jump" → Space, "fire" → Mouse1, etc.
- Permet configuration personnalisée par l'utilisateur final

## 🛠️ Commandes de développement

```bash
# Tests input
ctest -R Input
./test --filter="*Input*"
```

## 🔗 Fichiers importants

- `Manager.cpp/.hpp` - Gestionnaire central, dispatch événements, accès Controllers
- `KeyboardInterface.hpp` - Interface pour événements clavier
- `MouseInterface.hpp` - Interface pour événements souris
- `KeyboardController.cpp/.hpp` - Consultation directe état clavier
- `MouseController.cpp/.hpp` - Consultation directe état souris
- Autres interfaces/controllers selon périphériques (Gamepad, Joystick)

## ⚡ Patterns de développement

### Utilisation par événements (reactive)
```cpp
// 1. Hériter de l'interface appropriée
class PlayerController : public KeyboardInterface, public MouseInterface {
public:
    // Événements clavier
    void onKeyPress(Key key) override {
        if (key == Key::Space) {
            player->jump();
        }
    }

    void onKeyRelease(Key key) override {
        if (key == Key::W) {
            player->stopMovingForward();
        }
    }

    // Événements souris
    void onMouseMove(double x, double y) override {
        camera->rotate(x, y);
    }

    void onMouseClick(MouseButton button, double x, double y) override {
        if (button == MouseButton::Left) {
            weapon->fire();
        }
    }
};

// 2. Enregistrer dans InputManager
PlayerController controller;
inputManager.registerKeyboardListener(&controller);
inputManager.registerMouseListener(&controller);
```

### Utilisation par polling (direct)
```cpp
// Dans la boucle de jeu (logique frame)
void updatePlayer(float dt) {
    // Consultation directe de l'état
    auto& keyboard = inputManager.keyboard();
    auto& mouse = inputManager.mouse();

    // Déplacement continu
    if (keyboard.isKeyPressed(Key::W)) {
        player->moveForward(dt);
    }
    if (keyboard.isKeyPressed(Key::S)) {
        player->moveBackward(dt);
    }
    if (keyboard.isKeyPressed(Key::A)) {
        player->moveLeft(dt);
    }
    if (keyboard.isKeyPressed(Key::D)) {
        player->moveRight(dt);
    }

    // Rotation caméra
    auto mousePos = mouse.position();
    camera->lookAt(mousePos.x, mousePos.y);
}
```

### Mapping d'actions (dans application)
```cpp
// Application définit son propre système de mapping
class ActionMapper {
    std::map<std::string, Key> keyBindings;

    void setBinding(const std::string& action, Key key) {
        keyBindings[action] = key;
    }

    bool isActionActive(const std::string& action) {
        auto it = keyBindings.find(action);
        if (it != keyBindings.end()) {
            return inputManager.keyboard().isKeyPressed(it->second);
        }
        return false;
    }
};

// Usage
ActionMapper mapper;
mapper.setBinding("jump", Key::Space);
mapper.setBinding("fire", Key::Mouse1);

if (mapper.isActionActive("jump")) {
    player->jump();
}
```

### Désenregistrement
```cpp
// Important : désenregistrer avant destruction
inputManager.unregisterKeyboardListener(&controller);
inputManager.unregisterMouseListener(&controller);
// Puis détruire controller
```

## 🚨 Points d'attention

- **Désenregistrement** : Désenregistrer les listeners avant destruction
- **Pas de mapping intégré** : Application responsable du mapping actions
- **Double approche** : Choisir événements (reactive) ou polling (direct) selon besoin
- **GLFW dépendance** : Input wraps GLFW, suit ses limitations/capabilities
- **Thread safety** : Événements GLFW viennent du thread principal
- **OverlayManager prioritaire** : Enregistré automatiquement, peut consommer événements

## 📚 Documentation détaillée

Systèmes liés:
→ **@src/Overlay/AGENTS.md** - Client majeur du système Input
→ **GLFW documentation** - Pour détails sur périphériques supportés
