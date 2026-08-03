# Emeraude Engine

![License](https://img.shields.io/badge/license-LGPLv3-blue.svg)
![Version](https://img.shields.io/badge/version-0.9.51%20beta-green.svg)
![Platform](https://img.shields.io/badge/platform-Linux%20%7C%20macOS%20%7C%20Windows-lightgrey.svg)
![Graphics](https://img.shields.io/badge/graphics-Vulkan%20only-red.svg)

A modern, cross-platform 3D **runtime** built with **Vulkan** and **C++20**: rendering, audio,
physics, resources and scene management in one library, driven by your own `EmEn::Core`
subclass.

The engine targets **production-grade real-time visual quality** — the runtime that ships on
every end-user machine, not an ecosystem. No editor suite, no blueprints, no marketplace: full
source access, LGPLv3, zero royalties.

## Where to read what

Each level of the cascade documents only what it owns:

```
ext-deps-generator  →  emeraude-base  →  emeraude-engine  →  your application
```

| Level | Documents |
|---|---|
| [`emeraude-base`](https://github.com/EmeraudeEngine/emeraude-base/blob/main/README.md) | The foundation library (`EmEn::Base`), the **toolchain requirements**, the **compile policy**, and **everything about external dependencies** (prebuilt archives, naming grammar, auto-download, overrides). |
| **`emeraude-engine`** (here) | The Vulkan runtime: systems, Vulkan SDK, engine submodules & options, the application lifecycle, runtime control and GPU debugging. |
| [`projet-nihil`](https://github.com/EmeraudeEngine/projet-nihil/blob/main/README.md) | A minimal, complete example application built on this engine — the newcomer's entry door. |

> [!IMPORTANT]
> **External dependencies are documented only in emeraude-base.** The engine does not configure
> them: `emeraude-base` owns every `Setup*.cmake` and resolves the prebuilt archives — including
> the C libraries only the engine links (OpenAL-Soft, reproc…). If a dependency fails to
> resolve, read the base README, not this one.

## Foundation: emeraude-base

The engine's foundation layer — math, image/audio/mesh factories, hashing, compression,
threading, traits, I/O — lives in the standalone
[emeraude-base](https://github.com/EmeraudeEngine/emeraude-base) library (`EmEn::Base`, the
former `EmEn::Libs`). `cmake/InstallEmeraudeBase.cmake` clones it if absent and adds it as a
subdirectory, so you never manage it by hand.

This is a **real split with independent lifecycles**: emeraude-base evolves on its own, and the
engine builds on a pinned package of it.

```cmake
# Track a base branch other than main when needed:
-DEMERAUDE_BASE_GIT_BRANCH=develop
```

## Features

### Graphics

- **Vulkan 1.2+ only** — no D3D11/D3D12/Metal/OpenGL abstraction layer. Render passes,
  subpasses and layout transitions are first-class citizens; synchronization is explicit.
- **Saphir shader system** — GLSL generated at runtime from material properties and geometry,
  which removes hundreds of hand-written shader variants; pipeline and program caching included.
- **Bindless texture set** and **BC7 texture compression** at load time, with an on-disk cache.
- **Photometric lighting** — light quantities in real-world units (lux for illuminance, lumens
  for luminous power), sky-driven image-based lighting, shadow mapping, offscreen render targets.
- **Post-processing chain** — tone mapping (mandatory sensor stage), bloom, god rays, and
  single-pass lens effects compiled into the composite shader.
- **Overlay** — 2D layer for UI and debug, plus an **ImGui** integration.

### Simulation & data

- **Scene graph** — hierarchical nodes with automatic transform inheritance; several scenes may
  be loaded at once (one active) and switched or destroyed at runtime.
- **Physics** — fixed-timestep simulation, collision models (including capsules), sleep/wake,
  grounded state, adaptive terrain LOD, sea level.
- **Audio** — OpenAL-Soft 3D spatial audio with automatic position updates for scene-attached
  sources; procedural and MIDI/SoundFont playback via the foundation's `WaveFactory`.
- **Resources** — asynchronous loading with **fail-safe fallbacks**: a getter never returns
  null, it returns a valid default instead.
- **Asset loaders** — composite formats (glTF, FBX) on top of the foundation's mesh loaders.
- **Input** — keyboard, pointer and gamepad (SDL controller database).
- **Networking** — HTTP/download, UDP/SSDP discovery, serial port, Wi-Fi scanning.

### Tooling built into the runtime

- **Remote console over TCP (port 7777)** — query the live engine, take screenshots, inject
  keyboard/mouse events, or push a whole JSON scene. See
  [`docs/ai-runtime-control.md`](docs/ai-runtime-control.md).
- **Scene editor primitives** — picking, gizmos, entity manipulation.
- **AVConsole** — virtual audio/video devices for routing and capture.
- **RenderDoc integration** — programmatic GPU frame capture (see below).

### Conventions

- **Right-handed, Y-DOWN** coordinate system, consistent across physics, rendering and audio.
  Gravity is `+Y`.
- **C++20**, zero-cost abstractions on the hot paths, RAII everywhere, VMA for GPU memory.

## Requirements

The toolchain (CMake, compilers, Python, Git) is specified once, in the
[emeraude-base requirements](https://github.com/EmeraudeEngine/emeraude-base/blob/main/README.md#requirements).
On top of it, the engine needs:

### Vulkan SDK — 1.4.357.0

The SDK must be installed manually, at its default location.

**Linux** (Debian 13+ / Ubuntu 24.04+ / Mint 22.3+) — Vulkan plus the windowing/text system
packages the engine links against:

```bash
sudo apt install libvulkan-dev vulkan-tools vulkan-validationlayers vulkan-validationlayers-dev \
    libfontconfig-dev libwayland-dev libxkbcommon-dev xorg-dev
```

**macOS:** [vulkansdk-macos-1.4.357.0.zip](https://sdk.lunarg.com/sdk/download/1.4.357.0/mac/vulkansdk-macos-1.4.357.0.zip)
→ installed in `~/VulkanSDK/1.4.357.0/`.

**Windows:** [VulkanSDK-1.4.357.0-Installer.exe](https://sdk.lunarg.com/sdk/download/1.4.357.0/windows/VulkanSDK-1.4.357.0-Installer.exe)
→ installed in `C:/VulkanSDK/1.4.357.0/`.

CMake fails at configure time with the download URL if the SDK is not found.

### Git submodules

Cloned with `--recurse-submodules` and compiled with the engine:

| Library | Version | Repository |
|---|---|---|
| **GLFW** | `3bbf4c12` (2025-01-12) | [EmeraudeEngine/glfw](https://github.com/EmeraudeEngine/glfw) **[FORK]** |
| **ImGui** | 1.92.8 | [ocornut/imgui](https://github.com/ocornut/imgui) |
| **magic_enum** | 0.9.8 | [Neargye/magic_enum](https://github.com/Neargye/magic_enum) |
| **Vulkan Memory Allocator** | 3.3.0+46 (`b3cbbb43`) | [GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator) |
| **SDL_GameControllerDB** | unversioned (`366c416`) | [gabomdq/SDL_GameControllerDB](https://github.com/gabomdq/SDL_GameControllerDB) |
| **RenderDoc** | v1.44 (branch `v1.x`) | [baldurk/renderdoc](https://github.com/baldurk/renderdoc) — optional, GPU debug tooling |

Everything else — prebuilt static libraries, `Setup*.cmake` scripts, compile policy — comes from
[emeraude-base](https://github.com/EmeraudeEngine/emeraude-base/blob/main/README.md#external-dependencies)
and is resolved automatically at configure time.

## Building from source

You normally do **not** build the engine on its own: an application clones it (see
[projet-nihil](https://github.com/EmeraudeEngine/projet-nihil/blob/main/README.md)). Build it
standalone to work on the engine itself.

**Linux:**
```bash
git clone --recurse-submodules https://github.com/EmeraudeEngine/emeraude-engine.git
cd emeraude-engine
cmake -S . -B cmake-build-release -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build cmake-build-release --config Release -j$(nproc)
```

**macOS** (`$(uname -m)` resolves to `arm64` or `x86_64`):
```bash
git clone --recurse-submodules https://github.com/EmeraudeEngine/emeraude-engine.git
cd emeraude-engine
cmake -S . -B cmake-build-release -G Ninja -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_OSX_ARCHITECTURES=$(uname -m)
cmake --build cmake-build-release --config Release -j$(sysctl -n hw.ncpu)
```

**Windows:**
```bash
git clone --recurse-submodules https://github.com/EmeraudeEngine/emeraude-engine.git
cd emeraude-engine
cmake -S . -B cmake-build-release -G "Visual Studio 17 2022" -A x64
cmake --build cmake-build-release --config Release -j
```

For a debug build, swap `Release` → `Debug` everywhere (build directory, `CMAKE_BUILD_TYPE`,
`--config`). The library lands in `cmake-build-<config>/<Config>/`.

### Build options

| Option | Default | Effect |
|---|---|---|
| `EMERAUDE_BUILD_SERVICES_ONLY` | `Off` | Build the engine services without the rendering stack. |
| `EMERAUDE_ENABLE_IMGUI` | `Off` | Compile the ImGui debug UI. |
| `EMERAUDE_ENABLE_RENDERDOC` | `Off` | RenderDoc in-application API (see below). |
| `EMERAUDE_USE_EXPLICIT_EXPORTS` | `On` | Windows: explicit `EMEN_API` export macro instead of `WINDOWS_EXPORT_ALL_SYMBOLS`. See [`docs/windows-export-api.md`](docs/windows-export-api.md). |
| `EMERAUDE_USE_STATIC_RUNTIME` | `Off` | Windows: static MSVC runtime (`/MT`). Also selects the matching prebuilt archive. |
| `EMERAUDE_ENABLE_ASAN` / `MSAN` / `LSAN` / `TSAN` / `UBSAN` | `Off` | Sanitizers (Linux only). |
| `EMERAUDE_DEBUG_KEYBOARD_INPUT`, `EMERAUDE_DEBUG_POINTER_INPUT`, `EMERAUDE_DEBUG_WINDOW_EVENTS`, `EMERAUDE_DEBUG_VULKAN_TRACKING` | `Off` | Console tracing of the matching subsystem. |

Options of the compile policy itself (standards, warnings-as-errors, exceptions, PCH) are
declared by
[emeraude-base](https://github.com/EmeraudeEngine/emeraude-base/blob/main/README.md#options).

## Writing an application

An application derives from `EmEn::Core` and overrides the lifecycle hooks it needs. This is the
whole contract:

```cpp
#include <EmEn/Core.hpp>

class MyApplication final : public EmEn::Core
{
    public:

        MyApplication (int argc, char * * argv) noexcept
            : Core{argc, argv, "MyApp", {1, 0, 0}, "MyOrg", "example.com"}
        {}

    private:

        /* Required: the engine is fully initialized — build your scene here. */
        bool onCoreStarted (const EmEn::Arguments & arguments, EmEn::Settings & settings) noexcept override
        {
            return true;   /* false aborts the run */
        }

        /* Required: called every logic cycle — update your game logic here. */
        void onCoreProcessLogics (size_t engineCycle) noexcept override {}
};

int main (int argc, char * * argv)
{
    MyApplication app{argc, argv};

    return app.run() ? EXIT_SUCCESS : EXIT_FAILURE;
}
```

Optional hooks: `onBeforeCoreSecondaryServicesInitialization()` (pre-init checks such as
`--help`), `onCorePaused()` / `onCoreResumed()`, `onBeforeCoreStop()`, `onCoreKeyPress()` /
`onCoreKeyRelease()`, `onCoreCharacterType()`, `onCoreNotification()`, `onCoreOpenFiles()`
(drag & drop), `onCoreSurfaceRefreshed()`.

> [!TIP]
> For a **complete, commented application** — scene, terrain, camera, photometric lighting,
> post-processing, animations, input — read
> [projet-nihil](https://github.com/EmeraudeEngine/projet-nihil/blob/main/README.md). It is the
> reference implementation of this contract.

## Built-in shortcuts

Every application built on the engine inherits these, before adding its own:

| Shortcut | Action |
|---|---|
| **Shift+Escape** | Quit the application. |
| **Shift+F1** | Print the active scene content in the console. |
| **Shift+F2** | Toggle the physical camera panel (requires ImGui). |
| **Shift+F3** | Toggle scene editor mode. |
| **Shift+F4** | Reset the window size to defaults. |
| **Shift+F5** | Open the settings file in a text editor. |
| **Shift+F6** / **F7** / **F8** | Open the configuration / cache / user-data directory. |
| **Shift+F9** | Clean up unused resources from the managers. |
| **Shift+F10** | Suspend the core thread for 3 seconds. |
| **Shift+F11** | Toggle fullscreen. |
| **Shift+F12** | Take a screenshot. |
| **Shift+Ctrl+F12** | Toggle video recording. |
| **Shift+C** | Trigger a RenderDoc frame capture (when built with RenderDoc). |

## Runtime control

The engine exposes a **remote console on TCP port 7777**, usable while the application runs:

```bash
python3 tools/remote-console.py "Core.RendererService.screenshot()"
python3 tools/remote-console.py "Core.SceneManagerService.getSceneInfo()"
python3 tools/remote-console.py "Core.InputManagerService.keyPress(292, 1)"   # Shift+F3
```

On Linux/macOS `nc` works too (`echo "CMD" | nc -q 2 localhost 7777`, `-w 2` on macOS). Full
command reference: [`docs/ai-runtime-control.md`](docs/ai-runtime-control.md).

## GPU debugging with RenderDoc (optional, Linux only)

Built-in support for [RenderDoc](https://renderdoc.org/) frame capture through the
in-application API. When `EMERAUDE_ENABLE_RENDERDOC=OFF` (the default), all of it compiles to
zero-cost no-ops.

**Prerequisites:**

```bash
sudo apt install python3-dev swig bison libxcb-keysyms1-dev

# RenderDoc runtime (renderdoccmd, qrenderdoc):
wget https://renderdoc.org/stable/1.43/renderdoc_1.43.tar.gz
sudo tar xzf renderdoc_1.43.tar.gz -C /opt/
```

> [!IMPORTANT]
> **Register the Vulkan implicit layer yourself** — the tarball ships a manifest with an
> incorrect `library_path`, and without a valid one the loader silently never captures anything.
> Adjust the path to your installation:
> ```bash
> mkdir -p ~/.config/vulkan/implicit_layer.d
> cat > ~/.config/vulkan/implicit_layer.d/renderdoc_capture.json << 'EOF'
> {
>   "file_format_version" : "1.1.2",
>   "layer" : {
>     "name": "VK_LAYER_RENDERDOC_Capture",
>     "type": "GLOBAL",
>     "library_path": "/opt/renderdoc_1.43/lib/librenderdoc.so",
>     "api_version": "1.4.324",
>     "implementation_version": "43",
>     "description": "Debugging capture layer for RenderDoc",
>     "functions": {
>       "vkGetInstanceProcAddr": "VK_LAYER_RENDERDOC_CaptureGetInstanceProcAddr",
>       "vkGetDeviceProcAddr": "VK_LAYER_RENDERDOC_CaptureGetDeviceProcAddr",
>       "vkNegotiateLoaderLayerInterfaceVersion": "VK_LAYER_RENDERDOC_CaptureNegotiateLoaderLayerInterfaceVersion"
>     },
>     "enable_environment": { "ENABLE_VULKAN_RENDERDOC_CAPTURE": "1" },
>     "disable_environment": { "DISABLE_VULKAN_RENDERDOC_CAPTURE_1_43": "1" }
>   }
> }
> EOF
> ```

**Build with capture support** (the MIT-licensed `renderdoc_app.h` header is used from the
submodule; no linking — the library is detected at runtime through the layer):

```bash
git submodule update --init dependencies/renderdoc
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DEMERAUDE_ENABLE_RENDERDOC=ON
cmake --build build --config Debug -j$(nproc)
```

**Capture:** launch under `renderdoccmd capture --wait-for-exit ./your-app`, or from
`qrenderdoc` (*File > Launch Application*). Press **F12** (RenderDoc) or **Shift+C** (engine
shortcut) to capture the next presented frame; the `.rdc` file lands in
`{userDataDir}/RenderDoc/{unix_timestamp}_capture.rdc`. In qrenderdoc, the Event Browser,
Pipeline State, Texture, Mesh and Resource views expose every draw call, render target and
buffer of the frame.

**Python analysis module** — for programmatic inspection of `.rdc` captures (draw-call counting,
pipeline state, texture/buffer enumeration):

```bash
cmake -P cmake/BuildRenderDocPython.cmake   # builds dependencies/renderdoc/build/lib/renderdoc.so

PYTHONPATH=dependencies/renderdoc/build/lib \
LD_LIBRARY_PATH=dependencies/renderdoc/build/lib \
python3 your_script.py capture.rdc
```

## Troubleshooting

### macOS: correct sky, pure black terrain

**MoltenVK 1.4 or newer is required.** SDK 1.4.357.0 bundles MoltenVK **1.2.11**, which fails to
bind the samplers of a bindless descriptor array into its Metal argument buffer. The scene then
renders with a correct sky but a **pure black terrain**, metal objects invisible — image-based
lighting contributes nothing, and there is no error message.

```bash
vulkaninfo | grep -i moltenvk          # or: MVK_CONFIG_LOG_LEVEL=3 ./your-app
brew install molten-vk
export VK_DRIVER_FILES=/opt/homebrew/Cellar/molten-vk/1.4.2/etc/vulkan/icd.d/MoltenVK_icd.json
```

Installing a newer MoltenVK next to the SDK (its own prefix, SDK untouched) or a newer Vulkan
SDK that bundles MoltenVK ≥ 1.4 both work. More in
[`docs/troubleshooting.md`](docs/troubleshooting.md) § *macOS / MoltenVK Issues*.

### Submodules missing after clone

```bash
git submodule update --init --recursive
```

### External dependencies fail to resolve

Read the
[emeraude-base external dependencies section](https://github.com/EmeraudeEngine/emeraude-base/blob/main/README.md#external-dependencies)
— folder grammar, download fallbacks and overrides all live there.

## Known issues

- **Linux/X11:** multi-monitor setups with the NVIDIA proprietary driver may freeze. See the
  [NVIDIA forums](https://forums.developer.nvidia.com/t/external-monitor-freezes-when-using-dedicated-gpu/265406)
  for workarounds.

## Status

Beta, actively developed. Version history: [`CHANGELOG.md`](CHANGELOG.md). Current tasks and
known defects: [`TODO.md`](TODO.md) and
[GitHub Issues](https://github.com/EmeraudeEngine/emeraude-engine/issues).

## Documentation

Engine-internal documentation lives in [`docs/`](docs/) — start with:

| Document | Content |
|---|---|
| [`docs/architecture-philosophy.md`](docs/architecture-philosophy.md) | Design axioms and why the engine is shaped this way. |
| [`docs/coordinate-system.md`](docs/coordinate-system.md) | The Y-DOWN convention, in full. |
| [`docs/graphics-system.md`](docs/graphics-system.md) | Rendering architecture (+ `graphics-subsystems.md`, `render-targets.md`, `shadow-mapping.md`). |
| [`docs/saphir-shader-system.md`](docs/saphir-shader-system.md) | Runtime shader generation. |
| [`docs/scene-graph-architecture.md`](docs/scene-graph-architecture.md) | Scenes, nodes, components (+ `multi-scene-resource-ownership.md`). |
| [`docs/resource-management.md`](docs/resource-management.md) | Async loading and fail-safe resources. |
| [`docs/physics-system.md`](docs/physics-system.md) | Physics and collision. |
| [`docs/ai-runtime-control.md`](docs/ai-runtime-control.md) | Remote console command reference. |
| [`docs/troubleshooting.md`](docs/troubleshooting.md) | Platform and driver issues. |
| [`AGENTS.md`](AGENTS.md) | Guidelines for AI coding assistants. |

## Contributing

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes
4. Push the branch and open a Pull Request

Follow the existing style ([`docs/cpp-conventions.md`](docs/cpp-conventions.md) — tabs, C++20,
cross-platform strict) and keep warnings clean: they are errors by default.

## License

**GNU Lesser General Public License v3.0 (LGPLv3)** — see [`LICENSE`](LICENSE). You can use the
engine in open-source **and** commercial projects, link against it without opening your own
code, and modify the engine itself (modifications stay LGPLv3).

## Support

- **Issues:** [GitHub Issues](https://github.com/EmeraudeEngine/emeraude-engine/issues)
- **Discussions:** [GitHub Discussions](https://github.com/EmeraudeEngine/emeraude-engine/discussions)
