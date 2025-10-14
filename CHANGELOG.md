# Development history log

## Beta version 0.7.53
 - Update the CommandPool with CommandBuffer usage, removing all the throwaway command buffers.
 - Fix a premature resource cleanup on GPU when exiting with the new CPU/GPU synchronization code.
 - Uniformize ServiceInterface initialization state handling.

## Beta version 0.7.52
 - Rewrite of TransferManager for a new GPU parallel strategy.
 - Rewrite frame rendering code.
 - Rewrite the general synchronization between CPU/GPU.
 - Improve external libraries selection between Release and Debug.

## Beta version 0.7.51
 - Improve scene dynamic loading.
 - Static lighting fixed.
 - Fix resize render targets.
 - Major improvement for the Settings service.
 - Improvement for the Arguments service.
 - Remove singletons usage.
 - Resource manager Improvement.
 - Cross-platform fixes.
 - Major CMakeList.txt update for MSVC.
 - Remove sub-module 'dependencies/googletest' for FetchContent logic.
 - Unify compilation options.
 - New method to identify observables.
 - Minor fix for MoltenVK integration to the binary.
 - Add the ability to build the project as a primary services provider.
 - Add the windowless mode.
 - Update CMakeLists.txt and scripts.
 - Enable OpenMP for MSVC.
 - Improve synchronization mechanism
 - TransferManager staging-buffer count bug.
 - MultipleInstance VBO update deadlock.
 - Update VkBuffer.

## Beta version 0.7.5
 - Major multi-threading improvement adding a lot of thread-safety behavior.
 - Add 'Scene State Double Buffering' to fix visual artifacts.
 - Add 'fastgltf' and 'magic_enum' libraries.
 - Add the 'Storage' class, a mix between std::vector and std::array to remove dynamic allocation for small structures.
 - Update 'FastJSON' utility functions for more reliability.
 - Vulkan, update queue synchronization.
 - Vulkan, remove all std::vector for StaticVector at object creation.
 - Vulkan, update object identifier for debugging.
 - Vulkan, fix offscreen-rendering synchronization.
 - Vulkan, fix the swap-chain creation with stencil buffer.
 - Vulkan, fix crash on termination.
 - AVConsole, Move the ability to create render target to the scene.
 - OpenAL, fix init on a faulty audio system.
 - Attempt to enable TSAN (bug with vulkan).

## Beta version 0.7.44
 - Move to Vulkan 1.1 API as the minimum requirement.

## Beta version 0.7.43
 - Code cleanup for compilation with Clang and MSVC.
 - Add a user service layer.
 - Add an option to disable default key behavior.
 - Disable notifier surface from rendering.
 - Instantiable Tracer. Now the Tracer isn't using a service interface anymore and is standalone.
 - Refactor Tracer initialization with logger to use std::location.
 - Update primary services identification.
 - Improve Window fullscreen management.
 - Add C++ constants instead of macros.
 - Add the visibility check per surface for input in OverlayManager.
 - Enable Wayland support.
 - Update some constexpr usage.
 - GTK3 removed.
 - Add a dialog box cancelable state.
 - Crash fixed on some situation.
 - Shader generators flexibility improved.
 - Renderable instance code improved.
 - Semaphores and fences improved in render service.
 - Compatible with the PCH compiler feature.
 - Improve thread sleep code.
 - Dynamic resource loading memory usage statistics.
 - Resource manager is now thread-safe.
 - Update resources manager verbosity and to be able to run without stores.
 - Remove Core/Window/InputManager singleton.
 - Update dependency version.
 - Remove almost Renderer singleton (two calls left).
 - Improve ThreadPool usage.
 - Inline some trivial constructors.
 - Remove ObservableTrait from ServiceInterface.

## Beta version 0.7.42
 - Add the reproc / reproc++ library to execute external commands.
 - Add a basic throttler to reduce CPU usage on the main thread.
 - Add a color conversion to OverlayManager when rendering (sRGB/linear color).
 - Fix OverlayManager for resizing surfaces (Crashes and desync).
 - Fix crashes with OverlayManager service on termination.
 - Fix events blocking in OverlayManager.
 - Various fixes for MSVC and macOS compilation.

## Beta version 0.7.41
 - Enable overlay surface double-buffering for a smooth resize operation.
 - Fix CMake scripts to avoid the full re-scan project bug.
 - Improve PixelFactory library.
 - Add cross-platform utilities.
 - Fix a major slow-down in Notifier with thread creation.

## Beta version 0.7.4
 - Improve Libs classes code.
 - Remove all dynamic_cast/dynamic_pointer_cast.
 - Update all submodules to a release version.

## Beta version 0.7.36
 - Fix window framebuffer for macOS and Retina display.
 - Fix default font resource.
 - Improve switches and settings for logging and debug.
 - Sort compilation condition macros.

## Beta version 0.7.35
 - Fix GTK 3 tokens with native dialogs.
 - Windows version fixes.
 - Overlay surface refactorization (no more abstraction with pixel buffer).
 - Fix concurrency crash when resizing the window and OverlayManager update.

## Beta version 0.7.31
 - Update OverlayManager with pixel buffer surface and resizing events.
 - Remove the unused Eigen library submodule.

## Beta version 0.7.3
 - Improvements made to use the engine like a multimedia framework.
 - Improve CMake scripts for external libraries to use the precompiled binary archive.
 - Rename 'Emeraude' namespace to 'EmEn' to reduce space.
 - Rename 'Libraries' namespace to 'Libs' and include it in 'EmEn' to avoid conflicts.
 - Rename 'MasterControl' to 'AVConsole' for clarification.
 - Code cleanup.

## Beta version 0.7.2
 - Switch to license LGPLv3.
 - Remove precompiled dependencies embedded in the repository.
 - Update external libraries CMake usage.
 - Add check for local lib directory presence.

## Beta version 0.7.12
 - Fix for the upcoming Vulkan 1.4 API.
 - Add CursorAtlas for standard and custom cursors.
 - Update Windows management (Centering, monitor selection, gamma)
 - Update resize handling for OverlayManager.

## Beta version 0.7.1
 - C++20 features are enabled and used extensively (no turning back available).
 - Unit tests are enabled.
 - Cross-Compilable code (Linux, macOS and Windows).
 - External libraries compiled from git sources.
 - Physics engine upgraded.
 - Settings management upgraded.
 - FileSystem management upgraded using std::filesystem.
 - Input system management upgraded.
 - Entities node tree and static entities upgraded.
 - New particle system based on instancing (Possible improvements towards Compute shader).
 - Euler coordinates system upgraded and fixed with the scaling.
 - Sprite rendition is upgraded and fully functional.
 - New system to display a faulty shader source.
 - Preparing code to receive skeletal animations.


# Previous major iterations

Here is the history of the engine's major refactorizations until finding the right structure. 
In short, how an immature video game idea with OpenGL became a game engine with Vulkan, with separate user-application code for testing the engine.

## Alpha version 0.7 (C++20, ~2023)

This is the version that is mostly equal to the OpenGL version.
New features, new technics and adoption of C++20.

## Alpha version 0.6 [NON-EXISTENT] (C++17, ~2021)

This is a transitional version started in 2020 to rewrite the graphics engine for using Vulkan instead of OpenGL and the first version to draw the minimal on the screen.
This change was a significant decision and a huge step backward in development with many feature losses.
It was deliberate to drop OpenGL support instead of adding a new rendering backend to take full advantage of Vulkan's benefits with multithreading.
From this version, the idea was to recover everything that had been done with the OpenGL version.

## Alpha version 0.5 [ARCHIVED] (C++14, ~2019)

This is a minor rewrite of the engine itself.
The idea behind this change was to better use large project management tools like git, CMake, external libraries.
This is the latest archived version and the very last version of the engine that uses OpenGL.

## Alpha version 0.4 [ARCHIVED] (C++11, ~2015)

This is the third complete rewrite of the engine in C++, it's mainly a new empty structure from scratch and then retaining what was good from the previous version.
By learning advanced techniques in C++ and OOP, it leads to a new way of thinking about the engine structure.
This version will be the last where the entire code structure will be reworked.

## Alpha version 0.3 [ARCHIVED] (C++11, ~2013)

This is the second attempt to rewrite the engine in C++ from zero.
Learning C++ is challenging and leads to bad decisions... Alpha 2 was a big failure!

## Alpha version 0.2 [ARCHIVED] (C++03/C++0x, ~2012)

This is the first complete rewrite of the engine but using C++.
C++ provides a better way to express abstract concepts for an engine and is the first attempt at engine standardization.

## Alpha version 0.1 [ARCHIVED] (C, ~2010)

This is the first try to create an engine with OpenGL using C.
The idea was mainly to automatize OpenGL rendering with a scene with a sky, a ground and entities.
