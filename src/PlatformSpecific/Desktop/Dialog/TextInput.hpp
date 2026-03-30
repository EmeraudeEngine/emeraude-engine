/*
 * src/PlatformSpecific/Desktop/Dialog/TextInput.hpp
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
#include <string>

/* Local inclusions for inheritances. */
#include "Abstract.hpp"

namespace EmEn::PlatformSpecific::Desktop::Dialog
{
	/**
	 * @brief The text input mode enumeration.
	 */
	enum class InputMode : uint8_t
	{
		SingleLine,
		MultiLine,
		Password
	};

	/**
	 * @brief The user dialog text input class.
	 * @extends EmEn::PlatformSpecific::Desktop::Dialog::Abstract This is a user dialog box.
	 */
	class TextInput final : public Abstract
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"TextInput"};

			/**
			 * @brief Constructs a text input dialog.
			 * @param title A reference to a string for the dialog title.
			 * @param message A string for the prompt message to display [std::move].
			 * @param inputMode The input mode. Default "SingleLine".
			 * @param defaultText A string for the default text value [std::move].
			 */
			TextInput (const std::string & title, std::string message, InputMode inputMode = InputMode::SingleLine, std::string defaultText = {}) noexcept
				: Abstract{title},
				m_message{std::move(message)},
				m_defaultText{std::move(defaultText)},
				m_inputMode{inputMode}
			{

			}

			/** @copydoc EmEn::PlatformSpecific::Desktop::Dialog::Abstract::execute() */
			bool execute (Window * window) noexcept override;

			/**
			 * @brief Returns whether the user has canceled the dialog.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			hasBeenCanceled () const noexcept
			{
				return m_canceled;
			}

			/**
			 * @brief Returns the text entered by the user.
			 * @return const std::string &
			 */
			[[nodiscard]]
			const std::string &
			text () const noexcept
			{
				return m_text;
			}

		private:

			std::string m_message;
			std::string m_defaultText;
			std::string m_text;
			InputMode m_inputMode;
			bool m_canceled{false};
	};
}
