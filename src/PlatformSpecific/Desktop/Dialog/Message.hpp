/*
 * src/PlatformSpecific/Desktop/Dialog/Message.hpp
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

/* Local inclusions for inheritances. */
#include "Abstract.hpp"

/* Local inclusions for usages. */
#include "Types.hpp"

namespace EmEn::PlatformSpecific::Desktop::Dialog
{
	/**
	 * @brief The user dialog message class.
	 * @extends EmEn::PlatformSpecific::Desktop::Dialog::Abstract This is an user dialog box.
	 */
	class EMEN_LEAN_API Message final : public Abstract
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"Message"};

			/** 
			 * @brief Constructs a message dialog.
			 * @param title A reference to a string for the dialog title.
			 * @param message A string for the message content to display [std::move].
			 * @param buttonLayout The button layout type. Default "OK".
			 * @param messageType The type of message. Default "Info".
			 */
			Message (const std::string & title, std::string message, ButtonLayout buttonLayout = ButtonLayout::OK, MessageType messageType = MessageType::Info) noexcept
				: Abstract{title},
				m_message{std::move(message)},
				m_buttonLayout{buttonLayout},
				m_messageType{messageType}
			{

			}

			/** @copydoc EmEn::PlatformSpecific::Desktop::Dialog::Abstract::execute() */
			bool execute (Window & window, bool parentToWindow = true) noexcept override;

			/**
			 * @brief Returns the user answer.
			 * @return Answer
			 */
			[[nodiscard]]
			Answer
			getUserAnswer () const noexcept
			{
				return m_userAnswer;
			}

			/**
			 * @brief Sets the answer of the default button — the one Enter / Space / Return press (YesNo, OKCancel).
			 * @note Without it, the first button (Yes / OK) is the default. Every Windows box is brought to the
			 * foreground (topmost when not parented) whatever the default: an unseen box once caught a stray key press
			 * (app_system's crash report, 2026-10-07). Linux kdialog keeps its own default.
			 * @param answer Answer::No or Answer::Cancel for the second button; anything else keeps the first.
			 */
			void
			setDefaultAnswer (Answer answer) noexcept
			{
				m_defaultAnswer = answer;
			}

		private:

			std::string m_message;
			ButtonLayout m_buttonLayout;
			MessageType m_messageType;
			Answer m_userAnswer{Answer::None};
			Answer m_defaultAnswer{Answer::None};
	};
}
