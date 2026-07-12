/*
 * src/Vulkan/DeviceQueueConfiguration.hpp
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

#pragma once

/* Project configuration. */
#include "emeraude_export.hpp"

/* STL inclusions. */
#include <cstdint>
#include <array>

/* Local inclusions for usages. */
#include "StaticVector.hpp"
#include "Queue.hpp"

namespace EmEn::Vulkan
{
	/** @brief Structure to sort queues by priority. */
	class EMEN_API DeviceQueueConfiguration final
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
			void registerQueue (Queue * queue, QueuePriority priority) const noexcept;

			/**
			 * @brief Returns queue priority structure.
			 * @return const Base::StaticVector< Queue *, 16 > &
			 */
			[[nodiscard]]
			const Base::StaticVector< Queue *, 16 > & queues (QueuePriority priority) const noexcept;

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
			bool enabled () const noexcept;

			/**
			 * @brief Clears data and links.
			 * @return void
			 */
			void clear () noexcept;

		private:

			uint32_t m_queueFamilyIndex{0};
			mutable std::array< std::pair< std::atomic< uint32_t >, Base::StaticVector< Queue *, 16 > >, 3 > m_queueByPriorities;
	};
}
