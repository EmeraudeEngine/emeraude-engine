/*
 * src/PlatformSpecific/Desktop/Notification.hpp
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
#include <cstdint>
#include <optional>
#include <string>

/* Forward declarations. */
namespace EmEn
{
	class Window;
}

namespace EmEn::PlatformSpecific::Desktop
{
	/**
	 * @brief The notification icon type enumeration.
	 */
	enum class NotificationIcon : uint8_t
	{
		Info,
		Warning,
		Error
	};

	/**
	 * @brief The desktop notification class.
	 * @note This class provides cross-platform OS-level notifications (system tray notifications).
	 */
	class Notification final
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"Notification"};

			/**
			 * @brief Constructs a notification.
			 * @param window A pointer to the parent window (optional, platform-specific usage).
			 * @param title A reference to a string for the notification title.
			 * @param message A string for the notification message [std::move].
			 * @param icon The notification icon type. Default none (no icon).
			 */
			Notification (Window * window, const std::string & title, std::string message, std::optional< NotificationIcon > icon = std::nullopt) noexcept
				: m_window{window},
				m_title{title},
				m_message{std::move(message)},
				m_icon{icon}
			{

			}

			/**
			 * @brief Shows the notification.
			 * @return bool True if the notification was shown successfully.
			 */
			bool show () noexcept;

			/**
			 * @brief Returns the notification title.
			 * @return const std::string &
			 */
			[[nodiscard]]
			const std::string &
			title () const noexcept
			{
				return m_title;
			}

			/**
			 * @brief Returns the notification message.
			 * @return const std::string &
			 */
			[[nodiscard]]
			const std::string &
			message () const noexcept
			{
				return m_message;
			}

			/**
			 * @brief Returns the notification icon type.
			 * @return std::optional< NotificationIcon >
			 */
			[[nodiscard]]
			std::optional< NotificationIcon >
			icon () const noexcept
			{
				return m_icon;
			}

		protected:

			Window * m_window{nullptr};
			std::string m_title;
			std::string m_message;
			std::optional< NotificationIcon > m_icon;
	};
}
