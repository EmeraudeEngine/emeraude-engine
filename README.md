# Emeraude Engine

This project is a cross-platform engine to render a scene in 3D using the Vulkan API and written in C++20. There is an audio layer and a minimal physics simulation layer.

This library provides :
- a scene manager based on a node system.
- a resources manager to load data hierarchically.
- a material manager with auto-generated shaders
- an overlay manager to draw on screen.
- ...

The name comes from a DOS game called "The Legend of Kyrandia" which makes extensive use of gemstones. As a child, I was a fan of emeralds in this game.

This software is distributed under the LGPLv3 license.


# Tools requirements

- CMake 3.25.1+, to generate the project.
- Python 3+, to configure some external dependencies.
- A C++20 compiler, the engine is maintained from :
   - "Debian 13 (GNU/Linux)" using "G++ 14.2.0" compiler
   - "Ubuntu 24.04 LTS (GNU/Linux)" using "G++ 13.3.0" compiler
   - "Apple macOS Sequoia 15.5" using "Apple Clang 17.0" compiler and the minimal SDK version 12.0
   - "Microsoft Windows 11" using "MSVC 19.43.34812.0" compiler ("Visual Studio 2022 Community Edition")

The engine is written with CLion, but it is not mandatory to use it. You can use any other IDE that supports CMake.


# External dependencies

## Precompiled binaries

The engine needs some external libraries, most of them are provided by the external repository https://github.com/EmeraudeEngine/ext-deps-generator
This project builds the Debug and Release binaries in its "output/" directory. These folders must be copied or symlinked into the "dependencies/" folder of the engine and are helping to speed up the overall compilation.
See the README.md file of the external repository for more information.

The result should be something like, for Linux:
- `./projet-nihil/dependencies/emeraude-engine/dependencies/linux.x86_64.Release`
- `./projet-nihil/dependencies/emeraude-engine/dependencies/linux.x86_64.Debug`
- 
or for macOS:
- `./projet-nihil/dependencies/emeraude-engine/dependencies/mac.arm64.Release`
- `./projet-nihil/dependencies/emeraude-engine/dependencies/mac.arm64.Debug`
- `./projet-nihil/dependencies/emeraude-engine/dependencies/mac.x86_64.Release`
- `./projet-nihil/dependencies/emeraude-engine/dependencies/mac.x86_64.Debug`
- 
or for Windows:
- `./projet-nihil/dependencies/emeraude-engine/dependencies/windows.x86_64.Release-MD`
- `./projet-nihil/dependencies/emeraude-engine/dependencies/windows.x86_64.Debug-MD`
- `./projet-nihil/dependencies/emeraude-engine/dependencies/windows.x86_64.Release-MT`
- `./projet-nihil/dependencies/emeraude-engine/dependencies/windows.x86_64.Debug-MT`

NOTE: These binaries can be downloaded with ZIP archives from https://drive.google.com/drive/folders/1nDv35NuAPEg-XAGQIMZ7uCoqK3SK0VxZ?usp=drive_link
But don't expect the URL will last forever or be stable.

## Git submodules

There are other dependencies compiled directly with the final binary:
 - SDL_GameControllerDB, ~master(2025.07.28) (https://github.com/gabomdq/SDL_GameControllerDB.git)
 - Asio C++ Library, 1.34.2 (https://github.com/chriskohlhoff/asio)
 - GLFW, ~master(2025.07.17) (https://github.com/EmeraudeEngine/glfw.git) [FORK]
 - Glslang, 15.4.0 (https://github.com/KhronosGroup/glslang.git)
 - Dear ImGui, 1.92.1 (https://github.com/ocornut/imgui.git)
 - JsonCpp, 1.9.7pre (https://github.com/open-source-parsers/jsoncpp.git)
 - Portable File Dialogs, ~main(2025.03.21) (https://github.com/samhocevar/portable-file-dialogs.git)
 - reproc, 14.2.5 (https://github.com/DaanDeMeyer/reproc)

## The Vulkan SDK

The current version of the Vulkan SDK is 1.4.309.0.

One day, the Vulkan SDK will be embedded in the repository. For now, you need to install it manually.

 - For Linux, use the SDK provided by your distribution.
```bash 
sudo apt install libvulkan-dev vulkan-tools vulkan-validationlayers vulkan-validationlayers-dev
```
 - For macOS, you can download it from https://sdk.lunarg.com/sdk/download/1.4.309.0/mac/vulkansdk-macos-1.4.309.0.zip
 - For Windows, you can download it from https://sdk.lunarg.com/sdk/download/1.4.309.0/windows/VulkanSDK-1.4.309.0-Installer.exe

NOTE: Leave setup options to default and let install files into the default location.


# Quick compilation from the terminal

## Release compilation

```bash
git clone --recurse-submodules https://github.com/EmeraudeEngine/emeraude-engine.git
cmake -S ./emeraude-engine -B ./emeraude-engine/cmake-build-release -DCMAKE_BUILD_TYPE=Release
cmake --build ./emeraude-engine/cmake-build-release --config Release
```

This will produce the release shared library.

## Debug compilation

```bash
git clone --recurse-submodules https://github.com/EmeraudeEngine/emeraude-engine.git
cmake -S ./emeraude-engine -B ./emeraude-engine/cmake-build-debug -DCMAKE_BUILD_TYPE=Debug
cmake --build ./emeraude-engine/cmake-build-debug --config Debug
```

This will produce the debug shared library.


# Troubleshooting

## Glslang library

If CMake returns some problems concerning this library, you have to finish its installation (once) by executing a local python script.

```bash
cd ./emeraude-engine/dependencies/glslang
./update_glslang_sources.py
cd ../../..
```
