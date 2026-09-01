/*
 * src/SystemNotification.cpp
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

#include "SystemNotification.hpp"

/* Local inclusions. */
#include "PlatformSpecific/Desktop/Notification.hpp"
#include "Tracer.hpp"

namespace EmEn
{
	bool
	SystemNotification::onInitialize () noexcept
	{
		Tracer::info(ClassId, "System notification service initialized.");

		return true;
	}

	bool
	SystemNotification::onTerminate () noexcept
	{
		Tracer::info(ClassId, "System notification service terminated.");

		return true;
	}

	NotificationPermission
	SystemNotification::permissionStatus () const noexcept
	{
		if ( !this->usable() )
		{
			Tracer::warning(ClassId, "Cannot read the permission: service not initialized.");

			return NotificationPermission::Denied;
		}

		/* TODO: macOS, query UNUserNotificationCenter. No other OS gates notifications. */
		return NotificationPermission::Granted;
	}

	NotificationPermission
	SystemNotification::requestPermission () const noexcept
	{
		if ( !this->usable() )
		{
			Tracer::warning(ClassId, "Cannot request permission: service not initialized.");

			return NotificationPermission::Denied;
		}

		/* TODO: macOS, request through UNUserNotificationCenter. No other OS gates notifications. */
		/* NOTE: requestAuthorization() answers later, so this signature will need a callback. */
		return NotificationPermission::Granted;
	}

	bool
	SystemNotification::show (const std::string & title, const std::string & message, std::optional< NotificationIcon > icon) const noexcept
	{
		if ( !this->usable() )
		{
			Tracer::warning(ClassId, "Cannot show notification: service not initialized.");

			return false;
		}

		if ( title.empty() )
		{
			Tracer::warning(ClassId, "Cannot show notification: title is empty.");

			return false;
		}

		if ( this->permissionStatus() != NotificationPermission::Granted )
		{
			Tracer::info(ClassId, "Notification blocked: not authorized by the operating system.");

			return false;
		}

		PlatformSpecific::Desktop::Notification notification{&m_window, title, message, icon};
		return notification.show();
	}
}
