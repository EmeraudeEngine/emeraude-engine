# AVConsole (Audio Video Console)

Context for developing the Emeraude Engine AVConsole system.

## Module Overview

Audio-video mixing console for managing connections between cameras, microphones, speakers, and render-targets. Each Scene has its own AVConsole.

## AVConsole-Specific Rules

### Concept: Mixing Console
- **Audio Video Console**: AV mixing console abstraction
- **Per Scene**: Each Scene has its dedicated AVConsole
- **Connection management**: Links cameras, mics, speakers together and to outputs

### Main Features

**Camera management**:
- Automatic registration of Camera components
- Camera → render-target binding
- Switch between multiple cameras
- Active camera definition for user display

**Audio management**:
- Automatic registration of Microphone components
- Mic → audio output binding
- Listener (listen point) configuration
- Scene audio mix

**Render-to-texture**:
- Camera → custom render-target (not just main screen)
- Multiple cameras for multiple simultaneous render-targets
- Usage: mirrors, security cameras, portals, mini-maps, etc.

### Scene Integration
- **Observers**: AVConsole observes Camera/Microphone component additions
- **Automatic registration**: Components register automatically
- **Lifecycle**: AVConsole follows Scene lifecycle

### Camera Switch
```cpp
// Scene with multiple cameras
auto cam1 = node1->newCamera(..., "main_camera");
auto cam2 = node2->newCamera(..., "security_camera");

// AVConsole automatically detects both

// Switch active camera
scene->avConsole().setActiveCamera("main_camera");  // Player view
// or
scene->avConsole().setActiveCamera("security_camera");  // Security view
```

### Render-to-texture
```cpp
// Create custom render-target
auto renderTarget = graphics.createRenderTarget("mirror_view", 512, 512);

// Bind camera to render-target
auto mirrorCam = mirrorNode->newCamera(..., "mirror_camera");
scene->avConsole().bindCameraToTarget("mirror_camera", renderTarget);

// mirrorCam renders to renderTarget instead of main screen
// renderTarget can be used as texture on a mesh (mirror)
```

## Development Commands

```bash
# AVConsole tests
ctest -R AVConsole
./test --filter="*AVConsole*"
```

## Important Files

- `Manager.cpp/.hpp` - AVConsole manager (one per Scene)
- Integrated in Scene lifecycle
- `@src/Scenes/AGENTS.md` - General scene graph context
- `@src/Scenes/Component/Camera.hpp` - Camera component
- `@src/Scenes/Component/Microphone.hpp` - Microphone component

## Development Patterns

### Multi-camera with Switch
```cpp
// Setup multiple cameras in scene
auto playerNode = scene->root()->createChild("player", playerPos);
auto playerCam = playerNode->newCamera(90.0f, 16.0f/9.0f, 0.1f, 1000.0f, "player_view");

auto droneNode = scene->root()->createChild("drone", dronePos);
auto droneCam = droneNode->newCamera(120.0f, 16.0f/9.0f, 0.1f, 5000.0f, "drone_view");

// Dynamic switch
if (userPressedKey(Key::V)) {
    scene->avConsole().setActiveCamera("drone_view");  // Drone view
} else {
    scene->avConsole().setActiveCamera("player_view");  // Player view
}
```

### Mini-map with Render-to-texture
```cpp
// Top-down camera for mini-map
auto minimapNode = scene->root()->createChild("minimap_cam", Vec3(0, -100, 0));
minimapNode->lookDown();  // Top view
auto minimapCam = minimapNode->newCamera(60.0f, 1.0f, 0.1f, 200.0f, "minimap");

// Render-target for mini-map
auto minimapTarget = graphics.createRenderTarget("minimap_rt", 256, 256);
scene->avConsole().bindCameraToTarget("minimap", minimapTarget);

// Use minimapTarget as texture in overlay
overlayManager.displayTexture(minimapTarget, screenWidth - 266, 10, 256, 256);
```

### 3D Audio Configuration
```cpp
// Microphone attached to active camera (listener follows camera)
auto listener = playerNode->newMicrophone("player_ears");

// AVConsole automatically configures OpenAL listener
// Listener position and orientation follow the Node
```

## Critical Points

- **One AVConsole per Scene**: Not global, tied to Scene
- **Automatic registration**: Do not manually register Camera/Microphone
- **Active camera required**: At least one camera must be active for rendering
- **Render-target lifetime**: Manage custom render-target lifetimes
- **Performance**: Multiple render-targets = multiple render passes (GPU cost)
- **Unique audio listener**: Only one active listener (generally tied to active camera)

## Detailed Documentation

Related systems:
- @src/Scenes/AGENTS.md - Scene graph and components
- @src/Audio/AGENTS.md - 3D audio system
- @src/Graphics/AGENTS.md - Render-targets and rendering
- @docs/scene-graph-architecture.md - Complete Scenes architecture
