# Input System

Context for developing the Emeraude Engine input management system.

## Module Overview

GLFW-based input management system offering both direct device state querying and an event system. Supports keyboard, mouse, gamepads, joysticks.

## Input-Specific Rules

### Dual Approach Architecture

**Direct State (Polling)**: Query current state via Controllers
- Keyboard: Keys pressed/released
- Mouse: Position, buttons, scroll
- Gamepads/Joysticks: Axes, buttons

**Event System**: Callbacks for real-time reactions
- All GLFW events implemented
- Dispatch via interfaces to inherit
- Registration in InputManager

### GLFW Integration
- **Native support**: Everything GLFW supports is available
- **Complete events**: Key press/release, mouse move/click, gamepad, etc.
- **Cross-platform**: Automatic OS difference handling via GLFW

### Registration System

**Interfaces to inherit**:
- Keyboard interface (key events)
- Mouse interface (mouse events)
- Other interfaces per device

**Registration in Manager**:
```cpp
// Object inherits from KeyboardInterface
class MyController : public KeyboardInterface {
    void onKeyPress(Key key) override { ... }
    void onKeyRelease(Key key) override { ... }
};

// Registration
MyController controller;
inputManager.registerKeyboardListener(&controller);
```

**Usage example**:
- OverlayManager inherits keyboard/mouse interfaces
- Automatically registered at engine startup
- Receives events and dispatches them to Screen/Surface

### Direct Query (Controllers)

Alternative to event system for state polling:
```cpp
// Query keyboard state
if (inputManager.keyboard().isKeyPressed(Key::W)) {
    moveForward();
}

// Query mouse state
auto mousePos = inputManager.mouse().position();
bool leftClick = inputManager.mouse().isButtonPressed(MouseButton::Left);
```

### Action Mapping (application responsibility)
- **Input provides**: Raw GLFW inputs
- **Application handles**: Mapping "jump" → Space, "fire" → Mouse1, etc.
- Allows user-customizable configuration

## Development Commands

```bash
# Input tests
ctest -R Input
./test --filter="*Input*"
```

## Important Files

- `Manager.cpp/.hpp` - Central manager, event dispatch, Controller access
- `KeyboardInterface.hpp` - Interface for keyboard events
- `MouseInterface.hpp` - Interface for mouse events
- `KeyboardController.cpp/.hpp` - Direct keyboard state query
- `MouseController.cpp/.hpp` - Direct mouse state query
- Other interfaces/controllers per device (Gamepad, Joystick)

## Development Patterns

### Event-based Usage (reactive)
```cpp
// 1. Inherit from appropriate interface
class PlayerController : public KeyboardInterface, public MouseInterface {
public:
    // Keyboard events
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

    // Mouse events
    void onMouseMove(double x, double y) override {
        camera->rotate(x, y);
    }

    void onMouseClick(MouseButton button, double x, double y) override {
        if (button == MouseButton::Left) {
            weapon->fire();
        }
    }
};

// 2. Register with InputManager
PlayerController controller;
inputManager.registerKeyboardListener(&controller);
inputManager.registerMouseListener(&controller);
```

### Polling-based Usage (direct)
```cpp
// In game loop (frame logic)
void updatePlayer(float dt) {
    // Direct state query
    auto& keyboard = inputManager.keyboard();
    auto& mouse = inputManager.mouse();

    // Continuous movement
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

    // Camera rotation
    auto mousePos = mouse.position();
    camera->lookAt(mousePos.x, mousePos.y);
}
```

### Action Mapping (in application)
```cpp
// Application defines its own mapping system
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

### Unregistration
```cpp
// Important: unregister before destruction
inputManager.unregisterKeyboardListener(&controller);
inputManager.unregisterMouseListener(&controller);
// Then destroy controller
```

## Critical Points

- **Unregistration**: Unregister listeners before destruction
- **No built-in mapping**: Application responsible for action mapping
- **Dual approach**: Choose events (reactive) or polling (direct) based on need
- **GLFW dependency**: Input wraps GLFW, follows its limitations/capabilities
- **Thread safety**: GLFW events come from main thread
- **OverlayManager priority**: Automatically registered, may consume events

## Detailed Documentation

Related systems:
- @src/Overlay/AGENTS.md - Major Input system client
- GLFW documentation - For supported device details
