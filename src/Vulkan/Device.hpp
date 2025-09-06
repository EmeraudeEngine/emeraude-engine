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

/* Engine configuration file. */
#include "emeraude_config.hpp"

/* STL inclusions. */
#include <array>
#include <cstdint>
#include <map>
#include <mutex>
#include <memory>

/* Local inclusions for inheritances. */
#include "AbstractObject.hpp"
#include "Libs/NameableTrait.hpp"

/* Local inclusions for usage. */
#include "Libs/StaticVector.hpp"
#include "PhysicalDevice.hpp"
#include "Types.hpp"

/* Forward declarations. */
namespace EmEn::Vulkan
{
	class Instance;
	class DeviceRequirements;
	class QueueFamilyInterface;
	class Queue;
}

namespace EmEn::Vulkan
{
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
			 * @param deviceName A string [std::move].
			 * @param physicalDevice A reference to a physical device smart pointer.
			 * @param showInformation Enable the device information in the terminal.
			 */
			Device (std::string deviceName, const std::shared_ptr< PhysicalDevice > & physicalDevice, bool showInformation) noexcept
				: NameableTrait{std::move(deviceName)},
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
			 * @return bool
			 */
			[[nodiscard]]
			bool create (const DeviceRequirements & requirements, const std::vector< const char * > & extensions) noexcept;

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
				return m_handle;
			}

			/**
			 * @brief Returns whether the device has only one family queue for all.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			hasBasicSupport () const noexcept
			{
				return m_hasBasicSupport;
			}

			/**
			 * @brief Returns whether the device has been set up for graphics.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			hasGraphicsQueues () const noexcept
			{
				return m_queueFamilyPerJob.contains(QueueJob::Graphics);
			}

			/**
			 * @brief Returns the queue family index for graphics.
			 * @warning Be sure of calling hasGraphicsQueues() before trusting the index.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t getGraphicsFamilyIndex () const noexcept;

			/**
			 * @brief Returns whether the device has been set up for compute queues.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			hasComputeQueues () const noexcept
			{
				return m_queueFamilyPerJob.contains(QueueJob::Compute);
			}

			/**
			 * @brief Returns the queue family index for compute queues.
			 * @warning Be sure of calling hasComputeQueues() before trusting the index.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t getComputeFamilyIndex () const noexcept;

			/**
			 * @brief Returns whether the device has been set up to have a separated transfer queue.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			hasTransferQueues () const noexcept
			{
				return m_queueFamilyPerJob.contains(QueueJob::Transfer);
			}

			/**
			 * @brief Returns the queue family index for transfer-only queues.
			 * @warning Be sure of calling hasTransferQueues() before trusting the index.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t getTransferFamilyIndex () const noexcept;

			/**
			 * @brief Returns a queue for a specific purpose.
			 * @note This can return always the same queue for a basic device.
			 * @warning This can also return nullptr.
			 * @param job The requested job type from the queue.
			 * @param priority The priority of the queue.
			 * @return Queue *
			 */
			[[nodiscard]]
			Queue * getQueue (QueueJob job, QueuePriority priority) const noexcept;

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
			 * @brief Finds the right Vulkan token for multisampling.
			 * @param samples The number of samples desired.
			 * @return VkSampleCountFlagBits
			 */
			[[nodiscard]]
			VkSampleCountFlagBits findSampleCount (uint32_t samples) const noexcept;

			/**
			 * @brief Lock the access to the device.
			 * @note std::lock_guard friendly.
			 * @return void
			 */
			void
			lock () const
			{
				m_deviceExternalAccess.lock();
			}

			/**
			 * @brief Unlock the access to the device.
			 * @note std::lock_guard friendly.
			 * @return void
			 */
			void
			unlock () const
			{
				m_deviceExternalAccess.unlock();
			}

		private:

			/**
			 * @brief Prepares queues configuration from requirements.
			 * @param requirements A reference to a device requirement.
			 * @param queueCreateInfos A reference to a list of CreateInfo for Vulkan queues to complete.
			 * @return bool
			 */
			[[nodiscard]]
			bool prepareQueues (const DeviceRequirements & requirements, Libs::StaticVector< VkDeviceQueueCreateInfo, 16 > & queueCreateInfos) noexcept;

			/**
			 * @brief Declares queues for a device with a single queue family.
			 * @param requirements A reference to a device requirement.
			 * @param queueFamilyProperty A reference to the queue family properties.
			 * @return bool
			 */
			[[nodiscard]]
			bool declareQueuesFromSingleQueueFamily (const DeviceRequirements & requirements, const VkQueueFamilyProperties2 & queueFamilyProperty) noexcept;

			/**
			 * @brief Declares queues for a device with multiple queue family.
			 * @param requirements A reference to a device requirement.
			 * @param queueFamilyProperties A reference to a vector of queue family properties.
			 * @return bool
			 */
			[[nodiscard]]
			bool declareQueuesFromMultipleQueueFamilies (const DeviceRequirements & requirements, const Libs::StaticVector< VkQueueFamilyProperties2, 8 > & queueFamilyProperties) noexcept;

			/**
			 * @brief Creates the device with the defined and verified queues.
			 * @param requirements A reference to a device requirement.
			 * @param queueCreateInfos A reference to a list of CreateInfo for Vulkan queues.
			 * @param extensions A reference to a list of extensions to enable with the device.
			 * @return bool
			 */
			[[nodiscard]]
			bool createDevice (const DeviceRequirements & requirements, const Libs::StaticVector< VkDeviceQueueCreateInfo, 16 > & queueCreateInfos, const std::vector< const char * > & extensions) noexcept;

			VkDevice m_handle{VK_NULL_HANDLE};
			std::shared_ptr< PhysicalDevice > m_physicalDevice;
			std::array< std::shared_ptr< QueueFamilyInterface >, 6 > m_queues;

			Libs::StaticVector< std::shared_ptr< QueueFamilyInterface >, 8 > m_queueFamilies;
			std::map< QueueJob, std::shared_ptr< QueueFamilyInterface > > m_queueFamilyPerJob;

			mutable std::mutex m_deviceInternalAccess;
			mutable std::mutex m_deviceExternalAccess;
			bool m_showInformation{false};
			bool m_hasBasicSupport{false};
	};
}
