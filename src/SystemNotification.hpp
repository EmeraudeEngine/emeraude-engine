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
 * https://github.com/EmeraudeEngine/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#pragma once

/* Project configuration. */
#include "emeraude_export.hpp"

/* STL inclusions. */
#include <cstdint>
#include <optional>
#include <string>

/* Local inclusions for inheritances. */
#include "ServiceInterface.hpp"

/* Local inclusions for usages. */
#include "PlatformSpecific/Desktop/Notification.hpp"

namespace EmEn
{
	class Window;
}

namespace EmEn
{
	/** @brief Alias for notification icon from PlatformSpecific::Desktop. */
	using NotificationIcon = PlatformSpecific::Desktop::NotificationIcon;

	/** @brief The OS-level notification authorization status. */
	enum class NotificationPermission : uint8_t
	{
		NotDetermined,
		Granted,
		Denied
	};

	/**
	 * @brief The system notification service.
	 * @note This service provides cross-platform OS-level notifications (system tray notifications).
	 * @note Uses portable-file-dialogs library for cross-platform support.
	 * @note Permission is asked to the operating system, never persisted.
	 * @extends EmEn::ServiceInterface This is a service.
	 */
	class EMEN_LEAN_API SystemNotification final : public ServiceInterface
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"SystemNotificationService"};

			/**
			 * @brief Constructs the system notification service.
			 * @param window A reference to the parent window.
			 */
			explicit
			SystemNotification (Window & window) noexcept
				: ServiceInterface{ClassId},
				m_window{window}
			{

			}

			/**
			 * @brief Shows a system notification.
			 * @note Returns false unless the OS already authorized notifications; never asks for it.
			 * @param title The notification title.
			 * @param message The notification message body.
			 * @param icon The notification icon type. Default none (no icon).
			 * @return bool True if the notification was shown successfully.
			 */
			[[nodiscard]]
			bool show (const std::string & title, const std::string & message, std::optional< NotificationIcon > icon = std::nullopt) const noexcept;

			/**
			 * @brief Asks the operating system for the current notification authorization.
			 * @return NotificationPermission
			 */
			[[nodiscard]]
			NotificationPermission permissionStatus () const noexcept;

			/**
			 * @brief Asks the operating system to authorize notifications for this application.
			 * @return NotificationPermission The authorization resulting from the request.
			 */
			[[nodiscard]]
			NotificationPermission requestPermission () const noexcept;

		private:

			/** @copydoc EmEn::ServiceInterface::onInitialize() */
			bool onInitialize () noexcept override;

			/** @copydoc EmEn::ServiceInterface::onTerminate() */
			bool onTerminate () noexcept override;

			Window & m_window; ///< Reference to the parent window.
	};
}
