/*
 * src/Vulkan/Device.hpp
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
#include <cstdint>
#include <ranges>
#include <array>
#include <map>
#include <memory>
#include <mutex>

/* Third-party inclusions. */
#include "vk_mem_alloc.h"

/* Local inclusions for inheritances. */
#include "AbstractObject.hpp"
#include "Libs/NameableTrait.hpp"

/* Local inclusions for usage. */
#include "Libs/StaticVector.hpp"
#include "PhysicalDevice.hpp"
#include "Queue.hpp"
#include "Types.hpp"

/* Forward declarations. */
namespace EmEn::Vulkan
{
	class Instance;
	class DeviceRequirements;
}

namespace EmEn::Vulkan
{
	/** @brief Structure to sort queues by priority. */
	class DeviceQueueConfiguration final
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"VulkanDeviceQueueConfiguration"};

			/**
			 * @brief Constructs a default queue configuration for a device.
			 */
			DeviceQueueConfiguration () noexcept = default;

			/**
			 * @brief Set the family queue index for this job from the logical device analysis.
			 * @param queueFamilyIndex An unsigned integer.
			 * @return void
			 */
			void
			setQueueFamilyIndex (uint32_t queueFamilyIndex) noexcept
			{
				m_queueFamilyIndex = queueFamilyIndex;
			}

			/**
			 * @brief Returns the queue family index for this job.
			 * @return bool
			 */
			[[nodiscard]]
			uint32_t
			queueFamilyIndex () const noexcept
			{
				return m_queueFamilyIndex;
			}

			/**
			 * @brief Registers a queue to the configuration.
			 * @param queue A pointer to a queue.
			 * @param priority The priority of the queue.
			 * @return void
			 */
			void
			registerQueue (Queue * queue, QueuePriority priority) const noexcept
			{
				m_queueByPriorities[static_cast< uint32_t >(priority)].second.emplace_back(queue);
			}

			/**
			 * @brief Returns queue priority structure.
			 * @return const Libs::StaticVector< Queue *, 16 > &
			 */
			[[nodiscard]]
			const Libs::StaticVector< Queue *, 16 > &
			queues (QueuePriority priority) const noexcept
			{
				return m_queueByPriorities[static_cast< uint32_t >(priority)].second;
			}

			/**
			 * @brief Returns a queue by priority.
			 * @param priority The priority desired. High, Medium or Low.
			 * @return Queue *
			 */
			[[nodiscard]]
			Queue * queue (QueuePriority priority) const noexcept;

			/**
			 * @brief Returns whether this configuration is enabled/available in the device.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			enabled () const noexcept
			{
				return std::ranges::any_of(m_queueByPriorities, [] (const auto & queueList) {
					return !queueList.second.empty();
				});
			}

			/**
			 * @brief Clears data and links.
			 * @return void
			 */
			void
			clear () noexcept
			{
				m_queueFamilyIndex = 0;

				for ( auto & queueList : m_queueByPriorities | std::views::values )
				{
					queueList.clear();
				}
			}

		private:

			uint32_t m_queueFamilyIndex{0};
			mutable std::array< std::pair< std::atomic< uint32_t >, Libs::StaticVector< Queue *, 16 > >, 3 > m_queueByPriorities;
	};

	/**
	 * @brief Defines a logical device from a physical device.
	 * @extends EmEn::Vulkan::AbstractObject This is the device, so a simple object is ok.
	 * @extends EmEn::Libs::NameableTrait To set a name on a device.
	 */
	class Device final : public std::enable_shared_from_this< Device >, public AbstractObject, public Libs::NameableTrait
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"VulkanDevice"};

			/**
			 * @brief Constructs a device.
			 * @param instance A reference to the Vulkan instance.
			 * @param deviceName A string [std::move].
			 * @param physicalDevice A reference to a physical device smart pointer.
			 * @param showInformation Enable the device information in the terminal.
			 */
			Device (const Instance & instance, std::string deviceName, const std::shared_ptr< PhysicalDevice > & physicalDevice, bool showInformation) noexcept
				: NameableTrait{std::move(deviceName)},
				m_instance{instance},
				m_physicalDevice{physicalDevice},
				m_showInformation{showInformation}
			{

			}

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			Device (const Device & copy) noexcept = delete;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			Device (Device && copy) noexcept = delete;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 */
			Device & operator= (const Device & copy) noexcept = delete;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 */
			Device & operator= (Device && copy) noexcept = delete;

			/**
			 * @brief Destructs the device.
			 */
			~Device () override
			{
				this->destroy();
			}

			/**
			 * @brief Creates the device.
			 * @param requirements A reference to a device requirement.
			 * @param extensions A reference to a vector of extensions.
			 * @param useVMA Use Vulkan Memory Allocator.
			 * @return bool
			 */
			[[nodiscard]]
			bool create (const DeviceRequirements & requirements, const std::vector< const char * > & extensions, bool useVMA) noexcept;

			/**
			 * @brief Destroys the device.
			 * @return void
			 */
			void destroy () noexcept;

			/**
			 * @brief Returns the physical device smart pointer.
			 * @return std::shared_ptr< PhysicalDevice >
			 */
			[[nodiscard]]
			std::shared_ptr< PhysicalDevice >
			physicalDevice () const noexcept
			{
				return m_physicalDevice;
			}

			/**
			 * @brief Returns the device handle.
			 * @return VkDevice
			 */
			[[nodiscard]]
			VkDevice
			handle () const noexcept
			{
				return m_deviceHandle;
			}

			/**
			 * @brief Returns whether the memory allocator is enabled.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			useMemoryAllocator () const noexcept
			{
				return m_useMemoryAllocator;
			}

			/**
			 * @brief Returns the memory allocator handle.
			 * @return VmaAllocator
			 */
			[[nodiscard]]
			VmaAllocator
			memoryAllocatorHandle () const noexcept
			{
				return m_memoryAllocatorHandle;
			}

			/**
			 * @brief Returns whether the device has only one family queue for all.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			hasBasicSupport () const noexcept
			{
				return m_basicSupport;
			}

			/**
			 * @brief Returns whether the device has been set up for graphics.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			hasGraphicsQueues () const noexcept
			{
				return m_graphicsQueueConfiguration.enabled();
			}

			/**
			 * @brief Returns the queue family index for graphics queues.
			 * @note This may return the same family queue index from another configuration.
			 * @warning Be sure of calling Device::hasGraphicsQueues() before trusting the index.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			getGraphicsFamilyIndex () const noexcept
			{
				return m_graphicsQueueConfiguration.queueFamilyIndex();
			}

			/**
			 * @brief Returns a graphics queue.
			 * @note This may return the same queue as another configuration.
			 * @warning This may return a nullptr!
			 * @param priority The priority of the queue.
			 * @return Queue *
			 */
			[[nodiscard]]
			Queue *
			getGraphicsQueue (QueuePriority priority) const noexcept
			{
				if ( !m_graphicsQueueConfiguration.enabled() )
				{
					return nullptr;
				}

				return m_graphicsQueueConfiguration.queue(priority);
			}

			/**
			 * @brief Returns whether the device has been set up for compute queues.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			hasComputeQueues () const noexcept
			{
				return m_computeQueueConfiguration.enabled();
			}

			/**
			 * @brief Returns the queue family index for compute queues.
			 * @note This may return the same family queue index from another configuration.
			 * @warning Be sure of calling Device::hasComputeQueues() before trusting the index.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			getComputeFamilyIndex () const noexcept
			{
				return m_computeQueueConfiguration.queueFamilyIndex();
			}

			/**
			 * @brief Returns a compute queue.
			 * @note This may return the same queue as another configuration.
			 * @warning This may return a nullptr!
			 * @param priority The priority of the queue.
			 * @return Queue *
			 */
			[[nodiscard]]
			Queue *
			getComputeQueue (QueuePriority priority) const noexcept
			{
				if ( !m_computeQueueConfiguration.enabled() )
				{
					return nullptr;
				}

				return m_computeQueueConfiguration.queue(priority);
			}

			/**
			 * @brief Returns whether the device has been set up for transfer-only queues.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			hasTransferQueues () const noexcept
			{
				return m_transferQueueConfiguration.enabled();
			}

			/**
			 * @brief Returns the transfer-only queue family index.
			 * @note This may return the same family queue index from another configuration.
			 * @warning Be sure of calling Device::hasTransferQueues() before trusting the index.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			getTransferFamilyIndex () const noexcept
			{
				return m_transferQueueConfiguration.queueFamilyIndex();
			}

			/**
			 * @brief Returns a transfer-only queue.
			 * @note This may return the same queue as another configuration.
			 * @warning This may return a nullptr!
			 * @param priority The priority of the queue.
			 * @return Queue *
			 */
			[[nodiscard]]
			Queue *
			getTransferQueue (QueuePriority priority) const noexcept
			{
				if ( !m_transferQueueConfiguration.enabled() )
				{
					return nullptr;
				}

				return m_transferQueueConfiguration.queue(priority);
			}

			/**
			 * @brief Returns the transfer-only queue family index for graphics if available.
			 * @note This may return the same family queue index from the graphics configuration.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			getGraphicsTransferFamilyIndex () const noexcept
			{
				if ( !m_transferQueueConfiguration.enabled() )
				{
					return m_graphicsQueueConfiguration.queueFamilyIndex();
				}

				return m_transferQueueConfiguration.queueFamilyIndex();
			}

			/**
			 * @brief Returns a transfer-only queue for graphics if available.
			 * @note This may return a queue from the graphics configuration.
			 * @warning This may return a nullptr!
			 * @param priority The priority of the queue.
			 * @return Queue *
			 */
			[[nodiscard]]
			Queue *
			getGraphicsTransferQueue (QueuePriority priority) const noexcept
			{
				if ( !m_transferQueueConfiguration.enabled() )
				{
					return m_graphicsQueueConfiguration.queue(priority);
				}

				return m_transferQueueConfiguration.queue(priority);
			}

			/**
			 * @brief Returns the transfer-only queue family index for compute if available.
			 * @note This may return the same family queue index from the compute configuration.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			getComputeTransferFamilyIndex () const noexcept
			{
				if ( !m_transferQueueConfiguration.enabled() )
				{
					return m_computeQueueConfiguration.queueFamilyIndex();
				}

				return m_transferQueueConfiguration.queueFamilyIndex();
			}

			/**
			 * @brief Returns a transfer-only queue for compute if available.
			 * @note This may return a queue from the compute configuration.
			 * @warning This may return a nullptr!
			 * @param priority The priority of the queue.
			 * @return Queue *
			 */
			[[nodiscard]]
			Queue *
			getComputeTransferQueue (QueuePriority priority) const noexcept
			{
				if ( !m_transferQueueConfiguration.enabled() )
				{
					return m_computeQueueConfiguration.queue(priority);
				}

				return m_transferQueueConfiguration.queue(priority);
			}

			/**
			 * @brief Waits for a device to become idle.
			 * @param location A point to string.
			 * @return void
			 */
			void waitIdle (const char * location) const noexcept;

			/**
			 * @brief Finds the suitable memory type.
			 * @param memoryTypeFilter The memory type.
			 * @param propertyFlags The access type of memory requested.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t findMemoryType (uint32_t memoryTypeFilter, VkMemoryPropertyFlags propertyFlags) const noexcept;

			/**
			 * @brief Finds a supported format from a device.
			 * @param formats A reference to a format vector.
			 * @param tiling
			 * @param featureFlags
			 * @return VkFormat
			 */
			[[nodiscard]]
			VkFormat findSupportedFormat (const std::vector< VkFormat > & formats, VkImageTiling tiling, VkFormatFeatureFlags featureFlags) const noexcept;

			/**
			 * @brief Returns the available samples against the desired for multisampling rendering.
			 * @param samples The number of samples desired.
			 * @return VkSampleCountFlagBits
			 */
			[[nodiscard]]
			uint32_t checkMultisampleCount (uint32_t samples) const noexcept;

			/**
			 * @brief Returns sample flag for Vulkan.
			 * @param samples The number of samples desired.
			 * @return VkSampleCountFlagBits
			 */
			[[nodiscard]]
			static VkSampleCountFlagBits getSampleCountFlag (uint32_t samples) noexcept;

			/**
			 * @brief Lock the access to the device.
			 * @note std::lock_guard friendly.
			 * @return void
			 */
			void
			lock () const
			{
				m_logicalDeviceAccess.lock();
			}

			/**
			 * @brief Unlock the access to the device.
			 * @note std::lock_guard friendly.
			 * @return void
			 */
			void
			unlock () const
			{
				m_logicalDeviceAccess.unlock();
			}

		private:

			/**
			 * @brief Creates the Vulkan memory allocator for this device.
			 * @note The memory allocator is part of the device to follow the recommendation that says one allocator per device.
			 * @return bool
			 */
			[[nodiscard]]
			bool createMemoryAllocator () noexcept;

			/**
			 * @brief Destroys the Vulkan memory allocator for this device.
			 * @return void
			 */
			void destroyMemoryAllocator () noexcept;

			/**
			 * @brief Adds a queue family to the createInfos list and returns the number of queues in this family.
			 * @return uint32_t
			 */
			[[nodiscard]]
			static
			uint32_t addQueueFamilyToCreateInfo (uint32_t queueFamilyIndex, const Libs::StaticVector<VkQueueFamilyProperties2, 8>& queueFamilyProperties, Libs::StaticVector< VkDeviceQueueCreateInfo, 8 > & queueCreateInfos, std::map< uint32_t, Libs::StaticVector< float, 16 > > & queuePriorities) noexcept;

			/**
			 * @brief Prepares queues for a graphics and compute device.
			 * @param requirements A reference to the device requirements.
			 * @param queueFamilyProperties A reference to the family properties.
			 * @param queueCreateInfos A writable reference to the queue creation information vector.
			 * @param queuePriorities A writable reference to a map for queue priorities.
			 * @return bool
			 */
			[[nodiscard]]
			bool searchGraphicsAndComputeQueueConfiguration (const DeviceRequirements & requirements, const Libs::StaticVector< VkQueueFamilyProperties2, 8 > & queueFamilyProperties, Libs::StaticVector< VkDeviceQueueCreateInfo, 8 > & queueCreateInfos, std::map< uint32_t, Libs::StaticVector< float, 16 > > & queuePriorities) noexcept;

			/**
			 * @brief Prepares queues for a graphics device.
			 * @param requirements A reference to the device requirements.
			 * @param queueFamilyProperties A reference to the family properties.
			 * @param queueCreateInfos A writable reference to the queue creation information vector.
			 * @param queuePriorities A writable reference to a map for queue priorities.
			 * @return bool
			 */
			[[nodiscard]]
			bool searchGraphicsQueueConfiguration (const DeviceRequirements & requirements, const Libs::StaticVector< VkQueueFamilyProperties2, 8 > & queueFamilyProperties, Libs::StaticVector< VkDeviceQueueCreateInfo, 8 > & queueCreateInfos, std::map< uint32_t, Libs::StaticVector< float, 16 > > & queuePriorities) noexcept;

			/**
			 * @brief Prepares queues for a compute device.
			 * @param queueFamilyProperties A reference to the family properties.
			 * @param queueCreateInfos A writable reference to the queue creation information vector.
			 * @param queuePriorities A writable reference to a map for queue priorities.
			 * @return bool
			 */
			[[nodiscard]]
			bool searchComputeQueueConfiguration (const Libs::StaticVector< VkQueueFamilyProperties2, 8 > & queueFamilyProperties, Libs::StaticVector< VkDeviceQueueCreateInfo, 8 > & queueCreateInfos, std::map< uint32_t, Libs::StaticVector< float, 16 > > & queuePriorities) noexcept;

			/**
			 * @brief Prepares transfer-only queues for the device (optional).
			 * @param queueFamilyProperties A reference to the family properties.
			 * @param queueCreateInfos A writable reference to the queue creation information vector.
			 * @param queuePriorities A writable reference to a map for queue priorities.
			 * @return bool
			 */
			[[nodiscard]]
			bool searchTransferOnlyQueueConfiguration (const Libs::StaticVector< VkQueueFamilyProperties2, 8 > & queueFamilyProperties, Libs::StaticVector< VkDeviceQueueCreateInfo, 8 > & queueCreateInfos, std::map< uint32_t, Libs::StaticVector< float, 16 > > & queuePriorities) noexcept;

			/**
			 * @brief Creates the device with the defined and verified queues.
			 * @param requirements A reference to a device requirement.
			 * @param queueCreateInfos A reference to a list of CreateInfo for Vulkan queues.
			 * @param extensions A reference to a list of extensions to enable with the device.
			 * @return bool
			 */
			[[nodiscard]]
			bool createDevice (const DeviceRequirements & requirements, const Libs::StaticVector< VkDeviceQueueCreateInfo, 8 > & queueCreateInfos, const std::vector< const char * > & extensions) noexcept;

			/**
			 * @brief Installs queues generated from the device.
			 * @param queuePriorityValues A reference to a map for the initial priorities selected.
			 * @param configuration A writable reference to the current configuration.
			 * @return bool
			 */
			[[nodiscard]]
			bool installQueues (const std::map< uint32_t, Libs::StaticVector< float, 16 > > & queuePriorityValues, DeviceQueueConfiguration & configuration) noexcept;

			const Instance & m_instance;
			std::shared_ptr< PhysicalDevice > m_physicalDevice;
			VkDevice m_deviceHandle{VK_NULL_HANDLE};
			VmaAllocator m_memoryAllocatorHandle{VK_NULL_HANDLE};
			Libs::StaticVector< std::unique_ptr< Queue >, 32 > m_queues;
			DeviceQueueConfiguration m_graphicsQueueConfiguration;
			DeviceQueueConfiguration m_computeQueueConfiguration;
			DeviceQueueConfiguration m_transferQueueConfiguration;
			mutable std::mutex m_logicalDeviceAccess;
			bool m_showInformation{false};
			bool m_basicSupport{false};
			bool m_useMemoryAllocator{false};
	};
}
