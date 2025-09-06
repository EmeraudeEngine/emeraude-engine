/*
 * src/Vulkan/Instance.cpp
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

#include "Instance.hpp"

/* STL inclusions. */
#include <cstring>
#include <algorithm>
#include <utility>
#include <sstream>

/* Third-party inclusions. */
#define GLFW_INCLUDE_NONE
#include "GLFW/glfw3.h"
#include "magic_enum/magic_enum.hpp"

/* Local inclusions. */
#include "PhysicalDevice.hpp"
#include "Device.hpp"
#include "DeviceRequirements.hpp"
#include "Utility.hpp"
#include "Identification.hpp"
#include "PrimaryServices.hpp"
#include "Window.hpp"
#include "SettingKeys.hpp"

namespace EmEn::Vulkan
{
	using namespace Libs;

	void
	Instance::readSettings () noexcept
	{
		m_showInformation = m_primaryServices.settings().getOrSetDefault< bool >(VkShowInformationKey, DefaultVkShowInformation);

		m_debugMode =
			m_primaryServices.arguments().isSwitchPresent("--debug-vulkan") ||
			m_primaryServices.settings().getOrSetDefault< bool >(VkInstanceEnableDebugKey, DefaultVkInstanceEnableDebug);

		/* NOTE: Only if the validation layer is enabled. */
		if ( this->isDebugModeEnabled() )
		{
			/* Enable the vulkan debug messenger. */
			m_useDebugMessenger = m_primaryServices.settings().getOrSetDefault< bool >(VkInstanceUseDebugMessengerKey, DefaultVkInstanceUseDebugMessenger);

			if ( this->isUsingDebugMessenger() )
			{
				m_debugCreateInfo = DebugMessenger::getCreateInfo();
			}
		}
	}

	bool
	Instance::onInitialize () noexcept
	{
		this->readSettings();

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
			m_applicationInfo.pEngineName = Identification::LibraryName;
			m_applicationInfo.engineVersion = VK_MAKE_VERSION(Identification::LibraryVersion.major(), Identification::LibraryVersion.minor(), Identification::LibraryVersion.revision());
			/* [VULKAN-API-SETUP] Vulkan API version selection. */
			if constexpr ( IsMacOS )
			{
				/* NOTE: macOS don't support the Vulkan API.
				 * MoltenVK is used to translate commands to Metal API, some features can be unsupported. */
				m_applicationInfo.apiVersion = VK_API_VERSION_1_3;
				m_createInfo.flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
			}
			else
			{
				m_applicationInfo.apiVersion = VK_API_VERSION_1_3;
				m_createInfo.flags = 0;
			}
			m_createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
			m_createInfo.pNext = this->isUsingDebugMessenger() ? &m_debugCreateInfo : nullptr;
			m_createInfo.pApplicationInfo = &m_applicationInfo;
			m_createInfo.enabledLayerCount = static_cast< uint32_t >(m_requiredValidationLayers.size());
			m_createInfo.ppEnabledLayerNames = m_requiredValidationLayers.data();
			m_createInfo.enabledExtensionCount = static_cast< uint32_t >(m_requiredInstanceExtensions.size());
			m_createInfo.ppEnabledExtensionNames = m_requiredInstanceExtensions.data();

			/* At this point, we create the vulkan instance.
			 * Beyond this point, Vulkan is in the pipe and usable. */
			const auto result = vkCreateInstance(&m_createInfo, nullptr, &m_instance);

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
		}

		if ( m_graphicsDevice != nullptr )
		{
			m_graphicsDevice->destroy();

			if ( m_graphicsDevice.use_count() > 1 )
			{
				TraceError{ClassId} << "The Vulkan selected graphics device '" << m_graphicsDevice->identifier() << "' smart pointer still have " << m_graphicsDevice.use_count() << " uses !";
			}
		}

		m_physicalDevices.clear();
		m_debugMessenger.reset();

		if ( m_instance != VK_NULL_HANDLE )
		{
			vkDestroyInstance(m_instance, nullptr);

			m_instance = VK_NULL_HANDLE;
		}

		m_requiredValidationLayers.clear();
		m_requiredInstanceExtensions.clear();
		m_requiredGraphicsDeviceExtensions.clear();

		return true;
	}

	bool
	Instance::preparePhysicalDevices () noexcept
	{
		std::vector< VkPhysicalDevice > physicalDevices{};

		uint32_t count = 0;

		auto result = vkEnumeratePhysicalDevices(m_instance, &count, nullptr);

		if ( result == VK_SUCCESS )
		{
			if ( count > 0 )
			{
				physicalDevices.resize(count);

				result = vkEnumeratePhysicalDevices(m_instance, &count, physicalDevices.data());

				if ( result != VK_SUCCESS )
				{
					TraceError{ClassId} << "Unable to get physical devices : " << vkResultToCString(result) << " !";

					return false;
				}
			}
		}
		else
		{
			TraceError{ClassId} << "Unable to get physical device count : " << vkResultToCString(result) << " !";

			return false;
		}

		m_physicalDevices.reserve(physicalDevices.size());

		std::ranges::transform(std::as_const(physicalDevices), std::back_inserter(m_physicalDevices), [] (const auto & physicalDevice) {
			return std::make_shared< PhysicalDevice >(physicalDevice);
		});

		return !m_physicalDevices.empty();
	}

	void
	Instance::configureValidationLayers () noexcept
	{
		/* [VULKAN-API-SETUP] Vulkan validation layers selection. */
		auto & settings = m_primaryServices.settings();

		const auto availableValidationLayers = Instance::getAvailableValidationLayers();

		/* NOTE: Save a copy of validation layers in settings for an easy settings edition. */
		if ( settings.isArrayEmpty(VkInstanceAvailableValidationLayersKey) )
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

		/* NOTE: Read the settings to get the desired validation layers. */
		static const auto desiredValidationLayers = m_primaryServices.settings().getArrayAs< std::string >(VkInstanceRequestedValidationLayersKey);

		if ( settings.isArrayEmpty(VkInstanceRequestedValidationLayersKey) )
		{
			TraceInfo{ClassId} <<
				"No validation layer is requested from settings !" "\n"
				"NOTE: You can change the validation layers selected in settings at the array key : '" << VkInstanceRequestedValidationLayersKey << "'.";

			return;
		}

		/* NOTE: Show desired validation layers from the settings. */
		if ( m_showInformation )
		{
			TraceInfo trace{ClassId};

			trace << "Desired Vulkan validation layers from settings :" "\n";

			for ( const auto & requestedValidationLayer : desiredValidationLayers )
			{
				trace << "\t" << requestedValidationLayer << "\n";
			}
		}

		/* NOTE: Here we check if the desired validations layers are available and create the vector for the instance createInfo.  */
		m_requiredValidationLayers = Instance::getSupportedValidationLayers(desiredValidationLayers, availableValidationLayers);

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

		/* If debug mode enabled, push back debug utilities. */
		if ( this->isDebugModeEnabled() )
		{
			//m_requiredGraphicsDeviceExtensions.emplace_back(VK_EXT_DEVICE_FAULT_EXTENSION_NAME);

			if ( this->isUsingDebugMessenger() )
			{
				/* NOTE: VK_EXT_debug_report (vk 1.0) has been deprecated in favor of VK_EXT_debug_utils (vk 1.2 ?). */
				//m_requiredInstanceExtensions.emplace_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
				m_requiredInstanceExtensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
			}
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

		uint32_t count = 0;

		auto result = vkEnumerateInstanceLayerProperties(&count, nullptr);

		if ( result == VK_SUCCESS )
		{
			if ( count > 0 )
			{
				validationLayers.resize(count);

				result = vkEnumerateInstanceLayerProperties(&count, validationLayers.data());

				if ( result != VK_SUCCESS )
				{
					TraceError{ClassId} << "Unable to get instance validation layers : " << vkResultToCString(result) << " !";
				}
			}
		}
		else
		{
			TraceError{ClassId} << "Unable to get instance validation layer count : " << vkResultToCString(result) << " !";
		}

		return validationLayers;
	}

	std::vector< VkExtensionProperties >
	Instance::getExtensions (const char * pLayerName) noexcept
	{
		std::vector< VkExtensionProperties > extensions{};

		uint32_t count = 0;

		auto result = vkEnumerateInstanceExtensionProperties(pLayerName, &count, nullptr);

		if ( result == VK_SUCCESS )
		{
			if ( count > 0 )
			{
				extensions.resize(count);

				result = vkEnumerateInstanceExtensionProperties(pLayerName, &count, extensions.data());

				if ( result != VK_SUCCESS )
				{
					TraceError{ClassId} << "Unable to get instance extensions : " << vkResultToCString(result) << " !";
				}
			}
		}
		else
		{
			TraceError{ClassId} << "Unable to get instance extension count : " << vkResultToCString(result) << " !";
		}

		return extensions;
	}

	VkFormat
	Instance::findColorFormat (const std::shared_ptr< Device > & device, uint32_t /*redBits*/, uint32_t /*greenBits*/, uint32_t /*blueBits*/, uint32_t /*alphaBits*/) noexcept
	{
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

	std::map< size_t, std::shared_ptr< PhysicalDevice > >
	Instance::getScoredGraphicsDevices (Window * window, DeviceRunMode runMode) const noexcept
	{
		std::map< size_t, std::shared_ptr< PhysicalDevice > > graphicsDevices{};

		for ( const auto & physicalDevice : m_physicalDevices )
		{
			size_t score = 0;

			if ( m_showInformation )
			{
				TraceInfo{ClassId} << physicalDevice->getPhysicalDeviceInformation();
			}

			if ( window != nullptr && window->usable() )
			{
				window->surface()->update(physicalDevice);

				if ( !Instance::checkDeviceCompatibility(physicalDevice, runMode, window, score) )
				{
					continue;
				}
			}
			else
			{
				if ( !Instance::checkDeviceCompatibility(physicalDevice, runMode, VK_QUEUE_GRAPHICS_BIT, score) )
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

			score += physicalDevice->getTotalQueueCount() * 100;

			if ( m_showInformation )
			{
				TraceInfo(ClassId) << "Physical device '" << physicalDevice->propertiesVK10().deviceName << "' reached a score of " << score << " !";
			}

			graphicsDevices.emplace(score, physicalDevice);
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
		const auto runModeString = settings.getOrSetDefault< std::string >(VkDeviceAutoSelectModeKey, DefaultVkDeviceAutoSelectMode);
		const auto forceGPUName = settings.getOrSetDefault< std::string >(VkDeviceForceGPUKey);
		const auto showInformation = settings.getOrSetDefault< bool >(VkShowInformationKey, DefaultVkShowInformation);

		/* NOTE: Get a list of available devices. */
		const auto scoredDevices = this->getScoredGraphicsDevices(window, magic_enum::enum_cast< DeviceRunMode >(runModeString).value());

		/* NOTE: Returns the device with the highest score. */
		if ( !scoredDevices.empty() )
		{
			settings.clearArray(VkDeviceAvailableGPUsKey);

			for ( const auto & physicalDevice : scoredDevices | std::views::values )
			{
				settings.setInArray(VkDeviceAvailableGPUsKey, physicalDevice->deviceName());
			}
		}
		else
		{
			Tracer::error(ClassId, "There is no physical device compatible with Vulkan.");

			return {};
		}

		std::shared_ptr< PhysicalDevice > selectedPhysicalDevice;

		if ( !forceGPUName.empty() )
		{
			TraceInfo{ClassId} << "Trying to force the GPU named '" << forceGPUName << "' ...";

			for ( const auto & physicalDevice: scoredDevices | std::views::values )
			{
				if ( physicalDevice->deviceName() == forceGPUName )
				{
					selectedPhysicalDevice = physicalDevice;

					break;
				}
			}
		}

		/* NOTE: If no GPU was forced or not found, which the best one. */
		if ( selectedPhysicalDevice == nullptr )
		{
			selectedPhysicalDevice = scoredDevices.rbegin()->second;
		}

		/* NOTE: Logical device creation for graphics rendering and presentation. */
		TraceSuccess{ClassId} << "The graphics capable physical device '" << selectedPhysicalDevice->propertiesVK10().deviceName << "' selected ! ";

		auto logicalDevice = std::make_shared< Device >(selectedPhysicalDevice->propertiesVK10().deviceName, selectedPhysicalDevice, showInformation);
		logicalDevice->setIdentifier(ClassId, (std::stringstream{} << selectedPhysicalDevice->propertiesVK10().deviceName << "(Graphics)").str(), "Device");

		/* [VULKAN-API-SETUP] Graphics device features configuration. */
		DeviceRequirements requirements{DeviceWorkType::Graphics};
		requirements.featuresVK10().fillModeNonSolid = VK_TRUE; // Required for wireframe mode!
		if constexpr ( !IsMacOS )
		{
			/* NOTE: macOS M1/M2/M3/M4 iGPU do not have the geometry shader stage. */
			requirements.featuresVK10().geometryShader = VK_TRUE; // Required for TBN space display
		}
		requirements.featuresVK10().samplerAnisotropy = VK_TRUE;
		requirements.featuresVK13().shaderDemoteToHelperInvocation = VK_TRUE;
		requirements.requireGraphicsQueues({1.0F}, {0.5F});
		requirements.requireTransferQueues({1.0F});

		if ( window != nullptr && window->usable() )
		{
			/* NOTE: Be sure the selected device is the one that update the surface. */
			window->surface()->update(logicalDevice);

			requirements.requirePresentationQueues({1.0F}, window->surface()->handle(), false);
		}

		if ( !logicalDevice->create(requirements, m_requiredGraphicsDeviceExtensions) )
		{
			return {};
		}

		m_graphicsDevice = logicalDevice;

		/* NOTE: Basic GPU do not support flexible textures. */
		m_standardTextureCheckEnabled = m_graphicsDevice->hasBasicSupport();

		return logicalDevice;
	}

	std::shared_ptr< Device >
	Instance::getComputeDevice () noexcept
	{
		if ( m_computeDevice != nullptr )
		{
			return m_computeDevice;
		}

		std::vector< const char * > requiredExtensions;

		std::map< size_t, std::shared_ptr< PhysicalDevice > > scoredDevices;

		for ( const auto & physicalDevice : m_physicalDevices )
		{
			size_t score = 0;

			if ( !Instance::checkDeviceCompatibility(physicalDevice, DeviceRunMode::Performance, VK_QUEUE_COMPUTE_BIT, score) )
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

			if ( m_showInformation )
			{
				TraceInfo{ClassId} << "Physical device '" << physicalDevice->propertiesVK10().deviceName << "' reached score of " << score;
			}

			scoredDevices.emplace(score, physicalDevice);
		}

		/* NOTE: Returns the device with the highest score. */
		if ( scoredDevices.empty() )
		{
			Tracer::fatal(ClassId, "There is no physical device compatible with Vulkan.");

			return {};
		}

		auto & settings = m_primaryServices.settings();
		//const auto forceGPUName =settings.getOrSetDefault< std::string >(VkDeviceForceGPUKey);
		const bool showInformation = settings.getOrSetDefault< bool >(VkShowInformationKey, DefaultVkShowInformation);

		const auto & selectedPhysicalDevice = scoredDevices.rbegin()->second;

		TraceSuccess{ClassId} <<
			"Compute capable physical device '" << selectedPhysicalDevice->propertiesVK10().deviceName << "' selected ! "
			"Creating the logical compute device ...";

		/* NOTE: Logical device creation for computing. */
		auto logicalDevice = std::make_shared< Device >(selectedPhysicalDevice->propertiesVK10().deviceName, selectedPhysicalDevice, showInformation);
		logicalDevice->setIdentifier(ClassId, (std::stringstream{} << selectedPhysicalDevice->propertiesVK10().deviceName << "(Physics)").str(), "Device");

		DeviceRequirements requirements{DeviceWorkType::Compute};
		requirements.requireComputeQueues({1.0F}, {0.5F});
		requirements.requireTransferQueues({1.0F});

		if ( !logicalDevice->create(requirements, requiredExtensions) )
		{
			return {};
		}

		m_computeDevice = logicalDevice;

		return logicalDevice;
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
	Instance::modulateDeviceScoring (const VkPhysicalDeviceProperties & deviceProperties, DeviceRunMode runMode, size_t & score) noexcept
	{
		switch ( runMode )
		{
			case DeviceRunMode::Performance :
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

			case DeviceRunMode::PowerSaving :
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

			case DeviceRunMode::DontCare :
				switch ( deviceProperties.deviceType )
				{
					case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU :
						score *= 3;
						break;

					case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU :
						score *= 2;
						break;

					default :
						break;
				}
				break;
		}
	}

	bool
	Instance::checkDeviceCompatibility (const std::shared_ptr< PhysicalDevice > & physicalDevice, DeviceRunMode runMode, VkQueueFlagBits type, size_t & score) noexcept
	{
		for ( const auto & queueFamilyProperty : physicalDevice->queueFamilyPropertiesVK11() )
		{
			if ( (queueFamilyProperty.queueFamilyProperties.queueFlags & type) == 0 )
			{
				continue;
			}

			Instance::modulateDeviceScoring(physicalDevice->propertiesVK10(), runMode, score);

			return true;
		}

		return false;
	}

	bool
	Instance::checkDeviceCompatibility (const std::shared_ptr< PhysicalDevice > & physicalDevice, DeviceRunMode runMode, const Window * window, size_t & score) noexcept
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

			score += window->surface()->formats().size();
			score += window->surface()->presentModes().size();

			Instance::modulateDeviceScoring(physicalDevice->propertiesVK10(), runMode, score);

			return true;
		}

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

		return true;
	}

	bool
	Instance::checkDevicesFeaturesForCompute (const std::shared_ptr< PhysicalDevice > & /*physicalDevice*/, size_t & /*score*/) noexcept
	{
		//const auto & properties = physicalDevice->properties();
		//const auto & features = physicalDevice->features();

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
