/*
 * src/Vulkan/QueueFamily.hpp
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
#include <utility>
#include <memory>

/* Local inclusions for inheritances. */
#include "QueueFamilyInterface.hpp"

/* Local inclusions for usages. */
#include "Libs/StaticVector.hpp"
#include "Queue.hpp"

namespace EmEn::Vulkan
{
	/**
	 * @brief Describe a queue family for a device using multiple queues.
	 * @extends EmEn::Vulkan::QueueFamilyInterface The base class of the queue family.
	 */
	class QueueFamily final : public QueueFamilyInterface
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"VulkanQueueFamily"};

			/**
			 * @brief Constructs a queue family structure.
			 * @param index The index inside the physical device.
			 * @param maxQueueCount The max number of queues for this family available with the physical device.
			 */
			QueueFamily (uint32_t index, size_t maxQueueCount) noexcept
				: m_queueFamilyIndex{index},
				m_maxQueueCount{maxQueueCount}
			{

			}

			/** @copydoc EmEn::Vulkan::QueueFamilyInterface::index() */
			[[nodiscard]]
			uint32_t
			index () const noexcept override
			{
				return m_queueFamilyIndex;
			}

			/** @copydoc EmEn::Vulkan::QueueFamilyInterface::maxQueueCount() */
			[[nodiscard]]
			size_t
			maxQueueCount () const noexcept override
			{
				return m_maxQueueCount;
			}

			/** @copydoc EmEn::Vulkan::QueueFamilyInterface::hasSingleQueue() */
			[[nodiscard]]
			bool
			hasSingleQueue () const noexcept override
			{
				return false;
			}

			/** @copydoc EmEn::Vulkan::QueueFamilyInterface::declareQueueStructure() */
			[[nodiscard]]
			bool declareQueueStructure (const Libs::StaticVector< std::pair< QueueJob, float >, 16 > & structure) noexcept override;

			/** @copydoc EmEn::Vulkan::QueueFamilyInterface::getCreateInfo() */
			[[nodiscard]]
			VkDeviceQueueCreateInfo getCreateInfo (VkDeviceQueueCreateFlags createFlag) const noexcept override;

			/** @copydoc EmEn::Vulkan::QueueFamilyInterface::retrieveQueuesFromDevice() */
			[[nodiscard]]
			bool retrieveQueuesFromDevice (const std::shared_ptr< Device > & device) noexcept override;

			/** @copydoc EmEn::Vulkan::QueueFamilyInterface::queue() */
			[[nodiscard]]
			Queue *
			queue (QueueJob /*job*/, QueuePriority /*priority*/) noexcept override
			{
				// FIXME : Use parameters to get the right queue !
				return m_queues[0].get();
			}

		private:

			uint32_t m_queueFamilyIndex;
			size_t m_maxQueueCount;
			Libs::StaticVector< QueueJob, 16 > m_queueJobs;
			Libs::StaticVector< float, 16 > m_queuePriorities;
			Libs::StaticVector< std::unique_ptr< Queue >, 16 > m_queues;
	};
}
