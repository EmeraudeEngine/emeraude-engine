/*
 * src/Vulkan/PhysicalDevice.hpp
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

/* STL inclusions. */
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

/* Third-party inclusions. */
#include <vulkan/vulkan.h>

/* Local inclusions for usages. */
#include "Libs/Version.hpp"

namespace EmEn::Vulkan
{
	/**
	 * @brief The physical device class to build a logical vulkan device.
	 */
	class PhysicalDevice final
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"VulkanPhysicalDevice"};

			/**
			 * @brief Constructs a physical device.
			 * @param physicalDevice A reference to the physical device handle.
			 */
			explicit PhysicalDevice (const VkPhysicalDevice & physicalDevice) noexcept;

			/**
			 * @brief Returns the vulkan handle.
			 * @return VkPhysicalDevice
			 */
			[[nodiscard]]
			VkPhysicalDevice handle () const noexcept
			{
				return m_physicalDevice;
			}

			/**
			 * @brief features
			 * @return const VkPhysicalDeviceFeatures &
			 */
			[[nodiscard]]
			const VkPhysicalDeviceFeatures &
			features () const noexcept
			{
				return m_features;
			}

			/**
			 * @brief Returns prefetched physical device properties.
			 * @return const VkPhysicalDeviceProperties &
			 */
			[[nodiscard]]
			const VkPhysicalDeviceProperties &
			properties () const noexcept
			{
				return m_properties;
			}

			/**
			 * @brief Returns the device type as a string.
			 * @note Shortcut to PhysicalDevice::properties().
			 * @return std::string
			 */
			[[nodiscard]]
			std::string
			deviceType () const noexcept
			{
				switch ( m_properties.deviceType )
				{
					case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU :
						return "Integrated GPU device";

					case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU :
						return "Discrete GPU device";

					case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU :
						return "Virtual GPU device";

					case VK_PHYSICAL_DEVICE_TYPE_CPU :
						return "CPU device";

					case VK_PHYSICAL_DEVICE_TYPE_OTHER :
					default :
						return "Other device";
				}
			}

			/**
			 * @brief Returns the device name as a string.
			 * @note Shortcut to PhysicalDevice::properties().
			 * @return std::string
			 */
			[[nodiscard]]
			std::string
			deviceName () const noexcept
			{
				return m_properties.deviceName;
			}

			/**
			 * @brief Returns the API driver version.
			 * @note Shortcut to PhysicalDevice::properties().
			 * @return Libraries::Version
			 */
			[[nodiscard]]
			Libs::Version
			APIDriver () const noexcept
			{
				return Libs::Version{m_properties.apiVersion};
			}

			/**
			 * @brief Returns the API driver as a string.
			 * @note Shortcut to PhysicalDevice::properties().
			 * @return std::string
			 */
			[[nodiscard]]
			std::string
			APIDriverString () const noexcept
			{
				return to_string(this->APIDriver());
			}

			/**
			 * @brief Returns the driver version.
			 * @note Shortcut to PhysicalDevice::properties().
			 * @return Libraries::Version
			 */
			[[nodiscard]]
			Libs::Version
			DriverVersion () const noexcept
			{
				return Libs::Version{m_properties.driverVersion};
			}

			/**
			 * @brief Returns the driver version as a string.
			 * @note Shortcut to PhysicalDevice::properties().
			 * @return std::string
			 */
			[[nodiscard]]
			std::string
			DriverVersionString () const noexcept
			{
				return to_string(this->DriverVersion());
			}

			/**
			 * @brief Returns the vendor ID as a string.
			 * @note Shortcut to PhysicalDevice::properties().
			 * @return std::string
			 */
			[[nodiscard]]
			std::string
			vendorId () const noexcept
			{
				switch ( m_properties.vendorID )
				{
					case 0x1002 :
						return "AMD (" + std::to_string(m_properties.vendorID) + ")";

					case 0x1010 :
						return "ImgTec (" + std::to_string(m_properties.vendorID) + ")";

					case 0x10DE :
						return "Nvidia (" + std::to_string(m_properties.vendorID) + ")";

					case 0x13B5 :
						return "ARM (" + std::to_string(m_properties.vendorID) + ")";

					case 0x5143 :
						return "Qualcomm (" + std::to_string(m_properties.vendorID) + ")";

					case 0x8086 :
						return "Intel (" + std::to_string(m_properties.vendorID) + ")";

					default :
						return std::to_string(m_properties.vendorID);
				}
			}

			/**
			 * @brief Returns the device ID as a string.
			 * @note Shortcut to PhysicalDevice::properties().
			 * @return std::string
			 */
			[[nodiscard]]
			std::string
			deviceId () const noexcept
			{
				return std::to_string(m_properties.deviceID);
			}

			/**
			 * @brief Returns the pipeline cache UUID as a string.
			 * @note Shortcut to PhysicalDevice::properties().
			 * @return std::string
			 */
			[[nodiscard]]
			std::string
			pipelineCacheUUID () const noexcept
			{
				return PhysicalDevice::UUIDToString(m_properties.pipelineCacheUUID);
			}

			/**
			 * @brief Returns prefetched physical device memory properties.
			 * @return const VkPhysicalDeviceMemoryProperties &
			 */
			[[nodiscard]]
			const VkPhysicalDeviceMemoryProperties &
			memoryProperties () const noexcept
			{
				return m_memoryProperties;
			}

			/**
			 * @brief Returns prefetched physical device queue family properties.
			 * @return const std::vector< VkQueueFamilyProperties > &
			 */
			[[nodiscard]]
			const std::vector< VkQueueFamilyProperties > &
			queueFamilyProperties () const noexcept
			{
				return m_queueFamilyProperties;
			}

			/**
			 * @brief Returns the index of the family queue type from the physical device.
			 * @param type The type of the family queue.
			 * @return uint32_t
			 */
			[[nodiscard]]
			std::optional< uint32_t > getFamilyQueueIndex (VkQueueFlagBits type) const noexcept;

			/**
			 * @brief Returns prefetched physical device tool properties.
			 * @return const std::vector< VkPhysicalDeviceToolProperties > &
			 */
			[[nodiscard]]
			const std::vector< VkPhysicalDeviceToolProperties > &
			toolProperties () const noexcept
			{
				return m_toolProperties;
			}

			/**
			 * @brief Returns prefetched physical device display properties.
			 * @return const std::vector< VkDisplayPropertiesKHR > &
			 */
			[[nodiscard]]
			const std::vector< VkDisplayPropertiesKHR > &
			displayProperties () const noexcept
			{
				return m_displayProperties;
			}

			/**
			 * @brief Returns prefetched physical device display plane properties.
			 * @return const std::vector< VkDisplayPlanePropertiesKHR > &
			 */
			[[nodiscard]]
			const std::vector< VkDisplayPlanePropertiesKHR > &
			displayPlaneProperties () const noexcept
			{
				return m_displayPlaneProperties;
			}

			/**
			 * @brief Returns prefetched physical device fragment shading rates.
			 * @return const std::vector< VkDisplayPlanePropertiesKHR > &
			 */
			[[nodiscard]]
			const std::vector< VkPhysicalDeviceFragmentShadingRateKHR > &
			fragmentShadingRates () const noexcept
			{
				return m_fragmentShadingRates;
			}

			/**
			 * @brief Returns prefetched physical device time domains.
			 * @return const std::vector< VkTimeDomainEXT > &
			 */
			[[nodiscard]]
			const std::vector< VkTimeDomainEXT > &
			timeDomains () const noexcept
			{
				return m_timeDomains;
			}

			/**
			 * @brief Returns prefetched physical device framebuffer mixed samples combinations.
			 * @return const std::vector< VkTimeDomainEXT > &
			 */
			[[nodiscard]]
			const std::vector< VkFramebufferMixedSamplesCombinationNV > &
			framebufferMixedSamplesCombinations () const noexcept
			{
				return m_framebufferMixedSamplesCombinations;
			}

			/**
			 * @brief Returns the physical device format properties.
			 * @param format
			 * @return VkFormatProperties
			 */
			[[nodiscard]]
			VkFormatProperties getFormatProperties (VkFormat format) const noexcept;

			/**
			 * @brief Returns the physical device image format properties.
			 * @param format
			 * @param type
			 * @param tiling
			 * @param usage
			 * @param flags
			 * @return VkImageFormatProperties
			 */
			[[nodiscard]]
			VkImageFormatProperties getImageFormatProperties (VkFormat format, VkImageType type, VkImageTiling tiling, VkImageUsageFlags usage, VkImageCreateFlags flags) const noexcept;

			/**
			 * @brief Returns the physical device sparse image format properties.
			 * @param format
			 * @param type
			 * @param tiling
			 * @param usage
			 * @param samples
			 * @return std::vector< VkSparseImageFormatProperties >
			 */
			[[nodiscard]]
			std::vector< VkSparseImageFormatProperties > getSparseImageFormatProperties (VkFormat format, VkImageType type, VkImageTiling tiling, VkImageUsageFlags usage, VkSampleCountFlagBits samples) const noexcept;

			/**
			 * @brief Returns the physical device external image format properties.
			 * @param format
			 * @param type
			 * @param tiling
			 * @param usage
			 * @param flags
			 * @param externalHandleType
			 * @return VkExternalImageFormatPropertiesNV
			 */
			[[nodiscard]]
			VkExternalImageFormatPropertiesNV getExternalFormatProperties (VkFormat format, VkImageType type, VkImageTiling tiling, VkImageUsageFlags usage, VkImageCreateFlags flags, VkExternalMemoryHandleTypeFlagsNV externalHandleType) const noexcept;

			/**
			 * @brief Returns the physical device external buffer properties.
			 * @param pExternalBufferInfo
			 * @return VkExternalBufferProperties
			 */
			[[nodiscard]]
			VkExternalBufferProperties getExternalBufferProperties (const VkPhysicalDeviceExternalBufferInfo * pExternalBufferInfo) const noexcept;

			/**
			 * @brief Returns the physical device external fence properties.
			 * @param pExternalFenceInfo
			 * @return VkExternalFenceProperties
			 */
			[[nodiscard]]
			VkExternalFenceProperties getExternalFenceProperties (const VkPhysicalDeviceExternalFenceInfo * pExternalFenceInfo) const noexcept;

			/**
			 * @brief Returns the physical device external semaphore properties.
			 * @param pExternalSemaphoreInfo
			 * @return VkExternalSemaphoreProperties
			 */
			[[nodiscard]]
			VkExternalSemaphoreProperties getExternalSemaphoreProperties (const VkPhysicalDeviceExternalSemaphoreInfo * pExternalSemaphoreInfo) const noexcept;

			/**
			 * @brief Returns the physical device surface support.
			 * @param surface
			 * @param queueFamilyIndex
			 * @return bool
			 */
			[[nodiscard]]
			bool getSurfaceSupport (VkSurfaceKHR surface, uint32_t queueFamilyIndex) const noexcept;

			/**
			 * @brief Returns the physical device surface capabilities.
			 * @param surface
			 * @return VkSurfaceCapabilitiesKHR
			 */
			[[nodiscard]]
			VkSurfaceCapabilitiesKHR getSurfaceCapabilities (VkSurfaceKHR surface) const noexcept;

			/**
			 * @brief Returns the physical device surface formats.
			 * @param surface
			 * @return std::vector< VkSurfaceFormatKHR >
			 */
			[[nodiscard]]
			std::vector< VkSurfaceFormatKHR > getSurfaceFormats (VkSurfaceKHR surface) const noexcept;

			/**
			 * @brief Returns the physical device surface present modes.
			 * @param surface
			 * @return std::vector< VkPresentModeKHR >
			 */
			[[nodiscard]]
			std::vector< VkPresentModeKHR > getSurfacePresentModes (VkSurfaceKHR surface) const noexcept;

			/**
			 * @brief Returns the physical device present rectangles.
			 * @param surface
			 * @return std::vector< VkRect2D >
			 */
			[[nodiscard]]
			std::vector< VkRect2D > getPresentRectangles (VkSurfaceKHR surface) const noexcept;

			/**
			 * @brief Returns the physical device queue family performance query passes.
			 * @param pPerformanceQueryCreateInfos
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t getQueueFamilyPerformanceQueryPasses (const VkQueryPoolPerformanceCreateInfoKHR * pPerformanceQueryCreateInfos) const noexcept;

			/**
			 * @brief Returns the physical device multisample properties.
			 * @param samples
			 * @return VkMultisamplePropertiesEXT
			 */
			[[nodiscard]]
			VkMultisamplePropertiesEXT getMultisampleProperties (VkSampleCountFlagBits samples) const noexcept;

			/**
			 * @brief Returns a list of validation layers available from physical device.
			 * @return std::vector< VkLayerProperties >
			 */
			[[deprecated("Vulkan has deprecated the device layer for instance layer only.")]]
			[[nodiscard]]
			std::vector< VkLayerProperties > getValidationLayers () const noexcept;

			/**
			 * @brief Returns a list of extensions available from physical device.
			 * @param pLayerName Default nullptr.
			 * @return std::vector< VkExtensionProperties >
			 */
			[[nodiscard]]
			std::vector< VkExtensionProperties > getExtensions (const char * pLayerName = nullptr) const noexcept;

			/**
			 * @brief Returns printable information about the physical device.
			 * @return std::string
			 */
			[[nodiscard]]
			std::string getPhysicalDeviceInformation () const noexcept;

			/**
			 * @brief Returns the total queue the device can handle.
			 * @note This is mainly to have a raw score from the device for selection.
			 * @return size_t
			 */
			[[nodiscard]]
			size_t getTotalQueueCount () const noexcept;

			/**
			 * @brief Gets the maximum sample the GPU can handle for multisample rendering.
			 * @note This method check the smallest support between the color buffer and the depth buffer.
			 * @return VkSampleCountFlagBits
			 */
			[[nodiscard]]
			VkSampleCountFlagBits getMaxAvailableSampleCount () const noexcept;

			/**
			 * @brief Converts UUID from vulkan to a printable string.
			 * @param uuid A point to uuid structure.
			 * @return std::string
			 */
			[[nodiscard]]
			static std::string UUIDToString (const uint8_t uuid[]) noexcept;

		private:

			VkPhysicalDevice m_physicalDevice;
			VkPhysicalDeviceFeatures m_features{};
			VkPhysicalDeviceProperties m_properties{};
			VkPhysicalDeviceMemoryProperties m_memoryProperties{};
			std::vector< VkQueueFamilyProperties > m_queueFamilyProperties;
			std::vector< VkPhysicalDeviceToolProperties > m_toolProperties;
			std::vector< VkDisplayPropertiesKHR > m_displayProperties;
			std::vector< VkDisplayPlanePropertiesKHR > m_displayPlaneProperties;
			std::vector< VkPhysicalDeviceFragmentShadingRateKHR > m_fragmentShadingRates;
			std::vector< VkTimeDomainEXT > m_timeDomains;
			std::vector< VkFramebufferMixedSamplesCombinationNV > m_framebufferMixedSamplesCombinations;
	};
}
