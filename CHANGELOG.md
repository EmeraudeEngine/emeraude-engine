# Development History Log

## Beta version 0.8.37b (2025-12-03)
 - Add FailSafe GPU selection mode for Nvidia Optimus workaround.
 - Implement automatic detection of Nvidia Optimus configurations (iGPU + dGPU Nvidia).
 - Add Failsafe mode that excludes the discrete Nvidia GPU to avoid known WSI swapchain recreation deadlock issues.
 - Add VendorID namespace with known GPU vendor constants.
 - Add HybridGPUConfig struct for hybrid GPU detection results.
 - Distinguish laptop Optimus from desktop hybrid GPU configurations.
 - Add isLikelyMobileGPU() to detect mobile Nvidia GPUs by name patterns.
 - Change default AutoSelectMode from "Performance" to "Failsafe".

## Beta version 0.8.37 (2025-12-02)
 - Remove undesired mutex unlock() on a scoped lock.
 - Use std::source_location everywhere and remove the trick include.
 - Fix Settings isArrayEmpty() method.
 - Add device extension when necessary for Vulkan.
 - Add user resizing tracking for Windows.
 - Make swap-chain status change an atomic operation.
 - Add atomic operation on Core flags.
 - Add constexpr option to select which thread can recreate the swap-chain.

## Beta version 0.8.36 (2025-12-01)
 - Add option to disable Notifier creation on startup.
 - Fix Settings root variables on file rewrite.

## Beta version 0.8.35 (2025-12-01)
 - Update AI documentation and create Claude agents.
 - Optimize render to eliminate std::vector (Zero Heap Allocation).
 - Update push_constant interface in RenderableInstance concept.
 - Enable suspend/wakeup layer.
 - Fix statistics display in terminal with a setting option.
 - Update dependencies.
 - Add quick unit testing scripts.
 - Add Texture1D, Texture3D and VolumetricImageResource.
 - Resource management refactorization.
 - Fix ASIO exception warning and enable ASIO on macOS.
 - Remove PNG long jump usage.
 - Revert OpenAL to version 1.1 (system) on macOS.
 - Add true hash for graphics pipeline to make it reusable.
 - Reduce Core methods requiring implementation on user side.
 - Refactor window resize management to avoid graphics pipelines recreation.
 - Implement smooth overlay resizing.
 - Add ability for Overlay Surface to map GPU texture for direct access.
 - Add new core settings key sorting.

## Beta version 0.8.34 (2025-11-24)
 - Cross-platform builds improvement.
 - Add ability for an entity to query its scene.
 - Remove static variables from Scene layer.
 - Remove static virtual device counter.
 - Sanitize macOS useless warning messages on extension usage.
 - Fix dark bug with std::any_cast on macOS.
 - Fix resource name extraction from path on Windows.
 - Fix default ImageResource loading.

## Beta version 0.8.33 (2025-11-24)
 - Add FileTimestamps class.
 - Add dynamic scan for resources.
 - Add fancier default cubemap in release mode.
 - Add screenshot capture ability with Shift+F12.
 - Restructure AGENTS.md following Claude Code best practices.
 - Optimize Renderer cache lookups with unordered_map (O(1) average).
 - Complete render-targets implementation (WIP).
 - Fix shader code generation bug for uniform blocks using structures.
 - Fix missing attributes with geometry primitive generation.
 - Update StaticVector class.
 - Add new Entity building system using builder pattern.

## Beta version 0.8.32 (2025-11-19)
 - Update project information.

## Beta version 0.8.31 (2025-11-19)
 - Enable MSAA for the swap-chain.
 - Enable frustum culling.
 - Update README.md.

## Beta version 0.8.3 (2025-11-18)
 - Update texture and render-target interfaces.
 - Add AGENTS.md for AI coding agent compatibility.
 - Add RLE to TARGA file format.
 - Update unit tests.
 - Add new robust physics simulation with rotation (WIP).

## Beta version 0.8.2 (2025-10-31)
 - Remove std::array for flags.
 - Update thread throttle management in Core.
 - Add pinch gesture support on macOS.

## Beta version 0.8.1 (2025-10-29)
 - Add windowless mode.
 - Update logging.
 - Fix light direction to correspond to camera.
 - Fix external commands execution on Windows.
 - Add openTextFile() method next to OpenFile() in desktop commands.

## Beta version 0.8.0 (2025-10-22)
 - Stable on all platforms.
 - Improve offscreen rendering (WIP).
 - Minor fix for OpenAL initialization.
 - Split push constants between rendering and shadow casting.
 - Fix GPU freezing issue on Linux.
 - New framebuffer system relying solely on the Vulkan swap chain.

## Beta version 0.7.54 (2025-10-19)
 - Update project information.
 - Improve command pools and buffers usage in the renderer.
 - Improve sampler creation by purpose.
 - Update ShadowMap render target.
 - Simplify namespace usage.
 - Add Vulkan Memory Allocator.
 - Update direct external dependencies.

## Beta version 0.7.53 (2025-10-14)
 - Update CommandPool and CommandBuffer usage, removing all throwaway command buffers.
 - Fix premature resource cleanup on GPU when exiting with the new CPU/GPU synchronization code.
 - Uniformize ServiceInterface initialization state handling.

## Beta version 0.7.52 (2025-10-13)
 - Rewrite TransferManager for a new GPU parallel strategy.
 - Rewrite frame rendering code.
 - Rewrite general synchronization between CPU/GPU.
 - Improve external libraries selection between Release and Debug.

## Beta version 0.7.51 (2025-09-30)
 - Improve scene dynamic loading.
 - Fix static lighting.
 - Fix resize render targets.
 - Major improvement for the Settings service.
 - Improvement for the Arguments service.
 - Remove singleton usage.
 - Improve resource manager.
 - Cross-platform fixes.
 - Major CMakeLists.txt update for MSVC.
 - Remove submodule 'dependencies/googletest' for FetchContent logic.
 - Unify compilation options.
 - New method to identify observables.
 - Minor fix for MoltenVK integration to the binary.
 - Add ability to build the project as a primary services provider.
 - Add windowless mode.
 - Update CMakeLists.txt and scripts.
 - Enable OpenMP for MSVC.
 - Improve synchronization mechanism.
 - Fix TransferManager staging-buffer count bug.
 - Fix MultipleInstance VBO update deadlock.
 - Update VkBuffer.

## Beta version 0.7.5 (2025-09-06)
 - Major multi-threading improvement adding extensive thread-safety behavior.
 - Add 'Scene State Double Buffering' to fix visual artifacts.
 - Add 'fastgltf' and 'magic_enum' libraries.
 - Add 'Libs::StaticVector' class, a mix between std::vector and std::array to remove dynamic allocation for small structures.
 - Update 'FastJSON' utility functions for more reliability.
 - Vulkan: update queue synchronization.
 - Vulkan: remove all std::vector for StaticVector at object creation.
 - Vulkan: update object identifier for debugging.
 - Vulkan: fix offscreen-rendering synchronization.
 - Vulkan: fix swap-chain creation with stencil buffer.
 - Vulkan: fix crash on termination.
 - AVConsole: move the ability to create render target to the scene.
 - OpenAL: fix initialization on a faulty audio system.
 - Attempt to enable TSAN (bug with Vulkan).

## Beta version 0.7.44 (2025-08-07)
 - Move to Vulkan 1.1 API as the minimum requirement.

## Beta version 0.7.43 (2025-08-07)
 - Code cleanup for compilation with Clang and MSVC.
 - Add user service layer.
 - Add option to disable default key behavior.
 - Disable notifier surface from rendering.
 - Make Tracer instantiable. The Tracer no longer uses a service interface and is standalone.
 - Refactor Tracer initialization with logger to use std::source_location.
 - Update primary services identification.
 - Improve Window fullscreen management.
 - Add C++ constants instead of macros.
 - Add visibility check per surface for input in OverlayManager.
 - Enable Wayland support.
 - Update some constexpr usage.
 - Remove GTK3.
 - Add dialog box cancelable state.
 - Fix crash in some situations.
 - Improve shader generators flexibility.
 - Improve renderable instance code.
 - Improve semaphores and fences in render service.
 - Make compatible with PCH compiler feature.
 - Improve thread sleep code.
 - Add dynamic resource loading memory usage statistics.
 - Make resource manager thread-safe.
 - Update resource manager verbosity and allow running without stores.
 - Remove Core/Window/InputManager singleton.
 - Update dependency versions.
 - Remove almost all Renderer singleton usage (two calls left).
 - Improve ThreadPool usage.
 - Inline some trivial constructors.
 - Remove ObservableTrait from ServiceInterface.

## Beta version 0.7.42 (2025-05-22)
 - Add reproc/reproc++ library to execute external commands.
 - Add basic throttler to reduce CPU usage on the main thread.
 - Add color conversion to OverlayManager when rendering (sRGB/linear color).
 - Fix OverlayManager for resizing surfaces (crashes and desync).
 - Fix crashes with OverlayManager service on termination.
 - Fix events blocking in OverlayManager.
 - Various fixes for MSVC and macOS compilation.

## Beta version 0.7.41 (2025-05-11)
 - Enable overlay surface double-buffering for smooth resize operation.
 - Fix CMake scripts to avoid full re-scan project bug.
 - Improve PixelFactory library.
 - Add cross-platform utilities.
 - Fix major slow-down in Notifier with thread creation.

## Beta version 0.7.4 (2025-04-13)
 - Improve Libs classes code.
 - Remove all dynamic_cast/dynamic_pointer_cast.
 - Update all submodules to release versions.

## Beta version 0.7.36 (2025-03-31)
 - Fix window framebuffer for macOS and Retina display.
 - Fix default font resource.
 - Improve switches and settings for logging and debug.
 - Sort compilation condition macros.

## Beta version 0.7.35 (2025-03-27)
 - Fix GTK3 tokens with native dialogs.
 - Windows version fixes.
 - Refactor overlay surface (no more abstraction with pixel buffer).
 - Fix concurrency crash when resizing the window and OverlayManager update.

## Beta version 0.7.31 (2025-03-22)
 - Update OverlayManager with pixel buffer surface and resizing events.
 - Remove unused Eigen library submodule.

## Beta version 0.7.3 (2025-03-18)
 - Improvements made to use the engine as a multimedia framework.
 - Improve CMake scripts for external libraries to use precompiled binary archives.
 - Rename 'Emeraude' namespace to 'EmEn' to reduce space.
 - Rename 'Libraries' namespace to 'Libs' and include it in 'EmEn' to avoid conflicts.
 - Rename 'MasterControl' to 'AVConsole' for clarification.
 - Code cleanup.

## Beta version 0.7.2 (2025-03-11)
 - Switch to LGPLv3 license.
 - Remove precompiled dependencies embedded in the repository.
 - Update external libraries CMake usage.
 - Add check for local lib directory presence.

## Beta version 0.7.12 (2025-02-21)
 - Fix for the upcoming Vulkan 1.4 API.
 - Add CursorAtlas for standard and custom cursors.
 - Update window management (centering, monitor selection, gamma).
 - Update resize handling for OverlayManager.

## Beta version 0.7.1 (2025-01-16)
 - C++20 features are enabled and used extensively (no turning back available).
 - Unit tests are enabled.
 - Cross-compilable code (Linux, macOS and Windows).
 - External libraries compiled from git sources.
 - Physics engine upgraded.
 - Settings management upgraded.
 - FileSystem management upgraded using std::filesystem.
 - Input system management upgraded.
 - Entity node tree and static entities upgraded.
 - New particle system based on instancing (possible improvements towards Compute shader).
 - Euler coordinates system upgraded and fixed with scaling.
 - Sprite rendering upgraded and fully functional.
 - New system to display faulty shader source.
 - Code prepared to receive skeletal animations.


# Previous Major Iterations

Here is the history of the engine's major refactorizations until finding the right structure.
In short, how an immature video game idea with OpenGL became a game engine with Vulkan, with separate user-application code for testing the engine.

## Alpha version 0.7 (C++20, ~2023)

This is the version that is mostly equivalent to the OpenGL version.
New features, new techniques, and adoption of C++20.

## Alpha version 0.6 [NON-EXISTENT] (C++17, ~2021)

This is a transitional version started in 2020 to rewrite the graphics engine for using Vulkan instead of OpenGL and the first version to draw the minimum on the screen.
This change was a significant decision and a huge step backward in development with many feature losses.
It was a deliberate choice to drop OpenGL support instead of adding a new rendering backend to take full advantage of Vulkan's benefits with multithreading.
From this version, the idea was to recover everything that had been done with the OpenGL version.

## Alpha version 0.5 [ARCHIVED] (C++14, ~2019)

This is a minor rewrite of the engine itself.
The idea behind this change was to make better use of large project management tools like git, CMake, and external libraries.
This is the latest archived version and the very last version of the engine that uses OpenGL.

## Alpha version 0.4 [ARCHIVED] (C++11, ~2015)

This is the third complete rewrite of the engine in C++. It is mainly a new empty structure from scratch, retaining what was good from the previous version.
By learning advanced techniques in C++ and OOP, it led to a new way of thinking about the engine structure.
This version was the last where the entire code structure would be reworked.

## Alpha version 0.3 [ARCHIVED] (C++11, ~2013)

This is the second attempt to rewrite the engine in C++ from scratch.
Learning C++ is challenging and leads to bad decisions... Alpha 2 was a big failure!

## Alpha version 0.2 [ARCHIVED] (C++03/C++0x, ~2012)

This is the first complete rewrite of the engine but using C++.
C++ provides a better way to express abstract concepts for an engine and was the first attempt at engine standardization.

## Alpha version 0.1 [ARCHIVED] (C, ~2010)

This is the first attempt to create an engine with OpenGL using C.
The idea was mainly to automate OpenGL rendering with a scene containing a sky, a ground, and entities.
