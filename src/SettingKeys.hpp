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
		constexpr auto GraphicsFieldOfViewKey{"Core/Graphics/FieldOfView"};
		constexpr auto DefaultGraphicsFieldOfView{85.0F}; /* NOTE: 85° */

			/* Texture */
			/* Magnification / minification / mipmap filtering (shared default). Values: "nearest", "linear". */
			constexpr auto GraphicsTextureMagFilteringKey{"Core/Graphics/Texture/MagFilter"};
			constexpr auto GraphicsTextureMinFilteringKey{"Core/Graphics/Texture/MinFilter"};
			constexpr auto GraphicsTextureMipFilteringKey{"Core/Graphics/Texture/MipFilter"};
			constexpr auto DefaultGraphicsTextureFiltering{"nearest"};
			/* Number of mipmap levels to generate. */
			constexpr auto GraphicsTextureMipMappingLevelsKey{"Core/Graphics/Texture/MipMappingLevels"};
			constexpr auto DefaultGraphicsTextureMipMappingLevels{1};
			/* Anisotropic filtering level (0 = off). */
			constexpr auto GraphicsTextureAnisotropyLevelsKey{"Core/Graphics/Texture/AnisotropyLevels"};
			constexpr auto DefaultGraphicsTextureAnisotropy{0};
			/* Distance up to which full-resolution textures are used (default ~5 km). */
			constexpr auto GraphicsTextureViewDistanceKey{"Core/Graphics/Texture/ViewDistance"};
			constexpr auto DefaultGraphicsTextureViewDistance{5000.0F}; /* NOTE: 5km */

			/* Ray Tracing */
			/* Master switch for hardware ray tracing. */
			constexpr auto GraphicsRayTracingEnabledKey{"Core/Graphics/RayTracing/Enabled"};
			constexpr auto DefaultGraphicsRayTracingEnabled{false};
			/* Ray-traced reflections. */
			constexpr auto GraphicsRayTracingEnableReflectionKey{"Core/Graphics/RayTracing/EnableReflection"};
			constexpr auto DefaultGraphicsRayTracingEnableReflection{true};
			/* Ray-traced ambient occlusion. */
			constexpr auto GraphicsRayTracingEnableAmbientOcclusionKey{"Core/Graphics/RayTracing/EnableAmbientOcclusion"};
			constexpr auto DefaultGraphicsRayTracingEnableAmbientOcclusion{true};
			/* Samples per pixel for ray-traced ambient occlusion. */
			constexpr auto GraphicsRayTracingAOSampleCountKey{"Core/Graphics/RayTracing/AOSampleCount"};
			constexpr auto DefaultGraphicsRayTracingAOSampleCount{4U};
			/* Ray-traced global illumination. */
			constexpr auto GraphicsRayTracingEnableGlobalIlluminationKey{"Core/Graphics/RayTracing/EnableGlobalIllumination"};
			constexpr auto DefaultGraphicsRayTracingEnableGlobalIllumination{true};
			/* Samples per pixel for ray-traced global illumination. */
			constexpr auto GraphicsRayTracingGISampleCountKey{"Core/Graphics/RayTracing/GISampleCount"};
			constexpr auto DefaultGraphicsRayTracingGISampleCount{16U};
			/* Compute GI at half resolution (pixel doubling) to save performance. */
			constexpr auto GraphicsRayTracingGIPixelDoublingKey{"Core/Graphics/RayTracing/GIPixelDoubling"};
			constexpr auto DefaultGraphicsRayTracingGIPixelDoubling{true};
			/* Ray-traced contact (short-range) shadows. */
			constexpr auto GraphicsRayTracingEnableContactShadowsKey{"Core/Graphics/RayTracing/EnableContactShadows"};
			constexpr auto DefaultGraphicsRayTracingEnableContactShadows{true};
			/* Max distance for the top-level acceleration structure, in world units. */
			constexpr auto GraphicsRayTracingTLASDistanceKey{"Core/Graphics/RayTracing/TLASDistance"};
			constexpr auto DefaultGraphicsRayTracingTLASDistance{1000.0F};

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
			/* Cache generated shader source on disk. */
			constexpr auto SourceCodeCacheEnabledKey{"Core/Graphics/Shader/EnableSourceCodeCache"};
			constexpr auto DefaultSourceCodeCacheEnabled{false};
			/* Cache compiled SPIR-V binaries on disk. */
			constexpr auto BinaryCacheEnabledKey{"Core/Graphics/Shader/EnableBinaryCache"};
			constexpr auto DefaultBinaryCacheEnabled{false};
			/* Generate higher-quality (more expensive) shader variants. */
			constexpr auto EnableHighQualityKey{"Core/Graphics/Shader/EnableHighQuality"};
			constexpr auto DefaultEnableHighQuality{false};
			/* Parallax occlusion mapping ray-march iteration count. */
			constexpr auto POMIterationsKey{"Core/Graphics/Shader/POMIterations"};
			constexpr auto DefaultPOMIterations{16};

		/* RushMaker (in-engine screencast / video recorder) */
		/* Enable video / audio capture in RushMaker (shared default). */
		constexpr auto RushMakerEnableVideoKey{"Core/RushMaker/EnableVideo"};
		constexpr auto RushMakerEnableAudioKey{"Core/RushMaker/EnableAudio"};
		constexpr auto DefaultRushMakerEnabled{false};
		/* RushMaker capture frame rate in FPS. */
		constexpr auto RushMakerVideoFramerateKey{"Core/RushMaker/VideoFramerate"};
		constexpr auto DefaultRushMakerVideoFramerate{30U};
		/* Capture in real time (vs. offline deterministic rendering). */
		constexpr auto RushMakerRealtimeModeKey{"Core/RushMaker/RealtimeMode"};
		constexpr auto DefaultRushMakerRealtimeMode{true};
		/* Encoding quality preset. Values: "Low", "Medium", "High", "Ultra". */
		constexpr auto RushMakerQualityPresetKey{"Core/RushMaker/QualityPreset"};
		constexpr auto DefaultRushMakerQualityPreset{"Medium"};
		/* Log RushMaker activity. */
		constexpr auto RushMakerShowInformationKey{"Core/RushMaker/ShowInformation"};
		constexpr auto DefaultRushMakerShowInformation{false};
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
