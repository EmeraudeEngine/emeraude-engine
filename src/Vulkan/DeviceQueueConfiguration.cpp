/*
 * src/Vulkan/DeviceQueueConfiguration.cpp
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

#include "DeviceQueueConfiguration.hpp"

namespace EmEn::Vulkan
{
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

	void
	DeviceQueueConfiguration::registerQueue (Queue * queue, QueuePriority priority) const noexcept
	{
		switch ( priority )
		{
			case QueuePriority::High :
				m_queueByPriorities[0].second.emplace_back(queue);
				break;

			case QueuePriority::Medium :
				m_queueByPriorities[1].second.emplace_back(queue);
				break;

			case QueuePriority::Low :
				m_queueByPriorities[2].second.emplace_back(queue);
				break;
		}
	}

	const Base::StaticVector< Queue *, 16 > &
	DeviceQueueConfiguration::queues (QueuePriority priority) const noexcept
	{
		switch ( priority )
		{
			case QueuePriority::Low :
				return m_queueByPriorities[2].second;

			case QueuePriority::Medium :
				return m_queueByPriorities[1].second;

			case QueuePriority::High :
			default:
				return m_queueByPriorities[0].second;
		}
	}

	bool
	DeviceQueueConfiguration::enabled () const noexcept
	{
		return std::ranges::any_of(m_queueByPriorities, [] (const auto & queueList) {
			return !queueList.second.empty();
		});
	}

	void
	DeviceQueueConfiguration::clear () noexcept
	{
		m_queueFamilyIndex = 0;

		for ( auto & queueList : m_queueByPriorities | std::views::values )
		{
			queueList.clear();
		}
	}
}
