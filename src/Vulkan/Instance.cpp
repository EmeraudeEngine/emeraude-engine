/*
 * src/Vulkan/Instance.cpp
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

#include "Instance.hpp"

/* Project configuration. */
#include "emeraude_platform.hpp"

/* STL inclusions. */
#include <algorithm>
#include <array>
#include <charconv>
#include <cstring>
#include <limits>
#include <optional>
#include <ranges>
#include <span>
#include <sstream>
#include <string_view>
#include <system_error>
#include <tuple>
#include <utility>
#include <vector>

/* Third-party inclusions. */
#include "GLFW/glfw3.h"
#include "magic_enum/magic_enum.hpp"

/* Local inclusions. */
#include "Arguments.hpp"
#include "Device.hpp"
#include "DeviceRequirements.hpp"
#include "Identification.hpp"
#include "PhysicalDevice.hpp"
#include "PortabilitySubset.hpp"
#include "PrimaryServices.hpp"
#include "SettingKeys.hpp"
#include "String.hpp"
#include "Utility.hpp"
#include "Window.hpp"

namespace EmEn::Vulkan
{
	using namespace Base;

	void
	Instance::readSettings () noexcept
	{
		const auto & arguments = m_primaryServices.arguments();
		auto & settings = m_primaryServices.settings();

		m_showInformation =
			settings.getOrSetDefault< bool >(VideoShowInformationKey, DefaultVideoShowInformation) ||
			arguments.isSwitchPresent("--show-all-infos") ||
			arguments.isSwitchPresent("--show-video-infos");

		/* NOTE: --set-vk-layers overrides the validation settings for this run only: they are neither read nor written. */
		m_requestedValidationLayers.clear();
		m_validationLayersFromArgument = false;

		if ( const auto layersArgument = arguments.get(SetValidationLayersArgument) )
		{
			if ( auto layers = Instance::parseValidationLayersArgument(layersArgument.value()) )
			{
				m_requestedValidationLayers = std::move(layers.value());
				m_validationLayersFromArgument = true;

				TraceInfo{ClassId} <<
					SetValidationLayersArgument << " : " << m_requestedValidationLayers.size() << " validation layer(s) for this run, "
					"the validation settings are neither read nor written.";
			}
		}
		else if ( arguments.isSwitchPresent(SetValidationLayersArgument) )
		{
			TraceError{ClassId} <<
				SetValidationLayersArgument << " needs a value: " << SetValidationLayersArgument << "=LAYER_A,LAYER_B "
				"(an empty value runs without any layer). The settings apply.";
		}

		if ( m_validationLayersFromArgument )
		{
			m_debugMode = !m_requestedValidationLayers.empty() || arguments.isSwitchPresent("--debug-vulkan");
		}
		else
		{
			m_debugMode =
				arguments.isSwitchPresent("--debug-vulkan") ||
				settings.getOrSetDefault< bool >(VkInstanceEnableDebugKey, DefaultVkInstanceEnableDebug);
		}

		/* NOTE: --vulkan-api-target LOWERS the target for this run only (a downstream application's path tested on any
		 * machine); it never raises the application's target. */
		if ( const auto targetArgument = arguments.get(VulkanAPITargetArgument) )
		{
			const auto & text = targetArgument.value();
			const auto separator = text.find('.');
			uint32_t major = 0;
			uint32_t minor = 0;

			const auto parse = [] (std::string_view part, uint32_t & value) {
				const auto [end, error] = std::from_chars(part.data(), part.data() + part.size(), value);

				return error == std::errc{} && end == part.data() + part.size() && !part.empty();
			};

			if ( separator == std::string::npos || !parse(std::string_view{text}.substr(0, separator), major) || !parse(std::string_view{text}.substr(separator + 1), minor) || major != 1 || minor > 3 )
			{
				TraceError{ClassId} << VulkanAPITargetArgument << "=" << text << " is not a Vulkan version (1.1, 1.2 or 1.3): ignored.";
			}
			else if ( const auto requested = VK_MAKE_API_VERSION(0, major, minor, 0); requested > m_targetAPIVersion )
			{
				TraceWarning{ClassId} << VulkanAPITargetArgument << "=" << text << " cannot raise the application's target (Vulkan " << VK_API_VERSION_MAJOR(m_targetAPIVersion) << '.' << VK_API_VERSION_MINOR(m_targetAPIVersion) << "): ignored.";
			}
			else
			{
				this->setTargetAPIVersion(requested);

				TraceInfo{ClassId} << VulkanAPITargetArgument << " : Vulkan " << VK_API_VERSION_MAJOR(m_targetAPIVersion) << '.' << VK_API_VERSION_MINOR(m_targetAPIVersion) << " targeted for this run.";
			}
		}
		else if ( arguments.isSwitchPresent(VulkanAPITargetArgument) )
		{
			TraceError{ClassId} << VulkanAPITargetArgument << " needs a value: " << VulkanAPITargetArgument << "=1.1. Ignored.";
		}

		/* NOTE: The debug messenger create-info must be ready before vkCreateInstance (instance pNext).
		 * It is only actually wired when validation layers are requested (see isUsingDebugMessenger()). */
		if ( this->isDebugModeEnabled() )
		{
			m_debugCreateInfo = DebugMessenger::getCreateInfo();
		}

		/* NOTE: Check the automatic device selection mode. */
		const auto autoSelectModeString = settings.getOrSetDefault< std::string >(VkDeviceAutoSelectModeKey, DefaultVkDeviceAutoSelectMode);

		m_autoSelectMode = magic_enum::enum_cast< DeviceAutoSelectMode >(autoSelectModeString).value_or(DeviceAutoSelectMode::Performance);
		m_enableFailSafe = settings.getOrSetDefault< bool >(VkDeviceEnableFailSafeKey, DefaultEnableFailSafe);

		/* NOTE: Usage of Vulkan Memory Allocator from AMD instead of manual memory management. */
		m_useVulkanMemoryAllocator = settings.getOrSetDefault< bool >(VkDeviceUseVMAKey, DefaultVkDeviceUseVMA);
	}

	bool
	Instance::onInitialize () noexcept
	{
		this->readSettings();

		/* NOTE: Debug mode only. EnableDebug is the master switch: when on, this exposes the
		 * available validation layers in the settings file (to help edit RequestedValidationLayers)
		 * and selects the requested ones. When off, nothing debug-related touches the settings.
		 * With --set-vk-layers, the argument decides alone and the settings are not touched at all. */
		if ( this->isDebugModeEnabled() )
		{
			this->configureValidationLayers();
		}

		this->configureInstanceExtensions();

		/* Vulkan instance creation */
		{
			m_applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
			m_applicationInfo.pNext = nullptr;
			m_applicationInfo.pApplicationName = m_identification.applicationId().c_str();
			m_applicationInfo.applicationVersion = VK_MAKE_VERSION(m_identification.applicationVersion().major(), m_identification.applicationVersion().minor(), m_identification.applicationVersion().revision());
			m_applicationInfo.pEngineName = EmEn::EngineName;
			m_applicationInfo.engineVersion = VK_MAKE_VERSION(Identification::EngineVersion.major(), Identification::EngineVersion.minor(), Identification::EngineVersion.revision());
			/* [VULKAN-API-SETUP] Vulkan API version selection. */
			if constexpr ( IsMacOS )
			{
				/* NOTE: macOS don't support the Vulkan API.
				 * MoltenVK is used to translate commands to Metal API, some features can be unsupported. */
				m_applicationInfo.apiVersion = m_targetAPIVersion;
				m_createInfo.flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
			}
			else
			{
				m_applicationInfo.apiVersion = m_targetAPIVersion;
				m_createInfo.flags = 0;
			}
			m_createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
			m_createInfo.pNext = this->isUsingDebugMessenger() ? &m_debugCreateInfo : nullptr;
			m_createInfo.pApplicationInfo = &m_applicationInfo;
			m_createInfo.enabledLayerCount = static_cast< uint32_t >(m_requiredValidationLayers.size());
			m_createInfo.ppEnabledLayerNames = m_requiredValidationLayers.data();
			m_createInfo.enabledExtensionCount = static_cast< uint32_t >(m_requiredInstanceExtensions.size());
			m_createInfo.ppEnabledExtensionNames = m_requiredInstanceExtensions.data();

			/* Initialize RenderDoc capture API (must happen before vkCreateInstance). */
			static_cast< void >(m_renderDocCapture.initialize());

			/* At this point, we create the vulkan instance.
			 * Beyond this point, Vulkan is in the pipe and usable. */
			auto result = vkCreateInstance(&m_createInfo, nullptr, &m_instance);

			/* NOTE: A debug layer never decides whether the application starts (owner decision 2026-10-11): a creation
			 * that fails with layers requested is retried without any, loudly. Measured on Windows: a layer library the
			 * loader cannot open made vkCreateInstance() answer VK_ERROR_OUT_OF_HOST_MEMORY and stopped the launch. */
			if ( result != VK_SUCCESS && !m_requiredValidationLayers.empty() )
			{
				{
					TraceWarning trace{ClassId};

					trace << "Unable to create the Vulkan instance with the validation layer(s)";

					for ( const auto * layer : m_requiredValidationLayers )
					{
						trace << " '" << layer << "'";
					}

					trace << " (" << vkResultToCString(result) << ") : created again WITHOUT any layer. This run is NOT validated !";
				}

				m_requiredValidationLayers.clear();
				m_createInfo.pNext = nullptr;
				m_createInfo.enabledLayerCount = 0;
				m_createInfo.ppEnabledLayerNames = nullptr;
				m_instance = VK_NULL_HANDLE;

				result = vkCreateInstance(&m_createInfo, nullptr, &m_instance);
			}

			m_creationResult = result;

			if ( result != VK_SUCCESS )
			{
				switch ( result )
				{
					case VK_ERROR_OUT_OF_HOST_MEMORY :
						Tracer::fatal(ClassId, "The host system is out of memory !");
						break;

					case VK_ERROR_OUT_OF_DEVICE_MEMORY :
						Tracer::fatal(ClassId, "The device is out of memory !");
						break;

					case VK_ERROR_INITIALIZATION_FAILED :
						Tracer::fatal(ClassId, "The Vulkan instance failed to initialize ! (No specific info)");
						break;

					case VK_ERROR_LAYER_NOT_PRESENT :
						TraceFatal{ClassId} << "Unable to create a Vulkan instance : " << vkResultToCString(result) << " !";
						break;

					case VK_ERROR_EXTENSION_NOT_PRESENT :
					{
						TraceFatal trace{ClassId};

						trace <<
							"Unable to create a Vulkan instance : " << vkResultToCString(result) << " !" "\n"
							"Required extensions :" "\n";

						for ( const auto & extension : m_requiredInstanceExtensions )
						{
							trace << '\t' << extension << '\n';
						}

						trace << getItemListAsString("Instance", Instance::getExtensions(nullptr));
					}
						break;

					case VK_ERROR_INCOMPATIBLE_DRIVER :
						Tracer::fatal(ClassId, "Incompatible driver !");
						break;

					default:
						break;
				}

				return false;
			}
		}

		/* NOTE: Every instance- and device-level function now comes from the engine's loader through volk. Device
		 * functions are NOT loaded per device (volkLoadDevice()): the instance dispatch serves the graphics AND the
		 * compute device alike. */
		volkLoadInstance(m_instance);

		/* Register the Vulkan instance handle with RenderDoc for capture operations. */
		m_renderDocCapture.setDevice(m_instance);

		/* NOTE: When debugging, we want to re-route the validation layer messages to the engine tracer. */
		if ( this->isUsingDebugMessenger() )
		{
			m_debugMessenger = std::make_unique< DebugMessenger >(*this);
			m_debugMessenger->setIdentifier(ClassId, "Main", "DebugMessenger");

			if ( !m_debugMessenger->isCreated() )
			{
				Tracer::warning(ClassId, "Unable to activate the validation layers debug messenger !");
			}
		}

		/* Probe all usable physical devices. */
		if ( !this->preparePhysicalDevices() )
		{
			TraceFatal{ClassId} << "No physical device available !";

			return false;
		}

		return true;
	}

	Instance::~Instance ()
	{
		/* NOTE: onTerminate() is idempotent (every handle is reset after its release); it is only needed when it did
		 * not run. */
		if ( m_instance != VK_NULL_HANDLE || m_graphicsDevice != nullptr || m_computeDevice != nullptr )
		{
			static_cast< void >(this->onTerminate());
		}
	}

	bool
	Instance::onTerminate () noexcept
	{
		/* Checking device usage to print out some closing resources bugs. */
		if ( m_computeDevice != nullptr )
		{
			m_computeDevice->destroy();

			if ( m_computeDevice.use_count() > 1 )
			{
				TraceError{ClassId} << "The Vulkan selected compute device '" << m_computeDevice->identifier() << "' smart pointer still have " << m_computeDevice.use_count() << " uses !";
			}

			m_computeDevice.reset();
		}

		if ( m_graphicsDevice != nullptr )
		{
			m_graphicsDevice->destroy();

			if ( m_graphicsDevice.use_count() > 1 )
			{
				TraceError{ClassId} << "The Vulkan selected graphics device '" << m_graphicsDevice->identifier() << "' smart pointer still have " << m_graphicsDevice.use_count() << " uses !";
			}

			m_graphicsDevice.reset();
		}

		m_physicalDevices.clear();
		m_debugMessenger.reset();
		m_renderDocCapture.shutdown();

		if ( m_instance != VK_NULL_HANDLE )
		{
			vkDestroyInstance(m_instance, nullptr);

			m_instance = VK_NULL_HANDLE;
		}

		m_requiredValidationLayers.clear();
		m_requestedValidationLayers.clear();
		m_requiredInstanceExtensions.clear();
		m_requiredGraphicsDeviceExtensions.clear();

		return true;
	}

	bool
	Instance::preparePhysicalDevices () noexcept
	{
		std::vector< VkPhysicalDevice > physicalDevices{};

		/* NOTE: VK_INCOMPLETE is a SUCCESS code: the device list changed between the count and the fill (a hybrid laptop
		 * whose discrete GPU appears late). Treating it as a failure stopped the engine on capable Windows AMD laptops
		 * (engine item physical-device-enumeration-refuses-vk-incomplete, user crash reports 2026-10-15). */
		const auto outcome = enumerate(physicalDevices, [this] (uint32_t * count, VkPhysicalDevice * data) {
			return vkEnumeratePhysicalDevices(m_instance, count, data);
		});

		if ( !outcome.succeeded() )
		{
			TraceError{ClassId} << "Unable to enumerate the physical devices : " << vkResultToCString(outcome.result) << " !";

			return false;
		}

		if ( outcome.retries > 0 || outcome.result == VK_INCOMPLETE )
		{
			TraceWarning{ClassId} << "The physical device list changed during its enumeration (" << outcome.retries << " retr" << ( outcome.retries == 1 ? "y" : "ies" ) << ", last answer " << vkResultToCString(outcome.result) << ") : the last list is used.";
		}

		TraceInfo{ClassId} << physicalDevices.size() << " physical device(s) found.";

		m_physicalDevices.reserve(physicalDevices.size());

		std::ranges::transform(std::as_const(physicalDevices), std::back_inserter(m_physicalDevices), [this] (const auto & physicalDevice) {
			return std::make_shared< PhysicalDevice >(physicalDevice, m_targetAPIVersion);
		});

		return !m_physicalDevices.empty();
	}

	bool
	Instance::isLikelyMobileGPU (const std::string & gpuName) noexcept
	{
		/* Mobile/Laptop GPU naming patterns used by Nvidia:
		 * - "GeForce RTX 3080 Mobile"
		 * - "GeForce RTX 4070 Laptop GPU"
		 * - "Quadro RTX 3000 Max-Q"
		 * - "GeForce MX450" (MX series are always mobile)
		 * Desktop GPUs never have these keywords in their names. */
		static const std::array< std::string, 4 > mobilePatterns = {
			"Mobile",
			"Laptop",
			"Max-Q",
			" MX"  /* Space prefix to avoid matching other patterns like "RTX" */
		};

		return std::ranges::any_of(mobilePatterns, [&gpuName] (const std::string & pattern) {
			return gpuName.find(pattern) != std::string::npos;
		});
	}

	HybridGPUConfig
	Instance::detectHybridGPUConfiguration () const noexcept
	{
		HybridGPUConfig config{};

		const PhysicalDevice * integratedGPU = nullptr;
		const PhysicalDevice * discreteGPU = nullptr;

		for ( const auto & device : m_physicalDevices )
		{
			const auto & props = device->propertiesVK10();
			const auto deviceType = props.deviceType;
			const auto vendorID = props.vendorID;

			/* Detect iGPU (Intel: 0x8086, AMD: 0x1002) */
			if ( deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU )
			{
				integratedGPU = device.get();
				config.integratedGPUName = props.deviceName;
				config.integratedVendorID = vendorID;
			}

			/* Detect discrete Nvidia GPU (0x10DE) */
			if ( deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && toVendor(vendorID) == Vendor::Nvidia )
			{
				discreteGPU = device.get();
				config.discreteGPUName = props.deviceName;
				config.discreteVendorID = vendorID;
			}
		}

		/* Optimus detection:
		 * 1. iGPU must be present
		 * 2. Nvidia dGPU must be present
		 * 3. The Nvidia GPU must be a MOBILE variant (Laptop/Mobile/Max-Q/MX series)
		 *
		 * This prevents false positives on desktops where the CPU has an iGPU
		 * (e.g., Intel UHD) but the RTX has its own display outputs. */
		const bool hasHybridConfig = (integratedGPU != nullptr) && (discreteGPU != nullptr);
		const bool isMobileNvidia = !config.discreteGPUName.empty() && Instance::isLikelyMobileGPU(config.discreteGPUName);

		config.isOptimusDetected = hasHybridConfig && isMobileNvidia;

		/* NOTE: Log warning if hybrid config detected but not identified as Optimus.
		 * This helps users understand why Failsafe isn't triggered. */
		if ( hasHybridConfig && !isMobileNvidia )
		{
			config.isHybridNonOptimus = true;
		}

		return config;
	}
	
	bool
	Instance::shouldExcludeForFailsafe (const HybridGPUConfig & hybridConfig, const std::shared_ptr< PhysicalDevice > & physicalDevice) const noexcept
	{
		/* NOTE: Optimus bug only manifests in windowed mode during swap-chain recreation (resize).
		 * In fullscreen exclusive mode, the dGPU is safe to use.
		 * skip Nvidia dGPU on Optimus configuration in windowed mode.
		 * The Optimus bug only manifests during swap-chain recreation (window resize).*/
		if ( !m_fullscreenExclusiveEnabled && hybridConfig.isOptimusDetected )
		{
			const auto & props = physicalDevice->propertiesVK10();

			/* Exclude only Nvidia discrete GPU on Optimus configuration in windowed mode */
			if ( toVendor(props.vendorID) == Vendor::Nvidia && props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU )
			{
				TraceWarning{ClassId} << "Failsafe: Excluding '" << physicalDevice->propertiesVK10().deviceName << "' (Nvidia Optimus + windowed mode - known driver issues with swap-chain resize)";

				return true;
			}
		}

		/* NOTE: Other exclusion case will follow... */

		return false;
	}

	void
	Instance::configureValidationLayers () noexcept
	{
		/* [VULKAN-API-SETUP] Vulkan validation layers selection. */
		auto & settings = m_primaryServices.settings();

		const auto availableValidationLayers = Instance::getAvailableValidationLayers();

		/* NOTE: Save a copy of validation layers in settings for an easy settings edition (not for a run that
		 * overrides them from the command line: such a run leaves the settings untouched). */
		if ( !m_validationLayersFromArgument && settings.isArrayEmpty(VkInstanceAvailableValidationLayersKey) )
		{
			settings.clearArray(VkInstanceAvailableValidationLayersKey);

			for ( const auto & availableValidationLayer : availableValidationLayers )
			{
				settings.setInArray(VkInstanceAvailableValidationLayersKey, availableValidationLayer.layerName);
			}
		}

		/* NOTE: Show available validation layers on the current system. */
		if ( m_showInformation )
		{
			TraceInfo{ClassId} << getItemListAsString(availableValidationLayers);
		}

		if ( !m_validationLayersFromArgument )
		{
			/* NOTE: Read the settings to get the desired validation layers. */
			m_requestedValidationLayers = settings.getArrayAs< std::string >(VkInstanceRequestedValidationLayersKey);

			if ( m_requestedValidationLayers.empty() )
			{
				TraceInfo{ClassId} <<
					"No validation layer is requested from settings !" "\n"
					"NOTE: You can change the validation layers selected in settings at the array key : '" << VkInstanceRequestedValidationLayersKey << "', "
					"or for one run with " << SetValidationLayersArgument << "=LAYER_A,LAYER_B.";

				return;
			}
		}
		else if ( m_requestedValidationLayers.empty() )
		{
			TraceInfo{ClassId} << "No validation layer for this run (" << SetValidationLayersArgument << " is empty) : the settings are ignored.";

			return;
		}

		/* NOTE: Show desired validation layers. */
		if ( m_showInformation )
		{
			TraceInfo trace{ClassId};

			trace << "Desired Vulkan validation layers from " << ( m_validationLayersFromArgument ? "the command line" : "settings" ) << " :" "\n";

			for ( const auto & requestedValidationLayer : m_requestedValidationLayers )
			{
				trace << "\t" << requestedValidationLayer << "\n";
			}
		}

		/* NOTE: Here we check if the desired validations layers are available and create the vector for the instance createInfo.
		 * The pointers point into m_requestedValidationLayers, which is not modified again while the instance lives. */
		m_requiredValidationLayers = Instance::getSupportedValidationLayers(m_requestedValidationLayers, availableValidationLayers);

		if ( m_requiredValidationLayers.empty() )
		{
			Tracer::warning(ClassId, "None of the Vulkan validation layers requested are available on this system ! Check your dev Vulkan setup.");
		}
		else
		{
			TraceInfo trace{ClassId};

			trace << "Vulkan validation layers selected :" "\n";

			for ( const auto & requiredValidationLayer : m_requiredValidationLayers )
			{
				trace << "\t" << requiredValidationLayer << "\n";
			}
		}
	}

	void
	Instance::configureInstanceExtensions () noexcept
	{
		/* [VULKAN-API-SETUP] Vulkan instance extensions selection. */

		/* NOTE: Show available extensions on the current system. */
		if ( m_showInformation )
		{
			TraceInfo{ClassId} << getItemListAsString("Instance", Instance::getExtensions(nullptr));
		}

		/* NOTE: Set extension requested by GLFW. */
		uint32_t glfwExtensionCount = 0;

		const auto * glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		for ( uint32_t index = 0; index < glfwExtensionCount; index++ )
		{
			m_requiredInstanceExtensions.emplace_back(glfwExtensions[index]);
		}

		/* NOTE: EnableDebug is the master switch for the debug-utils channel: object/label naming
		 * for RenderDoc & Nsight, plus support for the (layer-driven) validation messenger. */
		if ( this->isDebugModeEnabled() )
		{
			/* NOTE: VK_EXT_debug_report (vk 1.0) has been deprecated in favor of VK_EXT_debug_utils (vk 1.2 ?). */
			//m_requiredInstanceExtensions.emplace_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
			m_requiredInstanceExtensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}

		/* NOTE: Specific for MoltenVK. */
		if constexpr ( IsMacOS )
		{
			/* NOTE: This extension allows applications to control whether devices
			 * that expose the VK_KHR_portability_subset extension are included in
			 * the results of physical device enumeration. */
			m_requiredInstanceExtensions.emplace_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
		}

		if ( m_showInformation )
		{
			if ( m_requiredInstanceExtensions.empty() )
			{
				Tracer::info(ClassId, "No extension required.");
			}
			else
			{
				TraceInfo trace{ClassId};

				trace << "Required extensions :" "\n";

				for ( const auto & requiredExtension : m_requiredInstanceExtensions )
				{
					trace << "\t" << requiredExtension << "\n";
				}
			}
		}
	}

	std::vector< VkLayerProperties >
	Instance::getAvailableValidationLayers () noexcept
	{
		std::vector< VkLayerProperties > validationLayers;

		/* NOTE: VK_INCOMPLETE is a success code (the list changed between the two calls): enumerate() handles it. */
		if ( const auto outcome = enumerate(validationLayers, [&] (uint32_t * count, auto * data) { return vkEnumerateInstanceLayerProperties(count, data); }); !outcome.succeeded() )
		{
			TraceError{ClassId} << "Unable to get instance validation layers : " << vkResultToCString(outcome.result) << " !";
		}

		return validationLayers;
	}

	std::vector< VkExtensionProperties >
	Instance::getExtensions (const char * pLayerName) noexcept
	{
		std::vector< VkExtensionProperties > extensions{};

		/* NOTE: VK_INCOMPLETE is a success code (the list changed between the two calls): enumerate() handles it. */
		if ( const auto outcome = enumerate(extensions, [&] (uint32_t * count, auto * data) { return vkEnumerateInstanceExtensionProperties(pLayerName, count, data); }); !outcome.succeeded() )
		{
			TraceError{ClassId} << "Unable to get instance extensions : " << vkResultToCString(outcome.result) << " !";
		}

		return extensions;
	}

	VkFormat
	Instance::findColorFormat (const std::shared_ptr< Device > & device, uint32_t redBits, uint32_t greenBits, uint32_t blueBits, uint32_t alphaBits) noexcept
	{
		/* HDR request (the bits used to be IGNORED — every render target was locked to
		 * R8G8B8A8_SRGB, which clamped photometric scenes to 1.0 in reflection probes). */
		if ( std::max({redBits, greenBits, blueBits, alphaBits}) >= 16U )
		{
			return device->findSupportedFormat(
				{
					VK_FORMAT_R16G16B16A16_SFLOAT,
					VK_FORMAT_R8G8B8A8_SRGB
				},
				VK_IMAGE_TILING_OPTIMAL,
				VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT
			);
		}

		return device->findSupportedFormat(
			{
				VK_FORMAT_R8G8B8A8_SRGB
			},
			VK_IMAGE_TILING_OPTIMAL,
			VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT
		);
	}

	VkFormat
	Instance::findDepthStencilFormat (const std::shared_ptr< Device > & device, uint32_t depthBits, uint32_t stencilBits) noexcept
	{
		std::vector< VkFormat > formats{};

		switch ( depthBits )
		{
			case 32U :
				if ( stencilBits == 0 )
				{
					formats.emplace_back(VK_FORMAT_D32_SFLOAT);
				}
				else
				{
					formats.emplace_back(VK_FORMAT_D32_SFLOAT_S8_UINT);
				}
				[[fallthrough]];

			case 24U :
				if ( stencilBits == 0 )
				{
					formats.emplace_back(VK_FORMAT_X8_D24_UNORM_PACK32);
				}
				else
				{
					formats.emplace_back(VK_FORMAT_D24_UNORM_S8_UINT);
				}
				[[fallthrough]];

			case 16U :
				if ( stencilBits == 0 )
				{
					formats.emplace_back(VK_FORMAT_D16_UNORM);
				}
				else
				{
					formats.emplace_back(VK_FORMAT_D16_UNORM_S8_UINT);
				}
				[[fallthrough]];

			case 0U :
				if ( stencilBits > 0 )
				{
					formats.emplace_back(VK_FORMAT_S8_UINT);
				}
				break;

			default:
				TraceError{ClassId} << "Unable to get a " << depthBits << "bits depth buffer !";

				return VK_FORMAT_UNDEFINED;
		}

		return device->findSupportedFormat(
			formats,
			VK_IMAGE_TILING_OPTIMAL,
			VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
		);
	}

	Instance::ScoredPhysicalDevice
	Instance::makeScoredDevice (const std::shared_ptr< PhysicalDevice > & device, size_t score, size_t enumerationIndex) noexcept
	{
		ScoredPhysicalDevice scored{};
		scored.device = device;
		scored.score = score;
		scored.enumerationIndex = enumerationIndex;

		switch ( device->propertiesVK10().deviceType )
		{
			case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU :
				scored.typeRank = 4;
				break;

			case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU :
				scored.typeRank = 3;
				break;

			case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU :
				scored.typeRank = 2;
				break;

			case VK_PHYSICAL_DEVICE_TYPE_CPU :
				scored.typeRank = 1;
				break;

			default :
				scored.typeRank = 0;
				break;
		}

		const auto & memory = device->memoryPropertiesVK11().memoryProperties;

		for ( const auto & heap : std::span{memory.memoryHeaps}.first(std::min< size_t >(memory.memoryHeapCount, VK_MAX_MEMORY_HEAPS)) )
		{
			if ( (heap.flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) != 0 )
			{
				scored.deviceLocalMemory = std::max< uint64_t >(scored.deviceLocalMemory, heap.size);
			}
		}

		return scored;
	}

	void
	Instance::rankDevices (std::vector< ScoredPhysicalDevice > & devices) const noexcept
	{
		/* NOTE: The enumeration index is inverted so that, at equal keys, the device Vulkan listed FIRST ranks higher. */
		const auto keys = [this] (const ScoredPhysicalDevice & scored) {
			const auto order = std::numeric_limits< size_t >::max() - scored.enumerationIndex;

			if ( m_autoSelectMode == DeviceAutoSelectMode::RawPerformance )
			{
				return std::tuple< uint64_t, uint64_t, uint64_t, uint64_t >{scored.typeRank, scored.deviceLocalMemory, scored.score, order};
			}

			return std::tuple< uint64_t, uint64_t, uint64_t, uint64_t >{scored.score, scored.typeRank, scored.deviceLocalMemory, order};
		};

		std::ranges::sort(devices, [&keys] (const ScoredPhysicalDevice & left, const ScoredPhysicalDevice & right) {
			return keys(left) < keys(right);
		});
	}

	std::vector< Instance::ScoredPhysicalDevice >
	Instance::getScoredGraphicsDevices (Window * window) noexcept
	{
		const auto hybridConfig = this->detectHybridGPUConfiguration();

		std::vector< ScoredPhysicalDevice > graphicsDevices;
		graphicsDevices.reserve(m_physicalDevices.size());

		for ( size_t enumerationIndex = 0; enumerationIndex < m_physicalDevices.size(); ++enumerationIndex )
		{
			const auto & physicalDevice = m_physicalDevices[enumerationIndex];
			size_t score = 1;

			if ( m_showInformation )
			{
				TraceInfo{ClassId} << physicalDevice->getPhysicalDeviceInformation();
			}

			/* NOTE: Failsafe mode exclusion. */
			if ( m_enableFailSafe && this->shouldExcludeForFailsafe(hybridConfig, physicalDevice) )
			{
				continue;
			}

			/* NOTE: A device below the required API version (an outdated driver) is never selected. */
			if ( !this->checkDeviceAPIVersion(*physicalDevice) )
			{
				continue;
			}

			if ( window != nullptr && window->usable() )
			{
				TraceDebug{ClassId} << "Check for graphics presentation capabilities.";

				window->surface()->update(physicalDevice);

				if ( !this->checkDeviceCompatibility(physicalDevice, window, score) )
				{
					continue;
				}
			}
			else
			{
				TraceDebug{ClassId} << "Skip th check for graphics presentation capabilities due to unavailable window.";

				if ( !this->checkDeviceCompatibility(physicalDevice, VK_QUEUE_GRAPHICS_BIT, score) )
				{
					continue;
				}
			}

			if ( !this->checkDevicesFeaturesForGraphics(physicalDevice, score) )
			{
				continue;
			}

			if ( !Instance::checkDeviceForRequiredExtensions(physicalDevice, m_requiredGraphicsDeviceExtensions, score) )
			{
				continue;
			}

			if ( m_autoSelectMode == DeviceAutoSelectMode::Performance )
			{
				/* NOTE: More queues = higher score for performance. */
				score += physicalDevice->getTotalQueueCount() * 10;
			}
			else if ( m_autoSelectMode == DeviceAutoSelectMode::PowerSaving )
			{
				/* NOTE: Fewer queues = higher score for power saving (inverse relationship). */
				score += 100 / std::max(physicalDevice->getTotalQueueCount(), static_cast< size_t >(1));
			}

			this->modulateDeviceScoring(physicalDevice->propertiesVK10(), score);

			auto scored = Instance::makeScoredDevice(physicalDevice, score, enumerationIndex);

			if ( m_showInformation )
			{
				TraceInfo{ClassId} << "Physical device '" << physicalDevice->propertiesVK10().deviceName << "' reached a score of " << score << " (type rank " << scored.typeRank << ", " << scored.deviceLocalMemory / (1024ULL * 1024ULL) << " MiB device-local) !";
			}

			graphicsDevices.emplace_back(std::move(scored));
		}

		this->rankDevices(graphicsDevices);

		if ( m_showInformation )
		{
			/* NOTE: Log hybrid GPU configuration diagnostic as a single coherent trace. */
			if ( hybridConfig.isOptimusDetected )
			{
				TraceWarning{ClassId} <<
					"Nvidia Optimus (LAPTOP) detected: "
					"iGPU '" << hybridConfig.integratedGPUName << "' + "
					"dGPU '" << hybridConfig.discreteGPUName << "' (mobile). "
					"Failsafe mode will avoid dGPU in windowed mode.";
			}
			else if ( hybridConfig.isHybridNonOptimus )
			{
				TraceInfo{ClassId} <<
					"Hybrid GPU (Desktop, non-Optimus): "
					"iGPU '" << hybridConfig.integratedGPUName << "' + "
					"dGPU '" << hybridConfig.discreteGPUName << "'. "
					"All modes safe.";
			}
		}

		return graphicsDevices;
	}

	std::shared_ptr< Device >
	Instance::getGraphicsDevice (Window * window) noexcept
	{
		if ( m_graphicsDevice != nullptr )
		{
			return m_graphicsDevice;
		}

		auto & settings = m_primaryServices.settings();
		const auto forceGPUName = settings.getOrSetDefault< std::string >(VkDeviceForceGPUKey);

		/* NOTE: Save all physical devices to settings (informational, not filtered). */
		settings.clearArray(VkDeviceAvailableGPUsKey);

		for ( const auto & physicalDevice : m_physicalDevices )
		{
			settings.setInArray(VkDeviceAvailableGPUsKey, physicalDevice->deviceName());
		}

		/* NOTE: Get a list of available devices. */
		const auto scoredDevices = this->getScoredGraphicsDevices(window);

		/* NOTE: Returns the device with the highest score. */
		if ( scoredDevices.empty() )
		{
			Tracer::error(ClassId, "There is no physical device (Graphics) compatible with Vulkan.");

			return {};
		}

		std::shared_ptr< PhysicalDevice > selectedPhysicalDevice;

		if ( !forceGPUName.empty() )
		{
			TraceDebug{ClassId} << "Trying to force the physical device (Graphics) named '" << forceGPUName << "' ...";

			for ( const auto & scored : scoredDevices )
			{
				const auto & physicalDevice = scored.device;

				if ( physicalDevice->deviceName() == forceGPUName )
				{
					selectedPhysicalDevice = physicalDevice;

					break;
				}
			}
		}

		/* NOTE: The device the application prefers (Instance::preferGraphicsDevice()), only among the compatible ones. */
		bool preferred = false;

		if ( selectedPhysicalDevice == nullptr && m_preferredGraphicsDevice.has_value() )
		{
			for ( const auto & scored : scoredDevices )
			{
				const auto & physicalDevice = scored.device;

				if ( physicalDevice->identity().matches(m_preferredGraphicsDevice.value()) )
				{
					selectedPhysicalDevice = physicalDevice;
					preferred = true;

					break;
				}
			}

			if ( !preferred )
			{
				Tracer::warning(ClassId, "The physical device preferred by the application is absent or not compatible : the score decides.");
			}
		}

		/* NOTE: If no GPU was forced or preferred (or not found), pick the best one. */
		if ( preferred )
		{
			TraceSuccess{ClassId} << "Graphics capable physical device selected: " << selectedPhysicalDevice->propertiesVK10().deviceName << " (PREFERRED by the application)";
		}
		else if ( selectedPhysicalDevice != nullptr )
		{
			TraceSuccess{ClassId} << "Graphics capable physical device selected: " << selectedPhysicalDevice->propertiesVK10().deviceName << " (FORCED)";
		}
		else
		{
			selectedPhysicalDevice = scoredDevices.back().device;

			TraceSuccess{ClassId} << "Graphics capable physical device selected: " << selectedPhysicalDevice->propertiesVK10().deviceName << " (" << magic_enum::enum_name(m_autoSelectMode) << " mode)";
		}

		/* NOTE: The selected device first, then every other compatible device from the best to the worst: a device that
		 * cannot be created (a required feature missing, vkCreateDevice() failing) no longer ends the startup while a
		 * next one may work — an iGPU beside an outdated dGPU driver (engine item
		 * device-below-vulkan-1-3-reaches-vkcreatedevice, 2026-10-15). */
		std::vector< std::shared_ptr< PhysicalDevice > > candidates;
		candidates.reserve(scoredDevices.size());
		candidates.emplace_back(selectedPhysicalDevice);

		for ( const auto & scored : std::ranges::reverse_view(scoredDevices) )
		{
			if ( scored.device != selectedPhysicalDevice )
			{
				candidates.emplace_back(scored.device);
			}
		}

		/* NOTE: An attempt adds the optional extensions its device supports: each one starts from the same list. */
		const auto baseExtensions = m_requiredGraphicsDeviceExtensions;

		for ( const auto & candidate : candidates )
		{
			if ( candidate != selectedPhysicalDevice )
			{
				TraceWarning{ClassId} << "Trying the next compatible physical device: '" << candidate->propertiesVK10().deviceName << "' ...";
			}

			m_requiredGraphicsDeviceExtensions = baseExtensions;

			if ( auto logicalDevice = this->createGraphicsDevice(window, candidate); logicalDevice != nullptr )
			{
				/* NOTE: Always logged (docs/subsystems/vulkan/22-startup-refusals-told-to-the-user.md): every report states the GPU and its driver. */
				const auto & properties = candidate->propertiesVK10();
				const auto & propertiesVK12 = candidate->propertiesVK12();

				TraceInfo{ClassId} <<
					"Graphics device: '" << properties.deviceName << "' (" << candidate->deviceType() << ", vendor 0x" << std::hex << properties.vendorID << ", device 0x" << properties.deviceID << std::dec << "), "
					"driver '" << (propertiesVK12.driverName[0] != '\0' ? std::string_view{std::data(propertiesVK12.driverName)} : std::string_view{"n/a"}) << "' (" << propertiesVK12.driverInfo << "), driver version " << candidate->DriverVersionString() << ", "
					"Vulkan " << VK_API_VERSION_MAJOR(properties.apiVersion) << '.' << VK_API_VERSION_MINOR(properties.apiVersion) << '.' << VK_API_VERSION_PATCH(properties.apiVersion) << ".";

				m_graphicsDevice = logicalDevice;

				return logicalDevice;
			}

			TraceError{ClassId} << "The physical device '" << candidate->propertiesVK10().deviceName << "' could not be used for graphics.";
		}

		m_requiredGraphicsDeviceExtensions = baseExtensions;

		Tracer::error(ClassId, "No compatible physical device could be created for graphics.");

		return {};
	}

	std::shared_ptr< Device >
	Instance::createGraphicsDevice (Window * window, const std::shared_ptr< PhysicalDevice > & selectedPhysicalDevice) noexcept
	{
		auto logicalDevice = std::make_shared< Device >(*this, selectedPhysicalDevice->propertiesVK10().deviceName, selectedPhysicalDevice, m_showInformation);
		logicalDevice->setIdentifier(ClassId, (std::stringstream{} << selectedPhysicalDevice->propertiesVK10().deviceName << "(Graphics)").str(), "Device");

		/* Check window */
		if ( window != nullptr )
		{
			if ( !window->usable() )
			{
				Tracer::error(ClassId, "The window is not usable !");

				return {};
			}

			/* NOTE: Be sure the selected device is the one that update the surface. */
			window->surface()->update(logicalDevice);
		}

		/* [VULKAN-API-SETUP] Graphics device features configuration. */
		DeviceRequirements requirements{true, window, false};

		/* ⚠️ A feature requested here that the device does not advertise makes vkCreateDevice fail
		 * with VK_ERROR_FEATURE_NOT_PRESENT for the WHOLE set, naming none of them — so one absent
		 * feature costs the renderer, and the log says only that a device could not be created.
		 * `checkDevicesFeaturesForGraphics()` above warns per feature and lets the device through
		 * (its `return false` are deliberately commented out), so the warnings and the request were
		 * not connected to each other: the engine observed availability and then asked anyway.
		 *
		 * Measured 2026-09-17 on a GitHub Windows runner against CEF's SwiftShader (software
		 * device): 26 features absent, three of them requested below — `geometryShader`,
		 * `shaderImageGatherExtended` and `shaderInt64` — and the application died at startup.
		 * The macOS guard below was the same problem answered by platform: an Apple iGPU has no
		 * geometry stage, which is a statement about a DEVICE, not about an operating system. */
		const auto & availableFeatures = selectedPhysicalDevice->featuresVK10();

		const auto requestOptionalVK10 = [&] (VkBool32 VkPhysicalDeviceFeatures::* feature, const char * featureName, const char * lostCapability) {
			if ( availableFeatures.*feature == VK_FALSE )
			{
				TraceWarning{ClassId} <<
					"The physical device '" << selectedPhysicalDevice->propertiesVK10().deviceName <<
					"' does not advertise '" << featureName << "': not requested, " << lostCapability << " unavailable.";

				return;
			}

			requirements.featuresVK10().*feature = VK_TRUE;
		};

		const auto & availableFeaturesVK11 = selectedPhysicalDevice->featuresVK11();

		const auto requestOptionalVK11 = [&] (VkBool32 VkPhysicalDeviceVulkan11Features::* feature, const char * featureName, const char * lostCapability) {
			if ( availableFeaturesVK11.*feature == VK_FALSE )
			{
				TraceWarning{ClassId} <<
					"The physical device '" << selectedPhysicalDevice->propertiesVK10().deviceName <<
					"' does not advertise '" << featureName << "': not requested, " << lostCapability << " unavailable.";

				return;
			}

			requirements.featuresVK11().*feature = VK_TRUE;
		};

		const auto & availableFeaturesVK12 = selectedPhysicalDevice->featuresVK12();

		const auto requestOptionalVK12 = [&] (VkBool32 VkPhysicalDeviceVulkan12Features::* feature, const char * featureName, const char * lostCapability) {
			if ( availableFeaturesVK12.*feature == VK_FALSE )
			{
				TraceWarning{ClassId} <<
					"The physical device '" << selectedPhysicalDevice->propertiesVK10().deviceName <<
					"' does not advertise '" << featureName << "': not requested, " << lostCapability << " unavailable.";

				return;
			}

			requirements.featuresVK12().*feature = VK_TRUE;
		};

		/* NOTE: A REQUIRED feature is requested whatever the device says; one it does not advertise is recorded, so the
		 * device is refused BEFORE vkCreateDevice() — which answers VK_ERROR_FEATURE_NOT_PRESENT for the whole set and
		 * names none (engine item device-below-vulkan-1-3-reaches-vkcreatedevice, 2026-10-15). */
		std::vector< const char * > missingRequiredFeatures;

		const auto requireVK10 = [&] (VkBool32 VkPhysicalDeviceFeatures::* feature, const char * featureName) {
			if ( availableFeatures.*feature == VK_FALSE )
			{
				missingRequiredFeatures.emplace_back(featureName);
			}

			requirements.featuresVK10().*feature = VK_TRUE;
		};

		const auto requireVK11 = [&] (VkBool32 VkPhysicalDeviceVulkan11Features::* feature, const char * featureName) {
			if ( availableFeaturesVK11.*feature == VK_FALSE )
			{
				missingRequiredFeatures.emplace_back(featureName);
			}

			requirements.featuresVK11().*feature = VK_TRUE;
		};

		const auto requireVK12 = [&] (VkBool32 VkPhysicalDeviceVulkan12Features::* feature, const char * featureName) {
			if ( availableFeaturesVK12.*feature == VK_FALSE )
			{
				missingRequiredFeatures.emplace_back(featureName);
			}

			requirements.featuresVK12().*feature = VK_TRUE;
		};

		const auto & availableFeaturesVK13 = selectedPhysicalDevice->featuresVK13();

		const auto requireVK13 = [&] (VkBool32 VkPhysicalDeviceVulkan13Features::* feature, const char * featureName) {
			if ( availableFeaturesVK13.*feature == VK_FALSE )
			{
				missingRequiredFeatures.emplace_back(featureName);
			}

			requirements.featuresVK13().*feature = VK_TRUE;
		};

		const auto effectiveVersion = this->effectiveAPIVersion(*selectedPhysicalDevice);

		/* NOTE: The scene (3D) renderer's features, from its target on (SceneRenderingAPIVersion). Below it the engine draws
		 * the overlay only and requires nothing beyond Vulkan 1.1 and the timelines (engine item
		 * core-targets-a-vulkan-api-version). */
		if ( this->sceneRenderingEnabled() )
		{
			// FIXME: Check to enable "VK_EXT_non_seamless_cube_map" extension
			//requirements.featuresVK10().nonSeamlessCubeMap = VK_TRUE; // Required for cubemap rendering
			requireVK10(&VkPhysicalDeviceFeatures::fillModeNonSolid, "fillModeNonSolid"); // Required for wireframe mode!
			/* NOTE: was `if constexpr ( !IsMacOS )` — Apple M1/M2/M3/M4 iGPUs have no geometry stage,
			 * and neither does a software device. Asked of the device instead of the platform. */
			requestOptionalVK10(&VkPhysicalDeviceFeatures::geometryShader, "geometryShader", "TBN space display");
			requireVK10(&VkPhysicalDeviceFeatures::samplerAnisotropy, "samplerAnisotropy");
			requireVK10(&VkPhysicalDeviceFeatures::independentBlend, "independentBlend"); // Required for per-attachment MRT blend states (G-buffer full write vs color alpha blending)
			requireVK10(&VkPhysicalDeviceFeatures::depthBiasClamp, "depthBiasClamp"); // Required for shadow map depth bias clamping
			/* ⚠️ depthClamp is NOT depthBiasClamp — distinct features, both needed by the shadow cast
			 * pass. This one lets it set depthClampEnable, which FLATTENS casters sitting in front of
			 * the light's near plane onto that plane instead of CLIPPING them out of the map. Without
			 * it a caster closer to the light than the near plane writes nothing, so the map keeps its
			 * 1.0 clear there and the receiver below reads "lit": a shadow-shaped HOLE. */
			requireVK10(&VkPhysicalDeviceFeatures::depthClamp, "depthClamp"); // Required for shadow map depth clamping (see ShadowCasting)
			requestOptionalVK10(&VkPhysicalDeviceFeatures::shaderImageGatherExtended, "shaderImageGatherExtended", "PCF shadow filtering");
			requireVK10(&VkPhysicalDeviceFeatures::imageCubeArray, "imageCubeArray"); // Required for animated cubemap textures (VK_IMAGE_VIEW_TYPE_CUBE_ARRAY)
			requireVK11(&VkPhysicalDeviceVulkan11Features::multiview, "multiview"); // Required for cubemap render-to-texture (Vulkan 1.1+)
			{
				/* Descriptor indexing features (Vulkan 1.2) - Required for bindless textures. */
				requireVK12(&VkPhysicalDeviceVulkan12Features::descriptorIndexing, "descriptorIndexing");
				requireVK12(&VkPhysicalDeviceVulkan12Features::descriptorBindingPartiallyBound, "descriptorBindingPartiallyBound");
				requireVK12(&VkPhysicalDeviceVulkan12Features::descriptorBindingSampledImageUpdateAfterBind, "descriptorBindingSampledImageUpdateAfterBind");
				requireVK12(&VkPhysicalDeviceVulkan12Features::runtimeDescriptorArray, "runtimeDescriptorArray");
				requireVK12(&VkPhysicalDeviceVulkan12Features::shaderSampledImageArrayNonUniformIndexing, "shaderSampledImageArrayNonUniformIndexing");
			}
			requireVK12(&VkPhysicalDeviceVulkan12Features::bufferDeviceAddress, "bufferDeviceAddress"); // Required for buffer device addresses (VBO/IBO for RT, etc.)
			requestOptionalVK12(&VkPhysicalDeviceVulkan12Features::hostQueryReset, "hostQueryReset", "GPU timing of the shadow maps and render-to-textures");
			/* Multi-Draw Indirect features - Required for GPU-driven rendering (MDI). */
			requireVK10(&VkPhysicalDeviceFeatures::multiDrawIndirect, "multiDrawIndirect"); // Required for vkCmdDrawIndexedIndirect with drawCount > 1
			requireVK10(&VkPhysicalDeviceFeatures::drawIndirectFirstInstance, "drawIndirectFirstInstance"); // Required for firstInstance in indirect commands
			/* ⚠️ Unlike the two above, dropping this one is NOT cosmetic: BDA address reconstruction in
			 * shaders needs uint64_t. A device without it boots and renders, and any path relying on
			 * buffer device addresses has to be off. Made conditional so the engine starts and says so,
			 * rather than refusing to create a device and saying nothing. */
			requestOptionalVK10(&VkPhysicalDeviceFeatures::shaderInt64, "shaderInt64", "uint64_t in shaders (BDA address reconstruction)");
			/* A compute pass writing a storage image of a two-channel or 16-bit format (R16G16_SFLOAT, ...)
			 * needs it: the heightfield bakes its normal clipmap that way (Graphics::Geometry::HeightfieldSurface::NormalFormat).
			 * Universal on desktop GPUs and MoltenVK; a terrain on a device without it has no normals and says so. */
			requestOptionalVK10(&VkPhysicalDeviceFeatures::shaderStorageImageExtendedFormats, "shaderStorageImageExtendedFormats", "compute writes to two-channel/16-bit storage images (the heightfield normal clipmap)");
			/* Required for imageStore() from a FRAGMENT shader: the RTR trace writes its per-pixel glossy
			 * cone width map (a storage image) next to its colour attachment. Without the feature the SPIR-V
			 * validation rejects the pipeline (VUID-RuntimeSpirv-NonWritable-06340) and the effect fails to
			 * create. Universally supported on desktop GPUs; the physical-device probe above warns when absent. */
			requireVK10(&VkPhysicalDeviceFeatures::fragmentStoresAndAtomics, "fragmentStoresAndAtomics");
			/* ⚠️ Required for gl_DrawID in vertex shaders, which ONLY the Multi-Draw Indirect path uses —
			 * and MDI is opt-in and OFF by default (`Core/Graphics/MDI/Enabled`,
			 * DefaultGraphicsMDIEnabled == false). Requesting it unconditionally made the engine refuse
			 * to create a device, for a path nobody had switched on: measured 2026-09-17 against
			 * SwiftShader, where this single bit out of the nine non-VK10 features requested here is
			 * the only one absent (bufferDeviceAddress IS present there). Renderer::onInitialize()
			 * refuses to build the MDI batch builder when it is missing, so a device without it cannot
			 * silently render with a wrong per-draw index. */
			requestOptionalVK11(&VkPhysicalDeviceVulkan11Features::shaderDrawParameters, "shaderDrawParameters", "gl_DrawID, hence the Multi-Draw Indirect path");
		}

		/* NOTE: Required at every target: each queue numbers its tracked submissions with a timeline, and an object
		 * released while an upload writes it is destroyed once that value is reached. checkDeviceAPIVersion() refused a
		 * device without (core 1.2, VK_KHR_timeline_semaphore below). */
		this->requestTimelineSemaphore(*selectedPhysicalDevice, m_requiredGraphicsDeviceExtensions, requirements);

		/* NOTE: Compiling for SPIR-V 1.6 (a Vulkan 1.3 device), glslang declares the DemoteToHelperInvocation capability
		 * for fragment shaders that discard, with no `demote` in the GLSL (measured 2026-10-15: VUID-VkShaderModuleCreateInfo-
		 * pCode-08740 on RenderableInstancePointLightPassFragmentShader without the feature). Below 1.3 the shaders target
		 * SPIR-V 1.5 or 1.3 and glslang writes OpKill. */
		if ( effectiveVersion >= VK_API_VERSION_1_3 )
		{
			requireVK13(&VkPhysicalDeviceVulkan13Features::shaderDemoteToHelperInvocation, "shaderDemoteToHelperInvocation");
		}

		if ( !missingRequiredFeatures.empty() )
		{
			std::string list;

			for ( const auto * featureName : missingRequiredFeatures )
			{
				list += list.empty() ? "" : ", ";
				list += featureName;
			}

			TraceError{ClassId} <<
				"The physical device '" << selectedPhysicalDevice->propertiesVK10().deviceName << "' (driver " << selectedPhysicalDevice->DriverVersionString() << ") "
				"does not advertise " << missingRequiredFeatures.size() << " required feature(s): " << list << ". It is not created.";

			this->recordDeviceRefusal(*selectedPhysicalDevice, DeviceRefusal::Reason::MissingFeatures, list);

			return {};
		}

		/* NOTE: maintenance4 (Vulkan 1.3 core, optional): glslang writes the workgroup size of a task or
		 * mesh shader as OpExecutionMode LocalSizeId when it targets SPIR-V 1.6, and that execution mode is
		 * refused without this feature (VUID-RuntimeSpirv-LocalSizeId-06434 — found by the first compile
		 * of a mesh shader, 2026-09-22). Requested when the device reports it. */
		if ( effectiveVersion < VK_API_VERSION_1_3 )
		{
			/* NOTE: The shaders are compiled for this version's SPIR-V (no LocalSizeId below 1.6): nothing to request. */
		}
		else if ( selectedPhysicalDevice->featuresVK13().maintenance4 == VK_TRUE )
		{
			requirements.featuresVK13().maintenance4 = VK_TRUE;
		}
		else
		{
			TraceWarning{ClassId} << "The physical device '" << selectedPhysicalDevice->propertiesVK10().deviceName << "' does not advertise 'maintenance4': not requested, task and mesh shaders compiled for SPIR-V 1.6 will be refused.";
		}

		/* NOTE: Optional extension detection. Query all device extensions once and enable
		 * supported optional features (portability subset, ray tracing). */
		{
			const auto & physicalDevice = logicalDevice->physicalDevice();
			const auto extensions = physicalDevice->getExtensions();

			const auto hasExtension = [&extensions] (const char * name) {
				return std::ranges::any_of(extensions, [name] (const VkExtensionProperties & ext) {
					return std::strcmp(ext.extensionName, name) == 0;
				});
			};

			/* NOTE: According to Vulkan spec, if a device exposes VK_KHR_portability_subset,
			 * it MUST be enabled. This is the case for MoltenVK on macOS, but some Windows
			 * drivers or software renderers may also expose it — so the test is on the
			 * extension being advertised, NEVER on the platform. It used to be guarded by
			 * "if constexpr ( !IsMacOS )", which skipped it on the one platform that always
			 * needs it and left the device created in a spec-violating state. */
			if ( Instance::requestPortabilitySubset(*physicalDevice, m_requiredGraphicsDeviceExtensions, requirements) )
			{
				TraceInfo{ClassId} <<
					"VK_KHR_portability_subset detected and enabled, requesting every feature the device "
					"supports (mutableComparisonSamplers: " << (physicalDevice->portabilitySubsetFeatures().mutableComparisonSamplers == VK_TRUE ? "yes" : "NO") << ").";
			}

			/* NOTE: Ray tracing extensions (VK_KHR_acceleration_structure + VK_KHR_ray_query).
			 * These enable hardware-accelerated ray queries in fragment/compute shaders.
			 * VK_KHR_deferred_host_operations is a required dependency of acceleration structures. */
			/* NOTE: The memory budget VMA reads (VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT needs the extension enabled). */
			if ( hasExtension(VK_EXT_MEMORY_BUDGET_EXTENSION_NAME) )
			{
				m_requiredGraphicsDeviceExtensions.emplace_back(VK_EXT_MEMORY_BUDGET_EXTENSION_NAME);
			}

			/* NOTE: Ray tracing, mesh shaders and the hardware video encode belong to the scene renderer's targets: the
			 * acceleration structures need buffer device addresses (1.2), the video encode synchronization2 (1.3). */
			if ( this->sceneRenderingEnabled()
				&& physicalDevice->supportsRayTracing()
				&& hasExtension(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME)
				&& hasExtension(VK_KHR_RAY_QUERY_EXTENSION_NAME)
				&& hasExtension(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME) )
			{
				m_requiredGraphicsDeviceExtensions.emplace_back(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
				m_requiredGraphicsDeviceExtensions.emplace_back(VK_KHR_RAY_QUERY_EXTENSION_NAME);
				m_requiredGraphicsDeviceExtensions.emplace_back(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);

				/* NOTE: Enable RT features on the logical device. */
				requirements.accelerationStructureFeatures().accelerationStructure = VK_TRUE;
				requirements.rayQueryFeatures().rayQuery = VK_TRUE;

				Tracer::info(ClassId, "Ray tracing extensions detected and enabled (VK_KHR_acceleration_structure + VK_KHR_ray_query).");
			}
			else
			{
				Tracer::info(ClassId, "Ray tracing not supported by this device.");
			}

			/* NOTE: Mesh shaders (VK_EXT_mesh_shader) — OPTIONAL stages, like the geometry stage: enabled
			 * when advertised, and a consumer keeps its classic vertex path otherwise (Device::meshShadersEnabled()).
			 * Only what the device reports is requested: `meshShader` always, the task stage and the
			 * multiview variant (cubemap and cascade targets) when supported. Left off: the fragment
			 * shading rate variant (needs VK_KHR_fragment_shading_rate) and the pipeline statistics
			 * queries. MoltenVK does not expose the extension (checked 2026-09-22): never on macOS today.
			 * Its SPIR-V 1.4 requirement is met by the Vulkan 1.2 core the engine targets. */
			if ( this->sceneRenderingEnabled() && physicalDevice->supportsMeshShaders() && hasExtension(VK_EXT_MESH_SHADER_EXTENSION_NAME) )
			{
				m_requiredGraphicsDeviceExtensions.emplace_back(VK_EXT_MESH_SHADER_EXTENSION_NAME);

				const auto & supported = physicalDevice->meshShaderFeatures();
				auto & requested = requirements.meshShaderFeatures();

				requested.meshShader = VK_TRUE;
				requested.taskShader = supported.taskShader;
				requested.multiviewMeshShader = supported.multiviewMeshShader;

				const auto & limits = physicalDevice->meshShaderProperties();

				TraceInfo{ClassId} <<
					"Mesh shaders detected and enabled (VK_EXT_mesh_shader): task stage " << (supported.taskShader == VK_TRUE ? "yes" : "no") <<
					", multiview " << (supported.multiviewMeshShader == VK_TRUE ? "yes" : "no") <<
					", at most " << limits.maxMeshOutputVertices << " vertices and " << limits.maxMeshOutputPrimitives << " primitives per mesh workgroup.";
			}
			else
			{
				Tracer::info(ClassId, "Mesh shaders not supported by this device.");
			}

			/* NOTE: Vulkan Video H.265 hardware encode (RushMaker hardware path).
			 * VK_KHR_video_queue + VK_KHR_video_encode_queue are the codec-agnostic
			 * infrastructure, VK_KHR_video_encode_h265 the codec. When absent, the
			 * recorder falls back to the software VP9 path. */
			if ( effectiveVersion >= VK_API_VERSION_1_3
				&& hasExtension(VK_KHR_VIDEO_QUEUE_EXTENSION_NAME)
				&& hasExtension(VK_KHR_VIDEO_ENCODE_QUEUE_EXTENSION_NAME)
				&& hasExtension(VK_KHR_VIDEO_ENCODE_H265_EXTENSION_NAME) )
			{
				m_requiredGraphicsDeviceExtensions.emplace_back(VK_KHR_VIDEO_QUEUE_EXTENSION_NAME);
				m_requiredGraphicsDeviceExtensions.emplace_back(VK_KHR_VIDEO_ENCODE_QUEUE_EXTENSION_NAME);
				m_requiredGraphicsDeviceExtensions.emplace_back(VK_KHR_VIDEO_ENCODE_H265_EXTENSION_NAME);

				/* NOTE: The video encode command buffers use synchronization2 barriers
				 * (the VIDEO_ENCODE pipeline stages only exist there). Core 1.3 feature. */
				requirements.featuresVK13().synchronization2 = VK_TRUE;

				Tracer::info(ClassId, "Vulkan Video H.265 encode extensions detected and enabled (hardware RushMaker path).");

				/* Probe and log the H.265 encode capabilities (main profile, 4:2:0 8-bit). */
				const auto fpGetVideoCapabilities = reinterpret_cast< PFN_vkGetPhysicalDeviceVideoCapabilitiesKHR >(vkGetInstanceProcAddr(m_instance, "vkGetPhysicalDeviceVideoCapabilitiesKHR"));

				if ( fpGetVideoCapabilities != nullptr )
				{
					VkVideoEncodeH265ProfileInfoKHR h265Profile{};
					h265Profile.sType = VK_STRUCTURE_TYPE_VIDEO_ENCODE_H265_PROFILE_INFO_KHR;
					h265Profile.stdProfileIdc = STD_VIDEO_H265_PROFILE_IDC_MAIN;

					VkVideoProfileInfoKHR profile{};
					profile.sType = VK_STRUCTURE_TYPE_VIDEO_PROFILE_INFO_KHR;
					profile.pNext = &h265Profile;
					profile.videoCodecOperation = VK_VIDEO_CODEC_OPERATION_ENCODE_H265_BIT_KHR;
					profile.chromaSubsampling = VK_VIDEO_CHROMA_SUBSAMPLING_420_BIT_KHR;
					profile.lumaBitDepth = VK_VIDEO_COMPONENT_BIT_DEPTH_8_BIT_KHR;
					profile.chromaBitDepth = VK_VIDEO_COMPONENT_BIT_DEPTH_8_BIT_KHR;

					VkVideoEncodeH265CapabilitiesKHR h265Capabilities{};
					h265Capabilities.sType = VK_STRUCTURE_TYPE_VIDEO_ENCODE_H265_CAPABILITIES_KHR;

					VkVideoEncodeCapabilitiesKHR encodeCapabilities{};
					encodeCapabilities.sType = VK_STRUCTURE_TYPE_VIDEO_ENCODE_CAPABILITIES_KHR;
					encodeCapabilities.pNext = &h265Capabilities;

					VkVideoCapabilitiesKHR videoCapabilities{};
					videoCapabilities.sType = VK_STRUCTURE_TYPE_VIDEO_CAPABILITIES_KHR;
					videoCapabilities.pNext = &encodeCapabilities;

					if ( fpGetVideoCapabilities(physicalDevice->handle(), &profile, &videoCapabilities) == VK_SUCCESS )
					{
						TraceInfo{ClassId} <<
							"H.265 encode capabilities (main, 4:2:0 8-bit): "
							"max " << videoCapabilities.maxCodedExtent.width << "x" << videoCapabilities.maxCodedExtent.height <<
							", DPB slots " << videoCapabilities.maxDpbSlots <<
							", active references " << videoCapabilities.maxActiveReferencePictures <<
							", max bitrate " << (encodeCapabilities.maxBitrate / 1000000ULL) << " Mbps"
							", quality levels " << encodeCapabilities.maxQualityLevels <<
							", separate DPB images: " << ((videoCapabilities.flags & VK_VIDEO_CAPABILITY_SEPARATE_REFERENCE_IMAGES_BIT_KHR) != 0 ? "yes" : "NO (array required)") <<
							", rate control modes: 0x" << std::hex << encodeCapabilities.rateControlModes << std::dec <<
							", picture granularity " << videoCapabilities.pictureAccessGranularity.width << "x" << videoCapabilities.pictureAccessGranularity.height <<
							", bitstream alignment " << videoCapabilities.minBitstreamBufferSizeAlignment << "/" << videoCapabilities.minBitstreamBufferOffsetAlignment << ".";
					}
					else
					{
						Tracer::warning(ClassId, "Unable to query H.265 encode capabilities.");
					}
				}
			}
			else
			{
				Tracer::info(ClassId, "Vulkan Video H.265 encode not supported by this device (RushMaker will use software VP9).");
			}

			/* NOTE: GPU device-lost diagnostics (best-effort, ZERO runtime cost until a fault occurs).
			 * Enabled whenever supported so a DEVICE_LOST is self-documented even in normal/release runs
			 * — see Device::dumpDeviceLostDiagnostics(). The two extensions are vendor-complementary:
			 *  - VK_EXT_device_fault: faulting GPU virtual addresses (Mesa/AMD/Intel; absent on NVIDIA proprietary).
			 *  - VK_NV_device_diagnostic_checkpoints: last GPU command region reached (NVIDIA). */
			if ( hasExtension(VK_EXT_DEVICE_FAULT_EXTENSION_NAME) )
			{
				m_requiredGraphicsDeviceExtensions.emplace_back(VK_EXT_DEVICE_FAULT_EXTENSION_NAME);
				requirements.faultFeatures().deviceFault = VK_TRUE;

				Tracer::info(ClassId, "VK_EXT_device_fault detected and enabled (GPU fault address reporting).");
			}

			if ( hasExtension(VK_NV_DEVICE_DIAGNOSTIC_CHECKPOINTS_EXTENSION_NAME) )
			{
				m_requiredGraphicsDeviceExtensions.emplace_back(VK_NV_DEVICE_DIAGNOSTIC_CHECKPOINTS_EXTENSION_NAME);

				Tracer::info(ClassId, "VK_NV_device_diagnostic_checkpoints detected and enabled (GPU checkpoint markers).");
			}

			/* NOTE: External-memory image import (zero-copy CEF accelerated paint).
			 * VK_KHR_external_memory and VK_KHR_external_memory_capabilities are core since
			 * Vulkan 1.1 (the lowest target) — only the platform handle extension is needed.
			 * On Windows it allows importing a D3D11 shared texture (DXGI shared HANDLE) as a
			 * VkImage. The literal name is used because vulkan_win32.h is not included here. */
			if constexpr ( IsWindows )
			{
				if ( hasExtension("VK_KHR_external_memory_win32") )
				{
					m_requiredGraphicsDeviceExtensions.emplace_back("VK_KHR_external_memory_win32");

					Tracer::info(ClassId, "VK_KHR_external_memory_win32 detected and enabled (external D3D11 texture import).");
				}
			}

			/* NOTE: On macOS the equivalent import path is VK_EXT_metal_objects (MoltenVK): a CEF
			 * IOSurface becomes the backing MTLTexture of a VkImage at image creation
			 * (VkImportMetalIOSurfaceInfoEXT). This is not Vulkan external memory — no
			 * external-memory handle extension exists for IOSurface in this MoltenVK version.
			 * The literal name is used because vulkan_metal.h is not included here. */
			if constexpr ( IsMacOS )
			{
				if ( hasExtension("VK_EXT_metal_objects") )
				{
					m_requiredGraphicsDeviceExtensions.emplace_back("VK_EXT_metal_objects");

					Tracer::info(ClassId, "VK_EXT_metal_objects detected and enabled (external IOSurface import).");
				}
			}
		}

		if ( !logicalDevice->create(requirements, m_requiredGraphicsDeviceExtensions, m_useVulkanMemoryAllocator) )
		{
			this->recordDeviceRefusal(*selectedPhysicalDevice, DeviceRefusal::Reason::CreationFailed, "vkCreateDevice() failed (see the error above)");

			return {};
		}

		return logicalDevice;
	}

	void
	Instance::setTargetAPIVersion (uint32_t apiVersion) noexcept
	{
		/* NOTE: Compared on major.minor (the variant and the patch never decide). */
		auto target = VK_MAKE_API_VERSION(0, VK_API_VERSION_MAJOR(apiVersion), VK_API_VERSION_MINOR(apiVersion), 0);

		if ( VK_API_VERSION_VARIANT(apiVersion) != 0 || target > HighestTargetAPIVersion )
		{
			TraceWarning{ClassId} << "The targeted Vulkan API version (0x" << std::hex << apiVersion << std::dec << ") is above the highest the engine knows: Vulkan " << VK_API_VERSION_MAJOR(HighestTargetAPIVersion) << '.' << VK_API_VERSION_MINOR(HighestTargetAPIVersion) << " is targeted.";

			target = HighestTargetAPIVersion;
		}
		else if ( target < LowestTargetAPIVersion )
		{
			TraceWarning{ClassId} << "The targeted Vulkan API version " << VK_API_VERSION_MAJOR(target) << '.' << VK_API_VERSION_MINOR(target) << " is below the lowest the engine supports: Vulkan " << VK_API_VERSION_MAJOR(LowestTargetAPIVersion) << '.' << VK_API_VERSION_MINOR(LowestTargetAPIVersion) << " is targeted.";

			target = LowestTargetAPIVersion;
		}

		m_targetAPIVersion = target;
	}

	uint32_t
	Instance::effectiveAPIVersion (const PhysicalDevice & physicalDevice) const noexcept
	{
		const auto reported = physicalDevice.propertiesVK10().apiVersion;

		return std::min(m_targetAPIVersion, VK_MAKE_API_VERSION(0, VK_API_VERSION_MAJOR(reported), VK_API_VERSION_MINOR(reported), 0));
	}

	void
	Instance::requestTimelineSemaphore (const PhysicalDevice & physicalDevice, std::vector< const char * > & extensions, DeviceRequirements & requirements) const noexcept
	{
		requirements.featuresVK12().timelineSemaphore = VK_TRUE;

		if ( this->effectiveAPIVersion(physicalDevice) < VK_API_VERSION_1_2 && std::ranges::none_of(extensions, [] (const char * name) { return std::strcmp(name, VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME) == 0; }) )
		{
			extensions.emplace_back(VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME);
		}
	}

	bool
	Instance::checkDeviceAPIVersion (const PhysicalDevice & physicalDevice) noexcept
	{
		const auto apiVersion = physicalDevice.propertiesVK10().apiVersion;
		const auto minimum = this->minimumDeviceAPIVersion();

		/* NOTE: Compared on major.minor (the variant and the patch never decide). */
		if ( VK_MAKE_API_VERSION(0, VK_API_VERSION_MAJOR(apiVersion), VK_API_VERSION_MINOR(apiVersion), 0) >= minimum )
		{
			/* NOTE: The timelines carry the GPU-lifetime tracking at every target: core from 1.2, the extension below. A
			 * driver without them is an outdated one (owner decision 2026-10-15: the user is told to update it). */
			const bool timelines = this->effectiveAPIVersion(physicalDevice) >= VK_API_VERSION_1_2 ?
				physicalDevice.supportsTimelineSemaphore() :
				physicalDevice.hasTimelineSemaphoreExtension() && physicalDevice.supportsTimelineSemaphore();

			if ( timelines )
			{
				return true;
			}

			const std::string detail{"no timeline semaphores (VK_KHR_timeline_semaphore)"};

			TraceError{ClassId} <<
				"The physical device '" << physicalDevice.propertiesVK10().deviceName << "' (driver " << physicalDevice.DriverVersionString() << "): " << detail << ". "
				"Its graphics driver is too old: update the graphics driver. The device is not used.";

			this->recordDeviceRefusal(physicalDevice, DeviceRefusal::Reason::BelowRequiredAPIVersion, detail);

			return false;
		}

		std::stringstream detail;
		detail << "Vulkan " << VK_API_VERSION_MAJOR(apiVersion) << '.' << VK_API_VERSION_MINOR(apiVersion) << '.' << VK_API_VERSION_PATCH(apiVersion) << " reported, " << VK_API_VERSION_MAJOR(minimum) << '.' << VK_API_VERSION_MINOR(minimum) << " required";

		TraceError{ClassId} <<
			"The physical device '" << physicalDevice.propertiesVK10().deviceName << "' (driver " << physicalDevice.DriverVersionString() << "): " << detail.str() << ". "
			"Its graphics driver is too old: update the graphics driver. The device is not used.";

		this->recordDeviceRefusal(physicalDevice, DeviceRefusal::Reason::BelowRequiredAPIVersion, detail.str());

		return false;
	}

	void
	Instance::recordDeviceRefusal (const PhysicalDevice & physicalDevice, DeviceRefusal::Reason reason, std::string detail) noexcept
	{
		const auto & properties = physicalDevice.propertiesVK10();

		m_deviceRefusals.emplace_back(DeviceRefusal{
			.deviceName = properties.deviceName,
			.driverVersion = physicalDevice.DriverVersionString(),
			.detail = std::move(detail),
			.vendorID = properties.vendorID,
			.apiVersion = properties.apiVersion,
			.reason = reason
		});
	}

	std::shared_ptr< Device >
	Instance::getComputeDevice () noexcept
	{
		if ( m_computeDevice != nullptr )
		{
			return m_computeDevice;
		}

		auto & settings = m_primaryServices.settings();
		const auto autoSelectModeString = settings.getOrSetDefault< std::string >(VkDeviceAutoSelectModeKey, DefaultVkDeviceAutoSelectMode);
		const auto forceGPUName = settings.getOrSetDefault< std::string >(VkDeviceForceGPUKey);
		const auto useVMA = settings.getOrSetDefault< bool >(VkDeviceUseVMAKey, DefaultVkDeviceUseVMA);

		const std::vector< const char * > requiredExtensions;

		std::vector< ScoredPhysicalDevice > scoredDevices;
		scoredDevices.reserve(m_physicalDevices.size());

		for ( size_t enumerationIndex = 0; enumerationIndex < m_physicalDevices.size(); ++enumerationIndex )
		{
			const auto & physicalDevice = m_physicalDevices[enumerationIndex];
			size_t score = 1;

			if ( !this->checkDeviceAPIVersion(*physicalDevice) )
			{
				continue;
			}

			if ( !this->checkDeviceCompatibility(physicalDevice, VK_QUEUE_COMPUTE_BIT, score) )
			{
				continue;
			}

			if ( !Instance::checkDevicesFeaturesForCompute(physicalDevice, score) )
			{
				continue;
			}

			if ( !Instance::checkDeviceForRequiredExtensions(physicalDevice, requiredExtensions, score) )
			{
				continue;
			}

			if ( m_autoSelectMode == DeviceAutoSelectMode::Performance )
			{
				/* NOTE: More queues = higher score for performance. */
				score += physicalDevice->getTotalQueueCount() * 10;
			}
			else if ( m_autoSelectMode == DeviceAutoSelectMode::PowerSaving )
			{
				/* NOTE: Fewer queues = higher score for power saving (inverse relationship). */
				score += 100 / std::max(physicalDevice->getTotalQueueCount(), static_cast< size_t >(1));
			}

			this->modulateDeviceScoring(physicalDevice->propertiesVK10(), score);

			TraceInfo{ClassId} << "Physical device '" << physicalDevice->propertiesVK10().deviceName << "' reached score of " << score;

			scoredDevices.emplace_back(Instance::makeScoredDevice(physicalDevice, score, enumerationIndex));
		}

		this->rankDevices(scoredDevices);

		/* NOTE: Returns the best ranked device (the last). */
		if ( scoredDevices.empty() )
		{
			Tracer::fatal(ClassId, "There is no physical device (Compute) compatible with Vulkan.");

			return {};
		}

		/* NOTE: Owner ruling (2026-10-01): the forced GPU drives the compute device too, as it drives the graphics one,
		 * so a forced-GPU test runs physics on the same device; a forced GPU that cannot compute falls back to the score. */
		std::shared_ptr< PhysicalDevice > selectedPhysicalDevice;

		if ( !forceGPUName.empty() )
		{
			for ( const auto & scored : scoredDevices )
			{
				if ( scored.device->deviceName() == forceGPUName )
				{
					selectedPhysicalDevice = scored.device;

					break;
				}
			}

			if ( selectedPhysicalDevice == nullptr )
			{
				TraceWarning{ClassId} << "The forced physical device '" << forceGPUName << "' is not a compatible compute device ! Selecting by score.";
			}
		}

		if ( selectedPhysicalDevice != nullptr )
		{
			TraceSuccess{ClassId} << "Compute capable physical device '" << selectedPhysicalDevice->propertiesVK10().deviceName << "' selected (FORCED)";
		}
		else
		{
			selectedPhysicalDevice = scoredDevices.back().device;

			TraceSuccess{ClassId} << "Compute capable physical device '" << selectedPhysicalDevice->propertiesVK10().deviceName << "' selected (" << autoSelectModeString << " mode)";
		}

		/* NOTE: Logical device creation for computing. */
		auto logicalDevice = std::make_shared< Device >(*this, selectedPhysicalDevice->propertiesVK10().deviceName, selectedPhysicalDevice, m_showInformation);
		logicalDevice->setIdentifier(ClassId, (std::stringstream{} << selectedPhysicalDevice->propertiesVK10().deviceName << "(Physics)").str(), "Device");

		DeviceRequirements requirements{false, nullptr, true};

		/* NOTE: Mandatory when advertised, as on the graphics device (MoltenVK refused the compute device without it). */
		auto deviceExtensions = requiredExtensions;

		/* NOTE: Required, as on the graphics device: each queue numbers its tracked submissions with a timeline.
		 * Without it, the first queue fails to install and the compute device is refused. */
		this->requestTimelineSemaphore(*selectedPhysicalDevice, deviceExtensions, requirements);

		if ( Instance::requestPortabilitySubset(*selectedPhysicalDevice, deviceExtensions, requirements) )
		{
			Tracer::info(ClassId, "VK_KHR_portability_subset detected and enabled on the compute device.");
		}

		if ( !logicalDevice->create(requirements, deviceExtensions, useVMA) )
		{
			return {};
		}

		m_computeDevice = logicalDevice;

		return logicalDevice;
	}

	bool
	Instance::requestPortabilitySubset (const PhysicalDevice & physicalDevice, std::vector< const char * > & extensions, DeviceRequirements & requirements) noexcept
	{
		if ( !physicalDevice.hasPortabilitySubset() )
		{
			return false;
		}

		if ( std::ranges::none_of(extensions, [] (const char * name) { return std::strcmp(name, PortabilitySubset::ExtensionName) == 0; }) )
		{
			extensions.emplace_back(PortabilitySubset::ExtensionName);
		}

		/* NOTE: Enabling the extension is NOT enough. Every portability feature defaults to
		 * DISABLED, so the feature structure must be chained into VkDeviceCreateInfo::pNext with
		 * the features actually requested — otherwise the engine silently runs without
		 * capabilities the device has. Requesting exactly what the device reports is both
		 * maximal and safe: asking for an unsupported feature would fail device creation.
		 *
		 * This is what was missing when every shadow-map descriptor write was rejected with
		 * VUID-VkDescriptorImageInfo-mutableComparisonSamplers-04450: the M2 reports
		 * mutableComparisonSamplers = VK_TRUE, the engine simply never asked for it, so the
		 * descriptor stayed unwritten and every lit draw sampled undefined memory. */
		const auto & supported = physicalDevice.portabilitySubsetFeatures();
		auto & requested = requirements.portabilitySubsetFeatures();

		requested.constantAlphaColorBlendFactors = supported.constantAlphaColorBlendFactors;
		requested.events = supported.events;
		requested.imageViewFormatReinterpretation = supported.imageViewFormatReinterpretation;
		requested.imageViewFormatSwizzle = supported.imageViewFormatSwizzle;
		requested.imageView2DOn3DImage = supported.imageView2DOn3DImage;
		requested.multisampleArrayImage = supported.multisampleArrayImage;
		requested.mutableComparisonSamplers = supported.mutableComparisonSamplers;
		requested.pointPolygons = supported.pointPolygons;
		requested.samplerMipLodBias = supported.samplerMipLodBias;
		requested.separateStencilMaskRef = supported.separateStencilMaskRef;
		requested.shaderSampleRateInterpolationFunctions = supported.shaderSampleRateInterpolationFunctions;
		requested.tessellationIsolines = supported.tessellationIsolines;
		requested.tessellationPointMode = supported.tessellationPointMode;
		requested.triangleFans = supported.triangleFans;
		requested.vertexAttributeAccessBeyondStride = supported.vertexAttributeAccessBeyondStride;

		return true;
	}

	std::optional< std::vector< std::string > >
	Instance::parseValidationLayersArgument (const std::string & value) noexcept
	{
		/* NOTE: Locale-independent on purpose (std::isalnum follows the C locale). */
		const auto isLayerNameCharacter = [] (char character) noexcept {
			return ( character >= 'a' && character <= 'z' ) || ( character >= 'A' && character <= 'Z' ) || ( character >= '0' && character <= '9' ) || character == '_';
		};

		/* NOTE: What is echoed of a refused item is bounded: the value comes from the command line. */
		constexpr size_t EchoedCharacters{64};

		std::vector< std::string > layers;

		for ( const auto & item : String::explode(value, ',', false) )
		{
			auto name = String::trim(item);

			if ( name.empty() )
			{
				continue;
			}

			if ( name.size() >= VK_MAX_EXTENSION_NAME_SIZE || !std::ranges::all_of(name, isLayerNameCharacter) )
			{
				TraceError{ClassId} <<
					"'" << name.substr(0, EchoedCharacters) << "' is not a Vulkan layer name ([A-Za-z0-9_], fewer than " << VK_MAX_EXTENSION_NAME_SIZE << " characters) : " <<
					SetValidationLayersArgument << " is refused, the settings apply.";

				return std::nullopt;
			}

			if ( std::ranges::find(layers, name) != layers.cend() )
			{
				continue;
			}

			if ( layers.size() >= MaxValidationLayersFromArgument )
			{
				TraceError{ClassId} << SetValidationLayersArgument << " lists more than " << MaxValidationLayersFromArgument << " layers : it is refused, the settings apply.";

				return std::nullopt;
			}

			layers.emplace_back(std::move(name));
		}

		return layers;
	}

	std::vector< const char * >
	Instance::getSupportedValidationLayers (const std::vector< std::string > & requestedValidationLayers, const std::vector< VkLayerProperties > & availableValidationLayers) noexcept
	{
		std::vector< const char * > supportedValidationLayers;

		for ( const auto & requestedValidationLayer : requestedValidationLayers )
		{
			auto layerFound = false;

			for ( const auto & availableValidationLayer : availableValidationLayers )
			{
				if ( std::strcmp(requestedValidationLayer.c_str(), availableValidationLayer.layerName ) == 0 )
				{
					supportedValidationLayers.emplace_back(requestedValidationLayer.c_str());

					layerFound = true;

					break;
				}
			}

			if ( !layerFound )
			{
				TraceWarning{ClassId} << "The requested '" << requestedValidationLayer << "' validation layer is unavailable !";
			}
		}

		return supportedValidationLayers;
	}

	void
	Instance::modulateDeviceScoring (const VkPhysicalDeviceProperties & deviceProperties, size_t & score) const noexcept
	{
		switch ( m_autoSelectMode )
		{
			case DeviceAutoSelectMode::Performance :
				switch ( deviceProperties.deviceType )
				{
					case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU :
						score *= 3;
						break;

					case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU :
						score *= 5;
						break;

					case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU :
						score *= 2;
						break;

					default:
						break;
				}
				break;

			case DeviceAutoSelectMode::PowerSaving :
				switch ( deviceProperties.deviceType )
				{
					case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU :
						score *= 5;
						break;

					case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU :
						score *= 1;
						break;

					case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU :
						score *= 2;
						break;

					case VK_PHYSICAL_DEVICE_TYPE_CPU :
						score *= 3;
						break;

					default:
						break;
				}
				break;

			/* NOTE: RawPerformance ranks by type and VRAM (rankDevices()), the score only breaks ties; DontCare keeps the
			 * plain score. */
			case DeviceAutoSelectMode::DontCare :
			case DeviceAutoSelectMode::RawPerformance :
				break;
		}
	}

	bool
	Instance::checkDeviceCompatibility (const std::shared_ptr< PhysicalDevice > & physicalDevice, VkQueueFlagBits type, size_t & /*score*/) const noexcept
	{
		return std::ranges::any_of(physicalDevice->queueFamilyPropertiesVK11(), [type] (const auto & queueFamilyProperty) {
			return (queueFamilyProperty.queueFamilyProperties.queueFlags & static_cast< VkQueueFlags >(type)) != 0;
		});
	}

	bool
	Instance::checkDeviceCompatibility (const std::shared_ptr< PhysicalDevice > & physicalDevice, const Window * window, size_t & score) const noexcept
	{
		for ( const auto & queueFamilyProperty : physicalDevice->queueFamilyPropertiesVK11() )
		{
			/* Must be a graphics family queue. */
			if ( (queueFamilyProperty.queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0 )
			{
				continue;
			}

			/* Must support the presentation, have valid presents modes and surface formats. */
			if ( !window->surface()->presentationSupported() || window->surface()->presentModes().empty() || window->surface()->formats().empty() )
			{
				continue;
			}

			TraceSuccess{ClassId} << "'" << physicalDevice->deviceName() << "' support presentation!";

			score += window->surface()->formats().size();
			score += window->surface()->presentModes().size();

			return true;
		}

		TraceWarning{ClassId} << "'" << physicalDevice->deviceName() << "' no not support presentation!";

		return false;
	}

	bool
	Instance::checkDevicesFeaturesForGraphics (const std::shared_ptr< PhysicalDevice > & physicalDevice, size_t & /*score*/) const noexcept
	{
		const auto & properties = physicalDevice->propertiesVK10();
		const auto & features = physicalDevice->featuresVK10();

		if ( features.robustBufferAccess == 0 )
		{
			TraceWarning{ClassId} << "The physical device '" << properties.deviceName << "' is missing 'robustBufferAccess' feature !";

			//return false;
		}

		if ( features.fullDrawIndexUint32 == 0 )
		{
			TraceWarning{ClassId} << "The physical device '" << properties.deviceName << "' is missing 'fullDrawIndexUint32' feature !";

			//return false;
		}

		if ( features.imageCubeArray == 0 )
		{
			TraceWarning{ClassId} << "The physical device '" << properties.deviceName << "' is missing 'imageCubeArray' feature !";

			//return false;
		}

		if ( features.independentBlend == 0 )
		{
			TraceWarning{ClassId} << "The physical device '" << properties.deviceName << "' is missing 'independentBlend' feature !";

			//return false;
		}

		if ( features.geometryShader == 0 )
		{
			TraceWarning{ClassId} << "The physical device '" << properties.deviceName << "' is missing 'geometryShader' feature !";

			//return false;
		}

		if ( features.tessellationShader == 0 )
		{
			TraceWarning{ClassId} << "The physical device '" << properties.deviceName << "' is missing 'tessellationShader' feature !";

			//return false;
		}

		if ( features.sampleRateShading == 0 )
		{
			// TODO: Maybe incorrect assertion !
			if ( m_primaryServices.settings().getOrSetDefault< uint32_t >(VideoFramebufferSamplesKey, DefaultVideoFramebufferSamples) > 1 )
			{
				TraceError{ClassId} <<
					"MSAA is enabled in settings !"
					"The physical device '" << properties.deviceName << "' cannot perform multisampling !";

				return false;
			}

			TraceWarning{ClassId} << "The physical device '" << properties.deviceName << "' is missing 'sampleRateShading' feature !";
		}

		if ( features.dualSrcBlend == 0 )
		{
			TraceWarning{ClassId} << "The physical device '" << properties.deviceName << "' is missing 'dualSrcBlend' feature !";

			//return false;
		}

		if ( features.logicOp == 0 )
		{
			TraceWarning{ClassId} << "The physical device '" << properties.deviceName << "' is missing 'logicOp' feature !";

			//return false;
		}

		if ( features.multiDrawIndirect == 0 )
		{
			TraceWarning{ClassId} << "The physical device '" << properties.deviceName << "' is missing 'multiDrawIndirect' feature !";

			//return false;
		}

		if ( features.drawIndirectFirstInstance == 0 )
		{
			TraceWarning{ClassId} << "The physical device '" << properties.deviceName << "' is missing 'drawIndirectFirstInstance' feature !";

			//return false;
		}

		if ( features.depthClamp == 0 )
		{
			TraceWarning{ClassId} << "The physical device '" << properties.deviceName << "' is missing 'depthClamp' feature !";

			//return false;
		}

		if ( features.depthBiasClamp == 0 )
		{
			TraceWarning{ClassId} << "The physical device '" << properties.deviceName << "' is missing 'depthBiasClamp' feature !";

			//return false;
		}

		if ( features.fillModeNonSolid == 0 )
		{
			TraceWarning{ClassId} << "The physical device '" << properties.deviceName << "' is missing 'fillModeNonSolid' feature !";

			//return false;
		}

		if ( features.depthBounds == 0 )
		{
			TraceWarning{ClassId} << "The physical device '" << properties.deviceName << "' is missing 'depthBounds' feature !";

			//return false;
		}

		if ( features.wideLines == 0 )
		{
			TraceWarning{ClassId} << "The physical device '" << properties.deviceName << "' is missing 'wideLines' feature !";

			//return false;
		}

		if ( features.largePoints == 0 )
		{
			TraceWarning{ClassId} << "The physical device '" << properties.deviceName << "' is missing 'largePoints' feature !";

			//return false;
		}

		if ( features.alphaToOne == 0 )
		{
			TraceWarning{ClassId} << "The physical device '" << properties.deviceName << "' is missing 'alphaToOne' feature !";

			//return false;
		}

		if ( features.multiViewport == 0 )
		{
			TraceWarning{ClassId} << "The physical device '" << properties.deviceName << "' is missing 'multiViewport' feature !";

			//return false;
		}

		if ( features.samplerAnisotropy == 0 )
		{
			TraceWarning{ClassId} << "The physical device '" << properties.deviceName << "' is missing 'samplerAnisotropy' feature !";

			//return false;
		}

		if ( features.shaderImageGatherExtended == 0 )
		{
			TraceWarning{ClassId} << "The physical device '" << properties.deviceName << "' is missing 'shaderImageGatherExtended' feature !";

			//return false;
		}

		if ( features.textureCompressionETC2 == 0 )
		{
			TraceWarning{ClassId} << "The physical device '" << properties.deviceName << "' is missing 'textureCompressionETC2' feature !";

			//return false;
		}

		if ( features.textureCompressionASTC_LDR == 0 )
		{
			TraceWarning{ClassId} << "The physical device '" << properties.deviceName << "' is missing 'textureCompressionASTC_LDR' feature !";

			//return false;
		}

		if ( features.textureCompressionBC == 0 )
		{
			TraceWarning{ClassId} << "The physical device '" << properties.deviceName << "' is missing 'textureCompressionBC' feature !";

			//return false;
		}

		if ( features.occlusionQueryPrecise == 0 )
		{
			TraceWarning{ClassId} << "The physical device '" << properties.deviceName << "' is missing 'occlusionQueryPrecise' feature !";

			//return false;
		}

		if ( features.pipelineStatisticsQuery == 0 )
		{
			TraceWarning{ClassId} << "The physical device '" << properties.deviceName << "' is missing 'pipelineStatisticsQuery' feature !";

			//return false;
		}

		if ( features.vertexPipelineStoresAndAtomics == 0 )
		{
			TraceWarning{ClassId} << "The physical device '" << properties.deviceName << "' is missing 'vertexPipelineStoresAndAtomics' feature !";

			//return false;
		}

		if ( features.fragmentStoresAndAtomics == 0 )
		{
			TraceWarning{ClassId} << "The physical device '" << properties.deviceName << "' is missing 'fragmentStoresAndAtomics' feature !";

			//return false;
		}

		if ( features.shaderTessellationAndGeometryPointSize == 0 )
		{
			TraceWarning{ClassId} << "The physical device '" << properties.deviceName << "' is missing 'shaderTessellationAndGeometryPointSize' feature !";

			//return false;
		}

		if ( features.shaderImageGatherExtended == 0 )
		{
			TraceWarning{ClassId} << "The physical device '" << properties.deviceName << "' is missing 'shaderImageGatherExtended' feature !";

			//return false;
		}

		if ( features.shaderStorageImageExtendedFormats == 0 )
		{
			TraceWarning{ClassId} << "The physical device '" << properties.deviceName << "' is missing 'shaderStorageImageExtendedFormats' feature !";

			//return false;
		}

		if ( features.shaderStorageImageMultisample == 0 )
		{
			TraceWarning{ClassId} << "The physical device '" << properties.deviceName << "' is missing 'shaderStorageImageMultisample' feature !";

			//return false;
		}

		if ( features.shaderStorageImageReadWithoutFormat == 0 )
		{
			TraceWarning{ClassId} << "The physical device '" << properties.deviceName << "' is missing 'shaderStorageImageReadWithoutFormat' feature !";

			//return false;
		}

		if ( features.shaderStorageImageWriteWithoutFormat == 0 )
		{
			TraceWarning{ClassId} << "The physical device '" << properties.deviceName << "' is missing 'shaderStorageImageWriteWithoutFormat' feature !";

			//return false;
		}

		if ( features.shaderUniformBufferArrayDynamicIndexing == 0 )
		{
			TraceWarning{ClassId} << "The physical device '" << properties.deviceName << "' is missing 'shaderUniformBufferArrayDynamicIndexing' feature !";

			//return false;
		}

		if ( features.shaderSampledImageArrayDynamicIndexing == 0 )
		{
			TraceWarning{ClassId} << "The physical device '" << properties.deviceName << "' is missing 'shaderSampledImageArrayDynamicIndexing' feature !";

			//return false;
		}

		if ( features.shaderStorageBufferArrayDynamicIndexing == 0 )
		{
			TraceWarning{ClassId} << "The physical device '" << properties.deviceName << "' is missing 'shaderStorageBufferArrayDynamicIndexing' feature !";

			//return false;
		}

		if ( features.shaderStorageImageArrayDynamicIndexing == 0 )
		{
			TraceWarning{ClassId} << "The physical device '" << properties.deviceName << "' is missing 'shaderStorageImageArrayDynamicIndexing' feature !";

			//return false;
		}

		if ( features.shaderClipDistance == 0 )
		{
			TraceWarning{ClassId} << "The physical device '" << properties.deviceName << "' is missing 'shaderClipDistance' feature !";

			//return false;
		}

		if ( features.shaderCullDistance == 0 )
		{
			TraceWarning{ClassId} << "The physical device '" << properties.deviceName << "' is missing 'shaderCullDistance' feature !";

			//return false;
		}

		if ( features.shaderFloat64 == 0 )
		{
			TraceWarning{ClassId} << "The physical device '" << properties.deviceName << "' is missing 'shaderFloat64' feature !";

			//return false;
		}

		if ( features.shaderInt64 == 0 )
		{
			TraceWarning{ClassId} << "The physical device '" << properties.deviceName << "' is missing 'shaderInt64' feature !";

			//return false;
		}

		if ( features.shaderInt16 == 0 )
		{
			TraceWarning{ClassId} << "The physical device '" << properties.deviceName << "' is missing 'shaderInt16' feature !";

			//return false;
		}

		if ( features.shaderResourceResidency == 0 )
		{
			TraceWarning{ClassId} << "The physical device '" << properties.deviceName << "' is missing 'shaderResourceResidency' feature !";

			//return false;
		}

		if ( features.shaderResourceMinLod == 0 )
		{
			TraceWarning{ClassId} << "The physical device '" << properties.deviceName << "' is missing 'shaderResourceMinLod' feature !";

			//return false;
		}

		if ( features.sparseBinding == 0 )
		{
			TraceWarning{ClassId} << "The physical device '" << properties.deviceName << "' is missing 'sparseBinding' feature !";

			//return false;
		}

		if ( features.sparseResidencyBuffer == 0 )
		{
			TraceWarning{ClassId} << "The physical device '" << properties.deviceName << "' is missing 'sparseResidencyBuffer' feature !";

			//return false;
		}

		if ( features.sparseResidencyImage2D == 0 )
		{
			TraceWarning{ClassId} << "The physical device '" << properties.deviceName << "' is missing 'sparseResidencyImage2D' feature !";

			//return false;
		}

		if ( features.sparseResidencyImage3D == 0 )
		{
			TraceWarning{ClassId} << "The physical device '" << properties.deviceName << "' is missing 'sparseResidencyImage3D' feature !";

			//return false;
		}

		if ( features.sparseResidency2Samples == 0 )
		{
			TraceWarning{ClassId} << "The physical device '" << properties.deviceName << "' is missing 'sparseResidency2Samples' feature !";

			//return false;
		}

		if ( features.sparseResidency4Samples == 0 )
		{
			TraceWarning{ClassId} << "The physical device '" << properties.deviceName << "' is missing 'sparseResidency4Samples' feature !";

			//return false;
		}

		if ( features.sparseResidency8Samples == 0 )
		{
			TraceWarning{ClassId} << "The physical device '" << properties.deviceName << "' is missing 'sparseResidency8Samples' feature !";

			//return false;
		}

		if ( features.sparseResidency16Samples == 0 )
		{
			TraceWarning{ClassId} << "The physical device '" << properties.deviceName << "' is missing 'sparseResidency16Samples' feature !";

			//return false;
		}

		if ( features.sparseResidencyAliased == 0 )
		{
			TraceWarning{ClassId} << "The physical device '" << properties.deviceName << "' is missing 'sparseResidencyAliased' feature !";

			//return false;
		}

		if ( features.variableMultisampleRate == 0 )
		{
			TraceWarning{ClassId} << "The physical device '" << properties.deviceName << "' is missing 'variableMultisampleRate' feature !";

			//return false;
		}

		if ( features.inheritedQueries == 0 )
		{
			TraceWarning{ClassId} << "The physical device '" << properties.deviceName << "' is missing 'inheritedQueries' feature !";

			//return false;
		}

		/* NOTE: Below the scene-rendering target the engine draws the overlay only: no bindless, nothing more to check. */
		if ( !this->sceneRenderingEnabled() )
		{
			return true;
		}

		/* Vulkan 1.2 features - Descriptor indexing (required for bindless textures). */
		const auto & featuresVK12 = physicalDevice->featuresVK12();

		if ( featuresVK12.descriptorIndexing == 0 )
		{
			TraceError{ClassId} << "The physical device '" << properties.deviceName << "' is missing 'descriptorIndexing' feature !";

			return false;
		}

		if ( featuresVK12.descriptorBindingPartiallyBound == 0 )
		{
			TraceError{ClassId} << "The physical device '" << properties.deviceName << "' is missing 'descriptorBindingPartiallyBound' feature !";

			return false;
		}

		if ( featuresVK12.descriptorBindingSampledImageUpdateAfterBind == 0 )
		{
			TraceError{ClassId} << "The physical device '" << properties.deviceName << "' is missing 'descriptorBindingSampledImageUpdateAfterBind' feature !";

			return false;
		}

		if ( featuresVK12.runtimeDescriptorArray == 0 )
		{
			TraceError{ClassId} << "The physical device '" << properties.deviceName << "' is missing 'runtimeDescriptorArray' feature !";

			return false;
		}

		if ( featuresVK12.shaderSampledImageArrayNonUniformIndexing == 0 )
		{
			TraceError{ClassId} << "The physical device '" << properties.deviceName << "' is missing 'shaderSampledImageArrayNonUniformIndexing' feature !";

			return false;
		}

		return true;
	}

	bool
	Instance::checkDevicesFeaturesForCompute (const std::shared_ptr< PhysicalDevice > & physicalDevice, size_t & /*score*/) noexcept
	{
		/* NOTE: Every queue of a device numbers its tracked submissions with a timeline semaphore (Queue::createTimeline()),
		 * the compute device's queues included. Core from Vulkan 1.2, VK_KHR_timeline_semaphore below
		 * (checkDeviceAPIVersion() refused a device without). */
		if ( !physicalDevice->supportsTimelineSemaphore() )
		{
			TraceError{ClassId} << "The physical device '" << physicalDevice->propertiesVK10().deviceName << "' is missing 'timelineSemaphore' feature !";

			return false;
		}

		return true;
	}

	bool
	Instance::checkDeviceForRequiredExtensions (const std::shared_ptr< PhysicalDevice > & physicalDevice, const std::vector< const char * > & requiredExtensions, size_t & score) noexcept
	{
		const auto & properties = physicalDevice->propertiesVK10();
		const auto & extensions = physicalDevice->getExtensions();

		score += extensions.size();

		/* NOTE: If no requirements, we can stop here. */
		if ( requiredExtensions.empty() )
		{
			return true;
		}

		if ( extensions.empty() )
		{
			return false;
		}

		for ( const auto & requiredExtension : requiredExtensions )
		{
			const auto found = std::ranges::any_of(extensions, [requiredExtension] (const auto & extension) {
				return std::strcmp(requiredExtension, extension.extensionName) == 0;
			});

			/* NOTE: Missing required extension. */
			if ( !found )
			{
				TraceError{ClassId} << "The physical device '" << properties.deviceName << "' is missing the required '" << requiredExtension << "' extension !";

				return false;
			}
		}

		return true;
	}
}
