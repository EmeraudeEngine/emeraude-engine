/*
 * src/Vulkan/PhysicalDevice.cpp
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

#include "PhysicalDevice.hpp"

/* STL inclusions. */
#include <sstream>
#include <iomanip>
#include <iterator>

/* Local inclusions. */
#include "Libs/String.hpp"
#include "Utility.hpp"
#include "Tracer.hpp"

namespace EmEn::Vulkan
{
	using namespace Libs;

	PhysicalDevice::PhysicalDevice (const VkPhysicalDevice & physicalDevice) noexcept
		: m_physicalDevice{physicalDevice}
	{
		/* NOTE: Get the device features. */
		{
			/* NOTE: Device features from Vulkan 1.3 API. */
			m_featuresVK13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
			m_featuresVK13.pNext = nullptr;
			/* NOTE: Device features from Vulkan 1.2 API. */
			m_featuresVK12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
			m_featuresVK12.pNext = &m_featuresVK13;
			/* NOTE: Device features from Vulkan 1.1 API. */
			m_featuresVK11.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
			m_featuresVK11.pNext = &m_featuresVK12;
			/* NOTE: Device features from Vulkan 1.0 API. */
			m_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
			m_features.pNext = &m_featuresVK11;

			vkGetPhysicalDeviceFeatures2(m_physicalDevice, &m_features);
		}

		/* NOTE: Get the device properties. */
		{
			/* NOTE: Device properties from Vulkan 1.3 API. */
			m_propertiesVK13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_PROPERTIES;
			m_propertiesVK13.pNext = nullptr;
			/* NOTE: Device properties from Vulkan 1.2 API. */
			m_propertiesVK12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_PROPERTIES;
			m_propertiesVK12.pNext = &m_propertiesVK13;
			/* NOTE: Device properties from Vulkan 1.1 API. */
			m_propertiesVK11.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_PROPERTIES;
			m_propertiesVK11.pNext = &m_propertiesVK12;
			/* NOTE: Device properties from Vulkan 1.0 API. */
			m_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
			m_properties.pNext = &m_propertiesVK11;

			vkGetPhysicalDeviceProperties2(m_physicalDevice, &m_properties);
		}

		/* NOTE: Get the device memory properties. */
		{
			m_memoryProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2;
			m_memoryProperties.pNext = nullptr;

			vkGetPhysicalDeviceMemoryProperties2(m_physicalDevice, &m_memoryProperties);
		}

		uint32_t count = 0;
		VkResult result{};

		{
			vkGetPhysicalDeviceQueueFamilyProperties2(m_physicalDevice, &count, nullptr);

			if ( count > 0 )
			{
				m_queueFamilyProperties.resize(count, {
					.sType = VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2,
					.pNext = nullptr,
					.queueFamilyProperties{}
				});

				vkGetPhysicalDeviceQueueFamilyProperties2(m_physicalDevice, &count, m_queueFamilyProperties.data());
			}
		}

		/* NOTE: From extension 'VK_EXT_tooling_info'. */
		{
			result = vkGetPhysicalDeviceToolProperties(m_physicalDevice, &count, nullptr);

			if ( result == VK_SUCCESS )
			{
				if ( count > 0 )
				{
					m_toolProperties.resize(count);

					result = vkGetPhysicalDeviceToolProperties(m_physicalDevice, &count, m_toolProperties.data());

					if ( result != VK_SUCCESS )
					{
						TraceError{ClassId} << "Unable to get tool properties : " << vkResultToCString(result) << " !";
					}
				}
			}
			else
			{
				TraceError{ClassId} << "Unable to get tool property count : " << vkResultToCString(result) << " !";
			}
		}

		/* NOTE: From extension 'VK_KHR_display'. */
		/*{
			result = vkGetPhysicalDeviceDisplayPropertiesKHR(m_physicalDevice, &count, nullptr);

			if ( result == VK_SUCCESS )
			{
				if ( count > 0 )
				{
					m_displayProperties.resize(count);

					result = vkGetPhysicalDeviceDisplayPropertiesKHR(m_physicalDevice, &count, m_displayProperties.data());

					if ( result != VK_SUCCESS )
					{
						TraceError{ClassId} << "Unable to get display properties : " << vkResultToCString(result) << " !";
					}
				}
			}
			else
			{
				TraceError{ClassId} << "Unable to get display property count : " << vkResultToCString(result) << " !";
			}
		}*/

		/* NOTE: From extension 'VK_KHR_display'. */
		/*{
			result = vkGetPhysicalDeviceDisplayPlanePropertiesKHR(m_physicalDevice, &count, nullptr);

			if ( result == VK_SUCCESS )
			{
				if ( count > 0 )
				{
					m_displayPlaneProperties.resize(count);

					result = vkGetPhysicalDeviceDisplayPlanePropertiesKHR(m_physicalDevice, &count, m_displayPlaneProperties.data());

					if ( result != VK_SUCCESS )
					{
						TraceError{ClassId} << "Unable to get display plane properties : " << vkResultToCString(result) << " !";
					}
				}
			}
			else
			{
				TraceError{ClassId} << "Unable to get display plane property count : " << vkResultToCString(result) << " !";
			}
		}*/

		/* NOTE: From extension 'VK_KHR_fragment_shading_rate'. */
		/*{
			result = vkGetPhysicalDeviceFragmentShadingRatesKHR(m_physicalDevice, &count, nullptr);

			if ( result == VK_SUCCESS )
			{
				if ( count > 0 )
				{
					m_fragmentShadingRates.resize(count);

					result = vkGetPhysicalDeviceFragmentShadingRatesKHR(m_physicalDevice, &count, m_fragmentShadingRates.data());

					if ( result != VK_SUCCESS )
					{
						TraceError{ClassId} << "Unable to get fragment shading rates : " << vkResultToCString(result) << " !";
					}
				}
			}
			else
			{
				TraceError{ClassId} << "Unable to get fragment shading rate count : " << vkResultToCString(result) << " !";
			}
		}*/

		/* NOTE: From extensions 'VK_EXT_calibrated_timestamps' and 'VK_KHR_calibrated_timestamps'. */
		/*{
			result = vkGetPhysicalDeviceCalibrateableTimeDomainsEXT(m_physicalDevice, &count, nullptr);

			if ( result == VK_SUCCESS )
			{
				if ( count > 0 )
				{
					m_timeDomains.resize(count);

					result = vkGetPhysicalDeviceCalibrateableTimeDomainsEXT(m_physicalDevice, &count, m_timeDomains.data());

					if ( result != VK_SUCCESS )
					{
						TraceError{ClassId} << "Unable to get time domains : " << vkResultToCString(result) << " !";
					}
				}
			}
			else
			{
				TraceError{ClassId} << "Unable to get time domain count : " << vkResultToCString(result) << " !";
			}
		}*/

		/* NOTE: From extension 'VK_NV_coverage_reduction_mode'. */
		/*{
			result = vkGetPhysicalDeviceSupportedFramebufferMixedSamplesCombinationsNV(m_physicalDevice, &count, nullptr);

			if ( result == VK_SUCCESS )
			{
				if ( count > 0 )
				{
					m_framebufferMixedSamplesCombinations.resize(count);

					result = vkGetPhysicalDeviceSupportedFramebufferMixedSamplesCombinationsNV(m_physicalDevice, &count, m_framebufferMixedSamplesCombinations.data());

					if ( result != VK_SUCCESS )
					{
						TraceError{ClassId} << "Unable to get framebuffer mixed samples combinations : " << vkResultToCString(result) << " !";
					}
				}
			}
			else
			{
				TraceError{ClassId} << "Unable to get framebuffer mixed samples combination count : " << vkResultToCString(result) << " !";
			}
		}*/
	}

	std::optional< uint32_t >
	PhysicalDevice::getFamilyQueueIndex (VkQueueFlagBits type) const noexcept
	{
		if ( m_queueFamilyProperties.empty() )
		{
			TraceError{ClassId} << "The physical device " << m_physicalDevice << " has no family queue !";

			return {};
		}

		for ( auto it = m_queueFamilyProperties.cbegin(); it != m_queueFamilyProperties.cend(); ++it )
		{
			const auto index = static_cast< uint32_t >(std::distance(m_queueFamilyProperties.cbegin(), it));

			if ( (m_queueFamilyProperties[index].queueFamilyProperties.queueFlags & type) != 0 )
			{
				return index;
			}
		}

		TraceError{ClassId} << "The physical device " << m_physicalDevice << " don't have this family queue type (" << type << ") !";

		return {};
	}

	VkFormatProperties
	PhysicalDevice::getFormatProperties (VkFormat format) const noexcept
	{
		VkFormatProperties formatProperties{};

		vkGetPhysicalDeviceFormatProperties(m_physicalDevice, format, &formatProperties);

		return formatProperties;
	}

	VkImageFormatProperties
	PhysicalDevice::getImageFormatProperties (VkFormat format, VkImageType type, VkImageTiling tiling, VkImageUsageFlags usage, VkImageCreateFlags flags) const noexcept
	{
		VkImageFormatProperties imageFormatProperties{};

		if ( const auto result = vkGetPhysicalDeviceImageFormatProperties(m_physicalDevice, format, type, tiling, usage, flags, &imageFormatProperties); result != VK_SUCCESS )
		{
			TraceError{ClassId} << "Unable to get image format properties : " << vkResultToCString(result) << " !";
		}

		return imageFormatProperties;
	}

	std::vector< VkSparseImageFormatProperties >
	PhysicalDevice::getSparseImageFormatProperties (VkFormat format, VkImageType type, VkImageTiling tiling, VkImageUsageFlags usage, VkSampleCountFlagBits samples) const noexcept
	{
		uint32_t count = 0;

		vkGetPhysicalDeviceSparseImageFormatProperties(m_physicalDevice, format, type, samples, usage, tiling, &count, nullptr);

		std::vector< VkSparseImageFormatProperties > sparseImageFormatProperties{};

		if ( count > 0 )
		{
			sparseImageFormatProperties.resize(count);

			vkGetPhysicalDeviceSparseImageFormatProperties(m_physicalDevice, format, type, samples, usage, tiling, &count, sparseImageFormatProperties.data());
		}

		return sparseImageFormatProperties;
	}

	VkExternalImageFormatPropertiesNV
	PhysicalDevice::getExternalFormatProperties (VkFormat /*format*/, VkImageType /*type*/, VkImageTiling /*tiling*/, VkImageUsageFlags /*usage*/, VkImageCreateFlags /*flags*/, VkExternalMemoryHandleTypeFlagsNV /*externalHandleType*/) const noexcept
	{
		VkExternalImageFormatPropertiesNV externalImageFormatProperties{};

		//auto result = vkGetPhysicalDeviceExternalImageFormatPropertiesNV(m_physicalDevice, format, type, tiling, usage, flags, externalHandleType, &externalImageFormatProperties);

		//if ( result != VK_SUCCESS )
		//{
		//	TraceError{ClassId} << "Unable to get external image format properties : " << vkResultToCString(result) << " !");
		//}

		return externalImageFormatProperties;
	}

	VkExternalBufferProperties
	PhysicalDevice::getExternalBufferProperties (const VkPhysicalDeviceExternalBufferInfo * pExternalBufferInfo) const noexcept
	{
		VkExternalBufferProperties externalBufferProperties{};

		vkGetPhysicalDeviceExternalBufferProperties(m_physicalDevice, pExternalBufferInfo, &externalBufferProperties);

		return externalBufferProperties;
	}

	VkExternalFenceProperties
	PhysicalDevice::getExternalFenceProperties (const VkPhysicalDeviceExternalFenceInfo * pExternalFenceInfo) const noexcept
	{
		VkExternalFenceProperties externalFenceProperties{};

		vkGetPhysicalDeviceExternalFenceProperties(m_physicalDevice, pExternalFenceInfo, &externalFenceProperties);

		return externalFenceProperties;
	}

	VkExternalSemaphoreProperties
	PhysicalDevice::getExternalSemaphoreProperties (const VkPhysicalDeviceExternalSemaphoreInfo * pExternalSemaphoreInfo) const noexcept
	{
		VkExternalSemaphoreProperties externalSemaphoreProperties{};

		vkGetPhysicalDeviceExternalSemaphoreProperties(m_physicalDevice, pExternalSemaphoreInfo, &externalSemaphoreProperties);

		return externalSemaphoreProperties;
	}

	bool
	PhysicalDevice::getSurfaceSupport (VkSurfaceKHR surface, uint32_t queueFamilyIndex) const noexcept
	{
		VkBool32 supported = VK_FALSE;

		if ( const auto result = vkGetPhysicalDeviceSurfaceSupportKHR(m_physicalDevice, queueFamilyIndex, surface, &supported); result != VK_SUCCESS )
		{
			TraceError{ClassId} << "Unable to get surface support : " << vkResultToCString(result) << " !";
		}

		return supported != 0;
	}

	VkSurfaceCapabilitiesKHR
	PhysicalDevice::getSurfaceCapabilities (VkSurfaceKHR surface) const noexcept
	{
		VkSurfaceCapabilitiesKHR surfaceCapabilities{};

		if ( const auto result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physicalDevice, surface, &surfaceCapabilities); result != VK_SUCCESS )
		{
			TraceError{ClassId} << "Unable to get surface capabilities : " << vkResultToCString(result) << " !";
		}

		return surfaceCapabilities;
	}

	std::vector< VkSurfaceFormatKHR >
	PhysicalDevice::getSurfaceFormats (VkSurfaceKHR surface) const noexcept
	{
		std::vector< VkSurfaceFormatKHR > surfaceFormats{};

		uint32_t count = 0;

		if ( auto result = vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, surface, &count, nullptr); result == VK_SUCCESS )
		{
			if ( count > 0 )
			{
				surfaceFormats.resize(count);

				result = vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, surface, &count, surfaceFormats.data());

				if ( result != VK_SUCCESS )
				{
					TraceError{ClassId} << "Unable to get surface formats : " << vkResultToCString(result) << " !";
				}
			}
		}
		else
		{
			TraceError{ClassId} << "Unable to get surface format count : " << vkResultToCString(result) << " !";
		}

		return surfaceFormats;
	}

	std::vector< VkPresentModeKHR >
	PhysicalDevice::getSurfacePresentModes (VkSurfaceKHR surface) const noexcept
	{
		std::vector< VkPresentModeKHR > presentModes{};

		uint32_t count = 0;

		if ( auto result = vkGetPhysicalDeviceSurfacePresentModesKHR(m_physicalDevice, surface, &count, nullptr); result == VK_SUCCESS )
		{
			if ( count > 0 )
			{
				presentModes.resize(count);

				result = vkGetPhysicalDeviceSurfacePresentModesKHR(m_physicalDevice, surface, &count, presentModes.data());

				if ( result != VK_SUCCESS )
				{
					TraceError{ClassId} << "Unable to get present modes : " << vkResultToCString(result) << " !";
				}
			}
		}
		else
		{
			TraceError{ClassId} << "Unable to get present mode count : " << vkResultToCString(result) << " !";
		}

		return presentModes;
	}

	std::vector< VkRect2D >
	PhysicalDevice::getPresentRectangles (VkSurfaceKHR surface) const noexcept
	{
		std::vector< VkRect2D > presentRectangles{};

		uint32_t count = 0;

		if ( auto result = vkGetPhysicalDevicePresentRectanglesKHR(m_physicalDevice, surface, &count, nullptr); result == VK_SUCCESS )
		{
			if ( count > 0 )
			{
				presentRectangles.resize(count);

				result = vkGetPhysicalDevicePresentRectanglesKHR(m_physicalDevice, surface, &count, presentRectangles.data());

				if ( result != VK_SUCCESS )
				{
					TraceError{ClassId} << "Unable to get present rectangles : " << vkResultToCString(result) << " !";
				}
			}
		}
		else
		{
			TraceError{ClassId} << "Unable to get present rectangle count : " << vkResultToCString(result) << " !";
		}

		return presentRectangles;
	}

	uint32_t
	PhysicalDevice::getQueueFamilyPerformanceQueryPasses (const VkQueryPoolPerformanceCreateInfoKHR * /*pPerformanceQueryCreateInfos*/) const noexcept
	{
		uint32_t const numPasses = 0;

		//vkGetPhysicalDeviceQueueFamilyPerformanceQueryPassesKHR(m_physicalDevice, pPerformanceQueryCreateInfos, &numPasses);

		return numPasses;
	}

	VkMultisamplePropertiesEXT
	PhysicalDevice::getMultisampleProperties (VkSampleCountFlagBits /*samples*/) const noexcept
	{
		VkMultisamplePropertiesEXT multisampleProperties{};

		//vkGetPhysicalDeviceMultisamplePropertiesEXT(m_physicalDevice, samples, &multisampleProperties);

		return multisampleProperties;
	}

	std::vector< VkLayerProperties >
	PhysicalDevice::getValidationLayers () const noexcept
	{
		std::vector< VkLayerProperties > validationLayers{};

		uint32_t count = 0;

		if ( auto result = vkEnumerateDeviceLayerProperties(m_physicalDevice, &count, nullptr); result == VK_SUCCESS )
		{
			if ( count > 0 )
			{
				validationLayers.resize(count);

				result = vkEnumerateDeviceLayerProperties(m_physicalDevice, &count, validationLayers.data());

				if ( result != VK_SUCCESS )
				{
					TraceError{ClassId} << "Unable to get device validation layers : " << vkResultToCString(result) << " !";
				}
			}
		}
		else
		{
			TraceError{ClassId} << "Unable to get device validation layer count : " << vkResultToCString(result) << " !";
		}

		return validationLayers;
	}

	std::vector< VkExtensionProperties >
	PhysicalDevice::getExtensions (const char * pLayerName) const noexcept
	{
		std::vector< VkExtensionProperties > extensions{};

		uint32_t count = 0;

		auto result = vkEnumerateDeviceExtensionProperties(m_physicalDevice, pLayerName, &count, nullptr);

		if ( result == VK_SUCCESS )
		{
			if ( count > 0 )
			{
				extensions.resize(count);

				result = vkEnumerateDeviceExtensionProperties(m_physicalDevice, pLayerName, &count, extensions.data());

				if ( result != VK_SUCCESS )
				{
					TraceError{ClassId} << "Unable to get device extensions : " << vkResultToCString(result) << " !";
				}
			}
		}
		else
		{
			TraceError{ClassId} << "Unable to get device extension count : " << vkResultToCString(result) << " !";
		}

		return extensions;
	}

	std::string
	PhysicalDevice::getPhysicalDeviceInformation () const noexcept
	{
		std::stringstream output;

		output <<
			this->deviceType() << " '" << this->deviceName() << "' : \n"
			" - API driver : " << this->APIDriverString() << "\n"
			" - Driver version : " << this->DriverVersionString() << "\n"
			" - Vendor ID : " << this->vendorString() << "\n"
			" - Device ID : " << this->deviceId() << "\n"
			" - Pipeline cache UUID : " << this->pipelineCacheUUID() << "\n";

		/* Checks queue family properties. */
		if ( !m_queueFamilyProperties.empty() )
		{
			output << " - Queue family count : " << m_queueFamilyProperties.size() << "\n";

			for ( size_t index = 0; index < m_queueFamilyProperties.size(); index++ )
			{
				const auto & familyQueue = m_queueFamilyProperties[index];

				std::vector< std::string > flags{};

				if ( (familyQueue.queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0 )
				{
					flags.emplace_back("Graphics");
				}

				if ( (familyQueue.queueFamilyProperties.queueFlags & VK_QUEUE_COMPUTE_BIT) != 0 )
				{
					flags.emplace_back("Compute");
				}

				if ( (familyQueue.queueFamilyProperties.queueFlags & VK_QUEUE_TRANSFER_BIT) != 0 )
				{
					flags.emplace_back("Transfer");
				}

				if ( (familyQueue.queueFamilyProperties.queueFlags & VK_QUEUE_SPARSE_BINDING_BIT) != 0 )
				{
					flags.emplace_back("SparseBinding");
				}

				if ( (familyQueue.queueFamilyProperties.queueFlags & VK_QUEUE_PROTECTED_BIT) != 0 )
				{
					flags.emplace_back("Protected");
				}

				if ( (familyQueue.queueFamilyProperties.queueFlags & VK_QUEUE_VIDEO_DECODE_BIT_KHR) != 0 )
				{
					flags.emplace_back("VideoDecode");
				}

				if ( (familyQueue.queueFamilyProperties.queueFlags & VK_QUEUE_VIDEO_ENCODE_BIT_KHR) != 0 )
				{
					flags.emplace_back("VideoEncode");
				}

				/* Optical flow is fundamental algorithms in computer vision (CV) area.
				 * This extension allows applications to estimate 2D displacement of pixels between two frames.
				 * For use with: NVIDIA Optical Flow SDK Version 5 */
				if ( (familyQueue.queueFamilyProperties.queueFlags & VK_QUEUE_OPTICAL_FLOW_BIT_NV) != 0 )
				{
					flags.emplace_back("OpticalFlow");
				}

				output << "\t#" << index << " supports " << familyQueue.queueFamilyProperties.queueCount << "x {" << String::implode(flags, " | ") << "}";

				/* Additional info */
				output << " timestampValidBits: " << familyQueue.queueFamilyProperties.timestampValidBits << ", minImageTransferGranularity(" << familyQueue.queueFamilyProperties.minImageTransferGranularity.width << ", " << familyQueue.queueFamilyProperties.minImageTransferGranularity.height << ", " << familyQueue.queueFamilyProperties.minImageTransferGranularity.depth << ")" "\n";
			}
		}
		else
		{
			output << " - Queue family count : None !" "\n";
		}

		return output.str();
	}

	size_t
	PhysicalDevice::getTotalQueueCount () const noexcept
	{
		if ( m_queueFamilyProperties.empty() )
		{
			return 0;
		}

		size_t queueCount = 0;

		for ( const auto & queueFamilyProperties : m_queueFamilyProperties )
		{
			queueCount += queueFamilyProperties.queueFamilyProperties.queueCount;
		}

		return queueCount;
	}

	VkSampleCountFlagBits
	PhysicalDevice::getMaxAvailableSampleCount () const noexcept
	{
		/* NOTE: We are using the smallest support between a color buffer and the depth buffer. */
		const VkSampleCountFlags supportedSampleCount = std::min(m_properties.properties.limits.framebufferColorSampleCounts, m_properties.properties.limits.framebufferDepthSampleCounts);

		constexpr std::array< VkSampleCountFlagBits, 6 > samples {
			VK_SAMPLE_COUNT_64_BIT,
			VK_SAMPLE_COUNT_32_BIT,
			VK_SAMPLE_COUNT_16_BIT,
			VK_SAMPLE_COUNT_8_BIT,
			VK_SAMPLE_COUNT_4_BIT,
			VK_SAMPLE_COUNT_2_BIT
		};

		for ( const auto sample : samples )
		{
			if ( supportedSampleCount & sample )
			{
				return sample;
			}
		}

		return VK_SAMPLE_COUNT_1_BIT;
	}

	std::string
	PhysicalDevice::UUIDToString (const uint8_t uuid[]) noexcept
	{
		std::stringstream output;

		for ( size_t index = 0; index < VK_UUID_SIZE; ++index )
		{
			output << std::setfill('0') << std::setw(2) << std::hex << static_cast< uint32_t >(uuid[index]);

			switch ( index )
			{
				case 3 :
				case 7 :
				case 11 :
					output << '-';
					break;

				default:
					break;
			}
		}

		return output.str();
	}
}
