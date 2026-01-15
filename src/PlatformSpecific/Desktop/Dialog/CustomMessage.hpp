/*
 * src/PlatformSpecific/Desktop/Dialog/CustomMessage.hpp
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

/* Local inclusions for inheritances. */
#include "Abstract.hpp"

/* Local inclusions for usages. */
#include "Types.hpp"

namespace EmEn::PlatformSpecific::Desktop::Dialog
{
	/**
	 * @brief A dialog with custom button labels.
	 * @extends EmEn::PlatformSpecific::Desktop::Dialog::Abstract This is a user dialog box.
	 */
	class CustomMessage final : public Abstract
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"CustomMessage"};

			/**
			 * @brief Constructs a custom message dialog.
			 * @param title A reference to a string for the dialog title.
			 * @param message A string for the message content to display [std::move].
			 * @param buttons Button labels (1-6 buttons, first is primary/default) [std::move].
			 * @param messageType The type of message icon. Default "Question".
			 */
			CustomMessage (const std::string & title, std::string message, ButtonLabels buttons, MessageType messageType = MessageType::Question) noexcept
				: Abstract{title},
				m_message{std::move(message)},
				m_buttons{std::move(buttons)},
				m_messageType{messageType}
			{

			}

			/** @copydoc EmEn::PlatformSpecific::Desktop::Dialog::Abstract::execute() */
			bool execute (Window * window) noexcept override;

			/**
			 * @brief Returns the index of the clicked button.
			 * @return int Index (0-based), or -1 if dialog failed or was dismissed.
			 */
			[[nodiscard]]
			int
			getClickedButtonIndex () const noexcept
			{
				return m_clickedIndex;
			}

		private:

			std::string m_message;
			ButtonLabels m_buttons;
			MessageType m_messageType;
			int m_clickedIndex{-1};
	};
}
