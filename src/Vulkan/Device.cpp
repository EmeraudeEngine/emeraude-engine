/*
 * src/Vulkan/Device.cpp
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

#include "Device.hpp"

/* Local inclusions. */
#include "Utility.hpp"
#include "Instance.hpp"
#include "DeviceRequirements.hpp"
#include "QueueFamily.hpp"
#include "QueueFamilySQ.hpp"
#include "Tracer.hpp"

namespace EmEn::Vulkan
{
	using namespace EmEn::Libs;
	using namespace EmEn::Graphics;

	bool
	Device::create (const DeviceRequirements & requirements, const std::vector< const char * > & extensions) noexcept
	{
		if ( m_flags[ShowInformation] )
		{
			TraceInfo{ClassId} << getItemListAsString("Device", m_physicalDevice->getExtensions(nullptr));
		}

		/* Queue creation definitions. */
		std::vector< VkDeviceQueueCreateInfo > queueCreateInfos{};

		/* Check queues availabilities and possibilities against requirements. */
		if ( !this->prepareQueues(requirements, queueCreateInfos) )
		{
			Tracer::error(ClassId, "Unable to have a suitable queue configuration for the device !");

			return false;
		}

		if ( m_flags[ShowInformation] )
		{
			TraceInfo info{ClassId};

			info << "Device configuration : " "\n";

			for ( const auto & queueFamily : m_queueFamilies )
			{
				info << "Queue family #" << queueFamily->index() << " enabled !" "\n";
			}

			for ( const auto & [job, queue] : m_queueFamilyPerJob )
			{
				info << "'" << to_cstring(job) << "' job will be performed on queue family #" << queue->index() << "." "\n";
			}
		}

		/* Logical device creation. */
		if ( !this->createDevice(requirements, queueCreateInfos, extensions) )
		{
			Tracer::error(ClassId, "Logical device creation failed !");

			return false;
		}

		/* Retrieve every created queues. */
		auto device = this->shared_from_this();

		for ( auto & queueFamily : m_queueFamilies )
		{
			if ( !queueFamily->retrieveQueuesFromDevice(device) )
			{
				TraceError{ClassId} << "Unable to retrieve queue for queue family #" << queueFamily->index() << " of the device " << m_handle << " (" << this->identifier() << ") !";

				return false;
			}
		}

		this->setCreated();

		return true;
	}

	void
	Device::destroy () noexcept
	{
		if ( m_handle == VK_NULL_HANDLE )
		{
			return;
		}

		this->waitIdle("Destroying the device !");

		m_queueFamilyPerJob.clear();
		m_queueFamilies.clear();

		vkDestroyDevice(m_handle, nullptr);

		m_handle = VK_NULL_HANDLE;

		m_physicalDevice.reset();

		this->setDestroyed();
	}

	bool
	Device::declareQueuesFromSingleQueueFamily (const DeviceRequirements & requirements, const VkQueueFamilyProperties & queueFamilyProperty) noexcept
	{
		auto queueFamily = queueFamilyProperty.queueCount > 1 ?
			m_queueFamilies.emplace_back(std::make_shared< QueueFamily >(0, queueFamilyProperty.queueCount)) :
			m_queueFamilies.emplace_back(std::make_shared< QueueFamilySQ >(0));

		m_flags[HasBasicSupport] = true;

		/* NOTE: Without transfer nothing is possible ! This should never happen ! */
		if ( (queueFamilyProperty.queueFlags & VK_QUEUE_TRANSFER_BIT) == 0 )
		{
			TraceFatal{ClassId} << "The physical device '" << this->name() << "' has no queue for transfer !";

			return false;
		}

		m_queueFamilyPerJob[QueueJob::Transfer] = queueFamily;

		if ( requirements.needsGraphics() )
		{
			if ( (queueFamilyProperty.queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0 )
			{
				TraceError{ClassId} << "The physical device '" << this->name() << "' has no queue for graphics !";

				return false;
			}

			m_queueFamilyPerJob[QueueJob::Graphics] = queueFamily;
			m_queueFamilyPerJob[QueueJob::GraphicsTransfer] = queueFamily;

			if ( requirements.needsPresentation() )
			{
				if ( !m_physicalDevice->getSurfaceSupport(requirements.surface(), 0) )
				{
					TraceFatal{ClassId} << "The physical device '" << this->name() << "' has no queue for presentation !";

					return false;
				}

				m_queueFamilyPerJob[QueueJob::Presentation] = queueFamily;
			}
		}

		if ( requirements.needsCompute() )
		{
			if ( (queueFamilyProperty.queueFlags & VK_QUEUE_COMPUTE_BIT) == 0 )
			{
				TraceError{ClassId} << "The physical device '" << this->name() << "' has no queue for compute !";

				return false;
			}

			m_queueFamilyPerJob[QueueJob::Compute] = queueFamily;
			m_queueFamilyPerJob[QueueJob::ComputeTransfer] = queueFamily;
		}

		return true;
	}

	bool
	Device::declareQueuesFromMultipleQueueFamilies (const DeviceRequirements & requirements, const std::vector< VkQueueFamilyProperties > & queueFamilyProperties) noexcept
	{
		/* Loop over all existing queue family in the physical device. */
		for ( uint32_t queueFamilyIndex = 0; queueFamilyIndex < queueFamilyProperties.size(); queueFamilyIndex++ )
		{
			const auto & flag = queueFamilyProperties[queueFamilyIndex].queueFlags;

			const auto maxQueueCount = queueFamilyProperties[queueFamilyIndex].queueCount;

			auto queueFamily = maxQueueCount > 1 ?
				m_queueFamilies.emplace_back(std::make_shared< QueueFamily >(queueFamilyIndex, maxQueueCount)) :
				m_queueFamilies.emplace_back(std::make_shared< QueueFamilySQ >(queueFamilyIndex));

			if ( (flag & VK_QUEUE_GRAPHICS_BIT) != 0 )
			{
				if ( requirements.needsGraphics() )
				{
					/* If no graphic queue has been registered or if the queue family is exclusive to graphics. */
					if ( !m_queueFamilyPerJob.contains(QueueJob::Graphics) || (flag & VK_QUEUE_COMPUTE_BIT) == 0 )
					{
						m_queueFamilyPerJob[QueueJob::Graphics] = queueFamily;

						if ( (flag & VK_QUEUE_TRANSFER_BIT) != 0 )
						{
							m_queueFamilyPerJob[QueueJob::GraphicsTransfer] = queueFamily;
						}
					}
				}

				if ( !requirements.tryPresentationSeparateFromGraphics() && requirements.needsPresentation() && m_physicalDevice->getSurfaceSupport(requirements.surface(), queueFamilyIndex) )
				{
					m_queueFamilyPerJob[QueueJob::Presentation] = queueFamily;
				}
			}
			else if ( requirements.tryPresentationSeparateFromGraphics() && requirements.needsPresentation() && m_physicalDevice->getSurfaceSupport(requirements.surface(), queueFamilyIndex) )
			{
				m_queueFamilyPerJob[QueueJob::Presentation] = queueFamily;
			}

			if ( requirements.needsCompute() && (flag & VK_QUEUE_COMPUTE_BIT) != 0 )
			{
				/* If no compute queue has been registered or if the queue family is exclusive to compute. */
				if ( !m_queueFamilyPerJob.contains(QueueJob::Compute) || (flag & VK_QUEUE_GRAPHICS_BIT) == 0 )
				{
					m_queueFamilyPerJob[QueueJob::Compute] = queueFamily;

					if ( (flag & VK_QUEUE_TRANSFER_BIT) != 0 )
					{
						m_queueFamilyPerJob[QueueJob::ComputeTransfer] = queueFamily;
					}
				}
			}

			if ( requirements.needsTransfer() && (flag & VK_QUEUE_TRANSFER_BIT) != 0 && (flag & VK_QUEUE_GRAPHICS_BIT) == 0 && (flag & VK_QUEUE_COMPUTE_BIT) == 0 )
			{
				m_queueFamilyPerJob[QueueJob::Transfer] = queueFamily;
			}
		}

		/* Check if every need are met. */
		if ( requirements.needsGraphics() && !m_queueFamilyPerJob.contains(QueueJob::Graphics) )
		{
			TraceError{ClassId} << "The physical device '" << this->name() << "' has no queue for graphics !";

			return false;
		}

		if ( requirements.needsPresentation() && !m_queueFamilyPerJob.contains(QueueJob::Presentation) )
		{
			TraceError{ClassId} << "The physical device '" << this->name() << "' has no queue for presentation !";

			return false;
		}

		if ( requirements.needsCompute() && !m_queueFamilyPerJob.contains(QueueJob::Compute) )
		{
			TraceError{ClassId} << "The physical device '" << this->name() << "' has no queue for compute !";

			return false;
		}

		if ( requirements.needsTransfer() && !m_queueFamilyPerJob.contains(QueueJob::Transfer) )
		{
			switch ( requirements.jobHint() )
			{
				case DeviceJobHint::General :
					TraceError{ClassId} << "The physical device '" << this->name() << "' has no queue for separate transfer !";

					return false;

				case DeviceJobHint::Graphics :
					m_queueFamilyPerJob[QueueJob::Transfer] = m_queueFamilyPerJob[QueueJob::Graphics];
					break;

				case DeviceJobHint::Compute :
					m_queueFamilyPerJob[QueueJob::Transfer] = m_queueFamilyPerJob[QueueJob::Compute];
					break;
			}
		}

		return true;
	}

	bool
	Device::prepareQueues (const DeviceRequirements & requirements, std::vector< VkDeviceQueueCreateInfo > & queueCreateInfos) noexcept
	{
		if ( m_flags[ShowInformation] )
		{
			TraceInfo{ClassId} << requirements;
		}

		const auto & queueFamilyProperties = m_physicalDevice->queueFamilyProperties();

		if ( queueFamilyProperties.empty() )
		{
			TraceError{ClassId} << "The physical device '" << this->name() << "' has no family queue !";

			return false;
		}

		/* Check queue family requirements. */
		if ( queueFamilyProperties.size() == 1 )
		{
			if ( m_flags[ShowInformation] )
			{
				TraceInfo{ClassId} <<
				   "The physical device '" << this->name() << "' has a basic support. "
				   "There will be only one queue family and maybe only one queue at all.";
			}

			if ( !this->declareQueuesFromSingleQueueFamily(requirements, queueFamilyProperties[0]) )
			{
				return false;
			}
		}
		else
		{
			if ( m_flags[ShowInformation] )
			{
				TraceInfo{ClassId} << "The physical device '" << this->name() << "' has an advanced support.";
			}

			if ( !this->declareQueuesFromMultipleQueueFamilies(requirements, queueFamilyProperties) )
			{
				return false;
			}
		}

		/* Check queue count requirements. */
		for ( auto & queueFamily : m_queueFamilies )
		{
			if ( !queueFamily->hasSingleQueue() )
			{
				std::vector< std::pair< QueueJob, float > > structure{};

				for ( const auto & job : m_queueFamilyPerJob )
				{
					if ( job.second != queueFamily )
					{
						continue;
					}

					switch ( job.first )
					{
						case QueueJob::Graphics :
							for ( auto priority : requirements.graphicsQueuePriorities() )
							{
								structure.emplace_back(job.first, priority);
							}
							break;

						case QueueJob::GraphicsTransfer :
							for ( auto priority : requirements.graphicsTransferQueuePriorities() )
							{
								structure.emplace_back(job.first, priority);
							}
							break;

						case QueueJob::Presentation :
							for ( auto priority : requirements.presentationQueuePriorities() )
							{
								structure.emplace_back(job.first, priority);
							}
							break;

						case QueueJob::Compute :
							for ( auto priority : requirements.computeQueuePriorities() )
							{
								structure.emplace_back(job.first, priority);
							}
							break;

						case QueueJob::ComputeTransfer :
							for ( auto priority : requirements.computeTransferQueuePriorities() )
							{
								structure.emplace_back(job.first, priority);
							}
							break;

						case QueueJob::Transfer :
							for ( auto priority : requirements.transferQueuePriorities() )
							{
								structure.emplace_back(job.first, priority);
							}
							break;
					}
				}

				if ( structure.empty() )
				{
					if ( m_flags[ShowInformation] )
					{
						TraceInfo{ClassId} << "There is no queue required for queue family #" << queueFamily->index() << " !";
					}

					continue;
				}

				if ( !queueFamily->declareQueueStructure(structure) )
				{
					return false;
				}
			}

			queueCreateInfos.emplace_back(queueFamily->getCreateInfo(0));
		}

		return true;
	}

	bool
	Device::createDevice (const DeviceRequirements & requirements, const std::vector< VkDeviceQueueCreateInfo > & queueCreateInfos, const std::vector< const char * > & extensions) noexcept
	{
		/* Creates the logical device from the physical device information. */
		VkDeviceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		createInfo.pNext = nullptr;
		createInfo.flags = 0;
		createInfo.queueCreateInfoCount = static_cast< uint32_t >(queueCreateInfos.size());
		createInfo.pQueueCreateInfos = queueCreateInfos.data();
		/* NOTE: Device layer are ignored (Device layer deprecation) */
		createInfo.enabledLayerCount = 0;
		createInfo.ppEnabledLayerNames = nullptr;
		createInfo.enabledExtensionCount = static_cast< uint32_t >(extensions.size());
		createInfo.ppEnabledExtensionNames = extensions.data();
		createInfo.pEnabledFeatures = &(requirements.features());

		const auto result = vkCreateDevice(m_physicalDevice->handle(), &createInfo, nullptr, &m_handle);

		if ( result != VK_SUCCESS )
		{
			TraceFatal{ClassId} << "Unable to create a logical device : " << vkResultToCString(result) << " !";

			return false;
		}

		return true;
	}

	uint32_t
	Device::getGraphicsFamilyIndex () const noexcept
	{
		const auto queueFamilyIt = m_queueFamilyPerJob.find(QueueJob::Graphics);

		if ( queueFamilyIt == m_queueFamilyPerJob.cend() )
		{
			return 0;
		}

		return queueFamilyIt->second->index();
	}

	uint32_t
	Device::getComputeFamilyIndex () const noexcept
	{
		const auto queueFamilyIt = m_queueFamilyPerJob.find(QueueJob::Compute);

		if ( queueFamilyIt == m_queueFamilyPerJob.cend() )
		{
			return 0;
		}

		return queueFamilyIt->second->index();
	}

	uint32_t
	Device::getTransferFamilyIndex () const noexcept
	{
		const auto queueFamilyIt = m_queueFamilyPerJob.find(QueueJob::Transfer);

		if ( queueFamilyIt == m_queueFamilyPerJob.cend() )
		{
			return 0;
		}

		return queueFamilyIt->second->index();
	}

	Queue *
	Device::getQueue (QueueJob deviceJobType, QueuePriority priority) const noexcept
	{
		const auto queueFamilyIt = m_queueFamilyPerJob.find(deviceJobType);

		if ( queueFamilyIt == m_queueFamilyPerJob.cend() )
		{
			return nullptr;
		}

		return queueFamilyIt->second->queue(deviceJobType, priority);
	}

	void
	Device::waitIdle (const char * location) const noexcept
	{
		const std::lock_guard< std::mutex > lock{m_mutex};

		if ( m_handle == VK_NULL_HANDLE )
		{
			TraceError{ClassId} <<
				"The device is gone !" "\n"
				"Call location : " << location;

			return;
		}

		const auto result = vkDeviceWaitIdle(m_handle);

		if ( result != VK_SUCCESS )
		{
			TraceError{ClassId} <<
				"Unable to wait the device " << m_handle << " : " << vkResultToCString(result) << " !" "\n"
				"Call location : " << location;
		}
	}

	uint32_t
	Device::findMemoryType (uint32_t memoryTypeFilter, VkMemoryPropertyFlags propertyFlags) const noexcept
	{
		const auto & memoryProperties = m_physicalDevice->memoryProperties();

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
			auto formatProperties = m_physicalDevice->getFormatProperties(format);

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

	VkSampleCountFlagBits
	Device::findSampleCount (uint32_t samples) const noexcept
	{
		const auto maxSamples = m_physicalDevice->getMaxAvailableSampleCount();

		if ( samples > static_cast< uint32_t >(maxSamples) )
		{
			return maxSamples;
		}

		switch ( samples )
		{
			case 1 :
				return VK_SAMPLE_COUNT_1_BIT;

			case 2 :
				return VK_SAMPLE_COUNT_2_BIT;

			case 4 :
				return VK_SAMPLE_COUNT_4_BIT;

			case 8 :
				return VK_SAMPLE_COUNT_8_BIT;

			case 16 :
				return VK_SAMPLE_COUNT_16_BIT;

			case 32 :
				return VK_SAMPLE_COUNT_32_BIT;

			case 64 :
				return VK_SAMPLE_COUNT_64_BIT;

			default :
				return VK_SAMPLE_COUNT_1_BIT;
		}
	}
}
