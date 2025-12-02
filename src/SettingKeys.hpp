/*
 * src/SettingKeys.hpp
 * This file is part of Emeraude-Engine
 *
 * Copyright (C) 2010-2025 - Sébastien Léon Claude Christian Bémelmans "LondNoir" <londnoir@gmail.com>
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
 * https://github.com/londnoir/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#pragma once

namespace EmEn
{
	/* Core */
	constexpr auto CoreShowInformationKey{"Core/ShowInformation"}; // Logs
	constexpr auto DefaultCoreShowInformation{false};
	constexpr auto CoreEnableStatisticsKey{"Core/EnableStatistics"};
	constexpr auto DefaultCoreEnableStatistics{false};
	constexpr auto TextEditorKey{"Core/TextEditor"};
#if IS_LINUX
	constexpr auto DefaultTextEditor{"gedit"};
#elif IS_WINDOWS
	constexpr auto DefaultTextEditor{"notepad"};
#elif IS_MACOS
	constexpr auto DefaultTextEditor{"TextEdit"};
#endif

		/* Tracer */
		constexpr auto TracerPrintOnlyErrorsKey{"Core/Tracer/PrintOnlyErrors"};
		constexpr auto DefaultTracerPrintOnlyErrors{false};
		constexpr auto TracerEnableSourceLocationKey{"Core/Tracer/EnableSourceLocation"};
		constexpr auto DefaultTracerEnableSourceLocation{false};
		constexpr auto TracerEnableThreadInfosKey{"Core/Tracer/EnableThreadInfos"};
		constexpr auto DefaultTracerEnableThreadInfos{false};
		constexpr auto TracerEnableLoggerKey{"Core/Tracer/EnableLogger"};
		constexpr auto DefaultTracerEnableLogger{false};
		constexpr auto TracerLogFormatKey{"Core/Tracer/LogFormat"};
		constexpr auto DefaultTracerLogFormat{"Text"};

		/* Input manager */
		constexpr auto InputShowInformationKey{"Core/Input/ShowInformation"}; // Logs
		constexpr auto DefaultInputShowInformation{false};

		/* Resource manager */
		constexpr auto ResourcesShowInformationKey{"Core/Resources/ShowInformation"}; // Logs
		constexpr auto DefaultResourcesShowInformation{false};
		constexpr auto ResourcesDownloadEnabledKey{"Core/Resources/DownloadEnabled"};
		constexpr auto DefaultResourcesDownloadEnabled{true};
		constexpr auto ResourcesQuietConversionKey{"Core/Resources/QuietConversion"}; // Logs
		constexpr auto DefaultResourcesQuietConversion{true};
		constexpr auto ResourcesUseDynamicScanKey{"Core/Resources/UseDynamicScan"};
		constexpr auto DefaultResourcesUseDynamicScan{true};

		/* Audio layer */
		constexpr auto AudioEnableKey{"Core/Audio/Enable"};
		constexpr auto DefaultAudioEnable{true};
		constexpr auto AudioDeviceNameKey{"Core/Audio/DeviceName"};
		constexpr auto DefaultAudioDeviceName{"AutoDetect"};
		constexpr auto AudioPlaybackFrequencyKey{"Core/Audio/PlaybackFrequency"};
		constexpr auto DefaultAudioPlaybackFrequency{48000};
		constexpr auto AudioMasterVolumeKey{"Core/Audio/MasterVolume"};
		constexpr auto DefaultAudioMasterVolume{0.75F};
		constexpr auto AudioSFXVolumeKey{"Core/Audio/SFXVolume"};
		constexpr auto DefaultAudioSFXVolume{0.6F};
		constexpr auto AudioMusicVolumeKey{"Core/Audio/MusicVolume"};
		constexpr auto DefaultAudioMusicVolume{0.5F};
		constexpr auto AudioMusicChunkSizeKey{"Core/Audio/MusicChunkSize"};
		constexpr auto DefaultAudioMusicChunkSize{8192};
		constexpr auto AudioShowInformationKey{"Core/Audio/ShowInformation"}; // Logs
		constexpr auto DefaultAudioShowInformation{false};

			/* OpenAL */
			constexpr auto OpenALUseEFXExtensionsKey{"Core/Audio/OpenAL/UseEFXExtensions"};
			constexpr auto DefaultOpenALUseEFXExtensions{true};
			constexpr auto OpenALRefreshRateKey{"Core/Audio/OpenAL/RefreshRate"};
			constexpr auto DefaultOpenALRefreshRate{46};
			constexpr auto OpenALSyncStateKey{"Core/Audio/OpenAL/SyncState"};
			constexpr auto DefaultOpenALSyncState{0};
			constexpr auto OpenALMaxMonoSourceCountKey{"Core/Audio/OpenAL/MaxMonoSourceCount"};
			constexpr auto DefaultOpenALMaxMonoSourceCount{32};
			constexpr auto OpenALMaxStereoSourceCountKey{"Core/Audio/OpenAL/MaxStereoSourceCount"};
			constexpr auto DefaultOpenALMaxStereoSourceCount{2};

			/* Audio recorder */
			constexpr auto AudioRecorderEnableKey{"Core/Audio/Recorder/Enable"};
			constexpr auto DefaultAudioRecorderEnable{false};
			constexpr auto AudioRecorderDeviceNameKey{"Core/Audio/Recorder/DeviceName"};
			constexpr auto DefaultAudioRecorderDeviceName{"AutoDetect"};
			constexpr auto RecorderFrequencyKey{"Core/Audio/Recorder/Frequency"};
			constexpr auto DefaultRecorderFrequency{48000};
			constexpr auto RecorderBufferSizeKey{"Core/Audio/Recorder/BufferSize"};
			constexpr auto DefaultRecorderBufferSize{64};

		/* Video */
		constexpr auto VideoSavePropertiesAtExitKey{"Core/Video/SavePropertiesAtExit"};
		constexpr auto DefaultVideoSavePropertiesAtExit{true};
		constexpr auto VideoPreferredMonitorKey{"Core/Video/PreferredMonitor"};
		constexpr auto DefaultVideoPreferredMonitor{0};
		constexpr auto VideoEnableVSyncKey{"Core/Video/EnableVSync"};
		constexpr auto DefaultVideoEnableVSync{true};
		constexpr auto VideoEnableDoubleBufferingKey{"Core/Video/EnableDoubleBuffering"}; // Not in use
		constexpr auto DefaultEnableDoubleBuffering{false};
		constexpr auto VideoEnableTripleBufferingKey{"Core/Video/EnableTripleBuffering"};
		constexpr auto DefaultVideoEnableTripleBuffering{true};
		constexpr auto VideoEnableSRGBKey{"Core/Video/EnableSRGB"}; // Not in use
		constexpr auto DefaultEnableSRGB{false};
		constexpr auto VideoShowInformationKey{"Core/Video/ShowInformation"}; // Logs
		constexpr auto DefaultVideoShowInformation{false};

			/* Vulkan instance */
			constexpr auto VkInstanceEnableDebugKey{"Core/Video/VulkanInstance/EnableDebug"};
			constexpr auto DefaultVkInstanceEnableDebug{false};
			constexpr auto VkInstanceRequestedValidationLayersKey{"Core/Video/VulkanInstance/RequestedValidationLayers"};
			constexpr auto VkInstanceAvailableValidationLayersKey{"Core/Video/VulkanInstance/AvailableValidationLayers"};
			constexpr auto VkInstanceUseDebugMessengerKey{"Core/Video/VulkanInstance/UseDebugMessenger"};
			constexpr auto DefaultVkInstanceUseDebugMessenger{true};

			/* Vulkan device */
			constexpr auto VkDeviceAvailableGPUsKey{"Core/Video/VulkanDevice/AvailableGPUs"};
			constexpr auto VkDeviceAutoSelectModeKey{"Core/Video/VulkanDevice/AutoSelectMode"};
			constexpr auto DefaultVkDeviceAutoSelectMode{"Failsafe"};
			constexpr auto VkDeviceForceGPUKey{"Core/Video/VulkanDevice/ForceGPU"};
			constexpr auto VkDeviceUseVMAKey{"Core/Video/VulkanDevice/UseVMA"};
			constexpr auto DefaultVkDeviceUseVMA{true};
			/* Optimus workaround settings */
			constexpr auto VkDeviceOptimusWorkaroundKey{"Core/Video/VulkanDevice/OptimusWorkaround"};
			constexpr auto DefaultVkDeviceOptimusWorkaround{true};

			/* Window */
			constexpr auto WindowShowInformationKey{"Core/Video/Window/ShowInformation"}; // Logs
			constexpr auto DefaultWindowShowInformation{false};
			constexpr auto WindowAlwaysCenterOnStartupKey{"Core/Video/Window/AlwaysCenterOnStartup"};
			constexpr auto DefaultWindowAlwaysCenterOnStartup{false};
			constexpr auto WindowFramelessKey{"Core/Video/Window/Frameless"};
			constexpr auto DefaultWindowFrameless{false};
			constexpr auto WindowXPositionKey{"Core/Video/Window/XPosition"};
			constexpr auto DefaultWindowXPosition{64};
			constexpr auto WindowYPositionKey{"Core/Video/Window/YPosition"};
			constexpr auto DefaultWindowYPosition{64};
			constexpr auto WindowWidthKey{"Core/Video/Window/Width"};
			constexpr auto DefaultWindowWidth{1280U};
			constexpr auto WindowHeightKey{"Core/Video/Window/Height"};
			constexpr auto DefaultWindowHeight{720U};
			constexpr auto WindowGammaKey{"Core/Video/Window/Gamma"};
			constexpr auto DefaultWindowGamma{1.0F};

				/* GLFW */
				constexpr auto GLFWUsePlatformKey{"Core/Video/Window/GLFW/UsePlatform"};
				constexpr auto DefaultGLFWUsePlatform{"Auto"};
				constexpr auto GLFWEnableNativeCodeForVkSurfaceKey{"Core/Video/Window/GLFW/EnableNativeCodeForVkSurface"};
				constexpr auto DefaultEnableNativeCodeForVkSurface{false};
				constexpr auto GLFWWaylandEnableLibDecorKey{"Core/Video/Window/GLFW/Wayland/EnableLibDecor"};
				constexpr auto DefaultGLFWWaylandEnableLibDecor{true};
				constexpr auto GLFWX11UseXCBInsteadOfXLibKey{"Core/Video/Window/GLFW/X11/UseXCBInsteadOfXLib"};
				constexpr auto DefaultGLFWX11UseXCBInsteadOfXLib{true};

			/* Fullscreen */
			constexpr auto VideoFullscreenEnabledKey{"Core/Video/Fullscreen/Enabled"};
			constexpr auto DefaultVideoFullscreenEnabled{false};
			constexpr auto VideoFullscreenWidthKey{"Core/Video/Fullscreen/Width"};
			constexpr auto DefaultVideoFullscreenWidth{1920U};
			constexpr auto VideoFullscreenHeightKey{"Core/Video/Fullscreen/Height"};
			constexpr auto DefaultVideoFullscreenHeight{1080U};
			constexpr auto VideoFullscreenGammaKey{"Core/Video/Fullscreen/Gamma"};
			constexpr auto DefaultVideoFullscreenGamma{1.0F};
			constexpr auto VideoFullscreenRefreshRateKey{"Core/Video/Fullscreen/RefreshRate"};
			constexpr auto DefaultVideoFullscreenRefreshRate{-1};

			/* Overlay */
			constexpr auto OverlayForceScaleKey{"Core/Video/Overlay/ForceScale"};
			constexpr auto DefaultOverlayForceScale{false};
			constexpr auto OverlayScaleXKey{"Core/Video/Overlay/ScaleX"};
			constexpr auto OverlayScaleYKey{"Core/Video/Overlay/FScaleY"};
			constexpr auto DefaultOverlayScale{1.0F};

			/* Framebuffer */
			constexpr auto VideoFramebufferRedBitsKey{"Core/Video/Framebuffer/RedBits"};
			constexpr auto DefaultVideoFramebufferRedBits{8U};
			constexpr auto VideoFramebufferGreenBitsKey{"Core/Video/Framebuffer/GreenBits"};
			constexpr auto DefaultVideoFramebufferGreenBits{8U};
			constexpr auto VideoFramebufferBlueBitsKey{"Core/Video/Framebuffer/BlueBits"};
			constexpr auto DefaultVideoFramebufferBlueBits{8U};
			constexpr auto VideoFramebufferAlphaBitsKey{"Core/Video/Framebuffer/AlphaBits"};
			constexpr auto DefaultVideoFramebufferAlphaBits{8U};
			constexpr auto VideoFramebufferDepthBitsKey{"Core/Video/Framebuffer/DepthBits"};
			constexpr auto DefaultVideoFramebufferDepthBits{32U};
			constexpr auto VideoFramebufferStencilBitsKey{"Core/Video/Framebuffer/StencilBits"};
			constexpr auto DefaultVideoFramebufferStencilBits{0U};
			constexpr auto VideoFramebufferSamplesKey{"Core/Video/Framebuffer/Samples"};
			constexpr auto DefaultVideoFramebufferSamples{1U};
			constexpr auto VideoFramebufferEnableMLAAKey{"Core/Video/Framebuffer/EnableMLAA"};
			constexpr auto DefaultVideoEnableMLAA{false};

		/* Graphics */
		constexpr auto GraphicsViewDistanceKey{"Core/Graphics/ViewDistance"};
		constexpr auto DefaultGraphicsViewDistance{10000.0F}; /* NOTE: 10km */
		constexpr auto GraphicsFieldOfViewKey{"Core/Graphics/FieldOfView"};
		constexpr auto DefaultGraphicsFieldOfView{85.0F}; /* NOTE: 85° */

			/* Texture */
			constexpr auto GraphicsTextureMagFilteringKey{"Core/Graphics/Texture/MagFilter"};
			constexpr auto GraphicsTextureMinFilteringKey{"Core/Graphics/Texture/MinFilter"};
			constexpr auto GraphicsTextureMipFilteringKey{"Core/Graphics/Texture/MipFilter"};
			constexpr auto DefaultGraphicsTextureFiltering{"nearest"};
			constexpr auto GraphicsTextureMipMappingLevelsKey{"Core/Graphics/Texture/MipMappingLevels"};
			constexpr auto DefaultGraphicsTextureMipMappingLevels{1};
			constexpr auto GraphicsTextureAnisotropyLevelsKey{"Core/Graphics/Texture/AnisotropyLevels"};
			constexpr auto DefaultGraphicsTextureAnisotropy{0};
			constexpr auto GraphicsTextureViewDistanceKey{"Core/Graphics/Texture/ViewDistance"};
			constexpr auto DefaultGraphicsTextureViewDistance{5000.0F}; /* NOTE: 5km */

			/* Shadow Mapping */
			constexpr auto GraphicsShadowMappingEnabledKey{"Core/Graphics/ShadowMapping/Enabled"};
			constexpr auto DefaultGraphicsShadowMappingEnabled{true};
			constexpr auto GraphicsShadowMappingBaseResolutionKey{"Core/Graphics/ShadowMapping/BaseResolution"};
			constexpr auto DefaultGraphicsShadowMappingBaseResolution{512U};
			constexpr auto GraphicsShadowMappingPCFSampleKey{"Core/Graphics/ShadowMapping/PCFSample"};
			constexpr auto DefaultGraphicsShadowMappingPCFSample{0U};
			constexpr auto GraphicsShadowMappingPCFRadiusKey{"Core/Graphics/ShadowMapping/PCFRadius"};
			constexpr auto DefaultGraphicsShadowMappingPCFRadius{1.0F};
			constexpr auto GraphicsShadowMappingViewDistanceKey{"Core/Graphics/ShadowMapping/ViewDistance"};
			constexpr auto DefaultGraphicsShadowMappingViewDistance{5000.0F}; /* NOTE: 5km */

			/* Shader */
			constexpr auto ShowSourceCodeKey{"Core/Graphics/Shader/ShowSourceCode"}; // Logs
			constexpr auto DefaultShowSourceCode{false};
			constexpr auto SourceCodeCacheEnabledKey{"Core/Graphics/Shader/EnableSourceCodeCache"};
			constexpr auto DefaultSourceCodeCacheEnabled{false};
			constexpr auto BinaryCacheEnabledKey{"Core/Graphics/Shader/EnableBinaryCache"};
			constexpr auto DefaultBinaryCacheEnabled{false};
			constexpr auto NormalMappingEnabledKey{"Core/Graphics/Shader/EnableNormalMapping"};
			constexpr auto DefaultNormalMappingEnabled{true};
			constexpr auto HighQualityLightEnabledKey{"Core/Graphics/Shader/EnabledHighQualityLight"};
			constexpr auto DefaultHighQualityLightEnabled{false};
			constexpr auto HighQualityReflectionEnabledKey{"Core/Graphics/Shader/EnableHighQualityReflection"};
			constexpr auto DefaultHighQualityReflectionEnabled{false};

		/* Physics */
		constexpr auto EnablePhysicsAccelerationKey{"Core/Physics/EnableAcceleration"};
		constexpr auto DefaultEnablePhysicsAcceleration{false};
}
