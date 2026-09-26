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
			/* Parallax occlusion mapping: the ray-march layer count (at a grazing view, clamped to
			 * [0, 64]) of every material with a height map that does not set its own
			 * (StandardResource::setParallaxIterations()). Resolved when the material is created and
			 * written to its UBO; 0 = the height map is ignored (plain normal mapping). */
			constexpr auto GraphicsTexturePOMIterationsKey{"Core/Graphics/Texture/POMIterations"};
			constexpr auto DefaultGraphicsTexturePOMIterations{0};
			/** @brief Displays one packed lane of the material-properties G-buffer as the frame
			 * colour, in grey. 0 = off, 1 = reflectivity, 2 = AO response. ⚠️ It is the cheapest
			 * instrument this engine has for a "why is that surface reflecting" question — it
			 * settled the Sponza dirt-decal report in one capture where three A/Bs had circled.
			 * Read once per program generation, so it takes effect on the next launch. */
			constexpr auto GraphicsDebugMaterialPropertiesLaneKey{"Core/Graphics/DebugMaterialPropertiesLane"};
			constexpr auto DefaultGraphicsDebugMaterialPropertiesLane{0};
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

			/* Triangle budget for the ray-tracing proxy of a TERRAIN (Geometry::CDLODTerrainResource).
			 * The terrain draws a flat patch its vertex stage displaces, so nothing of its rendering buffers
			 * can be traced: a fixed proxy (a BLAS has no view-dependent LOD) is built on the CPU from the
			 * height grid, over `rayTracingProxySize` (4096 m) around the camera, at the finest power-of-two
			 * step whose triangle count fits this budget. The full-resolution surface is quadratic in the
			 * division count and unbounded: 4096 m at 1 m is 33.5 M triangles, MEASURED at +2471 MiB of VRAM
			 * on the former adaptive grid. At 2 M that window takes step 8 (524 288 triangles, 51 ms to
			 * build, 2026-09-22) — step 4 would be 2.1 M, one hair over — while a 128-division floor keeps
			 * its exact surface. Raising it buys exactness for the RT occlusion of fine relief, and costs
			 * VRAM quadratically. */
			constexpr auto GraphicsRayTracingTerrainBLASMaxTrianglesKey{"Core/Graphics/RayTracing/TerrainBLASMaxTriangles"};
			constexpr auto DefaultGraphicsRayTracingTerrainBLASMaxTriangles{2000000U};

			/* Ray Tracing > Irradiance probe volume — the engine's RADIANCE CACHE (DDGI, owner decision
			 * 2026-09-12). Ray tracing that is not an effect: it serves the traced effects (RTR reads it
			 * at every reflection hit) and lives next to the TLAS. All keys are read ONCE at renderer
			 * initialization; a change needs a relaunch. */
			/* Update the probes every frame and let the consumers read them. Off: the volume stays
			 * allocated and bound (the consumers' pipeline layouts need it) but is never traced and the
			 * shader-side flag makes every query return zero. */
			constexpr auto GraphicsRayTracingIrradianceProbesEnabledKey{"Core/Graphics/RayTracing/IrradianceProbes/Enabled"};
			constexpr auto DefaultGraphicsRayTracingIrradianceProbesEnabled{true};
			/* Probes per axis of the camera-centred volume (Y is up). 16 x 8 x 16 = 2048 probes. */
			constexpr auto GraphicsRayTracingIrradianceProbesCountXKey{"Core/Graphics/RayTracing/IrradianceProbes/ProbeCountX"};
			constexpr auto DefaultGraphicsRayTracingIrradianceProbesCountX{16U};
			constexpr auto GraphicsRayTracingIrradianceProbesCountYKey{"Core/Graphics/RayTracing/IrradianceProbes/ProbeCountY"};
			constexpr auto DefaultGraphicsRayTracingIrradianceProbesCountY{8U};
			constexpr auto GraphicsRayTracingIrradianceProbesCountZKey{"Core/Graphics/RayTracing/IrradianceProbes/ProbeCountZ"};
			constexpr auto DefaultGraphicsRayTracingIrradianceProbesCountZ{16U};
			/* Distance between two probes, in metres: the volume spans (count - 1) x spacing per axis
			 * around the camera (24 x 12 x 24 m by default). */
			constexpr auto GraphicsRayTracingIrradianceProbesSpacingKey{"Core/Graphics/RayTracing/IrradianceProbes/ProbeSpacing"};
			constexpr auto DefaultGraphicsRayTracingIrradianceProbesSpacing{1.5F};
			/* Where the camera sits in the volume's HEIGHT: 0 = bottom plane, 1 = top plane. The volume is
			 * centred on the camera horizontally, but a camera stands about 1.7 m above a floor with a
			 * ceiling several metres higher, so centring it vertically wasted half the probes under the
			 * floor and put the ceiling of a 6 m room exactly on the top plane, where the volume's
			 * influence fades to zero — measured black in the reflections. 0.35 leaves ~4 m below the
			 * eye and ~8 m above with the default 12 m of probes. */
			constexpr auto GraphicsRayTracingIrradianceProbesCameraHeightFractionKey{"Core/Graphics/RayTracing/IrradianceProbes/CameraHeightFraction"};
			constexpr auto DefaultGraphicsRayTracingIrradianceProbesCameraHeightFraction{0.35F};
			/* Rays traced per probe per frame (spherical Fibonacci set, randomly rotated each frame).
			 * 2048 probes x 128 rays = 0.26 M rays per frame, against ~9 M for RTGI at half resolution. */
			constexpr auto GraphicsRayTracingIrradianceProbesRaysPerProbeKey{"Core/Graphics/RayTracing/IrradianceProbes/RaysPerProbe"};
			constexpr auto DefaultGraphicsRayTracingIrradianceProbesRaysPerProbe{128U};
			/* Temporal hysteresis of the atlases: the weight of the PREVIOUS frame. 0.97 converges in
			 * about 100 frames and hides the per-frame ray rotation; lower reacts faster and shimmers. */
			constexpr auto GraphicsRayTracingIrradianceProbesHysteresisKey{"Core/Graphics/RayTracing/IrradianceProbes/Hysteresis"};
			constexpr auto DefaultGraphicsRayTracingIrradianceProbesHysteresis{0.97F};
			/* Weight of the volume's OWN irradiance re-injected at a probe ray's hit — the DDGI feedback
			 * that turns one traced bounce per frame into the converged multi-bounce solution. 1 = the
			 * full geometric series (physical for albedos < 1), 0 = single bounce only (the A/B lever
			 * against RTGI with its multi-bounce off). Same role as IndirectDiffuse/RayTracing/MultiBounce/Strength. */
			constexpr auto GraphicsRayTracingIrradianceProbesBounceFeedbackKey{"Core/Graphics/RayTracing/IrradianceProbes/BounceFeedback"};
			constexpr auto DefaultGraphicsRayTracingIrradianceProbesBounceFeedback{1.0F};
			/* Self-shadow bias of a query, in metres: the sampled position is pushed along the surface
			 * normal (NormalBias) and toward the viewer (ViewBias) so the Chebyshev visibility test does
			 * not take the receiving surface for an occluder. THE knob against light leaking through
			 * thin walls (raise) and against dark seams at creases (lower). */
			constexpr auto GraphicsRayTracingIrradianceProbesNormalBiasKey{"Core/Graphics/RayTracing/IrradianceProbes/NormalBias"};
			constexpr auto DefaultGraphicsRayTracingIrradianceProbesNormalBias{0.1F};
			constexpr auto GraphicsRayTracingIrradianceProbesViewBiasKey{"Core/Graphics/RayTracing/IrradianceProbes/ViewBias"};
			constexpr auto DefaultGraphicsRayTracingIrradianceProbesViewBias{0.1F};

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
			/* Ray Tracing > Reflection > Temporal accumulation (2026-09-13). The trace is deterministic
			 * — one mirror ray per texel, no frame term — but HALF-RES and fed by a G-buffer the TAA
			 * jitters by half a pixel every frame: a reflected silhouette aliases at half resolution and
			 * the TAA cannot reproject it (reflected content does not follow the reflector's velocity),
			 * so its clamp let the flicker through — the owner's "fourmillement", measured at 0.44 mean
			 * and 0.64 % of pixels above 16/255 on a static reflected object against 0.052 with the
			 * jitter off (projet-alpha light-and-shadow-debug, mirror floor). The GIDenoiser accumulates
			 * the RAW trace in REFLECTION mode: history reprojected through the VIRTUAL position of the
			 * reflected point (P + V·hitT), blended toward the surface reprojection as the roughness
			 * grows, validated on the virtual distance, variance-clipped. The blur and the glossy pyramid
			 * then integrate a stable signal. The knobs mirror IndirectDiffuse/Temporal/x; the lane level
			 * because the screen-space lane has no accumulation (yet). */
			constexpr auto GraphicsPPReflectionsRTTemporalEnabledKey{"Core/Graphics/PostProcessing/Reflections/RayTracing/Temporal/Enabled"};
			constexpr auto DefaultGraphicsPPReflectionsRTTemporalEnabled{true};
			/* Fixed blend weight of the current frame, ruling only when the 1/N counter is capped. */
			constexpr auto GraphicsPPReflectionsRTTemporalAlphaKey{"Core/Graphics/PostProcessing/Reflections/RayTracing/Temporal/Alpha"};
			constexpr auto DefaultGraphicsPPReflectionsRTTemporalAlpha{0.1F};
			/* Relative tolerance of the disocclusion test on the VIRTUAL distance (camera + hit). */
			constexpr auto GraphicsPPReflectionsRTTemporalDepthToleranceKey{"Core/Graphics/PostProcessing/Reflections/RayTracing/Temporal/DepthTolerance"};
			constexpr auto DefaultGraphicsPPReflectionsRTTemporalDepthTolerance{0.05F};
			/* Minimum world-normal dot between the reflector now and at the history pixel. */
			constexpr auto GraphicsPPReflectionsRTTemporalNormalThresholdKey{"Core/Graphics/PostProcessing/Reflections/RayTracing/Temporal/NormalThreshold"};
			constexpr auto DefaultGraphicsPPReflectionsRTTemporalNormalThreshold{0.8F};
			/* Variance-clipping width in standard deviations of the current 3x3 (anti-ghosting). */
			constexpr auto GraphicsPPReflectionsRTTemporalVarianceGammaKey{"Core/Graphics/PostProcessing/Reflections/RayTracing/Temporal/VarianceGamma"};
			constexpr auto DefaultGraphicsPPReflectionsRTTemporalVarianceGamma{1.0F};
			/* 1/N accumulation cap: the steady-state weight of the current frame is 1/N. */
			constexpr auto GraphicsPPReflectionsRTTemporalMaxAccumulationKey{"Core/Graphics/PostProcessing/Reflections/RayTracing/Temporal/MaxAccumulation"};
			constexpr auto DefaultGraphicsPPReflectionsRTTemporalMaxAccumulation{32U};

			/* Ray Tracing > Ambient Occlusion */
			/* Samples per pixel for ray-traced ambient occlusion. */
			constexpr auto GraphicsPPAmbientOcclusionRTSampleCountKey{"Core/Graphics/PostProcessing/AmbientOcclusion/RayTracing/SampleCount"};
			constexpr auto DefaultGraphicsPPAmbientOcclusionRTSampleCount{8U};
			/* Compute ambient occlusion at half resolution (pixel doubling) to save performance. */
			constexpr auto GraphicsPPAmbientOcclusionRTPixelDoublingKey{"Core/Graphics/PostProcessing/AmbientOcclusion/RayTracing/PixelDoubling"};
			constexpr auto DefaultGraphicsPPAmbientOcclusionRTPixelDoubling{true};
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

			/* Ray Tracing > Global Illumination — the traced lane's OWN knobs. Everything the two
			 * lanes share (range, intensity, sample count, blur, temporal chain, denoiser) lives at the
			 * concept level, `Core/Graphics/PostProcessing/IndirectDiffuse/<param>`, declared below with
			 * the concept's `Enabled` key. */
			/* Compute GI at half resolution (pixel doubling) to save performance. */
			constexpr auto GraphicsPPIndirectDiffuseRTPixelDoublingKey{"Core/Graphics/PostProcessing/IndirectDiffuse/RayTracing/PixelDoubling"};
			constexpr auto DefaultGraphicsPPIndirectDiffuseRTPixelDoubling{true};
			/* GI ray origin offset to prevent self-intersection, in world units. */
			constexpr auto GraphicsPPIndirectDiffuseRTBiasKey{"Core/Graphics/PostProcessing/IndirectDiffuse/RayTracing/Bias"};
			constexpr auto DefaultGraphicsPPIndirectDiffuseRTBias{0.02F};

			/* Ray Tracing > Global Illumination > Multi-bounce feedback.
			 * A bounce ray's hit picks up the indirect irradiance the IRRADIANCE PROBE VOLUME holds at
			 * that point (Sep 2026 — it used to reproject the previous frame's resolved history, a
			 * screen-space quantity that vanished exactly where the bounce light came from off
			 * screen): the geometric series converges to the multi-bounce solution, one traced bounce
			 * per frame, energy 1/(1-albedo*strength) at steady state, and the primary surfaces and
			 * the reflected ones now share ONE estimator. No longer tied to the temporal accumulation.
			 * ⚠️ `MultiBounce/Clamp` was deleted with the history: a converged probe average has no
			 * fireflies to clamp. A settings.json written before keeps it as an orphan. */
			constexpr auto GraphicsPPIndirectDiffuseRTMultiBounceEnabledKey{"Core/Graphics/PostProcessing/IndirectDiffuse/RayTracing/MultiBounce/Enabled"};
			constexpr auto DefaultGraphicsPPIndirectDiffuseRTMultiBounceEnabled{true};
			/* Damping of the feedback series: 0 = single bounce, 1 = full geometric series. */
			constexpr auto GraphicsPPIndirectDiffuseRTMultiBounceStrengthKey{"Core/Graphics/PostProcessing/IndirectDiffuse/RayTracing/MultiBounce/Strength"};
			constexpr auto DefaultGraphicsPPIndirectDiffuseRTMultiBounceStrength{1.0F};

			/* Post-processing > frame cut around the translucent pass. When the frame contains
			 * grab-pass (transmissive) objects and an enabled indirect-diffuse effect, the chain is
			 * run in two halves: the indirect diffuse first, composited back into the scene colour so
			 * a glass TRANSMITS it, then everything else after the translucent pass. OFF runs the whole
			 * chain after the translucent pass, as before Aug 2026 — an A/B switch for measurement,
			 * not a quality knob: with it off, nothing seen through a glass receives indirect light. */
			constexpr auto GraphicsPPCutFrameAroundTranslucencyKey{"Core/Graphics/PostProcessing/CutFrameAroundTranslucency"};
			constexpr auto DefaultGraphicsPPCutFrameAroundTranslucency{true};

			/* Post-processing > DEBUG VIEW of the NON-FINITE values (2026-09-13). ON, the chain PAINTS
			 * every NaN/Inf it meets instead of hiding it: the SSR resolve paints WHICH input is
			 * non-finite (RED = the grabbed scene colour, BLUE = the colour pyramid, GREEN = the trace
			 * data, MAGENTA = the mixed result only) and the TAA paints in red any pixel whose 3x3
			 * reconstruction holds one. OFF, the SSR rejects such a colour as a miss and the TAA's max()
			 * turns a NaN into 0 — a BLACK pixel on NVIDIA, undefined elsewhere. Read at effect creation:
			 * relaunch to flip. Born from the 7x7 black squares of projet-alpha's light-and-shadow-debug
			 * (a non-finite reflected colour on the screen-space lane, spread 5x5 by the SSR blur and 7x7
			 * by the TAA neighbourhood), whose source was never caught in the act: this is the instrument
			 * for the next occurrence — one key, one look, the guilty buffer. */
			constexpr auto GraphicsPPDebugNonFiniteKey{"Core/Graphics/PostProcessing/DebugNonFinite"};
			constexpr auto DefaultGraphicsPPDebugNonFinite{false};

			/* Post-processing > the OVERFLOW CENSUS starts ARMED (2026-09-26, scene-colour pre-exposure
			 * B1). The census counts, per frame, the texels of the scene-radiance images (the grabbed
			 * scene colour, the tone mapper's input, the traced effects' raw traces, the irradiance
			 * probe atlas) that are NaN, infinite, or finite at/above the binary16 ceiling (65 504) —
			 * what an RGBA16F target silently does to a physical luminance. It is ALWAYS created and
			 * DISARMED by default: disarmed it records nothing (one branch per chain call). Read once,
			 * when the census is created; `Core.RendererService.setOverflowCensus(1|0)` arms and
			 * disarms it live, and `Core.RendererService.testOverflowCensus()` is the positive control
			 * that must answer PASS on a machine before any count read there means anything. */
			constexpr auto GraphicsPPOverflowCensusEnabledKey{"Core/Graphics/PostProcessing/OverflowCensus/Enabled"};
			constexpr auto DefaultGraphicsPPOverflowCensusEnabled{false};

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

			/* Post-processing > CONCEPT-LEVEL knobs, shared by both lanes (owner decision, 2026-09-12).
			 * A parameter that exists in both lanes with the SAME meaning, the SAME unit and the SAME
			 * default is declared ONCE, at `<Concept>/<param>`, and both occupants of the slot read it.
			 * Only what describes ONE lane's mechanism stays under `<Concept>/RayTracing/` or
			 * `<Concept>/ScreenSpace/` (a ray origin bias vs a depth comparison bias, a march step
			 * count, a glossy cone, a pixel-doubling choice whose trade-off differs per lane).
			 * Rationale: the two occupants of a slot are an A/B of one concept on the same frame, and
			 * the A/B is only honest at equal settings. Until Sep 2026 the shared knobs were declared
			 * twice, with defaults that had silently diverged (`IndirectDiffuse` range 5 m against 8 m:
			 * in a 6 m corridor the screen-space lane could not light the opposite wall, which read as
			 * a technique gap and was a setting).
			 * ⚠️ C++ symbols carry no lane for these: `GraphicsPPIndirectDiffuseIntensityKey`, never
			 * `...RTIntensityKey`. ⚠️ projet-alpha does not reset its settings on a version bump: a
			 * file written before this change keeps the old `<Concept>/<Lane>/<param>` entries as
			 * orphans and gets the concept-level keys at their defaults — migrate the values by hand. */

			/* IndirectDiffuse — both lanes. */
			/* Rays (hemisphere samples) per pixel.
			 * Measured 2026-07-05 (Sponza+extras, RTX 3070 Ti @ 3840x1990): 8 spp is
			 * visually equivalent to 16 after the bilateral blur and ~16 ms/frame cheaper. */
			constexpr auto GraphicsPPIndirectDiffuseSampleCountKey{"Core/Graphics/PostProcessing/IndirectDiffuse/SampleCount"};
			constexpr auto DefaultGraphicsPPIndirectDiffuseSampleCount{8U};
			/* Maximum bounce range, in world units (a traced ray's length, a march's total distance).
			 * Both lanes fade a bounce over the LAST FIFTH of this range only. It was 5 m in the
			 * screen-space lane until Sep 2026, against 8 here. */
			constexpr auto GraphicsPPIndirectDiffuseMaxDistanceKey{"Core/Graphics/PostProcessing/IndirectDiffuse/MaxDistance"};
			constexpr auto DefaultGraphicsPPIndirectDiffuseMaxDistance{8.0F};
			/* Indirect lighting intensity multiplier. */
			constexpr auto GraphicsPPIndirectDiffuseIntensityKey{"Core/Graphics/PostProcessing/IndirectDiffuse/Intensity"};
			constexpr auto DefaultGraphicsPPIndirectDiffuseIntensity{0.8F};
			/* Bilateral denoising blur radius for GI, in pixels (inert while the SVGF denoiser runs). */
			constexpr auto GraphicsPPIndirectDiffuseBlurRadiusKey{"Core/Graphics/PostProcessing/IndirectDiffuse/BlurRadius"};
			constexpr auto DefaultGraphicsPPIndirectDiffuseBlurRadius{4U};
			/* Depth edge-stopping sigma for the GI bilateral blur. */
			constexpr auto GraphicsPPIndirectDiffuseDepthSigmaKey{"Core/Graphics/PostProcessing/IndirectDiffuse/DepthSigma"};
			constexpr auto DefaultGraphicsPPIndirectDiffuseDepthSigma{1.0F};
			/* Normal edge-stopping sigma for the GI bilateral blur. */
			constexpr auto GraphicsPPIndirectDiffuseNormalSigmaKey{"Core/Graphics/PostProcessing/IndirectDiffuse/NormalSigma"};
			constexpr auto DefaultGraphicsPPIndirectDiffuseNormalSigma{0.5F};

			/* IndirectDiffuse > Temporal accumulation (GIDenoiser resolve, both lanes).
			 * Exponential moving average over reprojected history: effective sample count
			 * becomes SampleCount / Alpha (8 spp @ 0.1 ≈ 80 effective samples). */
			constexpr auto GraphicsPPIndirectDiffuseTemporalEnabledKey{"Core/Graphics/PostProcessing/IndirectDiffuse/Temporal/Enabled"};
			constexpr auto DefaultGraphicsPPIndirectDiffuseTemporalEnabled{true};
			/* Blend weight of the CURRENT frame (lower = smoother, more history lag). */
			constexpr auto GraphicsPPIndirectDiffuseTemporalAlphaKey{"Core/Graphics/PostProcessing/IndirectDiffuse/Temporal/Alpha"};
			constexpr auto DefaultGraphicsPPIndirectDiffuseTemporalAlpha{0.1F};
			/* Relative camera-distance tolerance for history rejection (disocclusion test). */
			constexpr auto GraphicsPPIndirectDiffuseTemporalDepthToleranceKey{"Core/Graphics/PostProcessing/IndirectDiffuse/Temporal/DepthTolerance"};
			constexpr auto DefaultGraphicsPPIndirectDiffuseTemporalDepthTolerance{0.05F};
			/* Minimum cosine between current and history normals to accept history. */
			constexpr auto GraphicsPPIndirectDiffuseTemporalNormalThresholdKey{"Core/Graphics/PostProcessing/IndirectDiffuse/Temporal/NormalThreshold"};
			constexpr auto DefaultGraphicsPPIndirectDiffuseTemporalNormalThreshold{0.8F};
			/* Rectify the reprojected history against the current 3x3 neighbourhood statistics
			 * (anti-ghosting). Since 2026-08: VARIANCE CLIPPING (mean ± gamma * sigma, Salvi
			 * GDC 2016 — same technique as the TAA), no longer a min/max clamp.
			 * ⚠ DEFAULT OFF since the SVGF reorder (owner decision, measured 2026-08-06): the
			 * temporal resolve now integrates the RAW trace, and clipping against the raw 3x3
			 * statistics pulls the history toward the noisy local distribution — about 5% of
			 * GI energy lost on the Sponza corridor bench, no peak-to-peak gain. SVGF relies
			 * on the double disocclusion validation alone; the key remains for A/B. */
			constexpr auto GraphicsPPIndirectDiffuseTemporalNeighborhoodClampKey{"Core/Graphics/PostProcessing/IndirectDiffuse/Temporal/NeighborhoodClamp"};
			constexpr auto DefaultGraphicsPPIndirectDiffuseTemporalNeighborhoodClamp{false};
			/* Width of the variance-clipping bound, in standard deviations (gamma). Smaller =
			 * tighter anti-ghosting but slower convergence; larger = smoother accumulation. */
			constexpr auto GraphicsPPIndirectDiffuseTemporalVarianceGammaKey{"Core/Graphics/PostProcessing/IndirectDiffuse/Temporal/VarianceGamma"};
			constexpr auto DefaultGraphicsPPIndirectDiffuseTemporalVarianceGamma{1.0F};
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
			constexpr auto GraphicsPPIndirectDiffuseTemporalAnimatedNoiseKey{"Core/Graphics/PostProcessing/IndirectDiffuse/Temporal/AnimatedNoise"};
			constexpr auto DefaultGraphicsPPIndirectDiffuseTemporalAnimatedNoise{true};

			/* IndirectDiffuse > Denoiser (shared GIDenoiser component, SVGF, both lanes).
			 * À-trous iterations over the temporally integrated irradiance (5x5 kernel,
			 * footprint doubles each pass: 1, 2, 4, 8, 16 texels). 0 disables the spatial
			 * filter entirely (temporal resolve only — the A/B lever). Replaces the former
			 * shared bilateral blur H/V (the BlurRadius key is inert since then). */
			constexpr auto GraphicsPPIndirectDiffuseDenoiserIterationsKey{"Core/Graphics/PostProcessing/IndirectDiffuse/Denoiser/Iterations"};
			constexpr auto DefaultGraphicsPPIndirectDiffuseDenoiserIterations{4U};
			/* Luminance edge-stopping sigma, normalised by the LOCAL standard deviation
			 * (SVGF auto-dosage: noisy → smooth hard, converged → preserve detail).
			 * Larger = closer to a plain depth/normal bilateral (guidance off). */
			constexpr auto GraphicsPPIndirectDiffuseDenoiserLuminanceSigmaKey{"Core/Graphics/PostProcessing/IndirectDiffuse/Denoiser/LuminanceSigma"};
			constexpr auto DefaultGraphicsPPIndirectDiffuseDenoiserLuminanceSigma{4.0F};
			/* Per-pixel 1/N accumulation counter (SVGF): the temporal blend weight is
			 * max(1/(age+1), 1/MaxAccumulation) instead of the fixed Temporal/Alpha — fast
			 * convergence after a disocclusion (alpha 1, 1/2, 1/3...), tiny steady-state
			 * variance leak (about 0.8% at N=64 versus about 23% at fixed alpha 0.1, the
			 * factor that sank the first animated-noise attempt). Temporal/Alpha only rules
			 * when this is off (A/B lever). */
			constexpr auto GraphicsPPIndirectDiffuseDenoiserAccumulationCounterKey{"Core/Graphics/PostProcessing/IndirectDiffuse/Denoiser/AccumulationCounter"};
			constexpr auto DefaultGraphicsPPIndirectDiffuseDenoiserAccumulationCounter{true};
			/* Accumulation cap N (the steady-state blend weight floor is 1/N). Larger =
			 * smoother but slower to react to lighting changes. */
			constexpr auto GraphicsPPIndirectDiffuseDenoiserMaxAccumulationKey{"Core/Graphics/PostProcessing/IndirectDiffuse/Denoiser/MaxAccumulation"};
			constexpr auto DefaultGraphicsPPIndirectDiffuseDenoiserMaxAccumulation{64U};
			/* Debug view of the denoiser internals, drawn by the combine pass INSTEAD of the
			 * GI contribution: 0 = off, 1 = temporal variance (binary-amplified x1e6 — a linear
			 * scale is unreadable under the photometric exposure), 2 = accumulation age
			 * (white = young/disoccluded). Diagnostic only, costs nothing at 0. */
			constexpr auto GraphicsPPIndirectDiffuseDenoiserDebugViewKey{"Core/Graphics/PostProcessing/IndirectDiffuse/Denoiser/DebugView"};
			constexpr auto DefaultGraphicsPPIndirectDiffuseDenoiserDebugView{0U};

			/* AmbientOcclusion — both lanes. */
			/* AO darkening intensity multiplier (applied once, clamped; 1.0 = pure visibility term). */
			constexpr auto GraphicsPPAmbientOcclusionIntensityKey{"Core/Graphics/PostProcessing/AmbientOcclusion/Intensity"};
			constexpr auto DefaultGraphicsPPAmbientOcclusionIntensity{1.0F};

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
			 * ⚠️ Both default to ON since 2026-09-13 (owner decision): motion blur was off by
			 * default when the key was introduced, and was switched on on purpose afterwards.
			 * ⚠️ Camera motion blur smears every silhouette during a camera move BY DESIGN: rule it
			 * out before blaming the TAA for a trail (a 1/8000 s shutter at the same EV neutralises
			 * it). Measured 2026-09-26 on basic-scenery, a 1 m sideways step: 25-30 % of the pixels
			 * near silhouettes above 8/255 with it, 5-7 % without, for ONE frame per moving frame. */
			/* Post-processing > ContactShadows. The concept had NO settings key at all until Sep
			 * 2026, in either lane, which meant tuning it required a rebuild — for an effect whose
			 * whole quality question is "what value do these knobs want".
			 * `MaxDistance`, `NormalBias`, `Intensity` and `MaxBlurRadius` mean the SAME THING in the
			 * same units in both lanes (metres for the first, world units for the bias), so they are
			 * CONCEPT-LEVEL keys read by both occupants — the first slot to follow the rule, before
			 * it became the rule for every slot (see the concept-level block above). Only the
			 * screen-space lane's two knobs below are its own: they describe its mechanism and the
			 * traced one has no equivalent. */
			/* Maximum occluder search distance along the ray toward the light, in metres. */
			constexpr auto GraphicsPPContactShadowsMaxDistanceKey{"Core/Graphics/PostProcessing/ContactShadows/MaxDistance"};
			constexpr auto DefaultGraphicsPPContactShadowsMaxDistance{2.0F};
			/* Ray origin offset along the normal to prevent self-shadowing, in world units. */
			constexpr auto GraphicsPPContactShadowsNormalBiasKey{"Core/Graphics/PostProcessing/ContactShadows/NormalBias"};
			constexpr auto DefaultGraphicsPPContactShadowsNormalBias{0.01F};
			/* Shadow darkening multiplier (1.0 = full occlusion). */
			constexpr auto GraphicsPPContactShadowsIntensityKey{"Core/Graphics/PostProcessing/ContactShadows/Intensity"};
			constexpr auto DefaultGraphicsPPContactShadowsIntensity{0.8F};
			/* Upper bound of the distance-scaled penumbra blur, in pixels. */
			constexpr auto GraphicsPPContactShadowsMaxBlurRadiusKey{"Core/Graphics/PostProcessing/ContactShadows/MaxBlurRadius"};
			constexpr auto DefaultGraphicsPPContactShadowsMaxBlurRadius{10.0F};
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
			constexpr auto DefaultGraphicsPPMotionBlurEnabled{true};

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

			/* Volumetric clouds (Graphics::Effects::Atmosphere::VolumetricClouds, Sep 2026) — the pass that
			 * draws the scene's Scenes::Component::CloudVolume entities. A SCENE-DRIVEN effect: the stack
			 * files it the first frame the scene holds a cloud (PostProcessStack::syncSceneEffects()), an
			 * application never adds it. 'Enabled' = false declines that filing for the session. The look
			 * of a cloud belongs to its component; these are the integrator's cost/quality knobs. */
			constexpr auto GraphicsPPCloudsEnabledKey{"Core/Graphics/PostProcessing/Clouds/Enabled"};
			constexpr auto DefaultGraphicsPPCloudsEnabled{true};
			/* View-ray steps across the DIAGONAL of a cloud box (linear cost). Near the camera the step
			 * shrinks with the distance, down to an eighth of that. */
			constexpr auto GraphicsPPCloudsStepCountKey{"Core/Graphics/PostProcessing/Clouds/StepCount"};
			constexpr auto DefaultGraphicsPPCloudsStepCount{64U};
			/* Steps toward the sun inside the cloud, per view step: the self-shadowing. */
			constexpr auto GraphicsPPCloudsLightStepCountKey{"Core/Graphics/PostProcessing/Clouds/LightStepCount"};
			constexpr auto DefaultGraphicsPPCloudsLightStepCount{6U};
			/* Lambertian albedo of the ground UNDER the clouds, in [0, 1]: the bounce that lights their
			 * bottoms (L = albedo · E_ground / pi). An approximation of the integrator, not a scene fact. */
			constexpr auto GraphicsPPCloudsGroundAlbedoKey{"Core/Graphics/PostProcessing/Clouds/GroundAlbedo"};
			constexpr auto DefaultGraphicsPPCloudsGroundAlbedo{0.2F};
			/* The clouds' SHADOW on the world (stage 2, Sep 2026): a Beer shadow map seen from the main
			 * sun, square, centred on the camera and read by the sun term of every lit material
			 * (Graphics::CloudShadowMap). 'ShadowsEnabled' = false: the clouds cast nothing. */
			constexpr auto GraphicsPPCloudsShadowsEnabledKey{"Core/Graphics/PostProcessing/Clouds/ShadowsEnabled"};
			constexpr auto DefaultGraphicsPPCloudsShadowsEnabled{true};
			/* Side of the map, in texels. */
			constexpr auto GraphicsPPCloudsShadowResolutionKey{"Core/Graphics/PostProcessing/Clouds/ShadowResolution"};
			constexpr auto DefaultGraphicsPPCloudsShadowResolution{1024U};
			/* The LEAST side of the map, in metres: beyond it the clouds cast no shadow. The map sizes itself on the
			 * clouds (Scenes::CloudSet::AutomaticCoverageFactor times their mean width) and never goes below this. */
			constexpr auto GraphicsPPCloudsShadowCoverageKey{"Core/Graphics/PostProcessing/Clouds/ShadowCoverage"};
			constexpr auto DefaultGraphicsPPCloudsShadowCoverage{1024.0F};

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
			/* Depth comparison bias to prevent self-occlusion, in view-space units. */
			constexpr auto GraphicsPPAmbientOcclusionSSBiasKey{"Core/Graphics/PostProcessing/AmbientOcclusion/ScreenSpace/Bias"};
			constexpr auto DefaultGraphicsPPAmbientOcclusionSSBias{0.025F};
			/* Samples per pixel for screen-space ambient occlusion. */
			constexpr auto GraphicsPPAmbientOcclusionSSSampleCountKey{"Core/Graphics/PostProcessing/AmbientOcclusion/ScreenSpace/SampleCount"};
			constexpr auto DefaultGraphicsPPAmbientOcclusionSSSampleCount{32U};

			/* Screen Space > Global Illumination — the screen-space lane's OWN knobs: the two that
			 * describe its depth-buffer march and have no traced equivalent. Range, intensity,
			 * sample count, blur, temporal chain and denoiser are CONCEPT-LEVEL keys
			 * (`Core/Graphics/PostProcessing/IndirectDiffuse/<param>`, declared with the concept's
			 * `Enabled` key) that both occupants read. */
			/* Depth thickness assumed behind each depth sample, in view-space units. */
			constexpr auto GraphicsPPIndirectDiffuseSSThicknessKey{"Core/Graphics/PostProcessing/IndirectDiffuse/ScreenSpace/Thickness"};
			constexpr auto DefaultGraphicsPPIndirectDiffuseSSThickness{0.5F};
			/* Ray-march steps per ray. The step length is MaxDistance / StepCount, so raising the
			 * shared range without raising this thins the sampling. */
			constexpr auto GraphicsPPIndirectDiffuseSSStepCountKey{"Core/Graphics/PostProcessing/IndirectDiffuse/ScreenSpace/StepCount"};
			constexpr auto DefaultGraphicsPPIndirectDiffuseSSStepCount{16U};

			/* Screen Space > Global Illumination > SKY VISIBILITY (GTAO horizon search, Sep 2026).
			 * ⚠️ THE reason the two lanes disagreed in an enclosed space: RTGI's rays measure the
			 * real visibility of the sky (a ray that escapes returns sky radiance), SSGI had NO sky
			 * term at all, so the scene kept handing the raster its full, UNOCCLUDED irradiance
			 * cubemap — Sponza read 54.7/255 mean on the screen-space lane against 11.8 on the
			 * traced one, the sky lighting the galleries as if the courtyard had no roof. The
			 * horizon search gives the screen-space lane the visibility it lacked, and with it the
			 * indirect-diffuse OWNERSHIP (see PostProcessEffect::providesIndirectDiffuse()).
			 * @note Jimenez, Wu, Pesce, Jarabo, "Practical Realtime Strategies for Accurate Indirect
			 * Occlusion" (SIGGRAPH 2016 courses) for the slice integral and the bent normal; Intel
			 * XeGTAO (MIT) as the implementation reference. */
			/* Whether the screen-space lane computes its own sky visibility. Turning it OFF gives
			 * the indirect diffuse back to the raster (unoccluded sky) — the A/B of the defect. */
			constexpr auto GraphicsPPIndirectDiffuseSSSkyVisibilityEnabledKey{"Core/Graphics/PostProcessing/IndirectDiffuse/ScreenSpace/SkyVisibilityEnabled"};
			constexpr auto DefaultGraphicsPPIndirectDiffuseSSSkyVisibilityEnabled{true};
			/* How far the horizon search looks for an occluder of the SKY, in world units.
			 * ⚠️ NOT IndirectDiffuse/MaxDistance: that one is the BOUNCE range (8 m), and the two are
			 * independent by construction in the traced lane too — RTGI casts its ray to the FAR
			 * PLANE and only reads a bounce from a hit closer than MaxDistance ("nothing hit within
			 * 8 m does not mean sees the sky"). A roof 12 m above a courtyard occludes the sky
			 * without ever carrying a bounce. */
			constexpr auto GraphicsPPIndirectDiffuseSSSkyVisibilityRadiusKey{"Core/Graphics/PostProcessing/IndirectDiffuse/ScreenSpace/SkyVisibilityRadius"};
			constexpr auto DefaultGraphicsPPIndirectDiffuseSSSkyVisibilityRadius{16.0F};
			/* Directions sampled per pixel: each slice is a plane through the view vector, and the
			 * search walks BOTH of its sides. XeGTAO calls 3 "high quality". */
			constexpr auto GraphicsPPIndirectDiffuseSSSkyVisibilitySliceCountKey{"Core/Graphics/PostProcessing/IndirectDiffuse/ScreenSpace/SkyVisibilitySliceCount"};
			constexpr auto DefaultGraphicsPPIndirectDiffuseSSSkyVisibilitySliceCount{3U};
			/* Depth samples per slice SIDE. The step distribution is quadratic, so the near field
			 * keeps its detail while the last steps reach the radius. */
			constexpr auto GraphicsPPIndirectDiffuseSSSkyVisibilityStepCountKey{"Core/Graphics/PostProcessing/IndirectDiffuse/ScreenSpace/SkyVisibilityStepCount"};
			constexpr auto DefaultGraphicsPPIndirectDiffuseSSSkyVisibilityStepCount{6U};
			/* Fraction of the radius over which a far occluder fades out, in [0;1].
			 * ⚠️ Low ON PURPOSE, and NOT XeGTAO's 0.615: that value keeps an ARTISTIC ambient
			 * occlusion local, while a roof must occlude the sky at full strength however far it
			 * is. The fade only smooths the LAST FIFTH of the range so geometry does not pop as it
			 * crosses the boundary — the same choice, for the same reason, as the SSGI range fade. */
			constexpr auto GraphicsPPIndirectDiffuseSSSkyVisibilityFalloffRangeKey{"Core/Graphics/PostProcessing/IndirectDiffuse/ScreenSpace/SkyVisibilityFalloffRange"};
			constexpr auto DefaultGraphicsPPIndirectDiffuseSSSkyVisibilityFalloffRange{0.2F};

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

		/* The viewer's flat ambient illuminance, in lux — the DELIVERED illuminance (the ambient colour is a
		 * unit-luminance chromaticity since 2026-09-25).
		 *
		 * ⚠️ It exists as a FLOOR for the case where no background resource is available, and the sky
		 * irradiance dominates it by two orders of magnitude when one is. But ~80 lux of flat ambient
		 * is enough to WASH OUT a sheen rim or an iridescence fringe, which is precisely what the
		 * tests Khronos shoots on black are measuring. Dropping it to 0 is how those become
		 * judgeable; leaving the default keeps every existing viewer session identical.
		 * ⚠️ RENAMED 2026-09-25 from "Core/Viewers/AmbientIntensity": that key held 200 "lux" multiplied by the
		 * (0.4, 0.4, 0.45) colour, i.e. 80.72 lx delivered. A renamed key is the only way the look survives —
		 * projet-alpha never resets its settings, and a saved 200 read under the new meaning would be ×2.48. */
		constexpr auto ViewerAmbientIntensityKey{"Core/Viewers/AmbientIlluminance"};
		constexpr auto DefaultViewerAmbientIntensity{80.72F};

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
