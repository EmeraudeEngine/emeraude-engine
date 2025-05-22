/*
 * src/Vulkan/Sync/Event.cpp
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

#include "Event.hpp"

/* Local inclusions. */
#include "Vulkan/Device.hpp"
#include "Vulkan/Utility.hpp"
#include "Tracer.hpp"

namespace EmEn::Vulkan::Sync
{
	const char *
	to_cstring (EventStatus value) noexcept
	{
		switch ( value )
		{
			case EventStatus::Set :
				return SetString;

			case EventStatus::Reset :
				return ResetString;

			case EventStatus::Error :
				return ErrorString;

			default:
				return "Unknown";
		}
	}

	bool
	Event::createOnHardware () noexcept
	{
		if ( !this->hasDevice() )
		{
			Tracer::error(ClassId, "No device to create this event !");

			return false;
		}

		const auto result = vkCreateEvent(this->device()->handle(), &m_createInfo, nullptr, &m_handle);

		if ( result != VK_SUCCESS )
		{
			TraceError{ClassId} << "Unable to create an event : " << vkResultToCString(result) << " !";

			return false;
		}

		this->setCreated();

		return true;
	}

	bool
	Event::destroyFromHardware () noexcept
	{
		if ( !this->hasDevice() )
		{
			TraceError{ClassId} << "No device to destroy the event " << m_handle << " (" << this->identifier() << ") !";

			return false;
		}

		if (  m_handle != VK_NULL_HANDLE )
		{
			this->device()->waitIdle("Destroying an event");

			vkDestroyEvent(this->device()->handle(), m_handle, nullptr);

			m_handle = VK_NULL_HANDLE;
		}

		this->setDestroyed();

		return true;
	}

	EventStatus
	Event::getStatus () const noexcept
	{
		if ( !this->hasDevice() )
		{
			Tracer::error(ClassId, "No device to get status from this fence !");

			return EventStatus::Error;
		}

		const auto result = vkGetEventStatus(this->device()->handle(), m_handle);

		switch ( result )
		{
			case VK_EVENT_SET :
				return EventStatus::Set;

			case VK_EVENT_RESET :
				return EventStatus::Reset;

			default :
				return EventStatus::Error;
		}
	}

	bool
	Event::set () const noexcept
	{
		if ( !this->hasDevice() )
		{
			Tracer::error(ClassId, "No device to set this event !");

			return false;
		}

		const auto result = vkSetEvent(this->device()->handle(), m_handle);

		if ( result != VK_SUCCESS )
		{
			TraceError{ClassId} << "Unable to set an event : " << vkResultToCString(result) << " !";

			return false;
		}

		return true;
	}

	bool
	Event::reset () const noexcept
	{
		if ( !this->hasDevice() )
		{
			Tracer::error(ClassId, "No device to reset this event !");

			return false;
		}

		const auto result = vkResetEvent(this->device()->handle(), m_handle);

		if ( result != VK_SUCCESS )
		{
			TraceError{ClassId} << "Unable to reset an event : " << vkResultToCString(result) << " !";

			return false;
		}

		return true;
	}
}
