/*
 * src/Vulkan/Sync/Semaphore.cpp
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

#include "Semaphore.hpp"

/* Local inclusions. */
#include "Vulkan/Device.hpp"
#include "Vulkan/Utility.hpp"
#include "Tracer.hpp"

namespace EmEn::Vulkan::Sync
{
	bool
	Semaphore::createOnHardware () noexcept
	{
		if ( !this->hasDevice() )
		{
			Tracer::error(ClassId, "No device to create this semaphore !");

			return false;
		}

		const auto result = vkCreateSemaphore(this->device()->handle(), &m_createInfo, nullptr, &m_handle);

		if ( result != VK_SUCCESS )
		{
			TraceError{ClassId} << "Unable to create a semaphore : " << vkResultToCString(result) << " !";

			return false;
		}

		this->setCreated();

		return true;
	}

	bool
	Semaphore::destroyFromHardware () noexcept
	{
		if ( !this->hasDevice() )
		{
			TraceError{ClassId} << "No device to destroy the semaphore " << m_handle << " (" << this->identifier() << ") !";

			return false;
		}

		if (  m_handle != VK_NULL_HANDLE )
		{
			this->device()->waitIdle("Destroying a semaphore");

			vkDestroySemaphore(this->device()->handle(), m_handle, nullptr);

			m_handle = VK_NULL_HANDLE;
		}

		this->setDestroyed();

		return true;
	}
}
