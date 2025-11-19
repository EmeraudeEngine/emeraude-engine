# Emeraude Engine

![License](https://img.shields.io/badge/license-LGPLv3-blue.svg)
![Version](https://img.shields.io/badge/version-0.8.31-green.svg)
![Platform](https://img.shields.io/badge/platform-Linux%20%7C%20macOS%20%7C%20Windows-lightgrey.svg)

A modern, cross-platform 3D graphics engine built with **Vulkan API** and **C++20**. Emeraude Engine provides a complete framework for building 3D applications with integrated audio, physics simulation, and advanced rendering capabilities.

## Features

### Core Systems
- **Scene Graph System:** Hierarchical node-based scene management with automatic transform inheritance
- **Resource Management:** Asynchronous loading with automatic fallbacks - never returns null, always provides valid resources
- **Saphir Shader System:** Automatic GLSL shader generation at runtime based on material properties and geometry - eliminates hundreds of manual shader variants
- **Audio Layer:** 3D spatial audio using OpenAL with automatic position updates for scene-attached sources
- **Physics Simulation:** Fixed timestep physics with 4 entity types (Boundaries, Ground, StaticEntity, Nodes) for optimized collision handling
- **Overlay System:** 2D rendering layer for UI elements and debug information
- **ImGui Integration:** Built-in debug UI and development tools
- **Multi-Platform:** Native support for Linux, macOS, and Windows

### Technical Highlights
- **Vulkan API 1.2+** for modern graphics rendering
- **Right-handed Y-down coordinate system** consistent across all subsystems (physics, rendering, audio)
- **C++20** codebase with modern language features
- **Zero-cost abstractions** optimized for performance in critical paths

## About

**License:** LGPLv3 - Free for both open-source and commercial projects


## Requirements

### Build Tools

- **CMake:** 3.25.1 or higher
- **Python:** 3.0 or higher (for dependency configuration)
- **C++20 Compiler:**
  - **Linux:** GCC 13.3.0+ or GCC 14.2.0+
    - Tested on Debian 13 and Ubuntu 24.04 LTS
  - **macOS:** Apple Clang 17.0+ with SDK 12.0+
    - Tested on macOS Sequoia 15.5
  - **Windows:** MSVC 19.43+ (Visual Studio 2022)
    - Tested on Windows 11

**IDE Support:** While the engine is developed with CLion, any CMake-compatible IDE will work (Visual Studio, VSCode, Qt Creator, etc.).


## Dependencies

### Precompiled External Libraries

The engine requires several precompiled external libraries provided by [ext-deps-generator](https://github.com/EmeraudeEngine/ext-deps-generator). This separate repository builds Debug and Release binaries that must be copied or symlinked into the engine's `dependencies/` folder to significantly speed up compilation.

**Setup Instructions:**

1. Clone and build the ext-deps-generator repository (see its README for details)
2. Copy or symlink the output directories to your engine's dependencies folder

**Expected directory structure:**

**Linux:**
```
<your-project>/dependencies/emeraude-engine/dependencies/
├── linux.x86_64.Release/
└── linux.x86_64.Debug/
```

**macOS:**
```
<your-project>/dependencies/emeraude-engine/dependencies/
├── mac.arm64.Release/
├── mac.arm64.Debug/
├── mac.x86_64.Release/
└── mac.x86_64.Debug/
```

**Windows:**
```
<your-project>/dependencies/emeraude-engine/dependencies/
├── windows.x86_64.Release-MD/
├── windows.x86_64.Debug-MD/
├── windows.x86_64.Release-MT/
└── windows.x86_64.Debug-MT/
```

**Quick Download:** Pre-built binaries are available as ZIP archives from [Google Drive](https://drive.google.com/drive/folders/1nDv35NuAPEg-XAGQIMZ7uCoqK3SK0VxZ?usp=drive_link) (note: this URL may change or become unavailable in the future).

### Git Submodules

The following dependencies are included as git submodules and compiled directly with the engine:

| Library | Version | Repository |
|---------|---------|------------|
| **Asio** | 1.36.0 | [github.com/chriskohlhoff/asio](https://github.com/chriskohlhoff/asio) |
| **fastgltf** | 0.9.0~ | [github.com/spnda/fastgltf](https://github.com/spnda/fastgltf) |
| **GLFW** | master(2025.07.17) | [github.com/EmeraudeEngine/glfw](https://github.com/EmeraudeEngine/glfw.git) [FORK] |
| **Glslang** | 16.0.0 | [github.com/KhronosGroup/glslang](https://github.com/KhronosGroup/glslang.git) |
| **ImGui** | 1.92.4 | [github.com/ocornut/imgui](https://github.com/ocornut/imgui.git) |
| **JsonCpp** | 1.9.7~ | [github.com/open-source-parsers/jsoncpp](https://github.com/open-source-parsers/jsoncpp.git) |
| **libsndfile** | 1.2.2 | [github.com/libsndfile/libsndfile](https://github.com/libsndfile/libsndfile) |
| **magic_enum** | 0.9.7~ | [github.com/Neargye/magic_enum](https://github.com/Neargye/magic_enum) |
| **Portable File Dialogs** | unversioned | [github.com/samhocevar/portable-file-dialogs](https://github.com/samhocevar/portable-file-dialogs.git) |
| **reproc** | 14.2.4~ | [github.com/DaanDeMeyer/reproc](https://github.com/DaanDeMeyer/reproc) |
| **SDL_GameControllerDB** | unversioned | [github.com/gabomdq/SDL_GameControllerDB](https://github.com/gabomdq/SDL_GameControllerDB.git) |
| **Vulkan Memory Allocator** | 3.3.0~ | [github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator) |

### Vulkan SDK

**Required Version:** 1.4.309.0

The Vulkan SDK must be installed manually (future versions may include it in the repository).

**Installation:**

**Linux:**
```bash
sudo apt install libvulkan-dev vulkan-tools vulkan-validationlayers vulkan-validationlayers-dev
```

**macOS:**
Download from: https://sdk.lunarg.com/sdk/download/1.4.309.0/mac/vulkansdk-macos-1.4.309.0.zip

**Windows:**
Download from: https://sdk.lunarg.com/sdk/download/1.4.309.0/windows/VulkanSDK-1.4.309.0-Installer.exe

**Note:** Use default installation options and install to the default location.


## Building from Source

### Quick Start

**1. Clone the repository with submodules:**
```bash
git clone --recurse-submodules https://github.com/EmeraudeEngine/emeraude-engine.git
cd emeraude-engine
```

**2. Build Release version:**
```bash
cmake -S . -B cmake-build-release -DCMAKE_BUILD_TYPE=Release
cmake --build cmake-build-release --config Release
```

This produces the optimized shared library in `cmake-build-release/Release/`.

**3. Build Debug version:**
```bash
cmake -S . -B cmake-build-debug -DCMAKE_BUILD_TYPE=Debug
cmake --build cmake-build-debug --config Debug
```

This produces the debug shared library with symbols in `cmake-build-debug/Debug/`.

### Build Options

- **EMERAUDE_BUILD_SERVICES_ONLY:** Build only engine services without full rendering (default: OFF)

Example:
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DEMERАUDE_BUILD_SERVICES_ONLY=ON
```

## Troubleshooting

### Glslang Configuration Issues

If CMake reports errors related to Glslang, you need to run its configuration script once:

```bash
cd dependencies/glslang
./update_glslang_sources.py
cd ../..
```

Then re-run CMake configuration.

### Submodule Issues

If submodules weren't cloned properly:

```bash
git submodule update --init --recursive
```

### Missing Dependencies

Ensure you've installed the Vulkan SDK and copied/symlinked the precompiled dependencies as described in the Dependencies section above.


## Usage Example

Here's a minimal compilable example to get started:

```cpp
#include <EmEn/Core.hpp>

class MyApplication : public EmEn::Core {
public:
    MyApplication(int argc, char** argv) noexcept
        : Core{argc, argv, "MyApp", {1, 0, 0}, "MyOrg", "example.com"} {}

private:
    // Called before graphics/audio initialization
    bool onBeforeSecondaryServicesInitialization() noexcept override {
        return false;  // Return false to continue initialization
    }

    // Called when engine is fully initialized - setup your scene here
    bool onStart() noexcept override {
        return true;  // Return true to run the application
    }

    // Called when application resumes (e.g., after pause)
    void onResume() noexcept override {}

    // Called every frame - update game logic here
    void onProcessLogics(size_t engineCycle) noexcept override {}

    // Called when application pauses
    void onPause() noexcept override {}

    // Called when application stops
    void onStop() noexcept override {}

    // Keyboard input handling
    bool onAppKeyPress(int32_t key, int32_t scancode, int32_t modifiers, bool repeat) noexcept override {
        return false;  // Return true if event was consumed
    }

    bool onAppKeyRelease(int32_t key, int32_t scancode, int32_t modifiers) noexcept override {
        return false;  // Return true if event was consumed
    }

    // Character input handling (for text input)
    bool onAppCharacterType(uint32_t unicode) noexcept override {
        return false;  // Return true if event was consumed
    }

    // Notification system for observables
    bool onAppNotification(const ObservableTrait* observable, int notificationCode, const std::any& data) noexcept override {
        return false;  // Return true to keep observing, false to forget
    }

    // File open events (drag & drop, system open)
    void onOpenFiles(const std::vector<std::filesystem::path>& filepaths) noexcept override {}
};

int main(int argc, char** argv) {
    MyApplication app(argc, argv);
    app.run();
    return 0;
}
```

For a complete working example with scene setup, lighting, camera, and animations, see the [projet-nihil](https://github.com/EmeraudeEngine/projet-nihil) repository.

## Current Development Status

Emeraude Engine is actively developed and includes the following systems:

**Completed:**
- Core rendering pipeline with Vulkan
- Scene graph and node management
- Resource loading (GLTF, textures, audio)
- Basic physics simulation
- Audio playback and 3D spatial audio
- ImGui integration for debugging

**In Progress:**
- Advanced shadow mapping (WIP)
- Offscreen render targets for reflections and effects (WIP)
- Enhanced physics with rotation (WIP)
- Shader caching optimization

**Planned:**
- Cubemap rendering for environment mapping
- Compute shader-based particle systems
- Advanced animation system
- Material editor tool

For detailed development tasks and known issues, please check the [GitHub Issues](https://github.com/EmeraudeEngine/emeraude-engine/issues) page.

## Known Issues

- **Linux/X11:** Multi-monitor setup with NVIDIA proprietary drivers may cause freezing. See [NVIDIA Forums](https://forums.developer.nvidia.com/t/external-monitor-freezes-when-using-dedicated-gpu/265406) for workarounds.
- **macOS:** `std::format` (C++20) incompatible with older SDK targets (< 13.0). Engine uses `std::stringstream` as fallback.

## Contributing

Contributions are welcome! Please:

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

Please ensure your code follows the existing style and includes appropriate tests.

## Documentation

- **API Documentation:** Coming soon
- **Wiki:** [GitHub Wiki](https://github.com/EmeraudeEngine/emeraude-engine/wiki) (coming soon)
- **Examples:** See [projet-nihil](https://github.com/EmeraudeEngine/projet-nihil) for a complete working example
- **AI Agents:** See [AGENTS.md](AGENTS.md) for guidelines when using AI coding assistants with this project

## License

This project is licensed under the **GNU Lesser General Public License v3.0 (LGPLv3)**.

This means you can:
- Use the engine in both open-source and commercial projects
- Link against the library without requiring your code to be open-source
- Modify the engine itself (modifications must be released under LGPLv3)

See the [LICENSE](LICENSE) file for full details.

## Support and Community

- **Issues:** Report bugs and request features on [GitHub Issues](https://github.com/EmeraudeEngine/emeraude-engine/issues)
- **Discussions:** Join conversations on [GitHub Discussions](https://github.com/EmeraudeEngine/emeraude-engine/discussions)

## Acknowledgments

- Built with love for the open-source game development community
- Thanks to all contributors and users of the engine
