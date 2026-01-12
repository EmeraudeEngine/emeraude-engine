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
 * https://github.com/londnoir/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#include "SystemNotification.hpp"

/* Local inclusions. */
#include "PlatformSpecific/Desktop/Dialog/Message.hpp"
#include "PlatformSpecific/Desktop/Notification.hpp"
#include "Settings.hpp"
#include "SettingKeys.hpp"
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

	bool
	SystemNotification::requestPermission () const noexcept
	{
		using namespace PlatformSpecific::Desktop;

		if ( !this->usable() )
		{
			Tracer::warning(ClassId, "Cannot request permission: service not initialized.");

			return false;
		}

		/* Read current permission. */
		const auto permission = m_settings.get< std::string >(CorePermissionsNotificationsKey, DefaultCorePermissionsNotifications);

		/* Already granted. */
		if ( permission == "allow" )
		{
			return true;
		}

		/* Already denied. */
		if ( permission == "deny" )
		{
			return false;
		}

		/* Permission is "ask" - show dialog. */
		Tracer::info(ClassId, "Showing notification permission dialog...");

		Dialog::Message dialog{
			"Notification Permission",
			"This application wants to show desktop notifications.\n\nDo you want to allow notifications?",
			Dialog::ButtonLayout::YesNo,
			Dialog::MessageType::Question
		};

		dialog.execute(nullptr);

		if ( dialog.getUserAnswer() == Dialog::Answer::Yes )
		{
			m_settings.set< std::string >(CorePermissionsNotificationsKey, "allow");

			Tracer::info(ClassId, "User granted notification permission.");

			return true;
		}

		m_settings.set< std::string >(CorePermissionsNotificationsKey, "deny");

		Tracer::info(ClassId, "User denied notification permission.");

		return false;
	}

	bool
	SystemNotification::show (const std::string & title, const std::string & message, std::optional< NotificationIcon > icon) const noexcept
	{
		if ( !this->usable() )
		{
			Tracer::warning(ClassId, "Cannot show notification: service not initialized.");

			return false;
		}

		/* Validate input parameters. */
		if ( title.empty() )
		{
			Tracer::warning(ClassId, "Cannot show notification: title is empty.");

			return false;
		}

		/* Check/request permission. */
		const auto permission = m_settings.get< std::string >(CorePermissionsNotificationsKey, DefaultCorePermissionsNotifications);

		if ( permission == "deny" )
		{
			Tracer::info(ClassId, "Notification blocked: permission denied.");

			return false;
		}

		if ( permission == "ask" )
		{
			/* Request permission first. */
			if ( !this->requestPermission() )
			{
				/* User denied permission. */
				return false;
			}
		}

		/* Permission is "allow" - show notification. */
		PlatformSpecific::Desktop::Notification notification{&m_window, title, message, icon};

		return notification.show();
	}
}
