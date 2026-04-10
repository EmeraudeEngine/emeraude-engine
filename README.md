# Emeraude Engine

![License](https://img.shields.io/badge/license-LGPLv3-blue.svg)
![Version](https://img.shields.io/badge/version-0.8.37b-green.svg)
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

- **CMake:** 3.25.1+ or higher
- **Python:** 3.0+ or higher (for dependency configuration)
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

| Library                     | Version           | Repository |
|-----------------------------|-------------------|------------|
| **Asio**                    | 1.36.0            | [github.com/chriskohlhoff/asio](https://github.com/chriskohlhoff/asio) |
| **GLFW**                    | master(2026.02.16) | [github.com/EmeraudeEngine/glfw](https://github.com/EmeraudeEngine/glfw.git) [FORK] |
| **Glslang**                 | 16.2.0            | [github.com/KhronosGroup/glslang](https://github.com/KhronosGroup/glslang.git) |
| **ImGui**                   | 1.92.6            | [github.com/ocornut/imgui](https://github.com/ocornut/imgui.git) |
| **magic_enum**              | 0.9.7~            | [github.com/Neargye/magic_enum](https://github.com/Neargye/magic_enum) |
| **reproc**                  | 14.2.5            | [github.com/DaanDeMeyer/reproc](https://github.com/DaanDeMeyer/reproc) |
| **SDL_GameControllerDB**    | unversioned       | [github.com/gabomdq/SDL_GameControllerDB](https://github.com/gabomdq/SDL_GameControllerDB.git) |
| **tinysoundfont**          | unversioned       | [github.com/schellingb/TinySoundFont](https://github.com/schellingb/TinySoundFont.git) |
| **Vulkan Memory Allocator** | 3.3.0~            | [github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator) |

### Vulkan SDK

**Required Version:** 1.4.309.0

The Vulkan SDK must be installed manually (future versions may include it in the repository).

**Installation:**

**Linux:**

Debian 13
```bash
sudo apt install build-essential cmake python3 ninja-build libvulkan-dev vulkan-tools vulkan-validationlayers vulkan-validationlayers-dev libfontconfig-dev
```

Ubuntu 24.04 
```bash
sudo apt install build-essential cmake python3 ninja-build libvulkan-dev vulkan-tools vulkan-validationlayers vulkan-utility-libraries-dev libfontconfig-dev libwayland-bin
```

**macOS:**
Download from: https://sdk.lunarg.com/sdk/download/1.4.309.0/mac/vulkansdk-macos-1.4.309.0.zip

**Windows:**
Download from: https://sdk.lunarg.com/sdk/download/1.4.309.0/windows/VulkanSDK-1.4.309.0-Installer.exe

**Note:** Use default installation options and install to the default location.


## Building from Source

### Quick Start

Choose your platform and copy-paste the entire block:

**Linux:**
```bash
git clone --recurse-submodules https://github.com/EmeraudeEngine/emeraude-engine.git
cd emeraude-engine
cmake -S . -B cmake-build-release -G "Ninja" -DCMAKE_BUILD_TYPE=Release
cmake --build cmake-build-release --config Release
```

**macOS (Apple Silicon M1/M2/M3):**
```bash
git clone --recurse-submodules https://github.com/EmeraudeEngine/emeraude-engine.git
cd emeraude-engine
cmake -S . -B cmake-build-release -G "Ninja" -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES=arm64
cmake --build cmake-build-release --config Release
```

**macOS (Intel):**
```bash
git clone --recurse-submodules https://github.com/EmeraudeEngine/emeraude-engine.git
cd emeraude-engine
cmake -S . -B cmake-build-release -G "Ninja" -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES=x86_64
cmake --build cmake-build-release --config Release
```

**Windows:**
```bash
git clone --recurse-submodules https://github.com/EmeraudeEngine/emeraude-engine.git
cd emeraude-engine
cmake -S . -B cmake-build-release -G "Visual Studio 17 2022" -A x64
cmake --build cmake-build-release --config Release
```

This produces the optimized shared library in `cmake-build-release/Release/`.

### Build Debug Version (Optional)

**Linux:**
```bash
cmake -S . -B cmake-build-debug -G "Ninja" -DCMAKE_BUILD_TYPE=Debug
cmake --build cmake-build-debug --config Debug
```

**macOS (Apple Silicon):**
```bash
cmake -S . -B cmake-build-debug -G "Ninja" -DCMAKE_BUILD_TYPE=Debug -DCMAKE_OSX_ARCHITECTURES=arm64
cmake --build cmake-build-debug --config Debug
```

**macOS (Intel):**
```bash
cmake -S . -B cmake-build-debug -G "Ninja" -DCMAKE_BUILD_TYPE=Debug -DCMAKE_OSX_ARCHITECTURES=x86_64
cmake --build cmake-build-debug --config Debug
```

**Windows:**
```bash
cmake -S . -B cmake-build-debug -G "Visual Studio 17 2022" -A x64
cmake --build cmake-build-debug --config Debug
```

This produces the debug shared library with symbols in `cmake-build-debug/Debug/`.

### Build Options

- **EMERAUDE_BUILD_SERVICES_ONLY:** Build only engine services without full rendering (default: OFF)

Example:
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DEMERАUDE_BUILD_SERVICES_ONLY=ON
```

## GPU Debugging with RenderDoc (Optional — Linux Only)

Emeraude Engine has built-in support for [RenderDoc](https://renderdoc.org/), the industry-standard GPU frame capture and debugging tool. When enabled at build time (`-DEMERAUDE_ENABLE_RENDERDOC=ON`), the engine can detect RenderDoc at runtime and provide programmatic frame capture via the in-application API.

The RenderDoc integration includes a Python module (`renderdoc.so`) built from source, enabling programmatic analysis of `.rdc` captures (draw call counting, pipeline state inspection, texture/buffer enumeration).

> **Platform:** Linux only (Debian/Ubuntu). macOS and Windows are not supported for this integration.

### Prerequisites

Install all required packages:

```bash
sudo apt install python3-dev swig bison libxcb-keysyms1-dev
```

Install the RenderDoc runtime (for `renderdoccmd` and `qrenderdoc`):

```bash
# Download the latest release from https://renderdoc.org/builds
wget https://renderdoc.org/stable/1.43/renderdoc_1.43.tar.gz
sudo tar xzf renderdoc_1.43.tar.gz -C /opt/
```

Register the Vulkan implicit layer (the tarball ships a manifest with an incorrect `library_path`):

```bash
mkdir -p ~/.config/vulkan/implicit_layer.d
cat > ~/.config/vulkan/implicit_layer.d/renderdoc_capture.json << 'EOF'
{
  "file_format_version" : "1.1.2",
  "layer" : {
    "name": "VK_LAYER_RENDERDOC_Capture",
    "type": "GLOBAL",
    "library_path": "/opt/renderdoc_1.43/lib/librenderdoc.so",
    "api_version": "1.4.324",
    "implementation_version": "43",
    "description": "Debugging capture layer for RenderDoc",
    "functions": {
      "vkGetInstanceProcAddr": "VK_LAYER_RENDERDOC_CaptureGetInstanceProcAddr",
      "vkGetDeviceProcAddr": "VK_LAYER_RENDERDOC_CaptureGetDeviceProcAddr",
      "vkNegotiateLoaderLayerInterfaceVersion": "VK_LAYER_RENDERDOC_CaptureNegotiateLoaderLayerInterfaceVersion"
    },
    "enable_environment": {
      "ENABLE_VULKAN_RENDERDOC_CAPTURE": "1"
    },
    "disable_environment": {
      "DISABLE_VULKAN_RENDERDOC_CAPTURE_1_43": "1"
    }
  }
}
EOF
```

> **Important:** Adjust `library_path` in the JSON to match your actual installation path. Without this manifest, the Vulkan loader will not load the RenderDoc capture layer, and frame capture will silently fail.

### Building with RenderDoc Support

Initialize the submodule and build:

```bash
git submodule update --init dependencies/renderdoc

cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DEMERAUDE_ENABLE_RENDERDOC=ON
cmake --build build --config Debug
```

The RenderDoc header (`renderdoc_app.h`, MIT-licensed) is used from the submodule. No linking is required — the library is detected at runtime via the Vulkan layer.

When `EMERAUDE_ENABLE_RENDERDOC=OFF` (default), all RenderDoc code compiles to zero-cost no-ops.

### Building the Python Module

The Python module enables programmatic analysis of `.rdc` captures:

```bash
cmake -P cmake/BuildRenderDocPython.cmake
```

This builds `renderdoc.so` in `dependencies/renderdoc/build/lib/`. The script handles all intermediate steps automatically (PCRE1, custom SWIG fork, SWIG bindings). Requires the packages listed in Prerequisites above.

Once built, analyze captures with:

```bash
PYTHONPATH=dependencies/renderdoc/build/lib \
LD_LIBRARY_PATH=dependencies/renderdoc/build/lib \
python3 your_script.py capture.rdc
```

### Using RenderDoc

#### Automated Capture (CLI)

Use `renderdoccmd` to inject the capture layer and capture a frame externally:

```bash
cd <your-build-dir>/Debug

# Capture frames during runtime. Use Shift+C in-app to trigger a capture,
# or use the RenderDoc GUI for interactive capture.
renderdoccmd capture --wait-for-exit \
    ./your-app --load-demo <demo-name>
```

The `.rdc` capture file is saved to `{userDataDir}/RenderDoc/{unix_timestamp}_capture.rdc`.

#### Manual Capture (GUI)

1. Launch `qrenderdoc` (the RenderDoc GUI):
   ```bash
   /opt/renderdoc_1.43/bin/qrenderdoc
   ```
2. **File > Launch Application**
3. Set the executable path to your application binary
4. Set the working directory to the directory containing the executable
5. Add any command-line arguments (e.g., `--load-demo <demo-name>`)
6. Click **Launch**
7. Press **F12** (RenderDoc default) or **Shift+C** (engine shortcut) to capture a frame
8. The captured frame appears in the qrenderdoc UI for inspection

#### Keyboard Shortcut

While the application is running under RenderDoc:

| Shortcut | Action |
|----------|--------|
| **Shift+C** | Trigger capture of the next presented frame |

#### What to Look For

In qrenderdoc, after opening a `.rdc` capture:

- **Event Browser:** Shows all Vulkan commands in the frame — render passes, draw calls, dispatches, copies
- **Pipeline State:** Inspect bound shaders, descriptor sets, push constants, blend state for any draw call
- **Texture Viewer:** View any render target, shadow map, or texture at any point in the frame
- **Mesh Viewer:** Inspect vertex input/output for any draw call
- **Resource Inspector:** Browse all allocated buffers, images, and their contents


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
    // Required: Called when engine is fully initialized - setup your scene here
    bool onCoreStarted(const EmEn::Arguments & arguments, EmEn::Settings & settings) noexcept override {
        return true;  // Return true to run the application
    }

    // Required: Called every frame - update game logic here (runs on logic thread)
    void onCoreProcessLogics(size_t engineCycle) noexcept override {}

    // Optional overrides available:
    // - onBeforeCoreSecondaryServicesInitialization() : Pre-init checks (e.g., --help)
    // - onCorePaused() / onCoreResumed() : Pause handling
    // - onBeforeCoreStop() : Cleanup before shutdown
    // - onCoreKeyPress() / onCoreKeyRelease() : Keyboard input
    // - onCoreCharacterType() : Text input
    // - onCoreNotification() : Observer pattern events
    // - onCoreOpenFiles() : Drag & drop files
    // - onCoreSurfaceRefreshed() : Window resize handling
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
