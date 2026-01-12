/*
 * src/SystemNotification.hpp
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

#pragma once

/* STL inclusions. */
#include <optional>
#include <string>

/* Local inclusions for inheritances. */
#include "ServiceInterface.hpp"

/* Local inclusions for usages. */
#include "PlatformSpecific/Desktop/Notification.hpp"

/* Forward declarations. */
namespace EmEn
{
	class Settings;
	class Window;
}

namespace EmEn
{
	/** @brief Alias for notification icon from PlatformSpecific::Desktop. */
	using NotificationIcon = PlatformSpecific::Desktop::NotificationIcon;

	/**
	 * @brief The system notification service.
	 * @note This service provides cross-platform OS-level notifications (system tray notifications).
	 * @note Uses portable-file-dialogs library for cross-platform support.
	 * @note Permission is managed via Settings with key "Notifications/Permission".
	 * @extends EmEn::ServiceInterface This is a service.
	 */
	class SystemNotification final : public ServiceInterface
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"SystemNotificationService"};

			/**
			 * @brief Constructs the system notification service.
			 * @param settings A reference to the application settings.
			 * @param window A reference to the parent window.
			 */
			explicit
			SystemNotification (Settings & settings, Window & window) noexcept
				: ServiceInterface{ClassId},
				m_settings{settings},
				m_window{window}
			{

			}

			/**
			 * @brief Shows a system notification.
			 * @note If permission is "ask", shows a permission dialog first.
			 * @note If permission is "deny", does nothing and returns false.
			 * @param title The notification title.
			 * @param message The notification message body.
			 * @param icon The notification icon type. Default none (no icon).
			 * @return bool True if the notification was shown successfully.
			 */
			[[nodiscard]]
			bool show (const std::string & title, const std::string & message, std::optional< NotificationIcon > icon = std::nullopt) const noexcept;

			/**
			 * @brief Requests notification permission from the user.
			 * @note If permission is already "allow" or "deny", does nothing.
			 * @note If permission is "ask", shows the permission dialog and updates settings.
			 * @return bool True if permission was granted (or already granted).
			 */
			[[nodiscard]]
			bool requestPermission () const noexcept;

		private:

			/** @copydoc EmEn::ServiceInterface::onInitialize() */
			bool onInitialize () noexcept override;

			/** @copydoc EmEn::ServiceInterface::onTerminate() */
			bool onTerminate () noexcept override;

			Settings & m_settings; ///< Reference to application settings.
			Window & m_window; ///< Reference to the parent window.
	};
}
