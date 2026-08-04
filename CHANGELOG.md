# Development History Log

## Beta version 0.9.51 (in development)
 - **Revert `EMERAUDE_USE_EXPLICIT_EXPORTS` to Off by default** — `On` makes the consuming application's link much longer, on every link (explicit `dllimport`/`dllexport` gives the consumer's linker a far larger import-resolution surface than the compact export-all `.def`), on top of the standing duty to annotate every new consumer-referenced public symbol. Also move the MSVC "export-all excludes the PCH" guard out of emeraude-base's shared helper into the engine's own PCH call site. The engine target is the only one in the cascade using `WINDOWS_EXPORT_ALL_SYMBOLS`, so it is now the only one that loses its precompiled header — every other target keeps it. The explicit-export migration itself stays in the tree and can be turned back on with one line.
 - Fix Vulkan validation errors.
 - macOS: minor fixes.
 - Docs: the macOS/MoltenVK section (bindless samplers require MoltenVK 1.4+), and what not to re-investigate.
 - Rewrite the README as a layered, non-redundant set across the family: emeraude-base owns the toolchain, the compile policy and all external-dependency detail; the engine owns the Vulkan runtime, the SDK, its submodules and options; the applications own their own demonstration.

## Beta version 0.9.5 (2026-07-30)
 - Release of the photometric lighting + physical camera cycle (see 0.9.42).
 - Cross-platform fixes from macOS and MSVC.

## Beta version 0.9.42 (2026-07-22)
 - **Photometric lighting, phase 1.** The unit vocabulary for lights: a directional light takes an illuminance in **lux**, a point/spot light a luminous power in **lumens**. Physical inverse-square attenuation for point and spot lights, light generators take photometric units, and the legacy compensation is dropped.
 - **Photometric lighting, phase 2.** The ambient term becomes a photometric illuminance; middle grey is 0.18, not 0.5; the auto-exposure gets a photometric clamp range; materials carry an emissive strength and unlit materials apply their emission.
 - **A sky is a light source.** A sky declares its luminance, and that luminance lights the scene. Photometric background contract, **sky-driven lighting**, and removal of the pre-photometry `StaticLighting` shortcut — `SimplePass` is now strictly unlit. Thread-safe `applyBackgroundLighting()`, poll-based background lighting, safe entity removal and sky re-derivation; the default ambient illuminance is **measured** on the sky texels.
 - **Image-based lighting (4 lots).** GPU foundations (BRDF LUT, bakeable IBL textures, cubemap mips, Y convention), per-environment bake (GGX prefilter + irradiance, scene trigger, publication), shading (diffuse irradiance in the ambient pass, split-sum specular), and an SSR environment fallback on the bindless prefiltered slot. HDR environment cubemaps (Radiance RGBE, RGBA16F, D6 calibration).
 - **Physical camera system.** Absolute APEX exposure metered on ISO from the camera triad; the focal length **is** the field of view (derived, not stored) and reframes; the sensor format is a constant-lens change; a style declares a format; camera presets keep the sensor (tone mapping is not a style); the photographic authority is a weak reference that heals to null when a camera dies. A physical camera panel on **Shift+F2**.
 - **Lens effects are the camera's.** Bloom is materialized from the camera with a threshold in nits; depth of field uses the aperture diameter, not the f-number; motion blur joins the camera (McGuire reconstruction driven by the shutter speed). Scoped lens-effect locks, and the camera gets its chain whether or not the scene built one.
 - **Motion vectors (4 steps) and TAA.** Per-instance transforms SSBO replaces the scene-pass matrix push constants, real previous model matrices, an RG16F velocity MRT attachment, and RTGI temporal reprojection by motion vectors; double skinning gives limb-level velocity for skinned meshes; the sky needs the previous INFINITY view-projection. Temporal anti-aliasing with per-draw sub-pixel jitter and filtered source reconstruction.
 - Ray tracing: RTGI temporal accumulation & multi-bounce, RTR shadow rays at reflection hits ("reflections match the raster"), RTAO double-intensity fix with softened defaults, SSGI parameters exposed as settings keys.
 - AssetLoaders: **WADLoader** — a classic Doom-engine map materializer.
 - Lighting: the legacy specular becomes what its name already claimed, Blinn-Phong.
 - Window: the title bar follows the OS appearance, on the only platform that allows it.

## Beta version 0.9.41 (2026-07-22)
 - Core: opt-in settings reset when the settings file predates the current build.

## Beta version 0.9.40 (2026-07-16)
 - macOS: implement `SystemInfo::getFreeMemory()` via `host_statistics64`.
 - Linux: use `MemAvailable` for the free-memory probe.
 - Fix the per-axis scale in `Window::getFramebufferSize()`.
 - Overlay: scale-change notification + async-provider properties latch.
 - Update the clang-tidy rules.

## Beta version 0.9.39 (2026-07-13)
 - Add `Core::scheduleMainLoopCycle()`, a thread-safe main-loop cycle scheduling contract.
 - Linux: portable `INSTALL_RPATH` (`$ORIGIN`) on the shared library.
 - Update the GLFW source.
 - MSVC fixes; remove an excessive log.

## Beta version 0.9.38 (2026-07-08)
 - **Complete the explicit-export migration and enable the STL precompiled header on MSVC.** The public surface consumed by an application carries `EMERAUDE_API`; `EMERAUDE_USE_EXPLICIT_EXPORTS` becomes the default, which resolves the old `WINDOWS_EXPORT_ALL_SYMBOLS` + PCH incompatibility.
 - Add the missing STL includes so the engine still builds with the PCH disabled.
 - CI: add a PCH-OFF Linux lane to catch missing includes, trimmed to the compile-only dependency set, and clone emeraude-base from a configurable branch.
 - Docs: note the macOS Objective-C++ PCH auto-skip.

## Beta version 0.9.37 (2026-06-30)
 - Add an **on-demand rendering mode** to Core (and fix it not redrawing on window resize).
 - Vulkan: a central `DeferredDestructor` for the runtime destruction of GPU-visible objects.
 - Fix BLAS builds racing buffer uploads across transfer queues.
 - Fix the per-light binding: include the dynamic offset in the redundancy check.
 - Two-sided lighting: orient the shading normal with `dot(N,V)`, not `gl_FrontFacing`.
 - Light culling: test the light against the instance bounding sphere.
 - Settings: per-effect ray-tracing sub-groups exposing all RTGI/RTAO parameters, plus a PixelDoubling option for RTR and RTAO; RTGI default sample count 16 → 8 (measured).
 - Enable the overlay surface to use an external API.
 - Windows: redefine the file dialog box ownership (TECH-1927), add DNS hostname retrieval, and fix desktop commands failing on paths containing spaces.
 - Deliver the button release to press-observer surfaces under pointer capture; update the Linux dialog box and the settings key descriptions.

## Beta version 0.9.36 (2026-06-23)
 - Enable the configuration of the main loop update frequency.
 - Change the `Core/Video/VulkanDevice/EnableFailSafe` setting default to `false`.
 - Fix the pointer coordinate space on Wayland and on HiDPI monitor switches.
 - Windows: fix the file dialog box centering (TECH-1838).
 - Hotfix: mouse move tapping between surfaces.

## Beta version 0.9.35 (2026-06-18)
 - **Multi-scene robustness.** Fix the crash on unloading a scene and three render-resource lifetime bugs exposed by scene unload/switch; make the acceleration structure builder a single Renderer-owned instance; make scene switching hitch-free; fix the Overlay shared-sampler destruction; fix the device loss when deleting a scene that uses post-processing; stop the device-lost spam from the acceleration structure builder.
 - **Vulkan debug object naming** (`vkSetDebugUtilsObjectNameEXT`) on every remaining Vulkan object, for validation and GPU captures; restore leak detection for `DescriptorSetLayout`.
 - Include the pipeline layout in the graphics pipeline cache key.
 - MDI: fix the shader generation plus draw-time descriptor and push-constant bugs.
 - Fix `IntermediateRenderTarget` by-region dependency causing stale-frame block corruption.
 - Honor the double-sided material flag in the glTF and FBX loaders, add `LoaderOptions::forceDoubleSided`, and flip the shading normal on back-facing fragments.
 - Wire the `GISampleCount` setting to the RTGI trace (it was a dead key); replace the RTGI `sin()` noise hash with a PCG integer hash.
 - Diamond-square ground/terrain: snap a non-power-of-two division instead of failing.
 - Console: add the `triggerRenderDocCapture` command; fix Settings serialization emitting invalid JSON for empty stores.

## Beta version 0.9.34 (2026-06-17)
 - Update the identification usage.

## Beta version 0.9.33 (2026-06-11)
 - Fix the ImGui integration.
 - Add a manual request to resize a Surface from an external source (such as CEF), a safe-guard for invalid overlay surfaces, and a new overlay surface transfer strategy.
 - Windows: fix the dialog box latency (TECH-1838).
 - Fixes for the PCH option; remove useless default parameters on Surface; overlay and CMake cleanup.

## Beta version 0.9.32 (2026-06-02)
 - **Enable precompiled headers** (the shared STL hot-set from emeraude-base).
 - Uniformize `AARectangle` / `AACuboid`.
 - CMakeLists cleanup; bug fixes.

## Beta version 0.9.31 (2026-05-27)
 - **Split the engine's foundation layer into the standalone [emeraude-base](https://github.com/EmeraudeEngine/emeraude-base) library** — `src/Libs/` (`EmEn::Libs`) becomes `EmEn::Base`, with its own repository, versioning and roadmap. The engine clones it (`cmake/InstallEmeraudeBase.cmake`) and consumes a pinned package; emeraude-base becomes the single source of truth for external dependencies and the compile policy. (Briefly numbered 0.9.4 before the version settled back on the 0.9.3x line.)
 - "Ave robustus A.0": re-export `Severity` from emeraude-base in `CoreTypes`, and route the emeraude-base diagnostics through the Tracer.
 - Improve mouse event tracking on a single UI surface.
 - Remove `LOCAL_LIB_DIR`; CMakeLists cleanup.

## Beta version 0.9.3 (2026-05-19)
 - **Improve compilation time**: includes cleanup, and drop the usage of `__PRETTY_FUNCTION__`.
 - **Large clang-tidy campaign**: tier 1 cleanup + math-parens auto-fix, redundant member-initializers removed, positional → designated initializers, tightened underlying types on bounded-value enums (bitmask `FlagBits` stay `uint32_t` via NOLINT), `OBJVertex` encapsulated, 24 const getters marked `[[nodiscard]]`, and west-const local variables.
 - Improve the user application close policy.
 - Fix the MSVC build: include `<Windows.h>` before other Windows headers.

## Beta version 0.9.26 (2026-05-18)
 - Ultimate fix for `TCPClient`; minor Linux fix.

## Beta version 0.9.25 (2026-05-13)
 - **Automatic download of the external dependencies from GitHub** at configure time, with the build type taken into account when selecting them.
 - Unify the G-buffer.
 - Ray-traced reflections: fix multi-layer objects, sprite display and alpha.
 - Docs: caution-points entries for multi-geometry BLAS and the sprite RT pipeline.

## Beta version 0.9.24 (2026-05-12)
 - Clean up the external libraries to be compiled.

## Beta version 0.9.23 (2026-04-23)
 - Update the FBX loader with animations, fix its UVs, and add a renderable customization option.
 - Network: add `TCPClient` and `TCPServer` — ASIO is now mandatory.
 - OverlayManager: `UIScreen` can now sort surfaces naturally.
 - Update the keyboard/pointer input debug; fix the config file in release mode; update the hash tools and the clang-tidy rules.

## Beta version 0.9.22 (2026-04-21)
 - Support RMID-wrapped MIDI files (Microsoft RIFF MIDI container); WaveFactory MIDI reader cleanup and documented quirks.
 - Console: mandatory help, recursive help dump, MDI telemetry.
 - Fix the Observer/Observable traits for a data race, and a crash in the overlay screen destructor.

## Beta version 0.9.21 (2026-04-17)
 - Add `PlaylistResource` and enhance the TrackMixer remote control; expose playlist management for a runtime UI (`currentPlaylist` + `PlaylistSwapped`).
 - Fix lossy audio loading: VBR frame-count mismatch and int16 peak wraparound.
 - Add a Win32 API fallback for user dialogs.

## Beta version 0.9.2 (2026-04-17)
 - Minor fixes.

## Beta version 0.9.11 (2026-04-08)
 - **Scene Editor system**: CPU picking, standalone gizmo rendering and remote input injection. Interactive **translate** gizmo (drag, hover highlight, local/world space), **rotate** gizmo (3 RGB torus rings, interactive drag), and **scale** gizmo (per-axis and uniform, `setScalingFactor` on `LocatableInterface`); all gizmos pre-created at editor activation, with `Shift+T` to switch back to translation.
 - Geometry: add loading from raw buffers, with interleaved or separate attributes.
 - Promote `SSDPClient` to a general `UDPClient`.
 - Fix crashes caused by incorrect input manager usage.

## Beta version 0.9.1 (2026-03-30)
 - Network: add `SSDPClient`, serial port listening and Wi-Fi scanning (Windows switched from `netsh` to the native API).
 - System: improve the CPU info, add storage info (`DiskInfo` renamed `StorageInfo`), and convert `UserInfo`/`SystemInfo` into services.
 - Window: add monitor info. Core: add a way to specify the user exit code.
 - Fix the TLAS instance transform to include the renderable instance scale.
 - Add a setting to silence HWLOC; add colors in the Windows terminal.

## Beta version 0.9.0 (2026-03-30)
 - Milestone closing the 0.8.6x cycle: cross-platform compilation fixes.

## Beta version 0.8.64 (2026-03-25)
 - **Skeletal animation with GPU skinning**: animation data structures and loader integration, runtime and resource management, bone vertex attributes enabled in the VBO, VBO format bone handling, a `PerModel` descriptor set for the bone matrix SSBO, and generated vertex-shader skinning code.
 - GLTFLoader: new node-mode API, normal generation, hierarchy flattening and crash fixes.
 - MD5 loader rewrite: shared vertices, correct TBN, descriptor pool fix.
 - LOD settings, ThreadPool integration, read-only skinning SSBO fix.
 - Fix the `CartesianFrame` constructor ambiguity with `Quaternion`.

## Beta version 0.8.63 (2026-03-17)
 - **Enable the G-buffer** in the renderer.
 - **Unified console command system** with clean TCP responses: scene creation with camera, node manipulation, window control, `createScene`/`deleteScene`/`setBackground`/`setGround`/`addMesh`, and JSON scene loading through the TCP `RemoteListener` (`SceneDefinition`, a complete JSON scene description format).
 - Vulkan compute pipeline + GPU X-Ray volumetric scanner; fix the remote listener and add an `XRayAnalyzer` unpack option.
 - Rendering: add LOD capability. VertexFactory: add `ShapeSplitter`.
 - Resources: `ControllableTrait`, terrain types, and `ClassId` container identification in the console.
 - Docs: the AI Runtime Control guide, promoted to a GOLD RULE.

## Beta version 0.8.621 (2026-03-13)
 - **Runtime BC7 texture compression**: the `bc7enc_rdo` library, a `TextureCompressor` for block compression, a compressed-texture upload pipeline for multi-mip data, wiring into the `Texture2D` pipeline, and a disk cache for instant loading on subsequent launches.
 - Add sRGB texture format support for correct PBR color handling.
 - **Indirect rendering (MDI)**: phase 1A, then phase 1B activating the dispatch with a full batch fallback.
 - Ray tracing: eliminate the per-frame CPU stall from the synchronous TLAS rebuild, fix the pipeline barrier and TLAS/buffer lifetime for the async build, remove the legacy synchronous `buildTLAS` path, and add a pixel doubling option for RTGI.
 - Renderable: an instance-local program cache eliminates per-draw hash lookups.
 - Fix a race condition in the lazy `PostProcessStack` scene target creation, and a `GrabPass` use-after-free on lazy reconfiguration.
 - Bloom: add a Karis anti-firefly filter and NaN protection.
 - glTF: fix incorrect UV loading, fix scaling when loading as a `StaticEntity`, and defer `Texture2D` creation to material loading. ShadowMapping: fix loading with a mask.

## Beta version 0.8.62 (2026-03-10)
 - **Ray tracing enabled.**
 - GLTFLoader completed.
 - Minor fix for macOS when ray tracing is unavailable.

## Beta version 0.8.611 (2026-03-05)
 - Versioning scheme restructured (0.8.61 → 0.8.6xx) for the new development cycle.
 - macOS: fix the dock icon disappearing when showing message dialogs, fix the duplicate file extension in the save dialog, and only strip an extension when it matches a declared filter.
 - Manage the `not_allowed` cursor; fix a bug on enabling/disabling the post-processor.
 - IO: file/stream decoupling for media formats.

## Beta version 0.8.61 (2026-03-04)
 - New development cycle structuration.
 - Replace some `std::any` usage with `std::variant`.
 - Add a dialog box for data input.
 - Fix `Libs::IO::zipWriter` for MSVC and for multiple files.

## Beta version 0.8.6 "Push It To The Limit 🤟" (2026-02-20)
 - **New framebuffer post-processing family**, replacing the old `Scenes/Effect` lens effects: `ToneMapping`, `Bloom`, `SSR`, `SSAO`, `DepthOfField`, `FXAA` (+ `FXAASharpen`) and `AtmosphericFog`, with a reworked `PostProcessor` and a new `SceneRenderTarget`.
 - **RenderDoc integration**: `SetupRenderDoc.cmake`, `BuildRenderDocPython.cmake` and the RenderDoc submodule, for in-application GPU frame capture and programmatic `.rdc` analysis.
 - Massive `ShapeGenerator` expansion, and a reworked geometry `ResourceGenerator`.
 - Add `CubemapMovieResource` and `DummyColorProjectionTexture`.
 - Add `AI-COLLABORATION.md`.

## Beta version 0.8.52 (2026-02-12)
 - Recorder and `GrabPass` hardening; Vulkan `RenderPass` work.
 - PBR material additions; audio `Recorder` and `ExternalInput` improvements; IO fixes.
 - Add the `update-glfw.sh` helper.

## Beta version 0.8.51 (2026-02-05)
 - **Video capture devices** on Linux, macOS and Windows (`VideoCaptureDevice`), with libVPX and `SetupVideoDeviceCapture.cmake`.
 - **Screen capture / video recording**: a `Graphics::Recorder` and a `GrabPass`.
 - Audio: add a `Listener` and an `ExternalInput`; `AudioRecorder` becomes `Recorder`.
 - Extend the PBR material resource and its light generator.
 - New documentation: `caution-points.md`, `development-patterns.md`, `troubleshooting.md`, `trueglass-screen-capture-implementation.md`.

## Beta version 0.8.5 (2026-01-20)
 - **Bindless texture manager.**
 - **Cascaded shadow maps**: `ViewMatricesCascadedUBO`, an extended `ShadowMap` render target, a shadow-map light generator and a dummy shadow texture — documented in `docs/shadow-mapping.md`.
 - VertexFactory: split the mesh file formats into their own headers (MDx, STL) and rewrite the OBJ reader.

## Beta version 0.8.46b (2026-01-19)
 - Consolidate the platform-specific helpers: the Linux dialogs share a single implementation (~900 lines removed).
 - macOS dialog fixes; update the `PlatformSpecific` documentation.

## Beta version 0.8.46 (2026-01-15)
 - **PBR material**: a full `Material::PBRResource` and its Saphir PBR light generator.
 - Custom message dialogs on Linux, macOS and Windows.
 - Document the pipeline caching system.

## Beta version 0.8.45 (2026-01-12)
 - **Desktop notifications** on Linux, macOS and Windows, behind a `SystemNotification` service.
 - Rework the native dialogs (message, open file, save file), mainly on Linux.
 - Add the project tooling: `build.py`, `format-code.py` (+ `format-code.json`) and `run_unit_tests.py`.

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
