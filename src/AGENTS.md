# Core Framework Components

Context for the base components at the root of src/ in Emeraude Engine.

## Overview

Fundamental framework components located directly in `src/`. These files constitute the engine core and orchestrate all subsystems (Graphics, Audio, Physics, Scenes, etc.).

## Main Components

### Core - Framework Heart
**Files**: `Core.cpp/.hpp` (26KB header, 37KB cpp)

**Central role**: Central point of the entire framework, main class to inherit from to produce an application.

**Responsibilities**:
- **Orchestration**: Coordinates all engine subsystems
- **Main loops**: Manages three execution loops
  - Main loop
  - Logic loop (separate thread)
  - Render loop (separate thread)
- **Lifecycle**: Entry point for overriding application behavior

**Usage pattern**:
```cpp
class MyApplication : public EmEn::Core {
public:
    MyApplication(int argc, char** argv) noexcept
        : Core{argc, argv, "MyApp", {1, 0, 0}, "MyOrg", "example.com"} {}

private:
    // Required: Setup your scene here
    bool onCoreStarted() noexcept override { return true; }

    // Required: Update game logic here (runs on logic thread)
    void onCoreProcessLogics(size_t cycle) noexcept override {}

    // Optional overrides: onBeforeCoreSecondaryServicesInitialization(),
    // onCorePaused(), onCoreResumed(), onBeforeCoreStop(),
    // onCoreKeyPress(), onCoreKeyRelease(), onCoreCharacterType(),
    // onCoreNotification(), onCoreOpenFiles(), onCoreSurfaceRefreshed()
};
```

**Mandatory callbacks** (pure virtual):
- `onCoreStarted()` - Scene initialization, return true to continue
- `onCoreProcessLogics(size_t)` - Game logic (separate thread)

**Optional callbacks** (default implementation):
- `onBeforeCoreSecondaryServicesInitialization()` - Pre-init (e.g., --help)
- `onCorePaused()` / `onCoreResumed()` - Pause handling
- `onBeforeCoreStop()` - Cleanup before shutdown
- `onCoreKeyPress()` / `onCoreKeyRelease()` - Keyboard input
- `onCoreCharacterType()` - Unicode text input
- `onCoreNotification()` - Observer pattern
- `onCoreOpenFiles()` - File drag & drop
- `onCoreSurfaceRefreshed()` - Window resize

### Tracer - Logging System
**Files**: `Tracer.cpp/.hpp`

**Role**: Runtime logging system for program execution tracing.

**Architecture**:
- **Tracer** (singleton): Main console/file logging service. See `Tracer.hpp:297-851`
- **TracerLogger**: Async logger with dedicated thread for non-blocking file I/O. See `Tracer.hpp:189-295`
- **TracerEntry**: Single log entry (timestamp, severity, tag, message, location, thread). See `Tracer.hpp:61-187`
- **T_TraceHelperBase**: CRTP template for RAII helpers. See `Tracer.hpp:857-953`

**Message types**:
- `info` - General information
- `warning` - Warnings
- `error` - Recoverable errors
- `fatal` - Critical errors (with optional `std::terminate()`)
- `success` - Successful operations
- `debug` - Debug (eliminated in Release via zero-overhead dummy class)

**RAII helper classes** (use CRTP via `T_TraceHelperBase`):
- `TraceInfo`, `TraceSuccess`, `TraceWarning`, `TraceError` - Inherit from `T_TraceHelperBase`
- `TraceFatal` - Separate class with `terminate` option
- `TraceAPI` - For tracing external API calls (Vulkan, OpenAL, etc.)
- `TraceDebug` - Release version = empty dummy class (guaranteed zero-overhead)

**Features**:
- **Flexible output**: Terminal with ANSI colors + log files (Text/JSON/HTML)
- **Metadata**: `std::source_location`, thread ID, automatic timestamps
- **Thread-safe**: Mutex for console, thread-safe queue for file
- **Performance**: TraceDebug Release eliminated at compilation (zero-cost)

**Usage**: See @AGENTS.md section "Tracer System Usage" for complete conventions.

### Window - OS Window Management
**Files**: `Window.cpp/.hpp` + `Window.{linux,mac,windows}.cpp`

**Role**: Cross-platform window abstraction via GLFW.

**Responsibilities**:
- **OS Window**: Creation, resize, movement, fullscreen
- **Events**: OS event handling (close, focus, minimize, etc.)
- **Vulkan Surface**: SwapChain creation for rendering
- **Platform-specific**: OS-specialized code (Linux/macOS/Windows)

**Integration**:
- Used by Core to create main window
- Provides Vulkan surface to Renderer
- No direct link with Input or Overlay

### Settings - Application Configuration
**Files**: `Settings.cpp/.hpp` (23KB header), `SettingKeys.hpp` (13KB)

**Role**: Global framework configuration system with JSON persistence.

**Features**:
- **Format**: JSON storage in OS-specific user config folders
- **Persistence**: Automatic save on application close
- **Live editing**: SHIFT+F5 opens file in default text editor
- **SettingKeys**: Defines all available configuration keys

**Parameter types**:
- Resolution, window mode
- Graphics quality, vsync
- Audio volumes
- Resource paths
- Debug options

**Hierarchy**:
Arguments > Settings > Default values

### Arguments - Command Line Parsing
**Files**: `Arguments.cpp/.hpp` (8KB header)

**Role**: Program argument manager (argc/argv).

**Features**:
- **Parsing**: Command line argument analysis
- **Dynamic addition**: Ability to add arguments on the fly
- **Override Settings**: Arguments stronger than Settings
- **Distribution**: Arguments passed to relevant subsystems

**Core usage**: Used at startup to configure initialization

**Examples**:
```bash
./MyApp --fullscreen --resolution 1920x1080 --debug-renderer
```

### PrimaryServices - Service Container
**Files**: `PrimaryServices.cpp/.hpp` (7KB each)

**Role**: Container for easy transport of main engine services.

**Included services**:
- **Arguments**: Argument parser
- **FileSystem**: File system
- **Settings**: Configuration
- **Net::Manager**: Network manager
- **ThreadPool**: Worker pool for async work

**Usage**: Simplifies dependency passing between components

### FileSystem - File System Abstraction
**Files**: `FileSystem.cpp/.hpp` (11KB header, 13KB cpp)

**Role**: Cross-platform abstraction for accessing system directories.

**Managed directories**:
- **Config**: OS-specific user config folder
- **Cache**: Application cache folder
- **Data**: User data folder
- **Temp**: Temporary folder

**Cross-platform**:
- Windows: AppData
- Linux: ~/.config, ~/.cache
- macOS: ~/Library/Application Support

**Integration**: Used by Settings and Resources to locate files

### ServiceInterface - Common Service Interface
**Files**: `ServiceInterface.hpp` (4KB)

**Role**: Base interface for all framework services.

**Common contract**:
- **Initialization**: Standardized init protocol
- **Shutdown**: Clean resource cleanup
- **Lifecycle**: Uniform service management

**Inheriting services**: Renderer, Physics, Audio, Resources, Net::Manager, etc.

### PlatformManager - Platform Detection
**Files**: `PlatformManager.cpp/.hpp` (2KB header, 5KB cpp)

**Role**: Runtime platform manager.

**Responsibilities**:
- **OS Detection**: Dynamically identifies Linux/Windows/macOS/Web
- **GLFW Initialization**: GLFW framework bootstrap
- **Coordination**: Coordinates with PlatformSpecific/ folder code

**Usage**: Allows adapting behavior based on OS at runtime

### Notifier - Onscreen Notifications
**Files**: `Notifier.cpp/.hpp` (5KB each)

**Role**: Visual notification system for end user.

**Features**:
- **Onscreen messages**: Message display (toasts/popups)
- **Types**: Info, warning, error, success
- **Non-blocking**: Doesn't interrupt execution

**Usage**: User feedback for important events

### CursorAtlas - Cursor Management
**Files**: `CursorAtlas.cpp/.hpp` (4KB each)

**Role**: System and custom cursor manager.

**Features**:
- **Standard cursors**: Arrow, hand, crosshair, text, etc. (via OS)
- **Custom cursors**: Loading from images
- **GLFW wrapper**: GLFW abstraction for cursors
- **CEF integration**: Cursor transmission to external libraries (CEF)

**Integration**: Used with Window to change cursor based on context

### Help - Command Line Help
**Files**: `Help.cpp/.hpp` (10KB header)

**Role**: Command line help system (--help).

**Features**:
- **Arguments**: Displays available argument list with descriptions
- **Formatting**: Formatted and colored help in terminal
- **Framework info**: Version, build info, dependencies

**Trigger**: `./MyApp -h` or `./MyApp --help`

### User - User Representation
**Files**: `User.cpp/.hpp` (2KB header)

**Role**: Optional end user generalization.

**Usage**: Per-user Settings/paths customization (optional)

### Utility Files

**Types.hpp** (4KB):
- Common types used at engine first level
- Global enums and typedefs
- Framework message types

**Constants.hpp** (6KB):
- Engine constants
- Logic update frequency
- Limits and default values

**Identification.hpp** (10KB):
- Final application identification
- Name, version, author, copyright
- Application metadata

## Development Patterns

### Creating an Application
```cpp
#include <EmEn/Core.hpp>

class MyGame : public EmEn::Core {
public:
    MyGame(int argc, char** argv) noexcept
        : Core{argc, argv, "MyGame", {1, 0, 0}, "MyOrg", "example.com"} {}

private:
    // Required: Called when engine is fully initialized
    bool onCoreStarted() noexcept override {
        TRACE_INFO("Game initialized");
        // Load scenes, resources, etc.
        return true;  // Return true to start main loop
    }

    // Required: Called every logic frame (separate thread)
    void onCoreProcessLogics(size_t engineCycle) noexcept override {
        updateGameState();
    }

    // Optional: Cleanup before shutdown
    void onBeforeCoreStop() noexcept override {
        TRACE_INFO("Game shutdown");
    }
};

int main(int argc, char** argv) {
    MyGame game(argc, argv);
    return game.run() ? EXIT_SUCCESS : EXIT_FAILURE;
}
```

### Using Settings
```cpp
// Reading
int width = settings.get<int>("window.width");
bool vsync = settings.get<bool>("graphics.vsync");

// Writing (saved on close)
settings.set("window.width", 1920);
settings.set("audio.master_volume", 0.8f);

// Manual editing: SHIFT+F5 during execution
```

### Using Tracer
```cpp
// Simple messages (static methods)
Tracer::info(ClassId, "Loading level");
Tracer::warning(ClassId, "Low FPS detected");
Tracer::error(ClassId, "Failed to load texture");
Tracer::fatal(ClassId, "Vulkan device lost");

// Formatted messages (RAII classes)
TraceInfo{ClassId} << "Loaded " << count << " textures";
TraceWarning{ClassId} << "FPS dropped to " << fps;
TraceError{ClassId} << "Failed to load: " << path;
TraceDebug{ClassId} << "Position: " << x << ", " << y;  // Eliminated in Release

// API trace (for external calls)
TraceAPI{ClassId, "vkCreateDevice"} << "Creating logical device";

// Log file
tracer.enableLogger(filepath);  // Activates async logger
```

**Important**: See @AGENTS.md section "Tracer System Usage" for complete rules (mandatory braces `{}`, method vs class choice, etc.).

### Using FileSystem
```cpp
// Cross-platform directories
auto configPath = fileSystem.getConfigDirectory();  // ~/.config/MyApp/
auto cachePath = fileSystem.getCacheDirectory();    // ~/.cache/MyApp/
auto dataPath = fileSystem.getDataDirectory();      // ~/.local/share/MyApp/
auto tempPath = fileSystem.getTempDirectory();      // /tmp/MyApp/

// Settings automatically uses configPath
// Resources can use dataPath for user assets
```

### Override with Arguments
```bash
# Arguments override Settings
./MyGame --window.width=2560 --window.height=1440 --graphics.vsync=false

# Custom arguments
./MyGame --custom-flag --my-value=42
```

```cpp
// In code
if (arguments.has("custom-flag")) {
    int value = arguments.get<int>("my-value");
}
```

## Critical Points

- **Core is central**: Everything goes through Core, don't bypass
- **Three threads**: Main, Logic, Render - watch thread safety
- **Arguments > Settings**: Strict priority hierarchy
- **Free debug tracer**: Debug messages eliminated in Release (zero cost)
- **Settings auto-save**: Automatic save, no need to call save()
- **SHIFT+F5**: Live Settings editing, watch for invalid values
- **Cross-platform FileSystem**: Never hardcode OS-specific paths
- **ServiceInterface**: All services must respect init/shutdown protocol
- **Window owns Surface**: Don't manually create Vulkan Surface
- **PlatformManager init GLFW**: Must be initialized before Window

## Subsystem Integration

### Core Orchestrates Everything
```
Core
 ├─ Window (window + Vulkan Surface)
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

### Initialization Flow
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

## Additional Documentation

Subsystems orchestrated by Core:
- @src/Graphics/AGENTS.md - High-level graphics system
- @src/Vulkan/AGENTS.md - Low-level Vulkan backend
- @src/Physics/AGENTS.md - Jolt physics engine
- @src/Audio/AGENTS.md - OpenAL spatial 3D audio
- @src/Scenes/AGENTS.md - Hierarchical scene graph
- @src/Resources/AGENTS.md - Fail-safe resource loading
- @src/Input/AGENTS.md - GLFW input management
- @src/Overlay/AGENTS.md - 2D overlay interface

Related concepts:
- @docs/coordinate-system.md - Y-down convention (CRITICAL)
- @docs/resource-management.md - Fail-safe loading

Platform-specific:
- @src/PlatformSpecific/AGENTS.md - OS-specialized code
