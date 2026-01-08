# Development History Log

## Beta version 0.8.44 (2026-01-08)
 - Add swap-chain present mode selection.
 - Add software FPS limiter.
 - Add soundfonts (SF2) support for MIDI playback.
 - Fix bug when unloading wave resources.
 - Fix instruction order in CMakeLists.txt.
 - Print compiler flags and definitions in CMake setup output.
 - Fix overlay Surface buffer selection when transition is enabled.
 - Fix unit test code on Linux, macOS and Windows.
 - Update documentation.

## Beta version 0.8.43 (2026-01-08)
 - Move jsonCpp and fastGLTF to pre-compiled binaries.

## Beta version 0.8.42 (2025-12-21)
 - Upgrade physics engine.
 - Add Capsule primitive for collision detection.
 - Add collision detection model interface.
 - Fix AACuboid validity test.
 - Complete modifiers influence with new collision model.
 - Enable "grounded" feature in node physics.
 - Enable sleep/wake for physics simulation.
 - Add adaptive terrain LOD.
 - Enable sea level in scenes.
 - Fix opacity computation in shaders.
 - Add floating-point optimization flags option.
 - Add sRGB swap-chain option.
 - Remove color-space conversion from OverlayManager.
 - Add pre-multiplied alpha and color format (RGBA/BGRA) options to OverlayManager.
 - macOS: Remove redundant NSApplication initialization from dialogs.
 - Update dependencies.
 - Code cleanup.

## Beta version 0.8.41 (2025-12-17)
 - Enable -Werror/-WX flag on GCC, Clang and MSVC to treat warnings as errors.
 - Add C++ exceptions as a CMake option.
 - Add RTTI and exceptions options.
 - macOS: Fix deprecation warnings in dialog file type filters.

## Beta version 0.8.40 (2025-12-08)
 - Remove legacy collision physics system (WIP).
 - Add retro synthesizer for MIDI files and sound effects.
 - Upgrade IO functions.
 - Various minor fixes.
 - Windows: Add terminal wait to read logs after runtime.

## Beta version 0.8.39 (2025-12-08)
 - Unify "showInformation" settings with command-line argument support.
 - Add color-space conversion option per overlay UIScreen.
 - Minor fixes.

## Beta version 0.8.38 (2025-12-03)
 - Update README.md and AI documentation.
 - Improve device detection for PowerSaving mode.
 - Fix Tracer instantiation across multiple processes.
 - Update observable identification.
 - Fix missing default key for Window class.
 - Simplify shader source code generation logs.
 - Add TokenFormatter class for flexible token display.
 - Upgrade SourceCodeParser to control line number display and comment removal.
 - Fix getClassUID() removing unnecessary static local variable.
 - Fix light multi-pass rendering artifact.
 - Complete Program cache in Renderer.
 - Refactor: move program cache from RenderableInstance to Renderable.
 - Make RenderableInstance lightweight (no dynamic allocations).
 - Add ProgramCacheKey for cache lookups.
 - Remove RenderTargetPrograms classes (no longer needed).
 - Upgrade Octree system.
 - Add height shifting to Grid class.
 - Upgrade ThreadPool class.
 - macOS: Fix BlobTrait to use std::string_view.
 - Windows: Force CMake build type variable.
 - Windows: Add attachToParentConsole() to attach to parent console.

## Beta version 0.8.37b (2025-12-03)
 - Add FailSafe GPU selection mode for Nvidia Optimus workaround.
 - Implement automatic detection of Nvidia Optimus configurations (iGPU + dGPU).
 - Add Failsafe mode excluding discrete Nvidia GPU to avoid WSI swapchain deadlock.
 - Add VendorID namespace with known GPU vendor constants.
 - Add HybridGPUConfig struct for hybrid GPU detection results.
 - Distinguish laptop Optimus from desktop hybrid GPU configurations.
 - Add isLikelyMobileGPU() to detect mobile Nvidia GPUs by name patterns.
 - Change default AutoSelectMode from "Performance" to "Failsafe".

## Beta version 0.8.37 (2025-12-02)
 - Remove unintended mutex unlock() on scoped lock.
 - Use std::source_location everywhere and remove workaround include.
 - Fix Settings isArrayEmpty() method.
 - Add Vulkan device extension when necessary.
 - Add user resizing tracking for Windows.
 - Make swap-chain status change atomic.
 - Add atomic operations on Core flags.
 - Add constexpr option to select swap-chain recreation thread.

## Beta version 0.8.36 (2025-12-01)
 - Add option to disable Notifier creation on startup.
 - Fix Settings root variables on file rewrite.

## Beta version 0.8.35 (2025-12-01)
 - Update AI documentation and create Claude agents.
 - Optimize rendering to eliminate std::vector (zero heap allocation).
 - Update push_constant interface in RenderableInstance concept.
 - Enable suspend/wakeup layer.
 - Fix statistics display in terminal with setting option.
 - Add quick unit testing scripts.
 - Add Texture1D, Texture3D and VolumetricImageResource.
 - Refactor resource management.
 - Fix ASIO exception warning and enable ASIO on macOS.
 - Remove PNG longjmp usage.
 - Revert OpenAL to version 1.1 (system) on macOS.
 - Add proper hash for graphics pipeline reusability.
 - Reduce Core methods requiring user-side implementation.
 - Refactor window resize management to avoid graphics pipeline recreation.
 - Implement smooth overlay resizing.
 - Add direct GPU texture mapping for Overlay Surface.
 - Add core settings key sorting.
 - Update dependencies.

## Beta version 0.8.34 (2025-11-24)
 - Improve cross-platform builds.
 - Add ability for entities to query their scene.
 - Remove static variables from Scene layer.
 - Remove static virtual device counter.
 - Suppress spurious macOS extension usage warnings.
 - Fix std::any_cast bug on macOS.
 - Fix resource name extraction from path on Windows.
 - Fix default ImageResource loading.

## Beta version 0.8.33 (2025-11-24)
 - Add FileTimestamps class.
 - Add dynamic resource scanning.
 - Add improved default cubemap in release mode.
 - Add screenshot capture with Shift+F12.
 - Restructure AGENTS.md following Claude Code best practices.
 - Optimize Renderer cache lookups with unordered_map (O(1) average).
 - Continue render-targets implementation (WIP).
 - Fix shader code generation for uniform blocks using structures.
 - Fix missing attributes in geometry primitive generation.
 - Update StaticVector class.
 - Add Entity builder pattern.

## Beta version 0.8.32 (2025-11-19)
 - Update project information.

## Beta version 0.8.31 (2025-11-19)
 - Enable MSAA for swap-chain.
 - Enable frustum culling.
 - Update README.md.

## Beta version 0.8.3 (2025-11-18)
 - Update texture and render-target interfaces.
 - Add AGENTS.md for AI coding agent compatibility.
 - Add RLE support for TARGA file format.
 - Update unit tests.
 - Add robust physics simulation with rotation (WIP).

## Beta version 0.8.2 (2025-10-31)
 - Remove std::array for flags.
 - Update thread throttle management in Core.
 - Add pinch gesture support on macOS.

## Beta version 0.8.1 (2025-10-29)
 - Add windowless mode.
 - Update logging system.
 - Fix light direction to match camera orientation.
 - Fix external command execution on Windows.
 - Add openTextFile() alongside OpenFile() in desktop commands.

## Beta version 0.8.0 (2025-10-22)
 - Stable on all platforms.
 - Improve offscreen rendering (WIP).
 - Fix OpenAL initialization.
 - Split push constants between rendering and shadow casting.
 - Fix GPU freezing issue on Linux.
 - Implement new framebuffer system using Vulkan swap chain only.

## Beta version 0.7.54 (2025-10-19)
 - Improve command pool and buffer usage in renderer.
 - Improve sampler creation by purpose.
 - Update ShadowMap render target.
 - Simplify namespace usage.
 - Add Vulkan Memory Allocator.
 - Update project information.
 - Update external dependencies.

## Beta version 0.7.53 (2025-10-14)
 - Update CommandPool and CommandBuffer usage, removing throwaway command buffers.
 - Fix premature GPU resource cleanup on exit.
 - Standardize ServiceInterface initialization state handling.

## Beta version 0.7.52 (2025-10-13)
 - Rewrite TransferManager for GPU parallel strategy.
 - Rewrite frame rendering code.
 - Rewrite CPU/GPU synchronization.
 - Improve external library selection between Release and Debug.

## Beta version 0.7.51 (2025-09-30)
 - Improve dynamic scene loading.
 - Fix static lighting.
 - Fix render target resizing.
 - Major Settings service improvement.
 - Improve Arguments service.
 - Remove singleton usage.
 - Improve resource manager.
 - Cross-platform fixes.
 - Major CMakeLists.txt update for MSVC.
 - Replace googletest submodule with FetchContent.
 - Unify compilation options.
 - Add observable identification method.
 - Fix MoltenVK binary integration.
 - Add primary services provider build mode.
 - Add windowless mode.
 - Update CMakeLists.txt and scripts.
 - Enable OpenMP for MSVC.
 - Improve synchronization mechanism.
 - Fix TransferManager staging-buffer count bug.
 - Fix MultipleInstance VBO update deadlock.
 - Update VkBuffer.

## Beta version 0.7.5 (2025-09-06)
 - Major multi-threading improvement with extensive thread-safety.
 - Add Scene State Double Buffering to fix visual artifacts.
 - Add fastgltf and magic_enum libraries.
 - Add Libs::StaticVector class (std::vector/std::array hybrid for small structures).
 - Improve FastJSON utility functions reliability.
 - Vulkan: update queue synchronization.
 - Vulkan: replace std::vector with StaticVector at object creation.
 - Vulkan: update object identifier for debugging.
 - Vulkan: fix offscreen-rendering synchronization.
 - Vulkan: fix swap-chain creation with stencil buffer.
 - Vulkan: fix crash on termination.
 - AVConsole: move render target creation to scene.
 - OpenAL: fix initialization on faulty audio system.
 - Attempt to enable TSAN (Vulkan compatibility issues).

## Beta version 0.7.44 (2025-08-07)
 - Set Vulkan 1.1 API as minimum requirement.

## Beta version 0.7.43 (2025-08-07)
 - Clean up code for Clang and MSVC compilation.
 - Add user service layer.
 - Add option to disable default key behavior.
 - Disable notifier surface rendering.
 - Make Tracer standalone (no longer uses service interface).
 - Refactor Tracer initialization to use std::source_location.
 - Update primary services identification.
 - Improve Window fullscreen management.
 - Replace macros with C++ constants.
 - Add per-surface visibility check for input in OverlayManager.
 - Enable Wayland support.
 - Update constexpr usage.
 - Remove GTK3 dependency.
 - Add cancelable state to dialog boxes.
 - Fix various crash scenarios.
 - Improve shader generator flexibility.
 - Improve renderable instance code.
 - Improve semaphores and fences in render service.
 - Add PCH compiler support.
 - Improve thread sleep code.
 - Add dynamic resource loading memory statistics.
 - Make resource manager thread-safe.
 - Update resource manager verbosity and allow running without stores.
 - Remove Core/Window/InputManager singletons.
 - Remove most Renderer singleton usage (two calls remaining).
 - Improve ThreadPool usage.
 - Inline trivial constructors.
 - Remove ObservableTrait from ServiceInterface.
 - Update dependency versions.

## Beta version 0.7.42 (2025-05-22)
 - Add reproc/reproc++ library for external command execution.
 - Add basic throttler to reduce main thread CPU usage.
 - Add sRGB/linear color conversion to OverlayManager rendering.
 - Fix OverlayManager surface resizing (crashes and desync).
 - Fix OverlayManager service termination crashes.
 - Fix event blocking in OverlayManager.
 - Various MSVC and macOS compilation fixes.

## Beta version 0.7.41 (2025-05-11)
 - Enable overlay surface double-buffering for smooth resizing.
 - Fix CMake scripts to prevent full project re-scan.
 - Improve PixelFactory library.
 - Add cross-platform utilities.
 - Fix major Notifier slowdown from thread creation.

## Beta version 0.7.4 (2025-04-13)
 - Improve Libs classes code.
 - Remove all dynamic_cast/dynamic_pointer_cast usage.
 - Update all submodules to release versions.

## Beta version 0.7.36 (2025-03-31)
 - Fix window framebuffer for macOS Retina display.
 - Fix default font resource.
 - Improve logging and debug switches and settings.
 - Sort compilation condition macros.

## Beta version 0.7.35 (2025-03-27)
 - Fix GTK3 tokens with native dialogs.
 - Windows version fixes.
 - Refactor overlay surface (remove pixel buffer abstraction).
 - Fix concurrency crash during window resize and OverlayManager update.

## Beta version 0.7.31 (2025-03-22)
 - Update OverlayManager with pixel buffer surface and resize events.
 - Remove unused Eigen library submodule.

## Beta version 0.7.3 (2025-03-18)
 - Improve engine for use as multimedia framework.
 - Improve CMake scripts for precompiled binary archives.
 - Rename 'Emeraude' namespace to 'EmEn'.
 - Rename 'Libraries' namespace to 'Libs' and include in 'EmEn'.
 - Rename 'MasterControl' to 'AVConsole'.
 - Code cleanup.

## Beta version 0.7.2 (2025-03-11)
 - Switch to LGPLv3 license.
 - Remove embedded precompiled dependencies.
 - Update CMake external library handling.
 - Add local lib directory check.

## Beta version 0.7.12 (2025-02-21)
 - Prepare for Vulkan 1.4 API.
 - Add CursorAtlas for standard and custom cursors.
 - Update window management (centering, monitor selection, gamma).
 - Update OverlayManager resize handling.

## Beta version 0.7.1 (2025-01-16)
 - Enable C++20 features extensively.
 - Enable unit tests.
 - Cross-platform support (Linux, macOS, Windows).
 - Compile external libraries from git sources.
 - Upgrade physics engine.
 - Upgrade settings management.
 - Upgrade filesystem management using std::filesystem.
 - Upgrade input system.
 - Upgrade entity node tree and static entities.
 - Add instancing-based particle system.
 - Fix Euler coordinates with scaling.
 - Make sprite rendering fully functional.
 - Add faulty shader source display.
 - Prepare for skeletal animations.


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
