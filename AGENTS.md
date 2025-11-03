# AGENTS.md - AI Agent Guidelines for Emeraude Engine

This file follows the AGENTS.md standard convention for AI coding agents and defines the rules and conventions for any AI agent interacting with the `Emeraude Engine` project.

## General Rules

1.  **Language:** All code, code comments, and the content of this rules file must be written in standard English.
2.  **Cross-Platform:** All code must be written to be cross-platform compatible, avoiding platform-specific APIs or features unless absolutely necessary and properly abstracted.
3.  **Code Priorities:** Code should prioritize the following, in order: readability, maintainability, scalability, and finally, performance.
4.  **Performance:** Performance should only be prioritized in critical parts of the codebase.
5.  **C++ Standard & Targets:** The code must adhere to the C++20 standard. Existing code can be updated to this standard while respecting cross-platform compatibility. The general target platforms are Windows 11, Debian 13/Ubuntu 24.04 (as a Linux base), and macOS SDK 12.0.
6.  **Code Formatting:** All C++ code must be formatted using the project's `.clang-format` file. All code should also be compliant with the checks defined in the `.clang-tidy` file.
7.  **Documentation:** All public APIs (classes, methods, functions in header files) must be documented using Doxygen-style comments (`/** ... */` or `///`). The documentation should explain the purpose of the code, its parameters, and its return value.
8.  **Dependencies:** Do not add any new third-party libraries or dependencies to the project without explicit prior approval.
9.  **Licensing:** This project is open-source and licensed under the LGPLv3. All new code contributions must be compatible with this license.

---

## Project Specific Rules

1.  **Framework Architecture:** This project is a multimedia framework with the following technical specifications:
    *   **Graphics & Compute:** Exclusively uses the Vulkan API (1.2+).
    *   **Shaders:** Shaders are written in GLSL and compiled at runtime using GLSLang.
    *   **Memory Management:** Uses the Vulkan Memory Allocator (VMA) library for GPU memory allocation.
    *   **Audio:** Uses the OpenAL library for 3D positional audio.

2.  **Vulkan Integration:**
    *   All rendering must go through the Vulkan abstraction layer in `Vulkan/`.
    *   Never call Vulkan functions directly from high-level code; use the provided abstractions in `Graphics/`.
    *   Synchronization primitives (fences, semaphores) must be managed carefully to avoid deadlocks.
    *   All Vulkan resources must be properly destroyed when no longer needed.

3.  **Saphir Shader System:**
    *   Saphir automatically generates shader code based on material properties and geometry requirements.
    *   Do not manually write shader code unless creating new Saphir features.
    *   Shader compilation errors should be reported through the shader compiler notification system.
    *   GLSL shaders must be compatible with GLSLang compiler.

4.  **Scene Graph Architecture:**
    *   The scene graph is the primary organizational structure for game objects.
    *   All scene objects (nodes) inherit from a base node class.
    *   Parent-child relationships define transformations and lifecycle dependencies.
    *   Scene graph updates happen in a specific order (transform update, then render).

5.  **Resource Management:**
    *   All resources (textures, models, sounds) must be loaded through the resource management system.
    *   Resources are reference-counted and automatically unloaded when no longer used.
    *   Resource loading should be asynchronous when possible to avoid blocking the main thread.
    *   Support multiple resource formats and provide proper error handling for unsupported formats.

6.  **Physics System:**
    *   Physics simulation is decoupled from rendering; it runs at a fixed timestep.
    *   Physics objects are synchronized with scene graph nodes.
    *   Collision detection uses spatial acceleration structures (octree, BVH).
    *   Physics queries (raycasting, shape casting) should be efficient and thread-safe.

7.  **Audio System:**
    *   Audio uses OpenAL for 3D positional audio.
    *   Audio sources are attached to scene nodes for automatic position updates.
    *   Support streaming for large audio files (music, ambient sounds).
    *   Provide audio source pooling for efficient runtime performance.

8.  **Input System:**
    *   Input is abstracted to support multiple platforms (keyboard, mouse, gamepad).
    *   Input events are processed through a centralized input manager.
    *   Support for input mapping and remapping at runtime.
    *   Handle platform-specific input quirks transparently.

9.  **Platform Abstraction:**
    *   Platform-specific code must be isolated in `PlatformSpecific/` directory.
    *   Use platform-independent interfaces; implement platform-specific backends.
    *   File paths should use forward slashes internally; convert to native format when needed.
    *   Handle platform differences in window management, file systems, and system APIs.

10. **Service Interfaces:**
    *   Core systems are exposed through service interfaces defined in `ServiceInterface.hpp`.
    *   Services follow a consistent initialization and shutdown pattern.
    *   Services should be loosely coupled and communicate through well-defined interfaces.

---

## Coding Conventions

1.  **Naming Conventions:**
    *   **Classes:** PascalCase (e.g., `VulkanDevice`, `SceneNode`, `ResourceManager`)
    *   **Methods/Functions:** camelCase (e.g., `updateTransform()`, `loadTexture()`)
    *   **Member Variables:** camelCase with `m_` prefix (e.g., `m_device`, `m_swapchain`)
    *   **Constants:** UPPER_SNAKE_CASE (e.g., `MAX_FRAMES_IN_FLIGHT`, `DEFAULT_RESOLUTION`)
    *   **Namespaces:** PascalCase (e.g., `EmEn`, `Vulkan`, `Graphics`)
    *   **Private Methods:** Use same convention as public methods; no special prefix required.

2.  **File Organization:**
    *   Header files (`.hpp`) contain class declarations, inline functions, and template implementations.
    *   Implementation files (`.cpp`) contain method definitions.
    *   Platform-specific implementations use suffixes: `.linux.cpp`, `.windows.cpp`, `.mac.cpp`, `.mac.mm` (Objective-C++).
    *   Keep related functionality grouped in subdirectories (e.g., `Vulkan/`, `Graphics/`, `Audio/`).
    *   One class per file pair (`.hpp`/`.cpp`) unless classes are tightly coupled.

3.  **Error Handling:**
    *   Use exceptions for critical errors that cannot be recovered (e.g., Vulkan device creation failure).
    *   Return error codes or `std::optional` for expected failures (e.g., resource not found).
    *   Log errors with appropriate severity levels (ERROR, WARNING, INFO, DEBUG).
    *   Validate all public API inputs and provide clear error messages.
    *   Never silently ignore errors; always log or propagate them.

4.  **Memory Management:**
    *   Prefer smart pointers (`std::unique_ptr`, `std::shared_ptr`) over raw pointers.
    *   Follow RAII (Resource Acquisition Is Initialization) principles strictly.
    *   Avoid manual memory management (`new`/`delete`) when possible.
    *   GPU memory allocations must go through VMA; never use `vkAllocateMemory` directly.
    *   Be mindful of resource lifetimes; ensure proper cleanup order (children before parents).

5.  **Headers and Includes:**
    *   Use `#pragma once` in all header files (preferred over include guards).
    *   Include only what is necessary; use forward declarations when possible to reduce compile times.
    *   Group includes in the following order:
        1. Standard library headers (`<vector>`, `<memory>`, etc.)
        2. Third-party library headers (`<vulkan/vulkan.h>`, `<AL/al.h>`, etc.)
        3. Engine headers from other modules (`"Graphics/Types.hpp"`, etc.)
        4. Headers from the same module
    *   Use angle brackets (`<>`) for external libraries, quotes (`""`) for engine headers.

6.  **Threading and Concurrency:**
    *   Be explicit about thread safety in documentation.
    *   Use mutexes, locks, and atomic operations appropriately.
    *   Avoid global mutable state; prefer thread-local storage or explicit synchronization.
    *   Rendering commands must be recorded from the main thread unless explicitly designed for multi-threading.
    *   Physics and resource loading can run on separate threads.

7.  **Vulkan-Specific Conventions:**
    *   Always check Vulkan function return codes; wrap in helper functions if needed.
    *   Use validation layers during development; provide option to disable in release builds.
    *   Properly transition image layouts before use.
    *   Batch command buffer submissions when possible to reduce overhead.
    *   Use descriptor sets efficiently; avoid creating new descriptors per frame.

---

## Project Structure

The `emeraude-engine` source code (`src/`) is organized as follows:

- `Core.hpp/cpp`: The engine's core components, including the main loop, application lifecycle, and initialization sequence.
  - Manages the main update loop and frame timing.
  - Coordinates service initialization and shutdown.
  - Provides base class for applications to inherit from.

- `ServiceInterface.hpp`: Defines abstract interfaces for core engine services.
  - Provides a consistent pattern for service initialization, update, and cleanup.
  - Allows loose coupling between engine subsystems.

- `User.hpp`: Defines user-specific data and preferences.

- `Vulkan/`: Contains all Vulkan-specific rendering code (low-level abstraction).
  - Device management (physical device selection, logical device creation).
  - Swapchain creation and presentation.
  - Command buffer allocation and management.
  - Synchronization primitives (fences, semaphores).
  - Pipeline creation and management.
  - Descriptor set management.
  - Memory allocation through VMA.
  - Render pass and framebuffer management.

- `Saphir/`: Automatic shader source code generation system.
  - Analyzes material properties and geometry requirements.
  - Generates GLSL shader code dynamically.
  - Compiles shaders at runtime using GLSLang.
  - Manages shader variants and permutations.
  - Handles shader compilation errors and notifications.

- `Graphics/`: High-level graphics abstractions, built upon the Vulkan layer.
  - Material system for defining surface properties.
  - Mesh and geometry management.
  - Camera and viewport management.
  - Lighting system (directional, point, spot lights).
  - Shadow mapping and shadow casters.
  - Render queue and draw call management.
  - Post-processing effects.
  - Texture and image management.

- `Audio/`: OpenAL-based audio implementation.
  - Audio source management (3D positional audio).
  - Audio listener (camera-attached).
  - Audio buffer loading and streaming.
  - Audio source pooling for performance.
  - Distance attenuation and Doppler effects.
  - Audio format support (WAV, OGG, etc.).

- `Window/`: Cross-platform window creation and management.
  - Window creation, resizing, and destruction.
  - Event handling (close, minimize, maximize, focus).
  - Fullscreen and windowed mode switching.
  - Multi-monitor support.
  - Platform-specific window handles for Vulkan surface creation.

- `Input/`: Handles keyboard, mouse, and other input devices.
  - Keyboard input (key press, release, repeat).
  - Mouse input (movement, buttons, scroll).
  - Gamepad/controller support.
  - Input mapping and action bindings.
  - Input state queries (is key pressed, mouse position, etc.).
  - Platform-independent input abstraction.

- `Scenes/`: Manages the scene graph and game objects.
  - Scene node base class with transformation hierarchy.
  - Scene graph traversal and updates.
  - Node lifecycle management (creation, destruction, parenting).
  - Component-based architecture for attaching behaviors to nodes.
  - Visibility culling and spatial queries.

- `Resources/`: Handles loading and management of assets.
  - Resource loading (synchronous and asynchronous).
  - Resource caching and reference counting.
  - Support for multiple asset formats (textures, models, audio, etc.).
  - Resource path resolution and virtual file systems.
  - Streaming for large assets.

- `Physics/`: Physics engine integration.
  - Rigid body dynamics.
  - Collision detection (broad phase and narrow phase).
  - Collision response and contact resolution.
  - Spatial acceleration structures (octree, BVH).
  - Physics queries (raycasting, shape casting, overlap tests).
  - Physics material properties (friction, restitution).
  - Constraint and joint systems.

- `Animations/`: Manages animations and skeletal systems.
  - Skeletal animation (bones, joints, skinning).
  - Animation clips and blending.
  - Animation state machines.
  - Morph target animation.
  - Procedural animation.

- `PlatformSpecific/`: Contains platform-specific implementations.
  - `SystemInfo.hpp/cpp`: System information queries (OS, CPU, RAM, etc.).
  - `UserInfo.hpp/cpp`: User information (username, home directory, etc.).
  - `Helpers.linux.cpp`, `Helpers.windows.cpp`: Platform-specific utility functions.
  - Platform-specific file system operations.
  - Platform-specific threading primitives.

- `Net/`: Networking functionalities.
  - `Manager.hpp/cpp`: Network manager for HTTP/HTTPS requests.
  - `DownloadItem.hpp`: Download queue management.
  - `CachedDownloadItem.hpp`: Cached download with local storage.
  - HTTP client for downloading resources.
  - Network error handling and retry logic.

- `Console/`: In-game console and logging system.
  - `Controller.hpp/cpp`: Console controller and command execution.
  - `Controllable.hpp/cpp`: Interface for objects controllable through console.
  - `Command.hpp`: Console command definition.
  - `Expression.hpp/cpp`: Expression parsing and evaluation.
  - `Argument.hpp/cpp`: Command argument parsing.
  - `Output.hpp`: Console output formatting.
  - Logging with severity levels (DEBUG, INFO, WARNING, ERROR).
  - Command registration and auto-completion.

- `Overlay/`: UI overlay system for in-game UI elements.
  - 2D rendering on top of 3D scene.
  - UI widgets and controls.
  - Text rendering.
  - UI layout and positioning.

- `Libs/`: Internal helper libraries or embedded third-party code.
  - Utility functions and data structures.
  - Math libraries (vectors, matrices, quaternions).
  - String utilities.
  - File I/O helpers.

- `Tool/`: Utility tools for development or debugging.
  - Asset converters and processors.
  - Debug visualization tools.
  - Performance profiling utilities.

- `Testing/`: Unit tests and testing infrastructure.
  - Test fixtures and utilities.
  - Automated tests for engine components.
  - Integration tests.

---

## Testing and Debugging

1.  **Vulkan Validation Layers:** Always enable Vulkan validation layers during development to catch API misuse.
2.  **Console Commands:** Use the in-game console to inspect engine state and execute debug commands.
3.  **Logging System:** Use appropriate log levels (DEBUG, INFO, WARNING, ERROR) for different types of messages.
4.  **Debug Visualization:** Use debug rendering tools to visualize physics colliders, scene graph hierarchy, etc.
5.  **Performance Profiling:** Profile rendering performance regularly to identify bottlenecks.
6.  **Unit Tests:** Run unit tests regularly to catch regressions.
7.  **Memory Debugging:** Use tools like Valgrind (Linux), Dr. Memory (Windows), or Address Sanitizer to detect memory leaks.

---

## Build System Notes

1.  **CMake Configuration:** The engine uses CMake 3.25.1+ with modern CMake practices.
2.  **Parallel Builds:** Compilation automatically uses all available CPU cores.
3.  **Build Types:** Support for Debug, Release, and RelWithDebInfo configurations.
4.  **Third-Party Dependencies:** Managed through CMake's FetchContent or find_package.
5.  **Code Formatting:** Use the `format-code` script (if available) to format all C++ files before committing.
6.  **Vulkan SDK:** Requires Vulkan SDK 1.2+ to be installed on the system.
7.  **Platform-Specific Builds:**
    - Linux: Requires X11 or Wayland development libraries.
    - Windows: Requires MSVC 2019+ or MinGW with C++20 support.
    - macOS: Requires Xcode with MoltenVK for Vulkan support.

---

## Version Control

1.  **Commit Messages:** Write clear, descriptive commit messages in English using imperative mood.
2.  **Branch Strategy:**
    - `main`: Stable releases only.
    - `develop`: Active development branch.
    - Feature branches: For new features or significant changes.
3.  **Code Review:** All changes should be reviewed before merging to `main`.
4.  **Breaking Changes:** Document any API breaking changes clearly in commit messages and changelogs.
5.  **License Compliance:** Ensure all contributions are compatible with LGPLv3 license.

---

## AI Agent Specific Guidelines

1.  **Vulkan Complexity:** Be aware that Vulkan is verbose and complex. Always verify synchronization and resource lifetime management.
2.  **Cross-Platform Testing:** When modifying platform-specific code, consider the impact on all supported platforms.
3.  **Performance Implications:** Engine code runs in performance-critical paths. Be mindful of allocations, copies, and redundant operations.
4.  **Shader Generation:** Saphir is a complex system. Changes to shader generation require careful testing across multiple material types.
5.  **Scene Graph Integrity:** Maintain scene graph invariants (e.g., no cycles, proper parent-child relationships).
6.  **Resource Lifetimes:** Be extremely careful with resource lifetimes, especially Vulkan resources that may be in use by the GPU.
7.  **Thread Safety:** Document thread safety guarantees for all public APIs. Assume nothing is thread-safe unless explicitly stated.
8.  **Backward Compatibility:** As a library, maintain API backward compatibility when possible. Deprecate old APIs before removing them.
9.  **Documentation:** Engine APIs must be well-documented since they will be used by client applications like `projet-alpha`.
10. **Testing:** When adding new features, suggest appropriate tests to validate functionality.
