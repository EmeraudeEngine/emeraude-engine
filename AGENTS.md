# AGENTS.md - Emeraude Engine

This file follows the AGENTS.md standard convention for AI coding agents and defines the rules and conventions for any AI agent interacting with the `Emeraude Engine` project.

## 🎯 Project Overview

Emeraude Engine is a modern, cross-platform 3D graphics engine built with **Vulkan API** and **C++20**. It provides a complete framework for building 3D applications with integrated audio, physics simulation, and advanced rendering capabilities.

### 🏗️ Technical Architecture
- **Graphics API:** Vulkan 1.2+ (exclusive)
- **Language:** C++20 standard
- **Platforms:** Windows 11, Linux (Debian 13/Ubuntu 24.04), macOS SDK 12.0+
- **Build System:** CMake 3.25.1+
- **Coordinate System:** Right-handed Y-down (consistent across all subsystems)
- **License:** LGPLv3

### 🎨 Key Systems
- **Saphir Shader System:** Automatic GLSL generation at runtime
- **Resource Management:** Asynchronous loading with fail-safe fallbacks
- **Scene Graph:** Hierarchical node-based scene management
- **Physics:** 4-entity type system (Boundaries, Ground, StaticEntity, Nodes)
- **Audio:** OpenAL-based 3D spatial audio
- **Overlay:** ImGui integration for UI and debug tools

## 📋 Code Conventions and Standards

### General Rules
1. **Language:** All code, comments, and documentation in standard English
2. **Cross-Platform:** Compatible across Windows 11, Linux (Debian 13/Ubuntu 24.04), macOS SDK 12.0+
3. **Code Priorities:** Readability → Maintainability → Scalability → Performance
4. **C++20 Standard:** All code must adhere to C++20 standard
5. **Code Formatting:** Use `.clang-format` and `.clang-tidy` configurations
6. **Documentation:** Doxygen-style comments for all public APIs
7. **Dependencies:** No new third-party libraries without explicit approval
8. **Licensing:** LGPLv3 compatible code only

### Critical Project Rules
- **Coordinate System:** Y-down convention used throughout ALL systems (see @docs/coordinate-system.md)
- **Vulkan Integration:** Never call Vulkan functions directly; use abstractions in `Graphics/`
- **Resource Safety:** Never return nullptr; always provide valid fallback resources
- **Memory Management:** Use VMA for GPU memory, RAII for CPU memory
- **Synchronization:** Careful management of Vulkan fences and semaphores

## 🛠️ Essential Commands

```bash
# Project Setup
git submodule update --init --recursive
mkdir build && cd build
cmake ..

# Build
cmake --build . -j$(nproc)           # Linux/macOS
cmake --build . -j%NUMBER_OF_PROCESSORS%  # Windows

# Testing
ctest --parallel $(nproc)            # Run all tests
./Emeraude                          # Run main executable
./test                              # Run test executable

# Code Quality
clang-format -i src/**/*.{cpp,hpp}  # Format code
clang-tidy src/**/*.{cpp,hpp}       # Static analysis

# Dependencies (external precompiled libraries required)
# Download from: https://drive.google.com/drive/folders/1nDv35NuAPEg-XAGQIMZ7uCoqK3SK0VxZ
# Or build using: https://github.com/EmeraudeEngine/ext-deps-generator
```

## 🔗 Important Files and References

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

## ⚡ Development Workflows

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

## 🚨 Critical Points of Attention

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

## 📚 Detailed Documentation

For comprehensive information on specific systems, refer to:

- **Physics System:** @docs/physics-system.md
- **Resource Management:** @docs/resource-management.md  
- **Coordinate System:** @docs/coordinate-system.md
- **Saphir Shader System:** @docs/saphir-shader-system.md
- **Scene Graph:** @docs/scene-graph-architecture.md
- **Build Setup:** README.md (dependencies and compilation)
- **API Reference:** Doxygen comments in header files

## 🔧 Development Environment

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