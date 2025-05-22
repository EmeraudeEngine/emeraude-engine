/*
 * src/Vulkan/QueueFamilySQ.hpp
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
#include <vector>
#include <memory>

/* Local inclusions for inheritances. */
#include "QueueFamilyInterface.hpp"

/* Local inclusions for usages. */
#include "Queue.hpp"
#include "Tracer.hpp"

namespace EmEn::Vulkan
{
	/**
	 * @brief Describe a queue family for a device using one queue.
	 * @extends EmEn::Vulkan::QueueFamilyInterface The base class of queue family.
	 */
	class QueueFamilySQ final : public QueueFamilyInterface
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"VulkanQueueFamilySQ"};

			/**
			 * @brief Constructs a queue family structure.
			 * @param index The index inside the physical device.
			 */
			explicit
			QueueFamilySQ (uint32_t index) noexcept
				: m_queueFamilyIndex{index}
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
				return 1;
			}

			/** @copydoc EmEn::Vulkan::QueueFamilyInterface::hasSingleQueue() */
			[[nodiscard]]
			bool
			hasSingleQueue () const noexcept override
			{
				return true;
			}

			/** @copydoc EmEn::Vulkan::QueueFamilyInterface::declareQueueStructure() */
			[[nodiscard]]
			bool
			declareQueueStructure (const std::vector< std::pair< QueueJob, float > > & /*structure*/) noexcept override
			{
				Tracer::error(ClassId, "This is a single queue family !");

				return false;
			}

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
				/* NOTE: There is no use for job or priority here. */
				return m_queue.get();
			}

		private:

			uint32_t m_queueFamilyIndex;
			float m_queuePriority{1.0F};
			std::unique_ptr< Queue > m_queue;
	};
}
