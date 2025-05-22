# Emeraude Engine (LGPLv3)

This project is a cross-platform engine to render a scene in 3D using the Vulkan API and written in C++20. There is an audio layer and a minimal physics simulation layer.

This lib provide :
- a scene manager based on a node system.
- a resources manager to load data hierarchically.
- a material manager with auto-generated shaders
- an overlay manager to draw on screen.
- ...

The name comes from a DOS game called "The Legend of Kyrandia" which makes extensive use of gemstones. As a child, I was a fan of emeralds in this game.

# External dependencies

The engine needs some external libraries, most of them are provided by an external repository (https://github.com/londnoir/ext-deps-generator) that creates binaries.
The "ext-deps-generator" project builds two folders in its "output/" directory. These two folders must be copied into the "dependencies/" folder of this project.

There are other submodules compiled directly with the final binary.

# Requirements

 - A C++20 compiler. This library is maintained from :
   - "Debian 13 (GNU/Linux)" using "G++ 14.2.0" compiler
   - "Ubuntu 24.04 LTS (GNU/Linux)" using "G++ 13.3.0" compiler
   - "Apple macOS Sequoia 15.5" using "Apple Clang 17.0" compiler and the minimal SDK version 12.0
   - "Microsoft Windows 11" using "MSVC 19.43.34812.0" compiler ("Visual Studio 2022 Community Edition")
 - CMake 3.25.1+ to generate the project
 - Python 3
 - Vulkan SDK 1.3.296.0 from https://vulkan.lunarg.com/sdk/home

Note: Further information will come here to complete dependent pieces of software and libraries installation. 
But this shouldn't be complicated to resolve problems described by CMake logs.

# Quick compilation from the terminal

For now, you need to manually copy the results of "ext-deps-generator" into the directory "emeraude-engine/dependencies/" to CMake find the static libraries.
Keep the exact directory name (linux|mac|windows).(x86_64|arm64)-(Debug|Release).

## Release compilation

```bash
git clone --recurse-submodules https://github.com/londnoir/emeraude-engine.git
cmake -S ./emeraude-engine -B ./emeraude-engine/cmake-build-release -DCMAKE_BUILD_TYPE=Release
cmake --build ./emeraude-engine/cmake-build-release --config Release
```

This will produce the release shared library.

## Debug compilation

```bash
git clone --recurse-submodules https://github.com/londnoir/emeraude-engine.git
cmake -S ./emeraude-engine -B ./emeraude-engine/cmake-build-debug -DCMAKE_BUILD_TYPE=Debug
cmake --build ./emeraude-engine/cmake-build-debug --config Debug
```

This will produce the debug shared library.
