/*
 * src/Vulkan/QueueFamilySQ.cpp
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

#include "QueueFamilySQ.hpp"

/* Local inclusions. */
#include "Device.hpp"

namespace EmEn::Vulkan
{
	VkDeviceQueueCreateInfo
	QueueFamilySQ::getCreateInfo (VkDeviceQueueCreateFlags createFlag) const noexcept
	{
		VkDeviceQueueCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		createInfo.pNext = nullptr;
		createInfo.flags = createFlag;
		createInfo.queueFamilyIndex = m_queueFamilyIndex;
		createInfo.queueCount = 1;
		createInfo.pQueuePriorities = &m_queuePriority;

		return createInfo;
	}

	bool
	QueueFamilySQ::retrieveQueuesFromDevice (const std::shared_ptr< Device > & device) noexcept
	{
		VkQueue queueHandle{};

		vkGetDeviceQueue(device->handle(), m_queueFamilyIndex, 0, &queueHandle);

		if ( queueHandle == VK_NULL_HANDLE )
		{
			TraceError{ClassId} << "Unable to retrieve the queue (family #" << m_queueFamilyIndex << ") from the device  !";

			return false;
		}

		m_queue = std::make_unique< Queue >(queueHandle, m_queueFamilyIndex);
		m_queue->setIdentifier((std::stringstream{} << "Device-" << m_queueFamilyIndex << ".0-Queue").str());

		return true;
	}
}
