# AGENTS.md - Emeraude Engine

Rules and conventions for AI agents interacting with Emeraude Engine.

## Project Overview

Emeraude Engine is a modern, cross-platform 3D graphics engine built with **Vulkan API** and **C++20**. It provides a complete framework for building 3D applications with integrated audio, physics simulation, and advanced rendering capabilities.

### Technical Architecture
- **Graphics API:** Vulkan 1.2+ (exclusive)
- **Language:** C++20 standard
- **Platforms:** Windows 11, Linux (Debian 13/Ubuntu 24.04), macOS SDK 12.0+
- **Build System:** CMake 3.25.1+
- **Coordinate System:** Right-handed Y-down (consistent across all subsystems)
- **License:** LGPLv3

### Key Systems
- **Saphir Shader System:** Automatic GLSL generation at runtime
- **Resource Management:** Asynchronous loading with fail-safe fallbacks
- **Scene Graph:** Hierarchical node-based scene management
- **Physics:** 4-entity type system (Boundaries, Ground, StaticEntity, Nodes)
- **Audio:** OpenAL-based 3D spatial audio
- **Overlay:** ImGui integration for UI and debug tools

## Code Conventions and Standards

### Design Philosophy

Emeraude Engine follows three fundamental design principles that guide all architectural and implementation decisions:

#### Principle of Least Astonishment (POLA)
**"The code should behave as a reasonable user would expect."**

The system should be intuitive and predictable. Functions, classes, and APIs should do exactly what their names suggest, without hidden side effects or surprising behaviors.

Examples:
```cpp
// CORRECT: Clear, predictable behavior
auto camera = scene->createCamera("main");  // Creates a camera, nothing else
camera->lookAt(target);                     // Points camera at target

// CORRECT: Consistent naming conventions
geometry->getVertexCount();   // get* prefix for accessors
geometry->setVertexData();    // set* prefix for mutators

// WRONG: Surprising side effects
camera->render();  // Should only render, not also create shaders or modify scene
```

#### Pit of Success
**"Make the right way easier than the wrong way."**

The architecture should guide developers toward correct, safe implementations by default, making it harder to write buggy or unsafe code.

Examples:
```cpp
// CORRECT: Fail-safe by design (impossible to crash)
auto texture = resources.get<TextureResource>("missing.png");
// Returns neutral texture if missing, application continues safely

// CORRECT: RAII ensures cleanup (impossible to leak)
{
    auto buffer = device->createBuffer(size);
    // Automatic destruction, no manual cleanup needed
}

// CORRECT: Type safety prevents errors at compile time
Vector3 gravity(0, +9.81, 0);  // Y-down enforced by convention

// ANTI-PATTERN: Requires manual error checking (easy to forget)
auto texture = loadTexture("file.png");
if (texture == nullptr) {  // Developer MUST remember to check
    // Handle error...
}
```

#### Avoid the Gulf of Execution
**"Never create a gap between user intention and the actions needed to accomplish it."**

High-level (user-facing) APIs must be simple and direct, without complex parameters or convoluted logic. End-user APIs should be trivial to use.

Examples:
```cpp
// CORRECT: Simple API, clear intention, zero complexity
auto texture = resources.get<TextureResource>("albedo.png");
camera->lookAt(target);

// CORRECT: Minimal parameters, intelligent defaults
auto renderTarget = std::make_shared<RenderTarget::Texture>(
    "MyTarget", precisions, extent, viewDistance
);

// WRONG: Too many required parameters (Gulf of Execution)
auto texture = resources.get<TextureResource>(
    "albedo.png",
    VK_FORMAT_R8G8B8A8_SRGB,           // Why should user know this?
    VK_IMAGE_TILING_OPTIMAL,            // Unnecessary complexity
    VK_IMAGE_USAGE_SAMPLED_BIT,         // Should be automatic
    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    mipmapLevels, arrayLayers
);

// WRONG: Complex logic required (Gulf of Execution)
if (scene->hasCamera()) {
    auto camera = scene->getCamera();
    if (camera->isInitialized()) {
        if (camera->getType() == CameraType::Perspective) {
            // ... 10 lines of setup
        }
    }
}
```

Where this applies:
- User-facing APIs (Resources, Scene, Camera, Material): Maximum simplicity
- Mid-level APIs (Graphics, Physics): Complexity/control trade-off acceptable
- Low-level APIs (Vulkan abstractions): Complexity acceptable, not exposed to users

How these principles apply to Emeraude Engine:
- Fail-safe Resources = Pit of Success (impossible to crash from nullptr)
- Y-down Coordinate System = Least Astonishment (total consistency, no surprises)
- Vulkan Abstraction = Avoid Gulf of Execution (complexity hidden from users)
- RAII Memory Management = Pit of Success (automatic cleanup, can't forget)

### General Rules
1. **Language:** All code, comments, and documentation in standard English
2. **Cross-Platform:** Compatible across Windows 11, Linux (Debian 13/Ubuntu 24.04), macOS SDK 12.0+
3. **Code Priorities:** Readability → Maintainability → Scalability → Performance
4. **C++20 Standard:** All code must adhere to C++20 standard
5. **Code Formatting:** Use `.clang-format` and `.clang-tidy` configurations
6. **Documentation:** Doxygen-style comments for all public APIs with `@version` tag (see below)
7. **Dependencies:** No new third-party libraries without explicit approval
8. **Licensing:** LGPLv3 compatible code only

### Doxygen @version Convention

All Doxygen documentation blocks must include `@version` with the current engine version from CMakeLists.txt:

```cpp
/**
 * @brief Description of the class/method.
 * @param param Description.
 * @return Description.
 * @see RelatedClass
 * @version 0.8.35
 */
```

This enables:
- **Smart refresh detection**: `/update-doxygen-document` skips files where `@version` matches current + no Git changes
- **Drift detection**: Outdated `@version` + Git changes → documentation needs refresh
- **Visual freshness indicator**: Developers can quickly see documentation age

### Critical Project Rules
- **Coordinate System:** Y-down convention used throughout ALL systems (see @docs/coordinate-system.md)
- **Vulkan Integration:** Never call Vulkan functions directly; use abstractions in `Graphics/`
- **Resource Safety:** Never return nullptr; always provide valid fallback resources
- **Memory Management:** Use VMA for GPU memory, RAII for CPU memory
- **Synchronization:** Careful management of Vulkan fences and semaphores

## Essential Commands

### Quick Build

**Linux:**
```bash
git submodule update --init --recursive
cmake -S . -B cmake-build-release -G "Ninja" -DCMAKE_BUILD_TYPE=Release
cmake --build cmake-build-release --config Release
```

**macOS (Apple Silicon):**
```bash
git submodule update --init --recursive
cmake -S . -B cmake-build-release -G "Ninja" -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES=arm64
cmake --build cmake-build-release --config Release
```

**macOS (Intel):**
```bash
git submodule update --init --recursive
cmake -S . -B cmake-build-release -G "Ninja" -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES=x86_64
cmake --build cmake-build-release --config Release
```

**Windows:**
```bash
git submodule update --init --recursive
cmake -S . -B cmake-build-release -G "Visual Studio 17 2022" -A x64
cmake --build cmake-build-release --config Release
```

### Testing
```bash
ctest --test-dir cmake-build-release
```

### Code Quality
```bash
clang-format -i src/**/*.{cpp,hpp}
clang-tidy src/**/*.{cpp,hpp}
```

### Dependencies
External precompiled libraries required:
- Download from: https://drive.google.com/drive/folders/1nDv35NuAPEg-XAGQIMZ7uCoqK3SK0VxZ
- Or build using: https://github.com/EmeraudeEngine/ext-deps-generator

## Important Files and References

### Core Documentation
- `README.md` - Project overview and build instructions
- `CMakeLists.txt` - Main build configuration
- `@docs/coordinate-system.md` - Y-down coordinate system (CRITICAL)
- `@docs/physics-system.md` - 4-entity physics architecture
- `@docs/resource-management.md` - Fail-safe resource system
- `@docs/saphir-shader-system.md` - Automatic shader generation
- `@docs/scene-graph-architecture.md` - Entity-component scene organization

### Source Code Structure
`src/` - Main source organized by subsystems (see CLAUDE.md for complete list)

**Framework Core:**
- `src/` - Core components (Core, Tracer, Window, Settings, Arguments) (@src/AGENTS.md)

**Core Systems:**
- `src/Vulkan/` - Vulkan abstraction layer (@src/Vulkan/AGENTS.md)
- `src/Graphics/` - High-level graphics abstraction (@src/Graphics/AGENTS.md)
- `src/Saphir/` - Shader generation system (@src/Saphir/AGENTS.md)
- `src/Physics/` - Physics simulation and collision (@src/Physics/AGENTS.md)
- `src/Resources/` - Resource management system (@src/Resources/AGENTS.md)

**Scene & Entities:**
- `src/Scenes/` - Scene graph and entities (@src/Scenes/AGENTS.md)
- `src/Scenes/AVConsole/` - Audio-Video console (@src/Scenes/AVConsole/AGENTS.md)

**Audio & Input:**
- `src/Audio/` - OpenAL audio system (@src/Audio/AGENTS.md)
- `src/Input/` - Input management (@src/Input/AGENTS.md)

**UI & Platform:**
- `src/Overlay/` - 2D overlay system (@src/Overlay/AGENTS.md)
- `src/PlatformSpecific/` - OS-specific code (@src/PlatformSpecific/AGENTS.md)

**Foundation & Tools:**
- `src/Libs/` - Foundational libraries (@src/Libs/AGENTS.md)
- `src/Net/` - Network downloading (@src/Net/AGENTS.md)
- `src/Testing/` - Unit tests (@src/Testing/AGENTS.md)
- `src/Animations/`, `src/Console/`, `src/Tool/` - In development

### Dependencies and Configuration
- `dependencies/` - Git submodules and precompiled external libraries
- `cmake/` - CMake configuration scripts
- `.clang-format` - Code formatting rules
- `.clang-tidy` - Static analysis configuration

## Documentation Maintenance Reminder

**IMPORTANT FOR AI AGENT:** After completing significant work that introduces or modifies:
- New concepts, patterns, or architectural decisions
- New classes, systems, or subsystems
- Modified behaviors or deprecated approaches
- Bug fixes that reveal important constraints or edge cases
- Integration points between systems

**You MUST ask the user if the documentation should be updated.**

- **Claude Code:** Use the `/update-docs` command (runs in current conversation context, preserving all nuances)
- **Other AI agents:** Read `.claude/commands/update-docs.md` for the documentation update methodology, then follow it manually

This ensures knowledge is captured while the context is fresh.

## Development Workflows

### Application Development (Core Inheritance)

Applications inherit from `EmEn::Core` and implement lifecycle callbacks. See `src/Core.hpp:1111-1315`.

**Required callbacks** (pure virtual):
- `onCoreStarted()` - Scene initialization, return true to start main loop
- `onCoreProcessLogics(size_t)` - Game logic (runs on separate logic thread)

**Optional callbacks** (have default implementations):
- `onBeforeCoreSecondaryServicesInitialization()` - Pre-init checks (e.g., --help)
- `onCorePaused()` / `onCoreResumed()` - Pause handling
- `onBeforeCoreStop()` - Cleanup before shutdown
- `onCoreKeyPress()` / `onCoreKeyRelease()` - Keyboard input
- `onCoreCharacterType()` - Unicode text input
- `onCoreNotification()` - Observer pattern events
- `onCoreOpenFiles()` - Drag & drop file handling
- `onCoreSurfaceRefreshed()` - Window resize handling

**Minimal example:**
```cpp
class MyApp : public EmEn::Core {
public:
    MyApp(int argc, char** argv) : Core{argc, argv, "MyApp", {1,0,0}, "Org", "org.com"} {}
private:
    bool onCoreStarted() noexcept override { return true; }
    void onCoreProcessLogics(size_t) noexcept override {}
};
```

### Adding New Features
1. **Design Phase:** Consider architecture impact on existing systems
2. **Implementation:** Follow coordinate system conventions and resource patterns
3. **Integration:** Use existing abstractions (Vulkan/, Physics/, Resources/)
4. **Testing:** Add unit tests and integration tests
5. **Documentation:** Update relevant documentation and add Doxygen comments

### Physics Development
- **Entity Types:** Use appropriate type (Boundaries/Ground/StaticEntity/Nodes)
- **Coordinate System:** Respect Y-down convention in all calculations
- **Contact Manifolds:** Follow established collision resolution patterns
- **Performance:** Consider broad-phase vs narrow-phase collision detection

### Graphics Development
- **Vulkan Usage:** Use abstractions in `src/Vulkan/`; never direct API calls
- **Shader Generation:** Extend Saphir system rather than manual GLSL
- **Memory Management:** Use VMA for all GPU memory allocation
- **Synchronization:** Proper fence and semaphore management

### Resource Development
- **Neutral Resources:** Always implement fail-safe default versions
- **Dependencies:** Use dependency chain system for complex resources
- **Thread Safety:** Resources load asynchronously; handle properly
- **Fallbacks:** System must never crash on missing/corrupt resources

## Critical Points of Attention

### Coordinate System (BREAKING)
- **Y-DOWN throughout entire engine** - never flip coordinates
- Physics: Gravity is `+Y`, jump impulse is `-Y`
- Rendering: Camera and all transforms use Y-down
- Audio: 3D positioning uses consistent coordinate system

### Memory Management
- **CPU:** RAII patterns, smart pointers, no raw pointers
- **GPU:** VMA allocation for all Vulkan resources
- **Resources:** Reference counting with automatic cleanup

### Cross-Platform Compatibility
- **Platform Code:** Isolate in `PlatformSpecific/` directory
- **File Paths:** Use forward slashes internally
- **APIs:** Abstract platform differences through interfaces

### Performance Considerations
- **Critical Paths:** Rendering, physics simulation, audio processing
- **Non-Critical:** Resource loading, UI, debug systems
- **Optimization:** Profile before optimizing; readability first

## Detailed Documentation

For comprehensive information on specific systems, refer to:

- **Physics System:** @docs/physics-system.md
- **Resource Management:** @docs/resource-management.md
- **Coordinate System:** @docs/coordinate-system.md
- **Saphir Shader System:** @docs/saphir-shader-system.md
- **Scene Graph:** @docs/scene-graph-architecture.md
- **Component Builder Pattern:** @docs/component-builder-pattern.md
- **Graphics System:** @docs/graphics-system.md
- **RenderableInstance System:** @docs/renderable-instance-system.md
- **Build Setup:** README.md (dependencies and compilation)
- **API Reference:** Doxygen comments in header files

## Development Environment

### Required Tools
- **CMake:** 3.25.1+
- **Python:** 3.0+ (dependency configuration)
- **C++20 Compiler:**
  - Linux: GCC 13.3.0+ or GCC 14.2.0+
  - macOS: Apple Clang 17.0+ with SDK 12.0+
  - Windows: MSVC 19.43+ (Visual Studio 2022)

### IDE Support
Any CMake-compatible IDE works (CLion, Visual Studio, VSCode, Qt Creator). The engine is developed with CLion but not IDE-dependent.

### External Dependencies
Requires precompiled external libraries from [ext-deps-generator](https://github.com/EmeraudeEngine/ext-deps-generator) repository. See README.md for setup instructions.