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
 - Asio, 1.36.0 (https://github.com/chriskohlhoff/asio)
 - fastgltf, 0.9.0~ (https://github.com/spnda/fastgltf)
 - GLFW, ~master(2025.07.17) (https://github.com/EmeraudeEngine/glfw.git) [FORK]
 - Glslang, 16.0.0 (https://github.com/KhronosGroup/glslang.git)
 - ImGui, 1.92.4 (https://github.com/ocornut/imgui.git)
 - JsonCpp, 1.9.7~ (https://github.com/open-source-parsers/jsoncpp.git)
 - libsndfile, 1.2.2 (https://github.com/libsndfile/libsndfile) [SYSTEM+BINARY]
 - magic_enum, 0.9.7~ (https://github.com/Neargye/magic_enum)
 - Portable File Dialogs, unversioned (https://github.com/samhocevar/portable-file-dialogs.git)
 - reproc, 14.2.4~ (https://github.com/DaanDeMeyer/reproc)
 - SDL_GameControllerDB, unversioned (https://github.com/gabomdq/SDL_GameControllerDB.git)
 - Vulkan Memory Allocator, 3.3.0~ (https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator)

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


# Bugs and TODO-list

- GENERAL
  - Remove all invalid noexcept keyword. (WIP)
  - Increase inlining. (WIP)
  - Improve functions args to use "std::move" when useful.
  - Rewrite libs Observer/Observable pattern with the idea of static and shared objects.
  - Replace all "std::stringstream" by "std::format" (C++20) for simple keys, names or identifiers creation. WARNING: This doesn't work under macOS for targeting older SDKs.
  - Check for light coherence, create a built-in scene with a fixed directional light and multiple materials.
  - Issue on Linux with X11, multi-monitors and NVIDIA proprietary driver. More info: https://forums.developer.nvidia.com/t/external-monitor-freezes-when-using-dedicated-gpu/265406

- RENDERING SYSTEM
  - Enable additional render targets next to the main one to create shadow maps, 2D textures and reflections. (WIP)
  - Study and create the cubemap rendering (single-pass) to be able to produce reflections and shadow cubemap.
  - Improve the rendering branches to reduce cost.
  - Check sprite blending.
  - Check sprite texture clamping to edges.
  - Re-enable the screenshot from the engine.

- AUDIO SYSTEM
  - Check to stop sound from an inactive scene. (Shared with SCENE section)

- PHYSICS SYSTEM
  - Enable the rotational physics. (WIP)
  - Create a particle system using Compute Shader.

- RESOURCES SYSTEM
  - Merge Font from PixelFactory and FontResource.
  - Check the direct data description with the JSON resource description.
  - Check the store resource addition from the JSON resource description. (WIP)

- OVERLAY SYSTEM
  - Rework ComposedSurface from overlay to create a native menu.
  - Rewrite the TextWriter class.

- SCENE SYSTEM
  - Create a shared dynamic uniform buffer for static entities instead of using push_constants as they normally won't move between frames. Thus, it will only hold the model matrix.
  - Check enable/disable audio on scene switching. (Shared with the AUDIO section)

- ANIMATION SYSTEM
  - (Prior) Check all animatable properties for all objects.
  - Remove Variant for "std::any"

- CONSOLE SYSTEM
  - Bring back a useful console behavior.

- LIGHTING AND SHADOWING
  - Fix the ambient light update against the render target which uses it.
  - Re-enable the ambient light color generated by the averaging active light color.
  - Check the ambient light color generated by a texture.
  - Check what the light matrix for a shadow map is and how to update it. Hint, this is an MVP matrix from the light point of view.
  - Check light radius against entities to discard some useless rendering.
  - Shadow maps: Create a re-usable shadow map for ephemere lights.

- SHADERS CODE AND GENERATION
  - Check source and binary caches.
  - Prepare a way to use manual GLSL sources.
  - Re-enable normal calculation bypass when the surface is not facing a light.
  - Re-enable shadow maps. Depends on the offscreen rendering completion, but most of the code is done. (WIP)
  
- MATERIAL
  - Create a shared dynamic uniform buffer when a material does not use sampler. EDIT: Pay attention to the real benefit of that solution.
  - Create a material editor in JavaScript (application side). EDIT: Should be a tool for the engine.

- TEXTURING
  - Finish 1D and 3D textures.
  
- LIBRARIES AND EXTERNAL DEPENDENCIES
  - Determine a native 3D format for loading speedup.
  - Finish 3D formats loading/reading.

- VULKAN SPECIFIC
  - Find a better way to detect the UBO max capacity. For now the limit is hard-coded to 65,536 bytes.
  - Implement descriptorIndexing from Vulkan 1.2 API.
  - Implement VK_KHR_synchronization2 and VK_KHR_dynamic_rendering from Vulkan 1.3 API.
  - Check validation layers and debug messenger relationship. (According to khronos, this is valid to create the debug messenger without validation layers)
  - Fix texture loading. (UINT → UNORM) Convert data before? or not?
  - Use of a separated image from a sampler in GLSL when useful.
  - Find a better solution to handle shared uniform buffer objects.
  - Find a better way to create and store a descriptor set layout.
  - Find a way to order the rendering by pipeline layout to reduce the binding cost per draw. See VK_KHR_dynamic_rendering from Vulkan 1.1 API extension, upgraded in Vulkan 1.2 API and core in Vulkan 1.3 API.
  - Check for the right way to push constant from the right shader declaration to the right stage in vulkan. (GraphicsShaderGenerator.cpp:254)
  - Analyze UBO and instanced VBO usage and make a better and global shared UBO/VBO optimization for short life entity.
