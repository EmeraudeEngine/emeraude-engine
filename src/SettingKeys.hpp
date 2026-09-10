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

		/* Net */
		/* Whether Net::Manager may download files (https only) for ExternalData resources. */
		constexpr auto NetDownloadEnabledKey{"Core/Net/DownloadEnabled"};
		constexpr auto DefaultNetDownloadEnabled{true};
		/* Ceiling of the download cache, in bytes. The least recently used files are dropped past
		 * it, except those a completed ticket still points at. 0 disables eviction. */
		constexpr auto NetCacheMaxBytesKey{"Core/Net/CacheMaxBytes"};
		constexpr auto DefaultNetCacheMaxBytes{static_cast< uint64_t >(2ULL * 1024 * 1024 * 1024)};
		/* Total budget of one download, in seconds, including every redirect hop. */
		constexpr auto NetDownloadTimeoutKey{"Core/Net/DownloadTimeoutSeconds"};
		constexpr auto DefaultNetDownloadTimeout{120U};
		/* Optional PEM bundle added to the system trust store (corporate / private CA). Empty = none.
		 * Shared by Net::Manager and Net::APIClient: a corporate CA is a property of the machine. */
		constexpr auto NetCABundleFileKey{"Core/Net/CABundleFile"};
		constexpr auto DefaultNetCABundleFile{""};

		/* Net / API (Net::APIClient — web APIs, distinct from the download manager) */
		/* Whether Net::APIClient may perform API calls (https only). */
		constexpr auto NetAPIEnabledKey{"Core/Net/API/Enabled"};
		constexpr auto DefaultNetAPIEnabled{true};
		/* Total budget of one API call, in seconds, including every redirect hop. Much shorter than
		 * a download's: an API that has not answered in half a minute is not going to. */
		constexpr auto NetAPITimeoutKey{"Core/Net/API/TimeoutSeconds"};
		constexpr auto DefaultNetAPITimeout{30U};
		/* How many terminal tickets are kept before the OLDEST are dropped. ⚠️ Unlike a download,
		 * an API response is held in memory, so a caller that never calls release() would grow the
		 * process without bound. 0 disables the ceiling — only for a caller that always releases. */
		constexpr auto NetAPIMaxRetainedTicketsKey{"Core/Net/API/MaxRetainedTickets"};
		constexpr auto DefaultNetAPIMaxRetainedTickets{64U};
		/* Ceiling of ONE response body, in bytes. It is held whole in memory: without this, a
		 * hostile or runaway endpoint grows the process until the OS kills it. */
		constexpr auto NetAPIMaxResponseBytesKey{"Core/Net/API/MaxResponseBytes"};
		constexpr auto DefaultNetAPIMaxResponseBytes{static_cast< uint64_t >(16ULL * 1024 * 1024)};

		/* Console */
		/* Whether the remote console (TCP, AI runtime control) is started at all. Default FALSE:
		 * the listener is an unauthenticated command channel (quit, settings, scene, screenshots),
		 * so a shipped application must not open it unless the operator asks for it. */
		constexpr auto ConsoleEnableRemoteListenerKey{"Core/Console/EnableRemoteListener"};
		constexpr auto DefaultConsoleEnableRemoteListener{false};
		/* Address the remote console binds to. Default loopback: only processes of the same host
		 * can connect. Set "0.0.0.0" (or a NIC address, or "::" for IPv6) to drive the application
		 * from another machine. An unparsable value falls back to loopback, never to any-address. */
		constexpr auto ConsoleRemoteListenerAddressKey{"Core/Console/RemoteListenerAddress"};
		constexpr auto DefaultConsoleRemoteListenerAddress{"127.0.0.1"};
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
			/* Measurement only: dump per-surface GPU upload statistics to the tracer once per second.
			 * Answers "how much of the pixmap does a paint actually touch?" - see
			 * EmEn::Overlay::Surface::UploadStatistics. Off by default: it is a diagnostic, not a feature. */
			constexpr auto OverlayUploadStatisticsKey{"Core/Video/Overlay/EnableUploadStatistics"};
			constexpr auto DefaultOverlayUploadStatistics{false};

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
			/* Persist BC7-compressed mip chains on disk, so a texture is encoded once and not at
			 * every launch. Measured on material-debug: 231 mip levels cost 7705 ms of CPU BC7 on a
			 * cold cache against 0 ms warm, which makes it the most profitable of the three on-disk
			 * renderer caches. Turn it OFF to measure encoding cost, or to force a re-encode without
			 * wiping the directory. */
			constexpr auto GraphicsTextureCacheEnabledKey{"Core/Graphics/Texture/EnableTextureCache"};
			constexpr auto DefaultGraphicsTextureCacheEnabled{true};

			/* GPU Profiler (Vulkan timestamp queries).
			 * Per-pass GPU timings of the main frame command buffer, harvested without
			 * stall (one query pool per frame in flight) and served by the remote console
			 * command Core.RendererService.getGPUTimings(). Zero cost when disabled. */
			constexpr auto GraphicsGPUProfilerEnabledKey{"Core/Graphics/GPUProfiler/Enabled"};
			constexpr auto DefaultGraphicsGPUProfilerEnabled{false};

			/* Ray Tracing — everything about ray tracing that is NOT a post-process effect:
			 * the acceleration structures themselves and their geometry. The effects that CONSUME
			 * them live under Core/Graphics/PostProcessing/, by concept then by lane.
			 * ⚠️ There is NO master switch here any more (`Enabled`, removed 2026-09-10, owner
			 * decision). Two reasons, and the second is the one that settles it:
			 *  - It said the same thing as Core/Graphics/PostProcessing/LightingLane while
			 *    silently DOMINATING it — a settings file could read "RayTracing" and render in
			 *    screen space with nothing to explain why (owner report, same day).
			 *  - It cannot coexist with the runtime lane switch at all. Selecting the ray-traced
			 *    lane at runtime needs a BLAS on every geometry that is ALREADY LOADED, and a
			 *    geometry cannot gain one after the fact. So the acceleration structures must
			 *    exist whenever the device can build them; whether anything READS them is what
			 *    the post-processing tree decides.
			 * Consequently `Vulkan::Device::rayTracingEnabled()` — pure hardware detection — is
			 * now the single answer to "can this machine trace?". */
			/* Max distance for the top-level acceleration structure, in world units. */
			constexpr auto GraphicsRayTracingTLASDistanceKey{"Core/Graphics/RayTracing/TLASDistance"};
			constexpr auto DefaultGraphicsRayTracingTLASDistance{1000.0F};

			/* Ray Tracing > Reflection */
			/* Compute reflections at half resolution (pixel doubling) to save performance. */
			constexpr auto GraphicsPPReflectionsRTPixelDoublingKey{"Core/Graphics/PostProcessing/Reflections/RayTracing/PixelDoubling"};
			constexpr auto DefaultGraphicsPPReflectionsRTPixelDoubling{true};
			/* Ray Tracing > Reflection > Glossy cone (pre-convolved reflection pyramid lookup).
			 * v2 (Aug 2026): the cone width is computed PER PIXEL by the trace from the hit distance,
			 * the roughness (GGX alpha = roughness²) and the camera distance — a contact reflection
			 * stays sharp, a distant one tends to the lobe's angular size — and written to a width map
			 * the combine reads. The v1 cone was uniform in screen space (an assumed hit fraction of the
			 * screen height): measured on the projet-alpha post-processor-effect-debug bench, it was
			 * 2-3x too sharp for hits 6-16 m away and blurred contacts that must stay sharp. A flat
			 * reflector is still assumed (a sphere compresses its reflected image). */
			/* Master switch for the cone lookup: false = the pyramid is never read, the composite
			 * shows the RAW traced reflection at its full trace resolution (sharpness reference). */
			constexpr auto GraphicsPPReflectionsRTGlossyConeEnabledKey{"Core/Graphics/PostProcessing/Reflections/RayTracing/GlossyCone/Enabled"};
			constexpr auto DefaultGraphicsPPReflectionsRTGlossyConeEnabled{true};
			/* Cone width (in trace texels) below which the reflection stays PURELY the sharp
			 * traced buffer: under one texel of spread there is nothing to convolve. */
			constexpr auto GraphicsPPReflectionsRTGlossyConeBlendStartKey{"Core/Graphics/PostProcessing/Reflections/RayTracing/GlossyCone/BlendStartTexels"};
			constexpr auto DefaultGraphicsPPReflectionsRTGlossyConeBlendStart{2.0F};
			/* Cone width (in trace texels) at which the reflection comes ENTIRELY from the
			 * pyramid gather. Between start and full the two are cross-faded linearly, so a
			 * near-mirror keeps its full-resolution traced reflection. 24 until Aug 2026: with the
			 * per-pixel cone a roughness-0.1 metal asks for a 7-texel kernel and the fade kept 78 %
			 * of the sharp trace there (kernel σ 2.4 px measured against 6 expected); 6 hands the
			 * gather over as soon as it has a real kernel to apply. */
			constexpr auto GraphicsPPReflectionsRTGlossyConeBlendFullKey{"Core/Graphics/PostProcessing/Reflections/RayTracing/GlossyCone/BlendFullTexels"};
			constexpr auto DefaultGraphicsPPReflectionsRTGlossyConeBlendFull{6.0F};
			/* Hard ceiling on the pyramid LOD the cone may reach, on top of the mip count: caps
			 * how coarse a rough surface is allowed to get (each LOD halves the resolution). */
			constexpr auto GraphicsPPReflectionsRTGlossyConeMaxLodKey{"Core/Graphics/PostProcessing/Reflections/RayTracing/GlossyCone/MaxLod"};
			constexpr auto DefaultGraphicsPPReflectionsRTGlossyConeMaxLod{8.0F};

			/* Ray Tracing > Ambient Occlusion */
			/* Samples per pixel for ray-traced ambient occlusion. */
			constexpr auto GraphicsPPAmbientOcclusionRTSampleCountKey{"Core/Graphics/PostProcessing/AmbientOcclusion/RayTracing/SampleCount"};
			constexpr auto DefaultGraphicsPPAmbientOcclusionRTSampleCount{8U};
			/* Compute ambient occlusion at half resolution (pixel doubling) to save performance. */
			constexpr auto GraphicsPPAmbientOcclusionRTPixelDoublingKey{"Core/Graphics/PostProcessing/AmbientOcclusion/RayTracing/PixelDoubling"};
			constexpr auto DefaultGraphicsPPAmbientOcclusionRTPixelDoubling{true};
			/* AO darkening intensity multiplier (applied once, clamped; 1.0 = pure visibility term). */
			constexpr auto GraphicsPPAmbientOcclusionRTIntensityKey{"Core/Graphics/PostProcessing/AmbientOcclusion/RayTracing/Intensity"};
			constexpr auto DefaultGraphicsPPAmbientOcclusionRTIntensity{1.0F};
			/* AO ray origin offset to prevent self-intersection, in world units. */
			constexpr auto GraphicsPPAmbientOcclusionRTBiasKey{"Core/Graphics/PostProcessing/AmbientOcclusion/RayTracing/Bias"};
			constexpr auto DefaultGraphicsPPAmbientOcclusionRTBias{0.005F};
			/* Maximum AO occluder search distance, in world units (near-field effect). */
			constexpr auto GraphicsPPAmbientOcclusionRTMaxDistanceKey{"Core/Graphics/PostProcessing/AmbientOcclusion/RayTracing/MaxDistance"};
			constexpr auto DefaultGraphicsPPAmbientOcclusionRTMaxDistance{2.0F};
			/* Bilateral denoising blur radius for AO, in pixels. */
			constexpr auto GraphicsPPAmbientOcclusionRTBlurRadiusKey{"Core/Graphics/PostProcessing/AmbientOcclusion/RayTracing/BlurRadius"};
			constexpr auto DefaultGraphicsPPAmbientOcclusionRTBlurRadius{4U};
			/* Normal edge-stopping sigma for the AO bilateral blur. */
			constexpr auto GraphicsPPAmbientOcclusionRTNormalSigmaKey{"Core/Graphics/PostProcessing/AmbientOcclusion/RayTracing/NormalSigma"};
			constexpr auto DefaultGraphicsPPAmbientOcclusionRTNormalSigma{0.5F};

			/* Ray Tracing > Global Illumination */
			/* Samples per pixel for ray-traced global illumination.
			 * Measured 2026-07-05 (Sponza+extras, RTX 3070 Ti @ 3840x1990): 8 spp is
			 * visually equivalent to 16 after the bilateral blur and ~16 ms/frame cheaper. */
			constexpr auto GraphicsPPIndirectDiffuseRTSampleCountKey{"Core/Graphics/PostProcessing/IndirectDiffuse/RayTracing/SampleCount"};
			constexpr auto DefaultGraphicsPPIndirectDiffuseRTSampleCount{8U};
			/* Compute GI at half resolution (pixel doubling) to save performance. */
			constexpr auto GraphicsPPIndirectDiffuseRTPixelDoublingKey{"Core/Graphics/PostProcessing/IndirectDiffuse/RayTracing/PixelDoubling"};
			constexpr auto DefaultGraphicsPPIndirectDiffuseRTPixelDoubling{true};
			/* Maximum GI bounce ray distance, in world units. */
			constexpr auto GraphicsPPIndirectDiffuseRTMaxDistanceKey{"Core/Graphics/PostProcessing/IndirectDiffuse/RayTracing/MaxDistance"};
			constexpr auto DefaultGraphicsPPIndirectDiffuseRTMaxDistance{8.0F};
			/* Indirect lighting intensity multiplier. */
			constexpr auto GraphicsPPIndirectDiffuseRTIntensityKey{"Core/Graphics/PostProcessing/IndirectDiffuse/RayTracing/Intensity"};
			constexpr auto DefaultGraphicsPPIndirectDiffuseRTIntensity{0.8F};
			/* GI ray origin offset to prevent self-intersection, in world units. */
			constexpr auto GraphicsPPIndirectDiffuseRTBiasKey{"Core/Graphics/PostProcessing/IndirectDiffuse/RayTracing/Bias"};
			constexpr auto DefaultGraphicsPPIndirectDiffuseRTBias{0.02F};
			/* Bilateral denoising blur radius for GI, in pixels. */
			constexpr auto GraphicsPPIndirectDiffuseRTBlurRadiusKey{"Core/Graphics/PostProcessing/IndirectDiffuse/RayTracing/BlurRadius"};
			constexpr auto DefaultGraphicsPPIndirectDiffuseRTBlurRadius{4U};
			/* Depth edge-stopping sigma for the GI bilateral blur. */
			constexpr auto GraphicsPPIndirectDiffuseRTDepthSigmaKey{"Core/Graphics/PostProcessing/IndirectDiffuse/RayTracing/DepthSigma"};
			constexpr auto DefaultGraphicsPPIndirectDiffuseRTDepthSigma{1.0F};
			/* Normal edge-stopping sigma for the GI bilateral blur. */
			constexpr auto GraphicsPPIndirectDiffuseRTNormalSigmaKey{"Core/Graphics/PostProcessing/IndirectDiffuse/RayTracing/NormalSigma"};
			constexpr auto DefaultGraphicsPPIndirectDiffuseRTNormalSigma{0.5F};

			/* Ray Tracing > Global Illumination > Temporal accumulation.
			 * Exponential moving average over reprojected history: effective sample count
			 * becomes SampleCount / Alpha (8 spp @ 0.1 ≈ 80 effective samples). */
			constexpr auto GraphicsPPIndirectDiffuseRTTemporalEnabledKey{"Core/Graphics/PostProcessing/IndirectDiffuse/RayTracing/Temporal/Enabled"};
			constexpr auto DefaultGraphicsPPIndirectDiffuseRTTemporalEnabled{true};
			/* Blend weight of the CURRENT frame (lower = smoother, more history lag). */
			constexpr auto GraphicsPPIndirectDiffuseRTTemporalAlphaKey{"Core/Graphics/PostProcessing/IndirectDiffuse/RayTracing/Temporal/Alpha"};
			constexpr auto DefaultGraphicsPPIndirectDiffuseRTTemporalAlpha{0.1F};
			/* Relative camera-distance tolerance for history rejection (disocclusion test). */
			constexpr auto GraphicsPPIndirectDiffuseRTTemporalDepthToleranceKey{"Core/Graphics/PostProcessing/IndirectDiffuse/RayTracing/Temporal/DepthTolerance"};
			constexpr auto DefaultGraphicsPPIndirectDiffuseRTTemporalDepthTolerance{0.05F};
			/* Minimum cosine between current and history normals to accept history. */
			constexpr auto GraphicsPPIndirectDiffuseRTTemporalNormalThresholdKey{"Core/Graphics/PostProcessing/IndirectDiffuse/RayTracing/Temporal/NormalThreshold"};
			constexpr auto DefaultGraphicsPPIndirectDiffuseRTTemporalNormalThreshold{0.8F};
			/* Rectify the reprojected history against the current 3x3 neighbourhood statistics
			 * (anti-ghosting). Since 2026-08: VARIANCE CLIPPING (mean ± gamma * sigma, Salvi
			 * GDC 2016 — same technique as the TAA), no longer a min/max clamp.
			 * ⚠ DEFAULT OFF since the SVGF reorder (owner decision, measured 2026-08-06): the
			 * temporal resolve now integrates the RAW trace, and clipping against the raw 3x3
			 * statistics pulls the history toward the noisy local distribution — about 5% of
			 * GI energy lost on the Sponza corridor bench, no peak-to-peak gain. SVGF relies
			 * on the double disocclusion validation alone; the key remains for A/B. */
			constexpr auto GraphicsPPIndirectDiffuseRTTemporalNeighborhoodClampKey{"Core/Graphics/PostProcessing/IndirectDiffuse/RayTracing/Temporal/NeighborhoodClamp"};
			constexpr auto DefaultGraphicsPPIndirectDiffuseRTTemporalNeighborhoodClamp{false};
			/* Width of the variance-clipping bound, in standard deviations (gamma). Smaller =
			 * tighter anti-ghosting but slower convergence; larger = smoother accumulation. */
			constexpr auto GraphicsPPIndirectDiffuseRTTemporalVarianceGammaKey{"Core/Graphics/PostProcessing/IndirectDiffuse/RayTracing/Temporal/VarianceGamma"};
			constexpr auto DefaultGraphicsPPIndirectDiffuseRTTemporalVarianceGamma{1.0F};
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
			constexpr auto GraphicsPPIndirectDiffuseRTTemporalAnimatedNoiseKey{"Core/Graphics/PostProcessing/IndirectDiffuse/RayTracing/Temporal/AnimatedNoise"};
			constexpr auto DefaultGraphicsPPIndirectDiffuseRTTemporalAnimatedNoise{true};

			/* Ray Tracing > Global Illumination > Denoiser (shared GIDenoiser component, SVGF).
			 * À-trous iterations over the temporally integrated irradiance (5x5 kernel,
			 * footprint doubles each pass: 1, 2, 4, 8, 16 texels). 0 disables the spatial
			 * filter entirely (temporal resolve only — the A/B lever). Replaces the former
			 * shared bilateral blur H/V (the BlurRadius key is inert for RTGI since then). */
			constexpr auto GraphicsPPIndirectDiffuseRTDenoiserIterationsKey{"Core/Graphics/PostProcessing/IndirectDiffuse/RayTracing/Denoiser/Iterations"};
			constexpr auto DefaultGraphicsPPIndirectDiffuseRTDenoiserIterations{4U};
			/* Luminance edge-stopping sigma, normalised by the LOCAL standard deviation
			 * (SVGF auto-dosage: noisy → smooth hard, converged → preserve detail).
			 * Larger = closer to a plain depth/normal bilateral (guidance off). */
			constexpr auto GraphicsPPIndirectDiffuseRTDenoiserLuminanceSigmaKey{"Core/Graphics/PostProcessing/IndirectDiffuse/RayTracing/Denoiser/LuminanceSigma"};
			constexpr auto DefaultGraphicsPPIndirectDiffuseRTDenoiserLuminanceSigma{4.0F};
			/* Per-pixel 1/N accumulation counter (SVGF): the temporal blend weight is
			 * max(1/(age+1), 1/MaxAccumulation) instead of the fixed Temporal/Alpha — fast
			 * convergence after a disocclusion (alpha 1, 1/2, 1/3...), tiny steady-state
			 * variance leak (about 0.8% at N=64 versus about 23% at fixed alpha 0.1, the
			 * factor that sank the first animated-noise attempt). Temporal/Alpha only rules
			 * when this is off (A/B lever). */
			constexpr auto GraphicsPPIndirectDiffuseRTDenoiserAccumulationCounterKey{"Core/Graphics/PostProcessing/IndirectDiffuse/RayTracing/Denoiser/AccumulationCounter"};
			constexpr auto DefaultGraphicsPPIndirectDiffuseRTDenoiserAccumulationCounter{true};
			/* Accumulation cap N (the steady-state blend weight floor is 1/N). Larger =
			 * smoother but slower to react to lighting changes. */
			constexpr auto GraphicsPPIndirectDiffuseRTDenoiserMaxAccumulationKey{"Core/Graphics/PostProcessing/IndirectDiffuse/RayTracing/Denoiser/MaxAccumulation"};
			constexpr auto DefaultGraphicsPPIndirectDiffuseRTDenoiserMaxAccumulation{64U};
			/* Debug view of the denoiser internals, drawn by the combine pass INSTEAD of the
			 * GI contribution: 0 = off, 1 = temporal variance (binary-amplified x1e6 — a linear
			 * scale is unreadable under the photometric exposure), 2 = accumulation age
			 * (white = young/disoccluded). Diagnostic only, costs nothing at 0. */
			constexpr auto GraphicsPPIndirectDiffuseRTDenoiserDebugViewKey{"Core/Graphics/PostProcessing/IndirectDiffuse/RayTracing/Denoiser/DebugView"};
			constexpr auto DefaultGraphicsPPIndirectDiffuseRTDenoiserDebugView{0U};

			/* Ray Tracing > Global Illumination > Multi-bounce feedback.
			 * Bounce rays landing on a surface visible last frame pick up its accumulated
			 * indirect radiance: the geometric series converges to the multi-bounce solution
			 * (one traced bounce per frame, energy 1/(1-albedo*strength) at steady state).
			 * Requires the temporal accumulation to be enabled. */
			constexpr auto GraphicsPPIndirectDiffuseRTMultiBounceEnabledKey{"Core/Graphics/PostProcessing/IndirectDiffuse/RayTracing/MultiBounce/Enabled"};
			constexpr auto DefaultGraphicsPPIndirectDiffuseRTMultiBounceEnabled{true};
			/* Damping of the feedback series: 0 = single bounce, 1 = full geometric series. */
			constexpr auto GraphicsPPIndirectDiffuseRTMultiBounceStrengthKey{"Core/Graphics/PostProcessing/IndirectDiffuse/RayTracing/MultiBounce/Strength"};
			constexpr auto DefaultGraphicsPPIndirectDiffuseRTMultiBounceStrength{1.0F};
			/* Upper bound on the radiance re-injected per bounce (anti-firefly, divergence guard). */
			constexpr auto GraphicsPPIndirectDiffuseRTMultiBounceClampKey{"Core/Graphics/PostProcessing/IndirectDiffuse/RayTracing/MultiBounce/Clamp"};
			constexpr auto DefaultGraphicsPPIndirectDiffuseRTMultiBounceClamp{4.0F};

			/* Post-processing > frame cut around the translucent pass. When the frame contains
			 * grab-pass (transmissive) objects and an enabled indirect-diffuse effect, the chain is
			 * run in two halves: the indirect diffuse first, composited back into the scene colour so
			 * a glass TRANSMITS it, then everything else after the translucent pass. OFF runs the whole
			 * chain after the translucent pass, as before Aug 2026 — an A/B switch for measurement,
			 * not a quality knob: with it off, nothing seen through a glass receives indirect light. */
			constexpr auto GraphicsPPCutFrameAroundTranslucencyKey{"Core/Graphics/PostProcessing/CutFrameAroundTranslucency"};
			constexpr auto DefaultGraphicsPPCutFrameAroundTranslucency{true};

			/* Post-processing > which LANE the lighting family starts on: "Auto" (the best lane
			 * this machine can actually run), "RayTracing", "ScreenSpace", or "None" for no
			 * indirect lighting at all. Every lighting concept exists in both lanes and both stay
			 * resident, so this only decides the SELECTION at scene build; the runtime switch
			 * (Core.SceneManagerService.PostProcess.setLightingMode, KeyPad9) is unaffected and
			 * overrides it for the session.
			 * ⚠️⚠️ IT ALSO DECIDES WHETHER THE ACCELERATION STRUCTURES ARE BUILT AT ALL. The
			 * Renderer creates its AccelerationStructureBuilder only for "Auto" and "RayTracing"
			 * (and only on a capable device), and a BLAS is built when a geometry LOADS — a
			 * geometry cannot gain one afterwards. So with "ScreenSpace" or "None" there is
			 * nothing to trace against for the whole session, `installLightingFamily()` does not
			 * even file the traced lane, and `setLightingMode("RayTracing")` from the console is
			 * REFUSED rather than silently rendering nothing. Changing lane to a traced one is a
			 * relaunch, by design.
			 * ⚠️ "Auto" is the DEFAULT because a fixed default makes a freshly written file
			 * CONTRADICT ITSELF on a machine that cannot honour it (owner report, 2026-09-10 —
			 * "la clé est RayTracing alors que je vois le mode en ScreenSpace", back then caused
			 * by a second key that silently dominated this one; that key is now gone). "Auto"
			 * resolves against what the machine offers and can never be wrong.
			 * ⚠️ Naming a lane EXPLICITLY is therefore a REQUEST, and an unhonoured request is
			 * traced as a warning at startup — "Auto" is not a request and stays silent.
			 * ⚠️ Named after what it SELECTS rather than after a mechanism: a boolean called
			 * "EnableRayTracing" would read like a hardware switch, which this is not. */
			constexpr auto GraphicsPPLightingLaneKey{"Core/Graphics/PostProcessing/LightingLane"};
			/* The two values that are NOT a LightingLane: "Auto" is a resolution policy, "None"
			 * switches the whole family off. The two real lanes come from to_cstring(LightingLane). */
			constexpr auto GraphicsPPLightingLaneAuto{"Auto"};
			constexpr auto GraphicsPPLightingLaneNone{"None"};
			constexpr auto DefaultGraphicsPPLightingLane{GraphicsPPLightingLaneAuto};

			/* Post-processing > per-CONCEPT switch of the lighting family. Turning one off selects
			 * NO occupant for that slot, whichever lane is active — it is the concept that is
			 * switched off, not one of its implementations. */
			constexpr auto GraphicsPPContactShadowsEnabledKey{"Core/Graphics/PostProcessing/ContactShadows/Enabled"};
			constexpr auto DefaultGraphicsPPContactShadowsEnabled{true};
			constexpr auto GraphicsPPIndirectDiffuseEnabledKey{"Core/Graphics/PostProcessing/IndirectDiffuse/Enabled"};
			constexpr auto DefaultGraphicsPPIndirectDiffuseEnabled{true};
			constexpr auto GraphicsPPReflectionsEnabledKey{"Core/Graphics/PostProcessing/Reflections/Enabled"};
			constexpr auto DefaultGraphicsPPReflectionsEnabled{true};
			constexpr auto GraphicsPPAmbientOcclusionEnabledKey{"Core/Graphics/PostProcessing/AmbientOcclusion/Enabled"};
			constexpr auto DefaultGraphicsPPAmbientOcclusionEnabled{true};

			/* Post-processing > the two PHOTOGRAPHIC effects a user may refuse outright: they are
			 * expensive (DepthOfField is a 7-pass chain, MotionBlur a 4-pass one) and they are the
			 * two that people find intrusive.
			 * ⚠️⚠️ These OVERRIDE the scene. The camera is the photographic authority
			 * (Camera::enableDepthOfField() / enableMotionBlur()) but it only makes a REQUEST;
			 * PostProcessStack::syncCameraEffects() refuses to materialize the effect when the key
			 * says no, so a demo that turns it on for style is simply ignored. That is the whole
			 * point — until Sep 2026 the motion-blur key was consulted by the APPLICATION alone
			 * (projet-alpha's Player, at spawn time), so any demo calling enableMotionBlur() on
			 * the camera bypassed it completely.
			 * ⚠️ `Glare` and `ToneMapping` get no such key on purpose: the tone mapping is not an
			 * option but the sensor response, and refusing it sends raw photometric radiance to an
			 * LDR swap chain — measured as a white screen in daylight and a black one at night.
			 * ⚠️ The defaults are ASYMMETRIC, and deliberately: motion blur was already off by
			 * default before this key existed, depth of field was always honoured. Both defaults
			 * preserve the behaviour their effect had. */
			/* Post-processing > ContactShadows, both lanes. The concept had NO settings key at all
			 * until Sep 2026, in either lane, which meant tuning it required a rebuild — for an
			 * effect whose whole quality question is "what value do these knobs want".
			 * ⚠️ `MaxDistance`, `NormalBias`, `Intensity` and `MaxBlurRadius` exist in BOTH lanes
			 * and mean the SAME THING in the same units (metres for the first, world units for the
			 * bias). That is deliberate and it is what makes a lane A/B honest: the two occupants
			 * of a slot must be comparable at equal settings, or the comparison measures the
			 * settings instead of the techniques. Only the screen-space lane's last two are its
			 * own, because they describe its mechanism and the traced one has no equivalent. */
			constexpr auto GraphicsPPContactShadowsRTMaxDistanceKey{"Core/Graphics/PostProcessing/ContactShadows/RayTracing/MaxDistance"};
			constexpr auto DefaultGraphicsPPContactShadowsRTMaxDistance{2.0F};
			constexpr auto GraphicsPPContactShadowsRTNormalBiasKey{"Core/Graphics/PostProcessing/ContactShadows/RayTracing/NormalBias"};
			constexpr auto DefaultGraphicsPPContactShadowsRTNormalBias{0.01F};
			constexpr auto GraphicsPPContactShadowsRTIntensityKey{"Core/Graphics/PostProcessing/ContactShadows/RayTracing/Intensity"};
			constexpr auto DefaultGraphicsPPContactShadowsRTIntensity{0.8F};
			constexpr auto GraphicsPPContactShadowsRTMaxBlurRadiusKey{"Core/Graphics/PostProcessing/ContactShadows/RayTracing/MaxBlurRadius"};
			constexpr auto DefaultGraphicsPPContactShadowsRTMaxBlurRadius{10.0F};

			constexpr auto GraphicsPPContactShadowsSSMaxDistanceKey{"Core/Graphics/PostProcessing/ContactShadows/ScreenSpace/MaxDistance"};
			constexpr auto DefaultGraphicsPPContactShadowsSSMaxDistance{2.0F};
			constexpr auto GraphicsPPContactShadowsSSNormalBiasKey{"Core/Graphics/PostProcessing/ContactShadows/ScreenSpace/NormalBias"};
			constexpr auto DefaultGraphicsPPContactShadowsSSNormalBias{0.01F};
			constexpr auto GraphicsPPContactShadowsSSIntensityKey{"Core/Graphics/PostProcessing/ContactShadows/ScreenSpace/Intensity"};
			constexpr auto DefaultGraphicsPPContactShadowsSSIntensity{0.8F};
			constexpr auto GraphicsPPContactShadowsSSMaxBlurRadiusKey{"Core/Graphics/PostProcessing/ContactShadows/ScreenSpace/MaxBlurRadius"};
			constexpr auto DefaultGraphicsPPContactShadowsSSMaxBlurRadius{10.0F};
			/* ⚠️ THE knob of the screen-space lane. The depth buffer is a heightfield with no
			 * thickness, so an occluder is only recognised when the ray passes BEHIND a sample by
			 * less than this, in metres. Too thin and light leaks through thin geometry; too thick
			 * and a shadow halo trails behind every occluder. It has no ray-traced equivalent:
			 * a TLAS knows what is solid. */
			constexpr auto GraphicsPPContactShadowsSSThicknessKey{"Core/Graphics/PostProcessing/ContactShadows/ScreenSpace/Thickness"};
			constexpr auto DefaultGraphicsPPContactShadowsSSThickness{0.25F};
			/* Steps along the march. The step length is MaxDistance / StepCount, in metres, so
			 * raising the distance without raising this thins the sampling. */
			constexpr auto GraphicsPPContactShadowsSSStepCountKey{"Core/Graphics/PostProcessing/ContactShadows/ScreenSpace/StepCount"};
			constexpr auto DefaultGraphicsPPContactShadowsSSStepCount{16U};

			constexpr auto GraphicsPPDepthOfFieldEnabledKey{"Core/Graphics/PostProcessing/DepthOfField/Enabled"};
			constexpr auto DefaultGraphicsPPDepthOfFieldEnabled{true};
			constexpr auto GraphicsPPMotionBlurEnabledKey{"Core/Graphics/PostProcessing/MotionBlur/Enabled"};
			constexpr auto DefaultGraphicsPPMotionBlurEnabled{false};

			/* Anti-Aliasing > Temporal (TAA). HDR resolve BEFORE DoF/tone mapping (the only
			 * AA effect not bound by the runsAfterToneMapping contract — the Karis luminance
			 * weighting below is what makes HDR accumulation safe). Requires the velocity
			 * G-buffer and drives the Halton (2,3) projection jitter (requiresJitter). */
			constexpr auto GraphicsPPTemporalAAEnabledKey{"Core/Graphics/PostProcessing/TemporalAA/Enabled"};
			constexpr auto DefaultGraphicsPPTemporalAAEnabled{false};
			/* Blend weight of the CURRENT frame (0.1 = 90% history: strong AA, slower response). */
			constexpr auto GraphicsPPTemporalAAAlphaKey{"Core/Graphics/PostProcessing/TemporalAA/Alpha"};
			constexpr auto DefaultGraphicsPPTemporalAAAlpha{0.1F};
			/* Variance clipping gamma: half-size of the YCoCg statistical AABB in standard
			 * deviations (lower = less ghosting, more flicker). */
			constexpr auto GraphicsPPTemporalAAVarianceGammaKey{"Core/Graphics/PostProcessing/TemporalAA/VarianceGamma"};
			constexpr auto DefaultGraphicsPPTemporalAAVarianceGamma{1.0F};
			/* Karis inverse-luminance blend weighting (HDR anti-firefly / anti-flicker). */
			constexpr auto GraphicsPPTemporalAALumaWeightingKey{"Core/Graphics/PostProcessing/TemporalAA/LumaWeighting"};
			constexpr auto DefaultGraphicsPPTemporalAALumaWeighting{true};

			/* Motion Blur — effect QUALITY knobs only. The blur LENGTH is photographic and
			 * belongs to the active camera: shutter speed / frame duration = shutter angle
			 * (Scenes::Component::Camera::setShutterSpeed()). Requires the velocity G-buffer;
			 * runs in HDR, after the temporal resolve and before the photographic effects. */
			/* Samples walked along the dominant velocity (odd: one lands on the pixel centre). */
			constexpr auto GraphicsPPMotionBlurSampleCountKey{"Core/Graphics/PostProcessing/MotionBlur/SampleCount"};
			constexpr auto DefaultGraphicsPPMotionBlurSampleCount{24U};
			/* Depth interval, in meters, softening the foreground/background classification. */
			constexpr auto GraphicsPPMotionBlurSoftDepthExtentKey{"Core/Graphics/PostProcessing/MotionBlur/SoftDepthExtent"};
			constexpr auto DefaultGraphicsPPMotionBlurSoftDepthExtent{0.05F};

			/* Volumetric light — the SCREEN-SPACE radial-blur god rays (Mitchell, GPU Gems 3).
			 * ⚠️ These are radial-blur tuning knobs, NOT a participating medium: 'density' is a
			 * screen-space step multiplier and 'exposure' an arbitrary gain that converts the
			 * light's LUX into the nits buffer. A world-space single-scattering pass sharing the
			 * atmosphere's medium would make all of them meaningless — they are exposed so the two
			 * implementations can be compared at runtime without a rebuild, which is the only
			 * reason this effect had no settings key for so long. */
			constexpr auto GraphicsPPVolumetricLightDensityKey{"Core/Graphics/PostProcessing/VolumetricLight/Density"};
			constexpr auto DefaultGraphicsPPVolumetricLightDensity{1.0F};
			constexpr auto GraphicsPPVolumetricLightDecayKey{"Core/Graphics/PostProcessing/VolumetricLight/Decay"};
			constexpr auto DefaultGraphicsPPVolumetricLightDecay{0.975F};
			constexpr auto GraphicsPPVolumetricLightExposureKey{"Core/Graphics/PostProcessing/VolumetricLight/Exposure"};
			constexpr auto DefaultGraphicsPPVolumetricLightExposure{0.25F};
			constexpr auto GraphicsPPVolumetricLightSampleCountKey{"Core/Graphics/PostProcessing/VolumetricLight/SampleCount"};
			constexpr auto DefaultGraphicsPPVolumetricLightSampleCount{64U};
			/* EMA weight of the occlusion mask: 1 = no accumulation, 0.2 ~= 8 frames. */
			constexpr auto GraphicsPPVolumetricLightTemporalAlphaKey{"Core/Graphics/PostProcessing/VolumetricLight/TemporalAlpha"};
			constexpr auto DefaultGraphicsPPVolumetricLightTemporalAlpha{0.2F};

			/* Depth of Field — effect QUALITY knobs only. The optical parameters (aperture,
			 * focal length, focus) belong to the active camera (physical camera model,
			 * Scenes::Component::Camera), NOT to the settings. */
			/* Blur ceiling: maximum gather radius in half-res pixels. A pure performance/quality
			 * clamp — the blur AMOUNT is the thin-lens circle of confusion, converted to pixels
			 * from the sensor fraction (no scale factor). 32 half-res = ~64 full-res pixels of
			 * diameter, past which 48 spiral taps would start to ring. */
			constexpr auto GraphicsPPDepthOfFieldMaxRadiusKey{"Core/Graphics/PostProcessing/DepthOfField/MaxRadius"};
			constexpr auto DefaultGraphicsPPDepthOfFieldMaxRadius{32.0F};
			/* Golden-spiral gather taps per pixel (bokeh quality). */
			constexpr auto GraphicsPPDepthOfFieldSampleCountKey{"Core/Graphics/PostProcessing/DepthOfField/SampleCount"};
			constexpr auto DefaultGraphicsPPDepthOfFieldSampleCount{48U};
			/* Auto-focus adaptation speed (rack focus), in 1/seconds. */
			constexpr auto GraphicsPPDepthOfFieldAutoFocusSpeedKey{"Core/Graphics/PostProcessing/DepthOfField/AutoFocusSpeed"};
			constexpr auto DefaultGraphicsPPDepthOfFieldAutoFocusSpeed{3.0F};
			/* Near-field (foreground) blur with silhouette bleeding. */
			constexpr auto GraphicsPPDepthOfFieldNearFieldKey{"Core/Graphics/PostProcessing/DepthOfField/NearField"};
			constexpr auto DefaultGraphicsPPDepthOfFieldNearField{true};

			/* Screen Space > Ambient Occlusion (first screen-space effect group — SSGI keys will join it). */
			/* Hemisphere sampling radius, in world units. */
			constexpr auto GraphicsPPAmbientOcclusionSSRadiusKey{"Core/Graphics/PostProcessing/AmbientOcclusion/ScreenSpace/Radius"};
			constexpr auto DefaultGraphicsPPAmbientOcclusionSSRadius{0.5F};
			/* AO darkening intensity multiplier (applied once, clamped; 1.0 = pure visibility term). */
			constexpr auto GraphicsPPAmbientOcclusionSSIntensityKey{"Core/Graphics/PostProcessing/AmbientOcclusion/ScreenSpace/Intensity"};
			constexpr auto DefaultGraphicsPPAmbientOcclusionSSIntensity{1.0F};
			/* Depth comparison bias to prevent self-occlusion, in view-space units. */
			constexpr auto GraphicsPPAmbientOcclusionSSBiasKey{"Core/Graphics/PostProcessing/AmbientOcclusion/ScreenSpace/Bias"};
			constexpr auto DefaultGraphicsPPAmbientOcclusionSSBias{0.025F};
			/* Samples per pixel for screen-space ambient occlusion. */
			constexpr auto GraphicsPPAmbientOcclusionSSSampleCountKey{"Core/Graphics/PostProcessing/AmbientOcclusion/ScreenSpace/SampleCount"};
			constexpr auto DefaultGraphicsPPAmbientOcclusionSSSampleCount{32U};

			/* Screen Space > Global Illumination */
			/* Maximum GI ray-march distance, in world units. */
			constexpr auto GraphicsPPIndirectDiffuseSSMaxDistanceKey{"Core/Graphics/PostProcessing/IndirectDiffuse/ScreenSpace/MaxDistance"};
			constexpr auto DefaultGraphicsPPIndirectDiffuseSSMaxDistance{5.0F};
			/* Indirect lighting intensity multiplier. */
			constexpr auto GraphicsPPIndirectDiffuseSSIntensityKey{"Core/Graphics/PostProcessing/IndirectDiffuse/ScreenSpace/Intensity"};
			constexpr auto DefaultGraphicsPPIndirectDiffuseSSIntensity{0.8F};
			/* Depth thickness assumed behind each depth sample, in view-space units. */
			constexpr auto GraphicsPPIndirectDiffuseSSThicknessKey{"Core/Graphics/PostProcessing/IndirectDiffuse/ScreenSpace/Thickness"};
			constexpr auto DefaultGraphicsPPIndirectDiffuseSSThickness{0.5F};
			/* Rays per pixel for screen-space global illumination. */
			constexpr auto GraphicsPPIndirectDiffuseSSSampleCountKey{"Core/Graphics/PostProcessing/IndirectDiffuse/ScreenSpace/SampleCount"};
			constexpr auto DefaultGraphicsPPIndirectDiffuseSSSampleCount{8U};
			/* Ray-march steps per ray. */
			constexpr auto GraphicsPPIndirectDiffuseSSStepCountKey{"Core/Graphics/PostProcessing/IndirectDiffuse/ScreenSpace/StepCount"};
			constexpr auto DefaultGraphicsPPIndirectDiffuseSSStepCount{16U};
			/* Bilateral denoising blur radius for GI, in pixels. */
			constexpr auto GraphicsPPIndirectDiffuseSSBlurRadiusKey{"Core/Graphics/PostProcessing/IndirectDiffuse/ScreenSpace/BlurRadius"};
			constexpr auto DefaultGraphicsPPIndirectDiffuseSSBlurRadius{4U};
			/* Depth edge-stopping sigma for the GI bilateral blur. */
			constexpr auto GraphicsPPIndirectDiffuseSSDepthSigmaKey{"Core/Graphics/PostProcessing/IndirectDiffuse/ScreenSpace/DepthSigma"};
			constexpr auto DefaultGraphicsPPIndirectDiffuseSSDepthSigma{1.0F};
			/* Normal edge-stopping sigma for the GI bilateral blur. */
			constexpr auto GraphicsPPIndirectDiffuseSSNormalSigmaKey{"Core/Graphics/PostProcessing/IndirectDiffuse/ScreenSpace/NormalSigma"};
			constexpr auto DefaultGraphicsPPIndirectDiffuseSSNormalSigma{0.5F};

			/* Screen Space > Global Illumination > Temporal (GIDenoiser resolve — SSGI's
			 * first temporal accumulation; mirrors the RayTracing group, same semantics). */
			constexpr auto GraphicsPPIndirectDiffuseSSTemporalEnabledKey{"Core/Graphics/PostProcessing/IndirectDiffuse/ScreenSpace/Temporal/Enabled"};
			constexpr auto DefaultGraphicsPPIndirectDiffuseSSTemporalEnabled{true};
			constexpr auto GraphicsPPIndirectDiffuseSSTemporalAlphaKey{"Core/Graphics/PostProcessing/IndirectDiffuse/ScreenSpace/Temporal/Alpha"};
			constexpr auto DefaultGraphicsPPIndirectDiffuseSSTemporalAlpha{0.1F};
			constexpr auto GraphicsPPIndirectDiffuseSSTemporalDepthToleranceKey{"Core/Graphics/PostProcessing/IndirectDiffuse/ScreenSpace/Temporal/DepthTolerance"};
			constexpr auto DefaultGraphicsPPIndirectDiffuseSSTemporalDepthTolerance{0.05F};
			constexpr auto GraphicsPPIndirectDiffuseSSTemporalNormalThresholdKey{"Core/Graphics/PostProcessing/IndirectDiffuse/ScreenSpace/Temporal/NormalThreshold"};
			constexpr auto DefaultGraphicsPPIndirectDiffuseSSTemporalNormalThreshold{0.8F};
			constexpr auto GraphicsPPIndirectDiffuseSSTemporalVarianceGammaKey{"Core/Graphics/PostProcessing/IndirectDiffuse/ScreenSpace/Temporal/VarianceGamma"};
			constexpr auto DefaultGraphicsPPIndirectDiffuseSSTemporalVarianceGamma{1.0F};
			constexpr auto GraphicsPPIndirectDiffuseSSTemporalNeighborhoodClampKey{"Core/Graphics/PostProcessing/IndirectDiffuse/ScreenSpace/Temporal/NeighborhoodClamp"};
			constexpr auto DefaultGraphicsPPIndirectDiffuseSSTemporalNeighborhoodClamp{false};
			constexpr auto GraphicsPPIndirectDiffuseSSTemporalAnimatedNoiseKey{"Core/Graphics/PostProcessing/IndirectDiffuse/ScreenSpace/Temporal/AnimatedNoise"};
			constexpr auto DefaultGraphicsPPIndirectDiffuseSSTemporalAnimatedNoise{true};

			/* Screen Space > Global Illumination > Denoiser (shared GIDenoiser component —
			 * mirrors the RayTracing group, same semantics and defaults). */
			constexpr auto GraphicsPPIndirectDiffuseSSDenoiserIterationsKey{"Core/Graphics/PostProcessing/IndirectDiffuse/ScreenSpace/Denoiser/Iterations"};
			constexpr auto DefaultGraphicsPPIndirectDiffuseSSDenoiserIterations{4U};
			constexpr auto GraphicsPPIndirectDiffuseSSDenoiserLuminanceSigmaKey{"Core/Graphics/PostProcessing/IndirectDiffuse/ScreenSpace/Denoiser/LuminanceSigma"};
			constexpr auto DefaultGraphicsPPIndirectDiffuseSSDenoiserLuminanceSigma{4.0F};
			constexpr auto GraphicsPPIndirectDiffuseSSDenoiserAccumulationCounterKey{"Core/Graphics/PostProcessing/IndirectDiffuse/ScreenSpace/Denoiser/AccumulationCounter"};
			constexpr auto DefaultGraphicsPPIndirectDiffuseSSDenoiserAccumulationCounter{true};
			constexpr auto GraphicsPPIndirectDiffuseSSDenoiserMaxAccumulationKey{"Core/Graphics/PostProcessing/IndirectDiffuse/ScreenSpace/Denoiser/MaxAccumulation"};
			constexpr auto DefaultGraphicsPPIndirectDiffuseSSDenoiserMaxAccumulation{64U};
			constexpr auto GraphicsPPIndirectDiffuseSSDenoiserDebugViewKey{"Core/Graphics/PostProcessing/IndirectDiffuse/ScreenSpace/Denoiser/DebugView"};
			constexpr auto DefaultGraphicsPPIndirectDiffuseSSDenoiserDebugView{0U};

			/* Screen-space reflections (SSR). */
			/* Compute reflections at half resolution (pixel doubling) to save performance.
			 * Default FALSE (owner decision): screen-space effects run full-res by default —
			 * they are the cheap tier of the reflection ladder, quality is their selling point. */
			constexpr auto GraphicsPPReflectionsSSPixelDoublingKey{"Core/Graphics/PostProcessing/Reflections/ScreenSpace/PixelDoubling"};
			constexpr auto DefaultGraphicsPPReflectionsSSPixelDoubling{false};
			/* Bilateral blur radius, in pixels — scaled per-pixel by the surface roughness
			 * (a polished surface keeps a mirror-sharp reflection). */
			constexpr auto GraphicsPPReflectionsSSBlurRadiusKey{"Core/Graphics/PostProcessing/Reflections/ScreenSpace/BlurRadius"};
			constexpr auto DefaultGraphicsPPReflectionsSSBlurRadius{2U};
			/* Depth edge-stopping sigma for the reflection bilateral blur. */
			constexpr auto GraphicsPPReflectionsSSDepthSigmaKey{"Core/Graphics/PostProcessing/Reflections/ScreenSpace/DepthSigma"};
			constexpr auto DefaultGraphicsPPReflectionsSSDepthSigma{0.5F};
			/* Normal edge-stopping sigma for the reflection bilateral blur. */
			constexpr auto GraphicsPPReflectionsSSNormalSigmaKey{"Core/Graphics/PostProcessing/Reflections/ScreenSpace/NormalSigma"};
			constexpr auto DefaultGraphicsPPReflectionsSSNormalSigma{0.3F};

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
			/* Width of the cross-fade band between two cascades, as a FRACTION of the cascade's own
			 * depth range. 0 disables the blend and compiles the branch away entirely.
			 * Without it a cascade boundary is a hard plane locked to the camera, between two texel
			 * grids that are not aligned with each other: a static object's shadow switches grid in
			 * one frame as the camera advances — a localised pop travelling with you along the split.
			 * ⚠️ COST: inside the band a fragment samples TWO cascades, so it pays the PCF kernel
			 * twice. The band is a thin shell, so the average cost is small, but it is not free. */
			constexpr auto GraphicsShadowMappingCascadeBlendRatioKey{"Core/Graphics/ShadowMapping/CascadeBlendRatio"};
			constexpr auto DefaultGraphicsShadowMappingCascadeBlendRatio{0.1F};
			/* Normal-offset shadows: how far, in SHADOW TEXELS, the sampled position is pushed along
			 * the surface normal before it is projected into light space. 0 disables it and emits
			 * nothing — not even the world-normal varying it needs.
			 * It beats a pure depth bias on grazing surfaces: a depth bias fights acne along the
			 * LIGHT direction, where a shallow angle needs an ever larger push and pays for it with
			 * peter-panning, while a normal offset moves the sample off the surface it is testing,
			 * which is where the self-shadowing actually comes from. The two are complementary, not
			 * redundant — different axes — so shadowBias stays. */
			constexpr auto GraphicsShadowMappingNormalOffsetScaleKey{"Core/Graphics/ShadowMapping/NormalOffsetScale"};
			/* ⚠️ DEFAULT 0 — DISABLED, and that is a measurement, not caution. On reflexion-debug a
			 * scale of 1.0 REMOVES the sphere's and the dragon's contact shadows (palm shadow band
			 * 69.24 -> 73.15 at a pinned pose and exposure), while 0.05 is indistinguishable from off
			 * (69.49). The offset is proportional and correct — it is simply larger than the contact
			 * shadows of a scene whose cascade texels are metres wide. The acne it fights was never
			 * visible here, so enabling it by default would trade an artefact nobody sees for one
			 * everybody does. Raise it on a scene that shows acne at grazing incidence, and re-measure
			 * the contact shadows while you do. */
			constexpr auto DefaultGraphicsShadowMappingNormalOffsetScale{0.0F};
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

		/* Viewers (the +ImageViewer / +ModelViewer scenes behind the dropped-files pipeline) */
		/* Skybox resource shown behind a model in the +ModelViewer, and the source of its IBL.
		 * An environment is not decoration here: a reflective, transmissive, clearcoat, sheen or
		 * iridescent material has NOTHING to reflect without one, and the asset cannot be judged.
		 * The name is a setting because the resource belongs to the CONSUMER's data store, not to
		 * the engine; an unknown name degrades to no background (traced), never to a failure.
		 * Empty string = no background at all. */
		constexpr auto ViewerBackgroundKey{"Core/Viewers/Background"};
		constexpr auto DefaultViewerBackground{"GreenLandscape"};

		/* The environment cubemap the SUBJECT REFLECTS, independently of the backdrop behind it.
		 *
		 * ⚠️⚠️ THESE ARE TWO DIFFERENT AXES and the engine already separates them: `Scene` has both
		 * `setBackground()` and `setEnvironmentCubemap()`, and the latter's own documentation says it
		 * replaces whatever a background installed. Conflating them into one "environment" preset
		 * would make the configuration the Khronos conformance references actually use —
		 * a BLACK BACKDROP with a bright STUDIO reflection — inexpressible.
		 *
		 * Empty string = leave whatever the background installed (the previous behaviour). A name
		 * overrides it, and is applied AFTER the background for exactly that reason. An unknown name
		 * is traced and ignored, never a failure. */
		constexpr auto ViewerEnvironmentCubemapKey{"Core/Viewers/EnvironmentCubemap"};
		constexpr auto DefaultViewerEnvironmentCubemap{""};

		/* The viewer's flat ambient illuminance, in lux.
		 *
		 * ⚠️ It exists as a FLOOR for the case where no background resource is available, and the sky
		 * irradiance dominates it by two orders of magnitude when one is. But 200 lux of flat ambient
		 * is enough to WASH OUT a sheen rim or an iridescence fringe, which is precisely what the
		 * tests Khronos shoots on black are measuring. Dropping it to 0 is how those become
		 * judgeable; leaving the default keeps every existing viewer session identical. */
		constexpr auto ViewerAmbientIntensityKey{"Core/Viewers/AmbientIntensity"};
		constexpr auto DefaultViewerAmbientIntensity{200.0F};

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
