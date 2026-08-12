/*
 * src/SettingKeys.hpp
 * This file is part of Emeraude-Engine
 *
 * Copyright (C) 2010-2026 - Sébastien Léon Claude Christian Bémelmans "LondNoir" <londnoir@gmail.com>
 *
 * Emeraude-Engine is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * Emeraude-Engine is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Emeraude-Engine; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * Complete project and additional information can be found at :
 * https://github.com/EmeraudeEngine/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#pragma once

/* Project configuration. */
#include "emeraude_platform.hpp"

/* STL inclusions. */
#include <cstdint>

namespace EmEn
{
	/* Core */
	/* Log detailed core startup/service information. Also enabled by the "--show-core-infos" argument. */
	constexpr auto CoreShowInformationKey{"Core/ShowInformation"};
	constexpr auto DefaultCoreShowInformation{false};
	/* Collect and expose runtime engine statistics (timings, counters). */
	constexpr auto CoreEnableStatisticsKey{"Core/EnableStatistics"};
	constexpr auto DefaultCoreEnableStatistics{false};
	/* External text editor used to open generated files (e.g. shader sources). Default is platform-dependent. */
	constexpr auto TextEditorKey{"Core/TextEditor"};
#if IS_LINUX
	constexpr auto DefaultTextEditor{"gedit"};
#elif IS_WINDOWS
	constexpr auto DefaultTextEditor{"notepad"};
#elif IS_MACOS
	constexpr auto DefaultTextEditor{"TextEdit"};
#endif
	/* System notification permission policy. Values: "allow", "deny", "ask" (default). */
	constexpr auto CorePermissionsNotificationsKey{"Core/Permissions/Notifications"};
	constexpr auto DefaultCorePermissionsNotifications{"ask"};

		/* Tracer */
		/* Restrict console tracing to errors and fatal messages only. */
		constexpr auto TracerPrintOnlyErrorsKey{"Core/Tracer/PrintOnlyErrors"};
		constexpr auto DefaultTracerPrintOnlyErrors{false};
		/* Append the source file/line location to each trace entry. */
		constexpr auto TracerEnableSourceLocationKey{"Core/Tracer/EnableSourceLocation"};
		constexpr auto DefaultTracerEnableSourceLocation{false};
		/* Append the originating thread id/name to each trace entry. */
		constexpr auto TracerEnableThreadInfosKey{"Core/Tracer/EnableThreadInfos"};
		constexpr auto DefaultTracerEnableThreadInfos{false};
		/* Also write traces to a log file, in addition to the console. */
		constexpr auto TracerEnableLoggerKey{"Core/Tracer/EnableLogger"};
		constexpr auto DefaultTracerEnableLogger{false};
		/* Log file output format. Default "Text". */
		constexpr auto TracerLogFormatKey{"Core/Tracer/LogFormat"};
		constexpr auto DefaultTracerLogFormat{"Text"};

		/* Console */
		/* TCP port the remote console listens on for live commands (AI runtime control). */
		constexpr auto ConsoleRemoteListenerPortKey{"Core/Console/RemoteListenerPort"};
		constexpr auto DefaultConsoleRemoteListenerPort{static_cast< uint16_t >(7777)};

		/* Input manager */
		/* Log input-device (keyboard/mouse/gamepad) detection details. Also "--show-input-infos". */
		constexpr auto InputShowInformationKey{"Core/Input/ShowInformation"};
		constexpr auto DefaultInputShowInformation{false};

		/* Resource manager */
		/* Log resource manager activity. Also "--show-resources-infos". */
		constexpr auto ResourcesShowInformationKey{"Core/Resources/ShowInformation"};
		constexpr auto DefaultResourcesShowInformation{false};
		/* Allow downloading missing resources from remote stores. */
		constexpr auto ResourcesDownloadEnabledKey{"Core/Resources/DownloadEnabled"};
		constexpr auto DefaultResourcesDownloadEnabled{true};
		/* Suppress per-resource conversion log spam. */
		constexpr auto ResourcesQuietConversionKey{"Core/Resources/QuietConversion"};
		constexpr auto DefaultResourcesQuietConversion{true};
		/* Scan resource directories dynamically at runtime instead of relying on a static index. */
		constexpr auto ResourcesUseDynamicScanKey{"Core/Resources/UseDynamicScan"};
		constexpr auto DefaultResourcesUseDynamicScan{true};

		/* Audio layer */
		/* Master switch for the whole audio subsystem. */
		constexpr auto AudioEnableKey{"Core/Audio/Enable"};
		constexpr auto DefaultAudioEnable{true};
		/* Output device name. "AutoDetect" lets the engine pick the system default device. */
		constexpr auto AudioDeviceNameKey{"Core/Audio/DeviceName"};
		constexpr auto DefaultAudioDeviceName{"AutoDetect"};
		/* Runtime-populated list of detected output devices (read-only, no default). */
		constexpr auto AudioAvailableDevicesKey{"Core/Audio/AvailableDevices"};
		/* Output sample rate in Hz. */
		constexpr auto AudioPlaybackFrequencyKey{"Core/Audio/PlaybackFrequency"};
		constexpr auto DefaultAudioPlaybackFrequency{48000};
		/* Master output gain, range [0.0 .. 1.0]. */
		constexpr auto AudioMasterVolumeKey{"Core/Audio/MasterVolume"};
		constexpr auto DefaultAudioMasterVolume{0.75F};
		/* Sound-effects gain, range [0.0 .. 1.0]. */
		constexpr auto AudioSFXVolumeKey{"Core/Audio/SFXVolume"};
		constexpr auto DefaultAudioSFXVolume{0.6F};
		/* Music gain, range [0.0 .. 1.0]. */
		constexpr auto AudioMusicVolumeKey{"Core/Audio/MusicVolume"};
		constexpr auto DefaultAudioMusicVolume{0.5F};
		/* Music streaming buffer size in samples. */
		constexpr auto AudioMusicChunkSizeKey{"Core/Audio/MusicChunkSize"};
		constexpr auto DefaultAudioMusicChunkSize{8192};
		/* Path to a SoundFont (.sf2) used for MIDI music. Empty = none. */
		constexpr auto AudioMusicSoundfontKey{"Core/Audio/MusicSoundfont"};
		constexpr auto DefaultAudioMusicSoundfont{""};
		/* Register the engine's built-in procedural sounds. */
		constexpr auto AudioEnablePrebuiltSoundsKey{"Core/Audio/EnablePrebuiltSounds"};
		constexpr auto DefaultAudioEnablePrebuiltSounds{false};
		/* Log audio subsystem details. Also "--show-audio-infos". */
		constexpr auto AudioShowInformationKey{"Core/Audio/ShowInformation"};
		constexpr auto DefaultAudioShowInformation{false};
		/* Speaker layout. Values: "Auto", "Stereo", "Surround51". */
		constexpr auto AudioOutputModeKey{"Core/Audio/OutputMode"};
		constexpr auto DefaultAudioOutputMode{"Auto"};

			/* OpenAL */
			/* Enable OpenAL EFX effects (reverb, filters, ...) when the device supports them. */
			constexpr auto OpenALUseEFXExtensionsKey{"Core/Audio/OpenAL/UseEFXExtensions"};
			constexpr auto DefaultOpenALUseEFXExtensions{true};
			/* OpenAL context refresh rate in Hz. */
			constexpr auto OpenALRefreshRateKey{"Core/Audio/OpenAL/RefreshRate"};
			constexpr auto DefaultOpenALRefreshRate{46};
			/* OpenAL synchronous context flag (0 = asynchronous). */
			constexpr auto OpenALSyncStateKey{"Core/Audio/OpenAL/SyncState"};
			constexpr auto DefaultOpenALSyncState{0};
			/* Maximum number of simultaneous mono sources. */
			constexpr auto OpenALMaxMonoSourceCountKey{"Core/Audio/OpenAL/MaxMonoSourceCount"};
			constexpr auto DefaultOpenALMaxMonoSourceCount{32};
			/* Maximum number of simultaneous stereo sources. */
			constexpr auto OpenALMaxStereoSourceCountKey{"Core/Audio/OpenAL/MaxStereoSourceCount"};
			constexpr auto DefaultOpenALMaxStereoSourceCount{2};

			/* Audio Capture (Audio::ExternalInput) */
			/* Enable audio input capture (microphone / line-in). */
			constexpr auto AudioCaptureEnableKey{"Core/Audio/Capture/Enable"};
			constexpr auto DefaultAudioCaptureEnable{false};
			/* Capture device name. "AutoDetect" picks the system default. */
			constexpr auto AudioCaptureDeviceNameKey{"Core/Audio/Capture/DeviceName"};
			constexpr auto DefaultAudioCaptureDeviceName{"AutoDetect"};
			/* Runtime-populated list of detected capture devices (read-only, no default). */
			constexpr auto AudioCaptureAvailableDevicesKey{"Core/Audio/Capture/AvailableDevices"};
			/* Capture sample rate in Hz. */
			constexpr auto AudioCaptureFrequencyKey{"Core/Audio/Capture/Frequency"};
			constexpr auto DefaultAudioCaptureFrequency{48000};
			/* Capture buffer size in samples. */
			constexpr auto AudioCaptureBufferSizeKey{"Core/Audio/Capture/BufferSize"};
			constexpr auto DefaultAudioCaptureBufferSize{64};

		/* Video */
		/* Persist window/video geometry and state on exit. */
		constexpr auto VideoSavePropertiesAtExitKey{"Core/Video/SavePropertiesAtExit"};
		constexpr auto DefaultVideoSavePropertiesAtExit{true};
		/* Index of the monitor to open the window on (0 = primary). */
		constexpr auto VideoPreferredMonitorKey{"Core/Video/PreferredMonitor"};
		constexpr auto DefaultVideoPreferredMonitor{0};
		/* Synchronize presentation to the monitor refresh (vertical sync). */
		constexpr auto VideoEnableVSyncKey{"Core/Video/EnableVSync"};
		constexpr auto DefaultVideoEnableVSync{true};
		/* Double-buffered presentation (currently not in use). */
		constexpr auto VideoEnableDoubleBufferingKey{"Core/Video/EnableDoubleBuffering"};
		constexpr auto DefaultEnableDoubleBuffering{false};
		/* Triple-buffered (mailbox) presentation when available. */
		constexpr auto VideoEnableTripleBufferingKey{"Core/Video/EnableTripleBuffering"};
		constexpr auto DefaultVideoEnableTripleBuffering{true};
		/* Frame-rate cap in FPS. 0 = uncapped. */
		constexpr auto VideoFrameRateLimitKey{"Core/Video/FrameRateLimit"};
		constexpr auto DefaultVideoFrameRateLimit{0U};
		/* Present in an sRGB swapchain format. */
		constexpr auto VideoEnableSRGBKey{"Core/Video/EnableSRGB"};
		constexpr auto DefaultEnableSRGB{false};
		/* Log video/Vulkan setup details. Also "--show-video-infos". */
		constexpr auto VideoShowInformationKey{"Core/Video/ShowInformation"};
		constexpr auto DefaultVideoShowInformation{false};

		/* Video Capture (Graphics::ExternalInput) */
		/* Enable video capture input (webcam). NOTE: the key path below points to "Core/Audio/Capture/Enable" and collides with AudioCaptureEnableKey - likely a typo, should be "Core/Video/Capture/Enable". */
		constexpr auto VideoCaptureEnableKey{"Core/Audio/Capture/Enable"};
		constexpr auto DefaultVideoCaptureEnable{false};
		/* Capture device index. -1 = auto (first available). */
		constexpr auto VideoCaptureDeviceIndexKey{"Core/Video/Capture/DeviceIndex"};
		constexpr auto DefaultVideoCaptureDeviceIndex{-1};
		/* Requested capture width in pixels. */
		constexpr auto VideoCaptureDeviceWidthKey{"Core/Video/Capture/Width"};
		constexpr auto DefaultVideoCaptureDeviceWidth{640U};
		/* Requested capture height in pixels. */
		constexpr auto VideoCaptureDeviceHeightKey{"Core/Video/Capture/Height"};
		constexpr auto DefaultVideoCaptureDeviceHeight{480U};

			/* Vulkan instance */
			/* Enable Vulkan debug utils and the validation messenger. */
			constexpr auto VkInstanceEnableDebugKey{"Core/Video/VulkanInstance/EnableDebug"};
			constexpr auto DefaultVkInstanceEnableDebug{false};
			/* Validation layers to request at instance creation (no default). */
			constexpr auto VkInstanceRequestedValidationLayersKey{"Core/Video/VulkanInstance/RequestedValidationLayers"};
			/* Runtime-populated list of validation layers available on this system (read-only). */
			constexpr auto VkInstanceAvailableValidationLayersKey{"Core/Video/VulkanInstance/AvailableValidationLayers"};

			/* Vulkan device */
			/* Runtime-populated list of detected GPUs (read-only, no default). */
			constexpr auto VkDeviceAvailableGPUsKey{"Core/Video/VulkanDevice/AvailableGPUs"};
			/* GPU auto-selection strategy. Values: "DontCare", "Performance", "PowerSaving". */
			constexpr auto VkDeviceAutoSelectModeKey{"Core/Video/VulkanDevice/AutoSelectMode"};
			constexpr auto DefaultVkDeviceAutoSelectMode{"Performance"};
			/* Fall back to a minimal/safe device configuration on init failure. */
			constexpr auto VkDeviceEnableFailSafeKey{"Core/Video/VulkanDevice/EnableFailSafe"};
			constexpr auto DefaultEnableFailSafe{false};
			/* Force a specific GPU (by name), overriding auto-selection (no default = disabled). */
			constexpr auto VkDeviceForceGPUKey{"Core/Video/VulkanDevice/ForceGPU"};
			/* Use the Vulkan Memory Allocator (VMA) for GPU allocations. */
			constexpr auto VkDeviceUseVMAKey{"Core/Video/VulkanDevice/UseVMA"};
			constexpr auto DefaultVkDeviceUseVMA{true};

			/* Window */
			/* Ignore the saved position and center the window on each launch. */
			constexpr auto WindowAlwaysCenterOnStartupKey{"Core/Video/Window/AlwaysCenterOnStartup"};
			constexpr auto DefaultWindowAlwaysCenterOnStartup{false};
			/* Create a borderless window (no OS title bar / decorations). */
			constexpr auto WindowFramelessKey{"Core/Video/Window/Frameless"};
			constexpr auto DefaultWindowFrameless{false};
			/* Light/dark appearance of the OS title bar: "System" follows the OS preference, "Dark" and "Light" force it. */
			constexpr auto WindowTitleBarThemeKey{"Core/Video/Window/TitleBarTheme"};
			constexpr auto DefaultWindowTitleBarTheme{"System"};
			/* Windowed-mode X position in pixels. */
			constexpr auto WindowXPositionKey{"Core/Video/Window/XPosition"};
			constexpr auto DefaultWindowXPosition{64};
			/* Windowed-mode Y position in pixels. */
			constexpr auto WindowYPositionKey{"Core/Video/Window/YPosition"};
			constexpr auto DefaultWindowYPosition{64};
			/* Windowed-mode width in pixels. */
			constexpr auto WindowWidthKey{"Core/Video/Window/Width"};
			constexpr auto DefaultWindowWidth{1280U};
			/* Windowed-mode height in pixels. */
			constexpr auto WindowHeightKey{"Core/Video/Window/Height"};
			constexpr auto DefaultWindowHeight{720U};
			/* Gamma correction applied in windowed mode. */
			constexpr auto WindowGammaKey{"Core/Video/Window/Gamma"};
			constexpr auto DefaultWindowGamma{1.0F};

				/* GLFW */
				/* Force a GLFW windowing backend, or "Auto" to let GLFW decide. */
				constexpr auto GLFWUsePlatformKey{"Core/Video/Window/GLFW/UsePlatform"};
				constexpr auto DefaultGLFWUsePlatform{"Auto"};
				/* Create the Vulkan surface via native OS code instead of GLFW. */
				constexpr auto GLFWEnableNativeCodeForVkSurfaceKey{"Core/Video/Window/GLFW/EnableNativeCodeForVkSurface"};
				constexpr auto DefaultEnableNativeCodeForVkSurface{false};
				/* Use libdecor for client-side window decorations on Wayland. */
				constexpr auto GLFWWaylandEnableLibDecorKey{"Core/Video/Window/GLFW/Wayland/EnableLibDecor"};
				constexpr auto DefaultGLFWWaylandEnableLibDecor{true};
				/* Prefer XCB over Xlib for the Vulkan surface on X11. */
				constexpr auto GLFWX11UseXCBInsteadOfXLibKey{"Core/Video/Window/GLFW/X11/UseXCBInsteadOfXLib"};
				constexpr auto DefaultGLFWX11UseXCBInsteadOfXLib{true};

			/* Fullscreen */
			/* Start in fullscreen mode. */
			constexpr auto VideoFullscreenEnabledKey{"Core/Video/Fullscreen/Enabled"};
			constexpr auto DefaultVideoFullscreenEnabled{false};
			/* Fullscreen width in pixels. */
			constexpr auto VideoFullscreenWidthKey{"Core/Video/Fullscreen/Width"};
			constexpr auto DefaultVideoFullscreenWidth{1920U};
			/* Fullscreen height in pixels. */
			constexpr auto VideoFullscreenHeightKey{"Core/Video/Fullscreen/Height"};
			constexpr auto DefaultVideoFullscreenHeight{1080U};
			/* Gamma correction applied in fullscreen. */
			constexpr auto VideoFullscreenGammaKey{"Core/Video/Fullscreen/Gamma"};
			constexpr auto DefaultVideoFullscreenGamma{1.0F};
			/* Fullscreen refresh rate in Hz. -1 = use the monitor default. */
			constexpr auto VideoFullscreenRefreshRateKey{"Core/Video/Fullscreen/RefreshRate"};
			constexpr auto DefaultVideoFullscreenRefreshRate{-1};

			/* Overlay */
			/* Override the automatic overlay (UI) scaling with ScaleX/ScaleY below. */
			constexpr auto OverlayForceScaleKey{"Core/Video/Overlay/ForceScale"};
			constexpr auto DefaultOverlayForceScale{false};
			/* Manual overlay scale factor on X/Y (shared default). Only used when ForceScale is true. */
			constexpr auto OverlayScaleXKey{"Core/Video/Overlay/ScaleX"};
			constexpr auto OverlayScaleYKey{"Core/Video/Overlay/ScaleY"};
			constexpr auto DefaultOverlayScale{1.0F};

			/* Framebuffer */
			/* Red channel bit depth of the framebuffer. */
			constexpr auto VideoFramebufferRedBitsKey{"Core/Video/Framebuffer/RedBits"};
			constexpr auto DefaultVideoFramebufferRedBits{8U};
			/* Green channel bit depth of the framebuffer. */
			constexpr auto VideoFramebufferGreenBitsKey{"Core/Video/Framebuffer/GreenBits"};
			constexpr auto DefaultVideoFramebufferGreenBits{8U};
			/* Blue channel bit depth of the framebuffer. */
			constexpr auto VideoFramebufferBlueBitsKey{"Core/Video/Framebuffer/BlueBits"};
			constexpr auto DefaultVideoFramebufferBlueBits{8U};
			/* Alpha channel bit depth of the framebuffer. */
			constexpr auto VideoFramebufferAlphaBitsKey{"Core/Video/Framebuffer/AlphaBits"};
			constexpr auto DefaultVideoFramebufferAlphaBits{8U};
			/* Depth buffer bit depth. */
			constexpr auto VideoFramebufferDepthBitsKey{"Core/Video/Framebuffer/DepthBits"};
			constexpr auto DefaultVideoFramebufferDepthBits{32U};
			/* Stencil buffer bit depth. 0 = no stencil. */
			constexpr auto VideoFramebufferStencilBitsKey{"Core/Video/Framebuffer/StencilBits"};
			constexpr auto DefaultVideoFramebufferStencilBits{0U};
			/* MSAA sample count (1 = no multisampling). */
			constexpr auto VideoFramebufferSamplesKey{"Core/Video/Framebuffer/Samples"};
			constexpr auto DefaultVideoFramebufferSamples{1U};
			/* Enable morphological anti-aliasing (MLAA) post-process. */
			constexpr auto VideoFramebufferEnableMLAAKey{"Core/Video/Framebuffer/EnableMLAA"};
			constexpr auto DefaultVideoEnableMLAA{false};

		/* Graphics */
		/* Far clip / render distance in world units (default ~10 km). */
		constexpr auto GraphicsViewDistanceKey{"Core/Graphics/ViewDistance"};
		constexpr auto DefaultGraphicsViewDistance{10000.0F}; /* NOTE: 10km */
		/* Vertical field of view in degrees. */
		/* NOTE: The framing is authored as a LENS, never as an angle — a camera is configured
		 * like a real appliance and the field of view is derived (see Scenes::Component::Camera).
		 * 13.096 mm is the focal length that reproduces the historical 85° default on a 36x24 mm
		 * full-frame sensor, so this constant carries the same framing as the angle it replaces.
		 * ⚠️ The key itself has NEVER been read by anything — it was dead when it named an angle
		 * and it stays dead now; a user-facing framing control has yet to be wired. */
		constexpr auto GraphicsFocalLengthKey{"Core/Graphics/FocalLength"};
		constexpr auto DefaultGraphicsFocalLength{13.096F}; /* NOTE: 85° vertical, full frame. */

			/* Texture */
			/* Magnification / minification / mipmap filtering (shared default). Values: "nearest", "linear". */
			constexpr auto GraphicsTextureMagFilteringKey{"Core/Graphics/Texture/MagFilter"};
			constexpr auto GraphicsTextureMinFilteringKey{"Core/Graphics/Texture/MinFilter"};
			constexpr auto GraphicsTextureMipFilteringKey{"Core/Graphics/Texture/MipFilter"};
			constexpr auto DefaultGraphicsTextureFiltering{"nearest"};
			/* Number of mipmap levels to generate. */
			constexpr auto GraphicsTextureMipMappingLevelsKey{"Core/Graphics/Texture/MipMappingLevels"};
			constexpr auto DefaultGraphicsTextureMipMappingLevels{1};
			/* Largest accepted texture dimension, in pixels (0 = no clamping).
			 * NOTE: Only honored by sources that ship a ready-made mip chain (KTX2), where dropping the
			 * top levels is free. It divides the VRAM footprint by four every time it is halved. */
			constexpr auto GraphicsTextureMaxDimensionKey{"Core/Graphics/Texture/MaxDimension"};
			constexpr auto DefaultGraphicsTextureMaxDimension{4096};
			/* Anisotropic filtering level (0 = off). */
			constexpr auto GraphicsTextureAnisotropyLevelsKey{"Core/Graphics/Texture/AnisotropyLevels"};
			constexpr auto DefaultGraphicsTextureAnisotropy{0};
			/* Distance up to which full-resolution textures are used (default ~5 km). */
			constexpr auto GraphicsTextureViewDistanceKey{"Core/Graphics/Texture/ViewDistance"};
			constexpr auto DefaultGraphicsTextureViewDistance{5000.0F}; /* NOTE: 5km */
			/* Parallax occlusion mapping ray-march iteration count. */
			constexpr auto GraphicsTexturePOMIterationsKey{"Core/Graphics/Texture/POMIterations"};
			constexpr auto DefaultGraphicsTexturePOMIterations{0};

			/* GPU Profiler (Vulkan timestamp queries).
			 * Per-pass GPU timings of the main frame command buffer, harvested without
			 * stall (one query pool per frame in flight) and served by the remote console
			 * command Core.RendererService.getGPUTimings(). Zero cost when disabled. */
			constexpr auto GraphicsGPUProfilerEnabledKey{"Core/Graphics/GPUProfiler/Enabled"};
			constexpr auto DefaultGraphicsGPUProfilerEnabled{false};

			/* Ray Tracing.
			 * The root group holds the master switch and the acceleration-structure
			 * (BLAS/TLAS) options; each ray-traced effect has its own sub-group. */
			/* Master switch for hardware ray tracing. */
			constexpr auto GraphicsRayTracingEnabledKey{"Core/Graphics/RayTracing/Enabled"};
			constexpr auto DefaultGraphicsRayTracingEnabled{false};
			/* Max distance for the top-level acceleration structure, in world units. */
			constexpr auto GraphicsRayTracingTLASDistanceKey{"Core/Graphics/RayTracing/TLASDistance"};
			constexpr auto DefaultGraphicsRayTracingTLASDistance{1000.0F};

			/* Ray Tracing > Reflection */
			constexpr auto GraphicsRayTracingReflectionEnabledKey{"Core/Graphics/RayTracing/Reflection/Enabled"};
			constexpr auto DefaultGraphicsRayTracingReflectionEnabled{true};
			/* Compute reflections at half resolution (pixel doubling) to save performance. */
			constexpr auto GraphicsRayTracingReflectionPixelDoublingKey{"Core/Graphics/RayTracing/Reflection/PixelDoubling"};
			constexpr auto DefaultGraphicsRayTracingReflectionPixelDoubling{true};

			/* Ray Tracing > Reflection > Glossy cone (pre-convolved reflection pyramid lookup).
			 * The v1 cone is UNIFORM in screen space: it assumes a representative hit distance
			 * and ignores surface curvature, so a curved mirror (sphere) is over-blurred by an
			 * order of magnitude — the reflected environment is compressed into the silhouette,
			 * where the same GGX lobe covers far fewer screen texels than on a flat floor.
			 * These knobs exist to MEASURE that trade-off on the reflection bench; the definitive
			 * fix is the stochastic + temporal successor (per-pixel hit distance through an MRT). */
			/* Master switch for the cone lookup: false = the pyramid is never read, the composite
			 * shows the RAW traced reflection at its full trace resolution (sharpness reference). */
			constexpr auto GraphicsRayTracingReflectionGlossyConeEnabledKey{"Core/Graphics/RayTracing/Reflection/GlossyCone/Enabled"};
			constexpr auto DefaultGraphicsRayTracingReflectionGlossyConeEnabled{true};
			/* Assumed hit distance as a fraction of the screen height: the cone width in trace
			 * texels is 2 x thisFraction x traceHeight x roughness². NOTE that this makes the
			 * width proportional to the resolution, hence the perceived blur INVARIANT in
			 * resolution — which is why PixelDoubling alone can never sharpen the reflection. */
			constexpr auto GraphicsRayTracingReflectionGlossyConeHitFractionKey{"Core/Graphics/RayTracing/Reflection/GlossyCone/HitFraction"};
			constexpr auto DefaultGraphicsRayTracingReflectionGlossyConeHitFraction{0.15F};
			/* Cone width (in trace texels) below which the reflection stays PURELY the sharp
			 * traced buffer: under one texel of spread there is nothing to convolve. */
			constexpr auto GraphicsRayTracingReflectionGlossyConeBlendStartKey{"Core/Graphics/RayTracing/Reflection/GlossyCone/BlendStartTexels"};
			constexpr auto DefaultGraphicsRayTracingReflectionGlossyConeBlendStart{2.0F};
			/* Cone width (in trace texels) at which the reflection comes ENTIRELY from the
			 * pyramid. Between start and full the two are cross-faded linearly, so a near-mirror
			 * keeps most of its full-resolution traced reflection instead of being replaced
			 * wholesale by a coarse mip. */
			constexpr auto GraphicsRayTracingReflectionGlossyConeBlendFullKey{"Core/Graphics/RayTracing/Reflection/GlossyCone/BlendFullTexels"};
			constexpr auto DefaultGraphicsRayTracingReflectionGlossyConeBlendFull{24.0F};
			/* Hard ceiling on the pyramid LOD the cone may reach, on top of the mip count: caps
			 * how coarse a rough surface is allowed to get (each LOD halves the resolution). */
			constexpr auto GraphicsRayTracingReflectionGlossyConeMaxLodKey{"Core/Graphics/RayTracing/Reflection/GlossyCone/MaxLod"};
			constexpr auto DefaultGraphicsRayTracingReflectionGlossyConeMaxLod{8.0F};

			/* Ray Tracing > Ambient Occlusion */
			constexpr auto GraphicsRayTracingAOEnabledKey{"Core/Graphics/RayTracing/AmbientOcclusion/Enabled"};
			constexpr auto DefaultGraphicsRayTracingAOEnabled{true};
			/* Samples per pixel for ray-traced ambient occlusion. */
			constexpr auto GraphicsRayTracingAOSampleCountKey{"Core/Graphics/RayTracing/AmbientOcclusion/SampleCount"};
			constexpr auto DefaultGraphicsRayTracingAOSampleCount{8U};
			/* Compute ambient occlusion at half resolution (pixel doubling) to save performance. */
			constexpr auto GraphicsRayTracingAOPixelDoublingKey{"Core/Graphics/RayTracing/AmbientOcclusion/PixelDoubling"};
			constexpr auto DefaultGraphicsRayTracingAOPixelDoubling{true};
			/* AO darkening intensity multiplier (applied once, clamped; 1.0 = pure visibility term). */
			constexpr auto GraphicsRayTracingAOIntensityKey{"Core/Graphics/RayTracing/AmbientOcclusion/Intensity"};
			constexpr auto DefaultGraphicsRayTracingAOIntensity{1.0F};
			/* AO ray origin offset to prevent self-intersection, in world units. */
			constexpr auto GraphicsRayTracingAOBiasKey{"Core/Graphics/RayTracing/AmbientOcclusion/Bias"};
			constexpr auto DefaultGraphicsRayTracingAOBias{0.005F};
			/* Maximum AO occluder search distance, in world units (near-field effect). */
			constexpr auto GraphicsRayTracingAOMaxDistanceKey{"Core/Graphics/RayTracing/AmbientOcclusion/MaxDistance"};
			constexpr auto DefaultGraphicsRayTracingAOMaxDistance{2.0F};
			/* Bilateral denoising blur radius for AO, in pixels. */
			constexpr auto GraphicsRayTracingAOBlurRadiusKey{"Core/Graphics/RayTracing/AmbientOcclusion/BlurRadius"};
			constexpr auto DefaultGraphicsRayTracingAOBlurRadius{4U};
			/* Normal edge-stopping sigma for the AO bilateral blur. */
			constexpr auto GraphicsRayTracingAONormalSigmaKey{"Core/Graphics/RayTracing/AmbientOcclusion/NormalSigma"};
			constexpr auto DefaultGraphicsRayTracingAONormalSigma{0.5F};

			/* Ray Tracing > Contact Shadows */
			constexpr auto GraphicsRayTracingContactShadowsEnabledKey{"Core/Graphics/RayTracing/ContactShadows/Enabled"};
			constexpr auto DefaultGraphicsRayTracingContactShadowsEnabled{true};

			/* Ray Tracing > Global Illumination */
			constexpr auto GraphicsRayTracingGIEnabledKey{"Core/Graphics/RayTracing/GlobalIllumination/Enabled"};
			constexpr auto DefaultGraphicsRayTracingGIEnabled{true};
			/* Samples per pixel for ray-traced global illumination.
			 * Measured 2026-07-05 (Sponza+extras, RTX 3070 Ti @ 3840x1990): 8 spp is
			 * visually equivalent to 16 after the bilateral blur and ~16 ms/frame cheaper. */
			constexpr auto GraphicsRayTracingGISampleCountKey{"Core/Graphics/RayTracing/GlobalIllumination/SampleCount"};
			constexpr auto DefaultGraphicsRayTracingGISampleCount{8U};
			/* Compute GI at half resolution (pixel doubling) to save performance. */
			constexpr auto GraphicsRayTracingGIPixelDoublingKey{"Core/Graphics/RayTracing/GlobalIllumination/PixelDoubling"};
			constexpr auto DefaultGraphicsRayTracingGIPixelDoubling{true};
			/* Maximum GI bounce ray distance, in world units. */
			constexpr auto GraphicsRayTracingGIMaxDistanceKey{"Core/Graphics/RayTracing/GlobalIllumination/MaxDistance"};
			constexpr auto DefaultGraphicsRayTracingGIMaxDistance{8.0F};
			/* Indirect lighting intensity multiplier. */
			constexpr auto GraphicsRayTracingGIIntensityKey{"Core/Graphics/RayTracing/GlobalIllumination/Intensity"};
			constexpr auto DefaultGraphicsRayTracingGIIntensity{0.8F};
			/* GI ray origin offset to prevent self-intersection, in world units. */
			constexpr auto GraphicsRayTracingGIBiasKey{"Core/Graphics/RayTracing/GlobalIllumination/Bias"};
			constexpr auto DefaultGraphicsRayTracingGIBias{0.02F};
			/* Bilateral denoising blur radius for GI, in pixels. */
			constexpr auto GraphicsRayTracingGIBlurRadiusKey{"Core/Graphics/RayTracing/GlobalIllumination/BlurRadius"};
			constexpr auto DefaultGraphicsRayTracingGIBlurRadius{4U};
			/* Depth edge-stopping sigma for the GI bilateral blur. */
			constexpr auto GraphicsRayTracingGIDepthSigmaKey{"Core/Graphics/RayTracing/GlobalIllumination/DepthSigma"};
			constexpr auto DefaultGraphicsRayTracingGIDepthSigma{1.0F};
			/* Normal edge-stopping sigma for the GI bilateral blur. */
			constexpr auto GraphicsRayTracingGINormalSigmaKey{"Core/Graphics/RayTracing/GlobalIllumination/NormalSigma"};
			constexpr auto DefaultGraphicsRayTracingGINormalSigma{0.5F};

			/* Ray Tracing > Global Illumination > Temporal accumulation.
			 * Exponential moving average over reprojected history: effective sample count
			 * becomes SampleCount / Alpha (8 spp @ 0.1 ≈ 80 effective samples). */
			constexpr auto GraphicsRayTracingGITemporalEnabledKey{"Core/Graphics/RayTracing/GlobalIllumination/Temporal/Enabled"};
			constexpr auto DefaultGraphicsRayTracingGITemporalEnabled{true};
			/* Blend weight of the CURRENT frame (lower = smoother, more history lag). */
			constexpr auto GraphicsRayTracingGITemporalAlphaKey{"Core/Graphics/RayTracing/GlobalIllumination/Temporal/Alpha"};
			constexpr auto DefaultGraphicsRayTracingGITemporalAlpha{0.1F};
			/* Relative camera-distance tolerance for history rejection (disocclusion test). */
			constexpr auto GraphicsRayTracingGITemporalDepthToleranceKey{"Core/Graphics/RayTracing/GlobalIllumination/Temporal/DepthTolerance"};
			constexpr auto DefaultGraphicsRayTracingGITemporalDepthTolerance{0.05F};
			/* Minimum cosine between current and history normals to accept history. */
			constexpr auto GraphicsRayTracingGITemporalNormalThresholdKey{"Core/Graphics/RayTracing/GlobalIllumination/Temporal/NormalThreshold"};
			constexpr auto DefaultGraphicsRayTracingGITemporalNormalThreshold{0.8F};
			/* Rectify the reprojected history against the current 3x3 neighbourhood statistics
			 * (anti-ghosting). Since 2026-08: VARIANCE CLIPPING (mean ± gamma * sigma, Salvi
			 * GDC 2016 — same technique as the TAA), no longer a min/max clamp.
			 * ⚠ DEFAULT OFF since the SVGF reorder (owner decision, measured 2026-08-06): the
			 * temporal resolve now integrates the RAW trace, and clipping against the raw 3x3
			 * statistics pulls the history toward the noisy local distribution — about 5% of
			 * GI energy lost on the Sponza corridor bench, no peak-to-peak gain. SVGF relies
			 * on the double disocclusion validation alone; the key remains for A/B. */
			constexpr auto GraphicsRayTracingGITemporalNeighborhoodClampKey{"Core/Graphics/RayTracing/GlobalIllumination/Temporal/NeighborhoodClamp"};
			constexpr auto DefaultGraphicsRayTracingGITemporalNeighborhoodClamp{false};
			/* Width of the variance-clipping bound, in standard deviations (gamma). Smaller =
			 * tighter anti-ghosting but slower convergence; larger = smoother accumulation. */
			constexpr auto GraphicsRayTracingGITemporalVarianceGammaKey{"Core/Graphics/RayTracing/GlobalIllumination/Temporal/VarianceGamma"};
			constexpr auto DefaultGraphicsRayTracingGITemporalVarianceGamma{1.0F};
			/* Advance the per-pixel noise every frame along the R2 low-discrepancy sequence so
			 * the temporal accumulation averages the sampling error instead of freezing it as a
			 * static pattern. Only effective when the temporal chain is enabled — animated noise
			 * without accumulation boils.
			 * DEFAULT ON since the SVGF chain landed (owner decision, measured 2026-08-06):
			 * with the variance-guided à-trous + the 1/N accumulation counter the animation is
			 * net-positive — energy restored (a frozen pattern turns stable bright outliers
			 * into "converged signal" the filter protects: fireflies), best distribution tails,
			 * peak-to-peak 0.55 vs the 0.67-0.83 marbled baseline. History (2026-08-05): with
			 * the FIXED-alpha EMA alone this regressed x2.4 — never enable it without the
			 * spatial filter ahead of the resolve. */
			constexpr auto GraphicsRayTracingGITemporalAnimatedNoiseKey{"Core/Graphics/RayTracing/GlobalIllumination/Temporal/AnimatedNoise"};
			constexpr auto DefaultGraphicsRayTracingGITemporalAnimatedNoise{true};

			/* Ray Tracing > Global Illumination > Denoiser (shared GIDenoiser component, SVGF).
			 * À-trous iterations over the temporally integrated irradiance (5x5 kernel,
			 * footprint doubles each pass: 1, 2, 4, 8, 16 texels). 0 disables the spatial
			 * filter entirely (temporal resolve only — the A/B lever). Replaces the former
			 * shared bilateral blur H/V (the BlurRadius key is inert for RTGI since then). */
			constexpr auto GraphicsRayTracingGIDenoiserIterationsKey{"Core/Graphics/RayTracing/GlobalIllumination/Denoiser/Iterations"};
			constexpr auto DefaultGraphicsRayTracingGIDenoiserIterations{4U};
			/* Luminance edge-stopping sigma, normalised by the LOCAL standard deviation
			 * (SVGF auto-dosage: noisy → smooth hard, converged → preserve detail).
			 * Larger = closer to a plain depth/normal bilateral (guidance off). */
			constexpr auto GraphicsRayTracingGIDenoiserLuminanceSigmaKey{"Core/Graphics/RayTracing/GlobalIllumination/Denoiser/LuminanceSigma"};
			constexpr auto DefaultGraphicsRayTracingGIDenoiserLuminanceSigma{4.0F};
			/* Per-pixel 1/N accumulation counter (SVGF): the temporal blend weight is
			 * max(1/(age+1), 1/MaxAccumulation) instead of the fixed Temporal/Alpha — fast
			 * convergence after a disocclusion (alpha 1, 1/2, 1/3...), tiny steady-state
			 * variance leak (about 0.8% at N=64 versus about 23% at fixed alpha 0.1, the
			 * factor that sank the first animated-noise attempt). Temporal/Alpha only rules
			 * when this is off (A/B lever). */
			constexpr auto GraphicsRayTracingGIDenoiserAccumulationCounterKey{"Core/Graphics/RayTracing/GlobalIllumination/Denoiser/AccumulationCounter"};
			constexpr auto DefaultGraphicsRayTracingGIDenoiserAccumulationCounter{true};
			/* Accumulation cap N (the steady-state blend weight floor is 1/N). Larger =
			 * smoother but slower to react to lighting changes. */
			constexpr auto GraphicsRayTracingGIDenoiserMaxAccumulationKey{"Core/Graphics/RayTracing/GlobalIllumination/Denoiser/MaxAccumulation"};
			constexpr auto DefaultGraphicsRayTracingGIDenoiserMaxAccumulation{64U};
			/* Debug view of the denoiser internals, drawn by the combine pass INSTEAD of the
			 * GI contribution: 0 = off, 1 = temporal variance (binary-amplified x1e6 — a linear
			 * scale is unreadable under the photometric exposure), 2 = accumulation age
			 * (white = young/disoccluded). Diagnostic only, costs nothing at 0. */
			constexpr auto GraphicsRayTracingGIDenoiserDebugViewKey{"Core/Graphics/RayTracing/GlobalIllumination/Denoiser/DebugView"};
			constexpr auto DefaultGraphicsRayTracingGIDenoiserDebugView{0U};

			/* Ray Tracing > Global Illumination > Multi-bounce feedback.
			 * Bounce rays landing on a surface visible last frame pick up its accumulated
			 * indirect radiance: the geometric series converges to the multi-bounce solution
			 * (one traced bounce per frame, energy 1/(1-albedo*strength) at steady state).
			 * Requires the temporal accumulation to be enabled. */
			constexpr auto GraphicsRayTracingGIMultiBounceEnabledKey{"Core/Graphics/RayTracing/GlobalIllumination/MultiBounce/Enabled"};
			constexpr auto DefaultGraphicsRayTracingGIMultiBounceEnabled{true};
			/* Damping of the feedback series: 0 = single bounce, 1 = full geometric series. */
			constexpr auto GraphicsRayTracingGIMultiBounceStrengthKey{"Core/Graphics/RayTracing/GlobalIllumination/MultiBounce/Strength"};
			constexpr auto DefaultGraphicsRayTracingGIMultiBounceStrength{1.0F};
			/* Upper bound on the radiance re-injected per bounce (anti-firefly, divergence guard). */
			constexpr auto GraphicsRayTracingGIMultiBounceClampKey{"Core/Graphics/RayTracing/GlobalIllumination/MultiBounce/Clamp"};
			constexpr auto DefaultGraphicsRayTracingGIMultiBounceClamp{4.0F};

			/* Anti-Aliasing > Temporal (TAA). HDR resolve BEFORE DoF/tone mapping (the only
			 * AA effect not bound by the runsAfterToneMapping contract — the Karis luminance
			 * weighting below is what makes HDR accumulation safe). Requires the velocity
			 * G-buffer and drives the Halton (2,3) projection jitter (requiresJitter). */
			constexpr auto GraphicsTAAEnabledKey{"Core/Graphics/AntiAliasing/Temporal/Enabled"};
			constexpr auto DefaultGraphicsTAAEnabled{false};
			/* Blend weight of the CURRENT frame (0.1 = 90% history: strong AA, slower response). */
			constexpr auto GraphicsTAAAlphaKey{"Core/Graphics/AntiAliasing/Temporal/Alpha"};
			constexpr auto DefaultGraphicsTAAAlpha{0.1F};
			/* Variance clipping gamma: half-size of the YCoCg statistical AABB in standard
			 * deviations (lower = less ghosting, more flicker). */
			constexpr auto GraphicsTAAVarianceGammaKey{"Core/Graphics/AntiAliasing/Temporal/VarianceGamma"};
			constexpr auto DefaultGraphicsTAAVarianceGamma{1.0F};
			/* Karis inverse-luminance blend weighting (HDR anti-firefly / anti-flicker). */
			constexpr auto GraphicsTAALumaWeightingKey{"Core/Graphics/AntiAliasing/Temporal/LumaWeighting"};
			constexpr auto DefaultGraphicsTAALumaWeighting{true};

			/* Motion Blur — effect QUALITY knobs only. The blur LENGTH is photographic and
			 * belongs to the active camera: shutter speed / frame duration = shutter angle
			 * (Scenes::Component::Camera::setShutterSpeed()). Requires the velocity G-buffer;
			 * runs in HDR, after the temporal resolve and before the photographic effects. */
			constexpr auto GraphicsMotionBlurEnabledKey{"Core/Graphics/MotionBlur/Enabled"};
			constexpr auto DefaultGraphicsMotionBlurEnabled{false};
			/* Samples walked along the dominant velocity (odd: one lands on the pixel centre). */
			constexpr auto GraphicsMotionBlurSampleCountKey{"Core/Graphics/MotionBlur/SampleCount"};
			constexpr auto DefaultGraphicsMotionBlurSampleCount{24U};
			/* Depth interval, in meters, softening the foreground/background classification. */
			constexpr auto GraphicsMotionBlurSoftDepthExtentKey{"Core/Graphics/MotionBlur/SoftDepthExtent"};
			constexpr auto DefaultGraphicsMotionBlurSoftDepthExtent{0.05F};

			/* Depth of Field — effect QUALITY knobs only. The optical parameters (aperture,
			 * focal length, focus) belong to the active camera (physical camera model,
			 * Scenes::Component::Camera), NOT to the settings. */
			/* Blur ceiling: maximum gather radius in half-res pixels. A pure performance/quality
			 * clamp — the blur AMOUNT is the thin-lens circle of confusion, converted to pixels
			 * from the sensor fraction (no scale factor). 32 half-res = ~64 full-res pixels of
			 * diameter, past which 48 spiral taps would start to ring. */
			constexpr auto GraphicsDepthOfFieldMaxRadiusKey{"Core/Graphics/DepthOfField/MaxRadius"};
			constexpr auto DefaultGraphicsDepthOfFieldMaxRadius{32.0F};
			/* Golden-spiral gather taps per pixel (bokeh quality). */
			constexpr auto GraphicsDepthOfFieldSampleCountKey{"Core/Graphics/DepthOfField/SampleCount"};
			constexpr auto DefaultGraphicsDepthOfFieldSampleCount{48U};
			/* Auto-focus adaptation speed (rack focus), in 1/seconds. */
			constexpr auto GraphicsDepthOfFieldAutoFocusSpeedKey{"Core/Graphics/DepthOfField/AutoFocusSpeed"};
			constexpr auto DefaultGraphicsDepthOfFieldAutoFocusSpeed{3.0F};
			/* Near-field (foreground) blur with silhouette bleeding. */
			constexpr auto GraphicsDepthOfFieldNearFieldKey{"Core/Graphics/DepthOfField/NearField"};
			constexpr auto DefaultGraphicsDepthOfFieldNearField{true};

			/* Screen Space > Ambient Occlusion (first screen-space effect group — SSGI keys will join it). */
			/* Hemisphere sampling radius, in world units. */
			constexpr auto GraphicsScreenSpaceAORadiusKey{"Core/Graphics/ScreenSpace/AmbientOcclusion/Radius"};
			constexpr auto DefaultGraphicsScreenSpaceAORadius{0.5F};
			/* AO darkening intensity multiplier (applied once, clamped; 1.0 = pure visibility term). */
			constexpr auto GraphicsScreenSpaceAOIntensityKey{"Core/Graphics/ScreenSpace/AmbientOcclusion/Intensity"};
			constexpr auto DefaultGraphicsScreenSpaceAOIntensity{1.0F};
			/* Depth comparison bias to prevent self-occlusion, in view-space units. */
			constexpr auto GraphicsScreenSpaceAOBiasKey{"Core/Graphics/ScreenSpace/AmbientOcclusion/Bias"};
			constexpr auto DefaultGraphicsScreenSpaceAOBias{0.025F};
			/* Samples per pixel for screen-space ambient occlusion. */
			constexpr auto GraphicsScreenSpaceAOSampleCountKey{"Core/Graphics/ScreenSpace/AmbientOcclusion/SampleCount"};
			constexpr auto DefaultGraphicsScreenSpaceAOSampleCount{32U};

			/* Screen Space > Global Illumination */
			/* Maximum GI ray-march distance, in world units. */
			constexpr auto GraphicsScreenSpaceGIMaxDistanceKey{"Core/Graphics/ScreenSpace/GlobalIllumination/MaxDistance"};
			constexpr auto DefaultGraphicsScreenSpaceGIMaxDistance{5.0F};
			/* Indirect lighting intensity multiplier. */
			constexpr auto GraphicsScreenSpaceGIIntensityKey{"Core/Graphics/ScreenSpace/GlobalIllumination/Intensity"};
			constexpr auto DefaultGraphicsScreenSpaceGIIntensity{0.8F};
			/* Depth thickness assumed behind each depth sample, in view-space units. */
			constexpr auto GraphicsScreenSpaceGIThicknessKey{"Core/Graphics/ScreenSpace/GlobalIllumination/Thickness"};
			constexpr auto DefaultGraphicsScreenSpaceGIThickness{0.5F};
			/* Rays per pixel for screen-space global illumination. */
			constexpr auto GraphicsScreenSpaceGISampleCountKey{"Core/Graphics/ScreenSpace/GlobalIllumination/SampleCount"};
			constexpr auto DefaultGraphicsScreenSpaceGISampleCount{8U};
			/* Ray-march steps per ray. */
			constexpr auto GraphicsScreenSpaceGIStepCountKey{"Core/Graphics/ScreenSpace/GlobalIllumination/StepCount"};
			constexpr auto DefaultGraphicsScreenSpaceGIStepCount{16U};
			/* Bilateral denoising blur radius for GI, in pixels. */
			constexpr auto GraphicsScreenSpaceGIBlurRadiusKey{"Core/Graphics/ScreenSpace/GlobalIllumination/BlurRadius"};
			constexpr auto DefaultGraphicsScreenSpaceGIBlurRadius{4U};
			/* Depth edge-stopping sigma for the GI bilateral blur. */
			constexpr auto GraphicsScreenSpaceGIDepthSigmaKey{"Core/Graphics/ScreenSpace/GlobalIllumination/DepthSigma"};
			constexpr auto DefaultGraphicsScreenSpaceGIDepthSigma{1.0F};
			/* Normal edge-stopping sigma for the GI bilateral blur. */
			constexpr auto GraphicsScreenSpaceGINormalSigmaKey{"Core/Graphics/ScreenSpace/GlobalIllumination/NormalSigma"};
			constexpr auto DefaultGraphicsScreenSpaceGINormalSigma{0.5F};

			/* Screen Space > Global Illumination > Temporal (GIDenoiser resolve — SSGI's
			 * first temporal accumulation; mirrors the RayTracing group, same semantics). */
			constexpr auto GraphicsScreenSpaceGITemporalEnabledKey{"Core/Graphics/ScreenSpace/GlobalIllumination/Temporal/Enabled"};
			constexpr auto DefaultGraphicsScreenSpaceGITemporalEnabled{true};
			constexpr auto GraphicsScreenSpaceGITemporalAlphaKey{"Core/Graphics/ScreenSpace/GlobalIllumination/Temporal/Alpha"};
			constexpr auto DefaultGraphicsScreenSpaceGITemporalAlpha{0.1F};
			constexpr auto GraphicsScreenSpaceGITemporalDepthToleranceKey{"Core/Graphics/ScreenSpace/GlobalIllumination/Temporal/DepthTolerance"};
			constexpr auto DefaultGraphicsScreenSpaceGITemporalDepthTolerance{0.05F};
			constexpr auto GraphicsScreenSpaceGITemporalNormalThresholdKey{"Core/Graphics/ScreenSpace/GlobalIllumination/Temporal/NormalThreshold"};
			constexpr auto DefaultGraphicsScreenSpaceGITemporalNormalThreshold{0.8F};
			constexpr auto GraphicsScreenSpaceGITemporalVarianceGammaKey{"Core/Graphics/ScreenSpace/GlobalIllumination/Temporal/VarianceGamma"};
			constexpr auto DefaultGraphicsScreenSpaceGITemporalVarianceGamma{1.0F};
			constexpr auto GraphicsScreenSpaceGITemporalNeighborhoodClampKey{"Core/Graphics/ScreenSpace/GlobalIllumination/Temporal/NeighborhoodClamp"};
			constexpr auto DefaultGraphicsScreenSpaceGITemporalNeighborhoodClamp{false};
			constexpr auto GraphicsScreenSpaceGITemporalAnimatedNoiseKey{"Core/Graphics/ScreenSpace/GlobalIllumination/Temporal/AnimatedNoise"};
			constexpr auto DefaultGraphicsScreenSpaceGITemporalAnimatedNoise{true};

			/* Screen Space > Global Illumination > Denoiser (shared GIDenoiser component —
			 * mirrors the RayTracing group, same semantics and defaults). */
			constexpr auto GraphicsScreenSpaceGIDenoiserIterationsKey{"Core/Graphics/ScreenSpace/GlobalIllumination/Denoiser/Iterations"};
			constexpr auto DefaultGraphicsScreenSpaceGIDenoiserIterations{4U};
			constexpr auto GraphicsScreenSpaceGIDenoiserLuminanceSigmaKey{"Core/Graphics/ScreenSpace/GlobalIllumination/Denoiser/LuminanceSigma"};
			constexpr auto DefaultGraphicsScreenSpaceGIDenoiserLuminanceSigma{4.0F};
			constexpr auto GraphicsScreenSpaceGIDenoiserAccumulationCounterKey{"Core/Graphics/ScreenSpace/GlobalIllumination/Denoiser/AccumulationCounter"};
			constexpr auto DefaultGraphicsScreenSpaceGIDenoiserAccumulationCounter{true};
			constexpr auto GraphicsScreenSpaceGIDenoiserMaxAccumulationKey{"Core/Graphics/ScreenSpace/GlobalIllumination/Denoiser/MaxAccumulation"};
			constexpr auto DefaultGraphicsScreenSpaceGIDenoiserMaxAccumulation{64U};
			constexpr auto GraphicsScreenSpaceGIDenoiserDebugViewKey{"Core/Graphics/ScreenSpace/GlobalIllumination/Denoiser/DebugView"};
			constexpr auto DefaultGraphicsScreenSpaceGIDenoiserDebugView{0U};

			/* Screen-space reflections (SSR). */
			/* Compute reflections at half resolution (pixel doubling) to save performance.
			 * Default FALSE (owner decision): screen-space effects run full-res by default —
			 * they are the cheap tier of the reflection ladder, quality is their selling point. */
			constexpr auto GraphicsScreenSpaceReflectionPixelDoublingKey{"Core/Graphics/ScreenSpace/Reflection/PixelDoubling"};
			constexpr auto DefaultGraphicsScreenSpaceReflectionPixelDoubling{false};
			/* Bilateral blur radius, in pixels — scaled per-pixel by the surface roughness
			 * (a polished surface keeps a mirror-sharp reflection). */
			constexpr auto GraphicsScreenSpaceReflectionBlurRadiusKey{"Core/Graphics/ScreenSpace/Reflection/BlurRadius"};
			constexpr auto DefaultGraphicsScreenSpaceReflectionBlurRadius{2U};
			/* Depth edge-stopping sigma for the reflection bilateral blur. */
			constexpr auto GraphicsScreenSpaceReflectionDepthSigmaKey{"Core/Graphics/ScreenSpace/Reflection/DepthSigma"};
			constexpr auto DefaultGraphicsScreenSpaceReflectionDepthSigma{0.5F};
			/* Normal edge-stopping sigma for the reflection bilateral blur. */
			constexpr auto GraphicsScreenSpaceReflectionNormalSigmaKey{"Core/Graphics/ScreenSpace/Reflection/NormalSigma"};
			constexpr auto DefaultGraphicsScreenSpaceReflectionNormalSigma{0.3F};

			/* Level of Detail */
			/* Automatically generate levels of detail for meshes. */
			constexpr auto GraphicsLODEnableAutomaticGenerationKey{"Core/Graphics/LOD/EnableAutomaticGeneration"};
			constexpr auto DefaultGraphicsLODEnableAutomaticGeneration{false};
			/* Meshes below this triangle count are not simplified. */
			constexpr auto GraphicsLODMinTriangleCountKey{"Core/Graphics/LOD/MinTriangleCount"};
			constexpr auto DefaultGraphicsLODMinTriangleCount{250U};
			/* Screen coverage ratio [0..1] that triggers an LOD switch. */
			constexpr auto GraphicsLODScreenCoverageThresholdKey{"Core/Graphics/LOD/ScreenCoverageThreshold"};
			constexpr auto DefaultGraphicsLODScreenCoverageThreshold{0.75F};
			/* Triangle reduction ratio per LOD step. */
			constexpr auto GraphicsLODReductionRatioKey{"Core/Graphics/LOD/ReductionRatio"};
			constexpr auto DefaultGraphicsLODReductionRatio{0.33F};

			/* Multi-Draw Indirect */
			/* Use multi-draw indirect batching for rendering. */
			constexpr auto GraphicsMDIEnabledKey{"Core/Graphics/MDI/Enabled"};
			constexpr auto DefaultGraphicsMDIEnabled{false};

			/* Shadow Mapping */
			/* Master switch for shadow mapping. */
			constexpr auto GraphicsShadowMappingEnabledKey{"Core/Graphics/ShadowMapping/Enabled"};
			constexpr auto DefaultGraphicsShadowMappingEnabled{true};
			/* Apply percentage-closer filtering (PCF) to soften shadow edges. */
			constexpr auto GraphicsShadowMappingEnablePCFKey{"Core/Graphics/ShadowMapping/EnablePCF"};
			constexpr auto DefaultGraphicsShadowMappingEnablePCF{false};
			/* PCF sample count. */
			constexpr auto GraphicsShadowMappingPCFSamplesKey{"Core/Graphics/ShadowMapping/PCFSamples"};
			constexpr auto DefaultGraphicsShadowMappingPCFSamples{2U};
			/* PCF filtering method. Values: "Performance" (Grid, max FPS), "Balanced" (VogelDisk, recommended), "Quality" (PoissonDisk), "Ultra" (OptimizedGather, best). */
			constexpr auto GraphicsShadowMappingPCFMethodKey{"Core/Graphics/ShadowMapping/PCFMethod"};
			constexpr auto DefaultGraphicsShadowMappingPCFMethod{"Balanced"};
			/* Max distance at which shadows are rendered (default ~5 km). */
			constexpr auto GraphicsShadowMappingViewDistanceKey{"Core/Graphics/ShadowMapping/ViewDistance"};
			constexpr auto DefaultGraphicsShadowMappingViewDistance{5000.0F}; /* NOTE: 5km */

			/* Shader */
			/* Log generated shader source code. */
			constexpr auto ShowSourceCodeKey{"Core/Graphics/Shader/ShowSourceCode"};
			constexpr auto DefaultShowSourceCode{false};
			/* Write every GENERATED GLSL source to disk for inspection. This is a DUMP, not a
			 * cache: nothing ever reads it back, and its key is a hash of the very source it
			 * stores, so it structurally could not serve as one. Named EnableSourceCodeCache
			 * until Aug 2026 -- the old key is simply ignored, there is no migration. */
			constexpr auto SourceCodeDumpEnabledKey{"Core/Graphics/Shader/EnableSourceCodeDump"};
			constexpr auto DefaultSourceCodeDumpEnabled{false};
			/* Cache compiled SPIR-V binaries on disk. */
			/* Persist the DRIVER's pipeline cache across runs. This is the one that matters:
			 * measured on material-debug, the driver-side pipeline compilation costs 5.4 s with a
			 * cold driver cache against 33 ms with a warm one, for 294 pipelines. */
			constexpr auto PipelineCacheEnabledKey{"Core/Graphics/Shader/EnablePipelineCache"};
			constexpr auto DefaultPipelineCacheEnabled{true};
			constexpr auto BinaryCacheEnabledKey{"Core/Graphics/Shader/EnableBinaryCache"};
			constexpr auto DefaultBinaryCacheEnabled{true};

		/* RushMaker (in-engine screencast / video recorder) */
		/* Enable video / audio capture in RushMaker (shared default). */
		constexpr auto RushMakerEnableVideoKey{"Core/RushMaker/EnableVideo"};
		constexpr auto RushMakerEnableAudioKey{"Core/RushMaker/EnableAudio"};
		constexpr auto DefaultRushMakerEnabled{false};
		/* RushMaker capture frame rate in FPS. */
		constexpr auto RushMakerVideoFramerateKey{"Core/RushMaker/VideoFramerate"};
		constexpr auto DefaultRushMakerVideoFramerate{30U};
		/* Encoding quality preset. Values: "Low", "Medium", "High", "Ultra". */
		constexpr auto RushMakerQualityPresetKey{"Core/RushMaker/QualityPreset"};
		constexpr auto DefaultRushMakerQualityPreset{"Medium"};
		/* Log RushMaker activity. */
		constexpr auto RushMakerShowInformationKey{"Core/RushMaker/ShowInformation"};
		constexpr auto DefaultRushMakerShowInformation{false};
		/* Force the software VP9 encoder even when the device supports hardware
		 * H.265 (Vulkan Video). For A/B comparison of the two paths, and to produce
		 * royalty-free WebM/VP9 on demand. */
		constexpr auto RushMakerForceCPUEncodingKey{"Core/RushMaker/ForceCPUEncoding"};
		constexpr auto DefaultRushMakerForceCPUEncoding{false};
		/* Grab buffer depth: frames buffered between the 30 Hz capture (the only
		 * realtime element) and the encoder thread, which encodes at its own pace
		 * (quality path) and drains after the recording stops. Above this depth
		 * captures are skipped (they become duplicated frames in the constant frame
		 * rate output) so a slow encode cannot balloon RAM — one buffered frame costs
		 * width x height x 4 bytes (default 90 = 3 s at 30 FPS, ~1.6 GB at 2880x1620).
		 * Raise it for short takes when RAM allows: zero skip, the encoder finishes
		 * in background. */
		constexpr auto RushMakerMaxQueuedFramesKey{"Core/RushMaker/MaxQueuedFrames"};
		constexpr auto DefaultRushMakerMaxQueuedFrames{90U};
		/* Capture a microphone voice-over track. */
		constexpr auto RushMakerEnableVoiceOverKey{"Core/RushMaker/EnableVoiceOver"};
		constexpr auto DefaultRushMakerEnableVoiceOver{false};

		/* Physics */
		/* Enable the spatial acceleration structure for physics. */
		constexpr auto EnablePhysicsAccelerationKey{"Core/Physics/EnableAcceleration"};
		constexpr auto DefaultEnablePhysicsAcceleration{false};

		/* User */
		/* Local user account id. */
		constexpr auto UserAccountIDKey{"Core/User/ID"};
		constexpr auto DefaultUserAccountID{0};
		/* Local user account display name. */
		constexpr auto UserAccountNameKey{"Core/User/AccountName"};
		constexpr auto DefaultUserAccountName{"John.Doe"};

		/* External libs control */
		/* hwloc library verbosity. Values: "0" (all), "1" (no ENOSYS), "2" (none). */
		constexpr auto HWLOCVerbosityKey{"Core/HWLOC/Verbosity"};
		constexpr auto DefaultHWLOCVerbosityKey{"2"};

		/* Cross-platform specific control */
		/* NOTE: false = modern COM file dialogs (IFileOpenDialog/IFileSaveDialog); true = legacy
		 * Win32 (GetOpenFileNameW/GetSaveFileNameW). The Win32 path is an accessibility
		 * compatibility fallback for Windows 11 cases where the COM dialog misbehaves with
		 * assistive tools - not dead code. See PlatformSpecific/AGENTS.md. */
		constexpr auto CompatibilityWindowsUseLegacyFileDialogsKey{"Core/Compatibility/Windows/UseLegacyFileDialogs"};
		constexpr auto DefaultCompatibilityWindowsUseLegacyFileDialogs{false};
		/*constexpr auto CompatibilityLinuxSampleKey{"Core/Compatibility/Linux/XXX"}; // Linux example
		constexpr auto DefaultCompatibilityLinuxSample{false};*/
		/*constexpr auto CompatibilityMacOSSampleKey{"Core/Compatibility/macOS/XXX"}; // macOS example
		constexpr auto DefaultCompatibilityMacOSSample{false};*/
}
