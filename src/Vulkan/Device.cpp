/*
 * src/Vulkan/Device.cpp
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
 * https://github.com/londnoir/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

/* Project configuration. */
#include "emeraude_config.hpp"

#if IS_WINDOWS
	#define VK_USE_PLATFORM_WIN32_KHR
#endif
#define VMA_IMPLEMENTATION

/* NOTE: Inverted includes order is required here! */
#include "Device.hpp"

/* Third-party inclusions. */
#include "magic_enum/magic_enum.hpp"

/* Local inclusions. */
#include "Utility.hpp"
#include "Instance.hpp"
#include "DeviceRequirements.hpp"
#include "Tracer.hpp"

namespace EmEn::Vulkan
{
	using namespace Libs;
	using namespace Graphics;

	Queue *
	DeviceQueueConfiguration::queue (QueuePriority priority) const noexcept
	{
		std::array< uint8_t, 3 > searchOrder{};

		switch ( priority )
		{
			/* NOTE: High -> Medium -> Low */
			case QueuePriority::High:
				searchOrder = {0, 1, 2};
				break;

				/* NOTE: Medium -> High -> Low */
			case QueuePriority::Medium:
				searchOrder = {1, 0, 2};
				break;

				/* NOTE: Low -> Medium -> High */
			case QueuePriority::Low:
			default:
				searchOrder = {2, 1, 0};
				break;
		}

		for ( const uint8_t priorityIndex : searchOrder )
		{
			if ( auto & [nextQueueIndex, queueList] = m_queueByPriorities[priorityIndex]; !queueList.empty() )
			{
				const uint32_t index = nextQueueIndex.fetch_add(1) % queueList.size();

				return queueList[index];
			}
		}

		return nullptr;
	}

	bool
	Device::installQueues (const std::map< uint32_t, StaticVector< float, 16 > > & queuePriorityValues, DeviceQueueConfiguration & configuration) noexcept
	{
		const auto queueFamilyIndex = configuration.queueFamilyIndex();

		const auto valuesIt = queuePriorityValues.find(queueFamilyIndex);

		if ( valuesIt == queuePriorityValues.end() )
		{
			return false;
		}

		const auto queueCount = valuesIt->second.size();

		for ( uint32_t queueIndex = 0; queueIndex < queueCount; queueIndex++ )
		{
			VkQueue queueHandle;

			vkGetDeviceQueue(m_deviceHandle, queueFamilyIndex, queueIndex, &queueHandle);

			if ( queueHandle == VK_NULL_HANDLE )
			{
				TraceError{ClassId} << "Unable to retrieve the queue #" << queueIndex << " (family #" << queueFamilyIndex << ") from the device  !";

				return false;
			}

			const auto & queue = m_queues.emplace_back(std::make_unique< Queue >(this->shared_from_this(), queueHandle, queueFamilyIndex));
			queue->setIdentifier(ClassId, (std::stringstream{} << queueFamilyIndex << '.' << queueIndex).str(), "Queue");

			configuration.registerQueue(queue.get(), QueuePriority::High);
		}

		return true;
	}

	[[nodiscard]]
	bool
	Device::createMemoryAllocator () noexcept
	{
		VmaAllocatorCreateInfo allocatorCreateInfo{};
		allocatorCreateInfo.flags = VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT;
		allocatorCreateInfo.physicalDevice = m_physicalDevice->handle();
		allocatorCreateInfo.device = m_deviceHandle;
		allocatorCreateInfo.preferredLargeHeapBlockSize = 0; /* Default: 256Kb */
		allocatorCreateInfo.pAllocationCallbacks = VK_NULL_HANDLE;
		allocatorCreateInfo.pDeviceMemoryCallbacks = VK_NULL_HANDLE;
		allocatorCreateInfo.pHeapSizeLimit = VK_NULL_HANDLE;
		allocatorCreateInfo.pVulkanFunctions = VK_NULL_HANDLE;
		allocatorCreateInfo.instance = m_instance.handle();
		allocatorCreateInfo.vulkanApiVersion = m_instance.info().pApplicationInfo->apiVersion;
		allocatorCreateInfo.pTypeExternalMemoryHandleTypes = VK_NULL_HANDLE;

		if ( const auto result = vmaCreateAllocator(&allocatorCreateInfo, &m_memoryAllocatorHandle); result != VK_SUCCESS )
		{
			TraceError{ClassId} << "Unable to create a memory allocator : " << vkResultToCString(result) << " !";

			return false;
		}

		m_useMemoryAllocator = true;

		return true;
	}

	void
	Device::destroyMemoryAllocator () noexcept
	{
		if ( m_memoryAllocatorHandle != VK_NULL_HANDLE )
		{
			vmaDestroyAllocator(m_memoryAllocatorHandle);

			m_memoryAllocatorHandle = VK_NULL_HANDLE;
		}
	}

	bool
	Device::create (const DeviceRequirements & requirements, const std::vector< const char * > & extensions, bool useVMA) noexcept
	{
		if ( m_showInformation )
		{
			TraceInfo{ClassId} <<
				"Creation of the logical device from the physical device '" << m_physicalDevice->deviceName() << "':" "\n" <<
				getItemListAsString("Device", m_physicalDevice->getExtensions(nullptr)) <<
				"The requirements for creation:" "\n" << requirements;
		}

		const auto & queueFamilyProperties = m_physicalDevice->queueFamilyPropertiesVK11();

		if ( queueFamilyProperties.empty() )
		{
			TraceFatal{ClassId} << "The physical device '" << this->name() << "' has no family queue !";

			return false;
		}

		m_basicSupport = queueFamilyProperties.size() <= 1;

		std::map< uint32_t, StaticVector< float, 16 > > queuePriorityValues;
		StaticVector< VkDeviceQueueCreateInfo, 8 > queueCreateInfos;

		/* NOTE: Split the strategy search for queue family. */
		if ( requirements.needsGraphics() && requirements.needsCompute() )
		{
			Tracer::info(ClassId, "Create a device requiring both graphics and compute capabilities !");

			if ( !this->searchGraphicsAndComputeQueueConfiguration(requirements, queueFamilyProperties, queueCreateInfos, queuePriorityValues) )
			{
				Tracer::error(ClassId, "Unable to found a graphics and compute capable configuration for this device!");

				return false;
			}
		}
		else if ( requirements.needsGraphics() )
		{
			Tracer::info(ClassId, "Create a device requiring graphics capabilities !");

			if ( !this->searchGraphicsQueueConfiguration(requirements, queueFamilyProperties, queueCreateInfos, queuePriorityValues) )
			{
				Tracer::error(ClassId, "Unable to found a graphics capable configuration for this device!");

				return false;
			}
		}
		else if ( requirements.needsCompute() )
		{
			Tracer::info(ClassId, "Create a device requiring compute capabilities !");

			if ( !this->searchComputeQueueConfiguration(queueFamilyProperties, queueCreateInfos, queuePriorityValues) )
			{
				Tracer::error(ClassId, "Unable to found a compute capable configuration for this device!");

				return false;
			}
		}
		else
		{
			Tracer::error(ClassId, "No queue requirement for this device!");

			return false;
		}

		const auto transferOnlyQueueFamilyFound = this->searchTransferOnlyQueueConfiguration(queueFamilyProperties, queueCreateInfos, queuePriorityValues);

		/* Logical device creation. */
		if ( !this->createDevice(requirements, queueCreateInfos, extensions) )
		{
			Tracer::error(ClassId, "Logical device creation failed!");

			return false;
		}

		/* Initialize the vulkan memory allocator. */
		if ( useVMA && !this->createMemoryAllocator() )
		{
			Tracer::error(ClassId, "Unable to create the memory allocator!");

			return false;
		}

		/* NOTE: Register the queue to the graphics/transfer configuration. */
		if ( requirements.needsGraphics() && !this->installQueues(queuePriorityValues, m_graphicsQueueConfiguration) )
		{
			return false;
		}

		/* NOTE: Register the queue to the compute/transfer configuration. */
		if ( requirements.needsCompute() && !this->installQueues(queuePriorityValues, m_computeQueueConfiguration) )
		{
			return false;
		}

		if ( !transferOnlyQueueFamilyFound )
		{
			Tracer::info(ClassId, "No queue transfer-only queue available with this device!");
		}
		else if ( !this->installQueues(queuePriorityValues, m_transferQueueConfiguration) )
		{
			return false;
		}

		if ( m_showInformation )
		{
			const std::array< std::pair< const DeviceQueueConfiguration &, std::string >, 3 > purposes{{
				{m_graphicsQueueConfiguration, "Graphics"},
				{m_computeQueueConfiguration, "Compute"},
				{m_transferQueueConfiguration, "Transfer"}
			}};

			TraceInfo info{ClassId};

			if ( m_basicSupport )
			{
				info << "The physical device has basic hardware capabilities." "\n";
			}
			else
			{
				info << "The physical device has advanced hardware capabilities." "\n";
			}

			info << "Logical device queue configuration: " "\n";

			for ( const auto & [configuration, purpose] : purposes )
			{
				if ( configuration.enabled() )
				{
					constexpr std::array< QueuePriority, 3 > priorities{
						QueuePriority::High,
						QueuePriority::Medium,
						QueuePriority::Low
					};

					info << purpose << " enabled with family #" << configuration.queueFamilyIndex() << "." "\n";

					for ( const auto priority : priorities )
					{
						const auto & queues = configuration.queues(priority);

						info << " - " << magic_enum::enum_name(priority) << " priority: " << queues.size() << " queue(s)." "\n";
					}
				}
				else
				{
					info << purpose <<" disabled." "\n";
				}
			}
		}

		this->setCreated();

		return true;
	}

	void
	Device::destroy () noexcept
	{
		/* NOTE: These vectors should only have simple pointers over the queue vector. */
		m_transferQueueConfiguration.clear();
		m_computeQueueConfiguration.clear();
		m_graphicsQueueConfiguration.clear();

		m_queues.clear();

		if ( m_memoryAllocatorHandle != VK_NULL_HANDLE )
		{
			vmaDestroyAllocator(m_memoryAllocatorHandle);

			m_memoryAllocatorHandle = VK_NULL_HANDLE;
		}

		if ( m_deviceHandle != VK_NULL_HANDLE )
		{
			/* [VULKAN-CPU-SYNC] vkDestroyDevice() through waidIdle() */
			this->waitIdle("Destroying the logical device !");

			vkDestroyDevice(m_deviceHandle, nullptr);

			m_deviceHandle = VK_NULL_HANDLE;
		}

		m_physicalDevice.reset();

		this->setDestroyed();
	}

	uint32_t
	Device::addQueueFamilyToCreateInfo (uint32_t queueFamilyIndex, const StaticVector< VkQueueFamilyProperties2, 8 > & queueFamilyProperties, StaticVector< VkDeviceQueueCreateInfo, 8 > & queueCreateInfos, std::map< uint32_t, StaticVector< float, 16 > > & queuePriorities) noexcept
	{
		/* NOTE: Avoid adding the same family twice. */
		for ( const auto & createInfo : queueCreateInfos )
		{
			if ( createInfo.queueFamilyIndex == queueFamilyIndex )
			{
				return createInfo.queueCount;
			}
		}

		const uint32_t queueCount = queueFamilyProperties[queueFamilyIndex].queueFamilyProperties.queueCount;

		auto & priorities = queuePriorities[queueFamilyIndex];
		priorities.resize(queueCount, 1.0F); /* NOTE: Default priority. */

		VkDeviceQueueCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		createInfo.pNext = nullptr;
		createInfo.flags = 0; // VK_DEVICE_QUEUE_CREATE_PROTECTED_BIT
		createInfo.queueFamilyIndex = queueFamilyIndex;
		createInfo.queueCount = queueCount;
		createInfo.pQueuePriorities = priorities.data();

		queueCreateInfos.emplace_back(createInfo);

		return queueCount;
	}

	bool
	Device::searchGraphicsAndComputeQueueConfiguration (const DeviceRequirements & requirements, const StaticVector< VkQueueFamilyProperties2, 8 > & queueFamilyProperties, StaticVector< VkDeviceQueueCreateInfo, 8 > & queueCreateInfos, std::map< uint32_t, StaticVector< float, 16 > > & queuePriorities) noexcept
	{
		/* 1. Discovering the best candidates. */
		std::optional< uint32_t > bestGraphicsIndex;
		std::optional< uint32_t > bestComputeIndex;

		/* NOTE: We look for the best indices for each task.
		 * Priority 1: A dedicated queue (e.g., graphics-only).
		 * Priority 2: A queue that supports the task (e.g., graphics + compute). */
		for ( uint32_t index = 0; index < queueFamilyProperties.size(); ++index )
		{
			const auto & properties = queueFamilyProperties[index].queueFamilyProperties;
			const bool hasGraphics = (properties.queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0;
			const bool hasCompute = (properties.queueFlags & VK_QUEUE_COMPUTE_BIT) != 0;

			/* NOTE: Check presentation support if necessary for graphics files. */
			bool presentationSupport = true;

			if ( requirements.needsPresentation() )
			{
				presentationSupport = m_physicalDevice->getSurfaceSupport(requirements.surface(), index);
			}

			if ( hasGraphics && presentationSupport )
			{
				if ( !bestGraphicsIndex.has_value() )
				{
					/* NOTE: If we don't have a candidate yet, we'll take this one. */
					bestGraphicsIndex = index;
				}
				else if ( !hasCompute )
				{
					TraceDebug{ClassId} << "The device has a graphics dedicated queue family at index #" << index;

					/* NOTE: If we find a DEDICATED graphics queue (without computing), it's even better! */
					bestGraphicsIndex = index;
				}
			}

			if ( hasCompute )
			{
				/* NOTE: Same idea but for compute here. */
				if ( !bestComputeIndex.has_value() )
				{
					bestComputeIndex = index;
				}
				else if ( !hasGraphics )
				{
					TraceDebug{ClassId} << "The device has a compute dedicated queue family at index #" << index;

					bestComputeIndex = index;
				}
			}
		}

		if ( !bestGraphicsIndex.has_value() )
		{
			Tracer::debug(ClassId, "The device lacks a graphics queue family!");

			return false;
		}

		if ( !bestComputeIndex.has_value() )
		{
			Tracer::debug(ClassId, "The device lacks a compute queue family!");

			return false;
		}

		if ( *bestGraphicsIndex != *bestComputeIndex )
		{
			{
				const auto queueCount = Device::addQueueFamilyToCreateInfo(*bestGraphicsIndex, queueFamilyProperties, queueCreateInfos, queuePriorities);

				m_graphicsQueueConfiguration.setQueueFamilyIndex(*bestGraphicsIndex);

				TraceSuccess{ClassId} << "Graphics configured with queue family index #" << *bestGraphicsIndex << " (queue count: " << queueCount << ").";
			}

			{
				const auto queueCount = Device::addQueueFamilyToCreateInfo(*bestComputeIndex, queueFamilyProperties, queueCreateInfos, queuePriorities);

				m_computeQueueConfiguration.setQueueFamilyIndex(*bestComputeIndex);

				TraceSuccess{ClassId} << "Compute configured with queue family index #" << *bestComputeIndex << " (queue count: " << queueCount << ").";
			}
		}
		else
		{
			const uint32_t universalIndex = *bestGraphicsIndex;

			const auto queueCount = Device::addQueueFamilyToCreateInfo(universalIndex, queueFamilyProperties, queueCreateInfos, queuePriorities);

			m_graphicsQueueConfiguration.setQueueFamilyIndex(universalIndex);

			m_computeQueueConfiguration.setQueueFamilyIndex(universalIndex);

			TraceSuccess{ClassId} << "Graphics and compute configured with queue family index #" << universalIndex << " (queue count: " << queueCount << ").";
		}

		return true;
	}

	bool
	Device::searchGraphicsQueueConfiguration (const DeviceRequirements & requirements, const StaticVector< VkQueueFamilyProperties2, 8 > & queueFamilyProperties, StaticVector< VkDeviceQueueCreateInfo, 8 > & queueCreateInfos, std::map< uint32_t, StaticVector< float, 16 > > & queuePriorities) noexcept
	{
		std::optional< uint32_t > bestGraphicsIndex;

		for ( uint32_t index = 0; index < queueFamilyProperties.size(); index++ )
		{
			const auto & properties = queueFamilyProperties[index].queueFamilyProperties;
			const bool hasGraphics = (properties.queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0;
			const bool hasCompute = (properties.queueFlags & VK_QUEUE_COMPUTE_BIT) != 0;

			if ( !hasGraphics )
			{
				continue;
			}

			if ( requirements.needsPresentation() && !m_physicalDevice->getSurfaceSupport(requirements.surface(), index) )
			{
				continue;
			}

			if ( !bestGraphicsIndex.has_value() )
			{
				bestGraphicsIndex = index;
			}
			else if ( !hasCompute )
			{
				TraceDebug{ClassId} << "The device has a graphics dedicated queue family at index #" << index;

				/* NOTE: We found a dedicated family (without computing), no need to look any further. */
				bestGraphicsIndex = index;

				break;
			}
		}

		if ( !bestGraphicsIndex.has_value() )
		{
			Tracer::debug(ClassId, "The device lacks a graphics queue family!");

			return false;
		}

		const auto queueCount = Device::addQueueFamilyToCreateInfo(bestGraphicsIndex.value(), queueFamilyProperties, queueCreateInfos, queuePriorities);

		m_graphicsQueueConfiguration.setQueueFamilyIndex(bestGraphicsIndex.value());

		TraceSuccess{ClassId} << "Graphics configured with queue family index #" << bestGraphicsIndex.value() << " (queue count: " << queueCount << ").";

		return true;
	}

	bool
	Device::searchComputeQueueConfiguration (const StaticVector< VkQueueFamilyProperties2, 8 > & queueFamilyProperties, StaticVector< VkDeviceQueueCreateInfo, 8 > & queueCreateInfos, std::map< uint32_t, StaticVector< float, 16 > > & queuePriorities) noexcept
	{
		std::optional< uint32_t > bestComputeIndex;

		for ( uint32_t index = 0; index < queueFamilyProperties.size(); index++ )
		{
			const auto & properties = queueFamilyProperties[index].queueFamilyProperties;
			const bool hasGraphics = (properties.queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0;
			const bool hasCompute = (properties.queueFlags & VK_QUEUE_COMPUTE_BIT) != 0;

			if ( !hasCompute )
			{
				continue;
			}

			if ( !bestComputeIndex.has_value() )
			{
				bestComputeIndex = index;
			}
			else if ( !hasGraphics )
			{
				TraceDebug{ClassId} << "The device has a compute dedicated queue family at index #" << index;

				/* NOTE: We found a dedicated family (without graphics), no need to look any further. */
				bestComputeIndex = index;

				break;
			}
		}

		if ( !bestComputeIndex.has_value() )
		{
			Tracer::debug(ClassId, "The device lacks a compute queue family!");

			return false;
		}

		const auto queueCount = Device::addQueueFamilyToCreateInfo(bestComputeIndex.value(), queueFamilyProperties, queueCreateInfos, queuePriorities);

		m_computeQueueConfiguration.setQueueFamilyIndex(bestComputeIndex.value());

		TraceSuccess{ClassId} << "Compute configured with queue family index #" << bestComputeIndex.value() << " (queue count: " << queueCount << ").";

		return true;
	}

	bool
	Device::searchTransferOnlyQueueConfiguration (const StaticVector< VkQueueFamilyProperties2, 8 > & queueFamilyProperties, StaticVector< VkDeviceQueueCreateInfo, 8 > & queueCreateInfos, std::map< uint32_t, StaticVector< float, 16 > > & queuePriorities) noexcept
	{
		std::optional< uint32_t > transferIndex;

		for ( uint32_t index = 0; index < queueFamilyProperties.size(); index++ )
		{
			const auto & properties = queueFamilyProperties[index].queueFamilyProperties;

			/* Check the transfer-only capabilities... */
			if ( (properties.queueFlags & VK_QUEUE_TRANSFER_BIT) == 0 )
			{
				continue;
			}

			/* ... and only the transfer-only capabilities. */
			if ( properties.queueFlags & ~(VK_QUEUE_TRANSFER_BIT | VK_QUEUE_SPARSE_BINDING_BIT) )
			{
				continue;
			}

			transferIndex = index;
		}

		if ( !transferIndex.has_value() )
		{
			Tracer::debug(ClassId, "The device lacks a transfer-only queue family!");

			return false;
		}

		const auto queueCount = Device::addQueueFamilyToCreateInfo(transferIndex.value(), queueFamilyProperties, queueCreateInfos, queuePriorities);

		m_transferQueueConfiguration.setQueueFamilyIndex(transferIndex.value());

		TraceSuccess{ClassId} << "Transfer-only configured with queue family index #" << transferIndex.value() << " (queue count: " << queueCount << ").";

		return true;
	}

	bool
	Device::createDevice (const DeviceRequirements & requirements, const StaticVector< VkDeviceQueueCreateInfo, 8 > & queueCreateInfos, const std::vector< const char * > & extensions) noexcept
	{
		/* Creates the logical device from the physical device information. */
		VkDeviceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		createInfo.pNext = &requirements.features();
		createInfo.flags = 0;
		createInfo.queueCreateInfoCount = static_cast< uint32_t >(queueCreateInfos.size());
		createInfo.pQueueCreateInfos = queueCreateInfos.data();
		{
			/* NOTE: These fields must stay unused, the validation layers for
			 * a device are deprecated after Vulkan 1.0 (Device Layer Deprecation). */
			createInfo.enabledLayerCount = 0;
			createInfo.ppEnabledLayerNames = nullptr;
		}
		createInfo.enabledExtensionCount = static_cast< uint32_t >(extensions.size());
		createInfo.ppEnabledExtensionNames = extensions.data();
		createInfo.pEnabledFeatures = nullptr; // VULKAN 1.0 API feature

		if ( const auto result = vkCreateDevice(m_physicalDevice->handle(), &createInfo, nullptr, &m_deviceHandle); result != VK_SUCCESS )
		{
			TraceFatal{ClassId} << "Unable to create a logical device : " << vkResultToCString(result) << " !";

			return false;
		}

		return true;
	}

	void
	Device::waitIdle (const char * location) const noexcept
	{
		if ( m_deviceHandle == VK_NULL_HANDLE )
		{
			TraceFatal{ClassId} << "The device is gone ! Call location:" "\n" << location;

			return;
		}

		const std::lock_guard< std::mutex > lock{m_logicalDeviceAccess};

		if ( const auto result = vkDeviceWaitIdle(m_deviceHandle); result != VK_SUCCESS )
		{
			TraceError{ClassId} << "Unable to wait the device " << m_deviceHandle << " : " << vkResultToCString(result) << " ! Call location:" "\n" << location;
		}
	}

	uint32_t
	Device::findMemoryType (uint32_t memoryTypeFilter, VkMemoryPropertyFlags propertyFlags) const noexcept
	{
		const auto & memoryProperties = m_physicalDevice->memoryPropertiesVK10();

		for ( auto memoryTypeIndex = 0U; memoryTypeIndex < memoryProperties.memoryTypeCount; memoryTypeIndex++ )
		{
			if ( (memoryTypeFilter & (1 << memoryTypeIndex)) != 0 && (memoryProperties.memoryTypes[memoryTypeIndex].propertyFlags & propertyFlags) == propertyFlags )
			{
				return memoryTypeIndex;
			}
		}

		return 0U;
	}

	VkFormat
	Device::findSupportedFormat (const std::vector< VkFormat > & formats, VkImageTiling tiling, VkFormatFeatureFlags featureFlags) const noexcept
	{
		for ( const auto & format : formats )
		{
			const auto formatProperties = m_physicalDevice->getFormatProperties(format);

			switch ( tiling )
			{
				case VK_IMAGE_TILING_OPTIMAL :
					if ( (formatProperties.optimalTilingFeatures & featureFlags) == featureFlags)
					{
						return format;
					}
					break;

				case VK_IMAGE_TILING_LINEAR :
					if ( (formatProperties.linearTilingFeatures & featureFlags) == featureFlags)
					{
						return format;
					}
					break;

				/* FIXME: Check this tiling mode. */
				case VK_IMAGE_TILING_DRM_FORMAT_MODIFIER_EXT :
					if ( (formatProperties.optimalTilingFeatures & featureFlags) == featureFlags || (formatProperties.linearTilingFeatures & featureFlags) != 0 )
					{
						return format;
					}
					break;

				case VK_IMAGE_TILING_MAX_ENUM :
				default:
					break;
			}
		}

		return VK_FORMAT_UNDEFINED;
	}

	uint32_t
	Device::checkMultisampleCount (uint32_t samples) const noexcept
	{
		if ( const auto maxSamples = m_physicalDevice->getMaxAvailableSampleCount(); samples > static_cast< uint32_t >(maxSamples) )
		{
			return maxSamples;
		}

		return Device::getSampleCountFlag(samples);
	}

	VkSampleCountFlagBits
	Device::getSampleCountFlag (uint32_t samples) noexcept
	{
		switch ( samples )
		{
			case 64 :
				return VK_SAMPLE_COUNT_64_BIT;

			case 32 :
				return VK_SAMPLE_COUNT_32_BIT;

			case 16 :
				return VK_SAMPLE_COUNT_16_BIT;

			case 8 :
				return VK_SAMPLE_COUNT_8_BIT;

			case 4 :
				return VK_SAMPLE_COUNT_4_BIT;

			case 2 :
				return VK_SAMPLE_COUNT_2_BIT;

			case 1 :
			default :
				return VK_SAMPLE_COUNT_1_BIT;
		}
	}
}
