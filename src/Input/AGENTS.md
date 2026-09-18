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

## Synthetic Event Injection (Remote Automation)

The Input::Manager supports injecting synthetic input events that bypass GLFW, enabling remote automation via the TCP console (port 7777). Manager inherits `Console::ControllableTrait` and registers as `Core.InputManagerService`.

### Console Commands
| Command | Example | Description |
|---------|---------|-------------|
| `keyPress(key, modifiers)` | `keyPress(292, 1)` = Shift+F3 | Injects key press + release |
| `mouseClick(x, y, button, mods)` | `mouseClick(1017, 1165)` = left click | Injects mouse press + release |
| `mouseMove(x, y)` | `mouseMove(960, 540)` | Injects pointer move (absolute) |

### Static Methods
```cpp
static void injectKeyEvent(int32_t key, int32_t modifiers, int32_t action = 1) noexcept;
static void injectMouseClickEvent(float positionX, float positionY, int32_t button = 0, int32_t modifiers = 0, int32_t action = 1) noexcept;
static void injectPointerMoveEvent(float positionX, float positionY) noexcept;
```

### Critical: Copy-Before-Iterate
Inject methods copy the listener vector before iterating. Handlers may add/remove listeners during dispatch (e.g., editor activation adds itself as listener), which would invalidate iterators on the original vector. GLFW callbacks don't hit this because press/release arrive in separate `glfwPollEvents()` calls.

### Coordinate Space
Injected mouse coordinates (`mouseClick`, `mouseMove` console commands, `injectMouseClickEvent`, `injectPointerMoveEvent`) are dispatched to listeners **as-is, without pointer scaling** — they must be given in the **dispatch space**, i.e. **physical framebuffer pixels** (`framebufferWidth`/`framebufferHeight` from `Core.WindowService.getState()`). Consumers (overlay hit-testing, scene editor picking) all work in that space.

### Pointer scaling (logical → physical)
`Manager::enablePointerScaling(xScale, yScale)` / `disablePointerScaling()` multiply the raw `glfwGetCursorPos` value before dispatch. The goal: **deliver pointer coordinates in physical framebuffer pixels** so they stay consistent with the framebuffer dimensions used by overlay hit-testing (`Overlay::Surface::isBelowPoint`, which compares against `framebufferProperties().width() * rectangle`, i.e. physical).

Whether scaling is enabled is decided by **`Core::updatePointerScaling()`** (`Core.cpp`):
- **macOS** (cursor in DIP) and **Linux/Wayland** (cursor in logical surface coords) → enabled, using the **surface** content scale `m_window.state().contentXScale/Y` (`glfwGetWindowContentScale`, fractional-aware — e.g. `1.5` at 150%, **not** the per-monitor integer scale `glfwGetMonitorContentScale`).
- **Linux/X11, Windows** → disabled (GLFW already reports physical pixels).

`Core::updatePointerScaling()` is called at init and again on **`Window::OSRequestsToRescaleContentBy`** (emitted by `windowContentScaleCallback` after updating `m_state.contentXScale`), so the factor follows live scale changes — window dragged to a monitor with a different scale (e.g. 2.0 → 1.0), or a Wayland fractional-scale change. This single mechanism fixes both the Wayland mixed-DPI input bug and the macOS monitor-switch (2.0→1.0) bug.

> Consumers that map this physical coordinate further (e.g. the consumer's `WebView::windowXToViewX`) must account for whether their own target buffer is physical or logical. See the consumer's own UI documentation, section "Pointer coordinate space".

## Critical Points

- **Unregistration**: Unregister listeners before destruction
- **No built-in mapping**: Application responsible for action mapping
- **Dual approach**: Choose events (reactive) or polling (direct) based on need
- **GLFW dependency**: Input wraps GLFW, follows its limitations/capabilities
- **Thread safety**: GLFW events come from main thread; inject methods are called from main thread via console poll
- **OverlayManager priority**: Automatically registered, may consume events
- **Copy-before-iterate**: Inject methods MUST copy listener list before dispatch (see above)

## Detailed Documentation

Related systems:
- [`../Overlay/AGENTS.md`](../Overlay/AGENTS.md) - Major Input system client
- [`../Scenes/Editor/AGENTS.md`](../Scenes/Editor/AGENTS.md) - Editor uses input injection for remote testing
- [`../Console/AGENTS.md`](../Console/AGENTS.md) - Console ControllableTrait for remote commands
- GLFW documentation - For supported device details

### Pointer priority — the overlay is served FIRST (fixed 2026-09-18)

`Input::Manager::addPointerListener()` inserts every listener **at the front**, so the newest one is
dispatched first. That is deliberate between scenes, and it was silently wrong for the UI: the
`Overlay::Manager` registers once at engine start-up, so **every scene created afterwards outranked
the interface drawn on top of it**.

> [!CAUTION]
> **Symptom: the application menu is perfectly drawn and completely dead.** Not frozen, not hidden —
> rendered, hover states and all, while every click falls through to the scene. Measured: clicking a
> menu entry changed **0.0000 %** of the pixels.
>
> It needs a scene whose `OrbitController` has a node to drive. `OrbitController::onButtonPress()`
> returns `false` when `m_controlledNode == nullptr` and `true` otherwise — so a **demo** scene, whose
> controller drives nothing, declines and the menu works, while the **model viewer**, which gives it
> the subject to orbit, consumes the click. That asymmetry is what hid the defect for so long: it
> only appeared after opening a viewer, which until 2026-09-18 meant dragging a file onto the window
> — never twice in a row.

`Overlay::Manager` now registers with `addPointerListener(this, true)`. The priority listener is
kept at **index 0 of the same vector**, so the five dispatch loops are untouched and none of them can
be forgotten; everything else keeps its newest-first order.

> [!NOTE]
> **No regression on the scene controls — MEASURED, 2026-09-18.** `Overlay::Manager::onButtonPress()`
> returns `false` for any screen that is empty, not visible or not listening, so with the menu closed
> the event reaches the orbit controller exactly as before. A held drag across the viewport of a
> model viewer moves **99.82 %** of the pixels after the change.
>
> ⚠️ That measurement only became possible with `mousePress`/`mouseRelease`, added the same day:
> `mouseClick` presses **and releases**, so every `mouseMove` sent afterwards arrives with the button
> already up and a drag-driven control sees nothing — the same drag through `mouseClick` reports
> **0.0000 %**. This note first shipped saying "verified by reading the code rather than by
> exercising it", which was true for about an hour. **A control that can only be driven by a held
> button is untestable until the console can hold one.**

> [!WARNING]
> `Scenes::Editor::Manager` also registers a pointer listener and had the same exposure. It is fixed
> by the same change, since the overlay now outranks every scene-side listener rather than just that
> one.

⚠️ `addPointerListener()` guarded against duplicates with `std::ranges::binary_search` over a vector
that is **never sorted** — front-insertion guarantees it is not. The guard could miss a real
duplicate or invent one. Now `std::ranges::find`.
