/*
 * src/ShortcutDoc.hpp
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

#pragma once

/* STL inclusions. */
#include <sstream>
#include <string>

/* Local inclusions for inheritances. */
#include "AbstractDoc.hpp"

/* Local inclusions for usages. */
#include "Input/Types.hpp"

namespace EmEn
{
	/**
 	 * @brief Class for shortcut documentation.
	 * @extends EmEn::AbstractDoc The base documentation class.
	 */
	class ShortcutDoc final : public AbstractDoc
	{
		public:

			/**
			 * @brief Constructs a shortcut documentation.
			 * @param description A reference to a string.
			 * @param key The main key for the shortcut.
			 * @param modifiers The additional modifiers. Default none.
			 */
			ShortcutDoc (const std::string & description, Input::Key key, int modifiers = 0) noexcept
				: AbstractDoc{description},
				m_key{key},
				m_modifiers{modifiers}
			{

			}

			/**
			 * @brief Returns the main key of the shortcut.
			 * @return EmEn::Input::Key
			 */
			[[nodiscard]]
			Input::Key
			key () const noexcept
			{
				return m_key;
			}

			/**
			 * @brief Returns the mask of modifier for shortcut.
			 * @note 0 means no modifier needed.
			 * @return int
			 */
			[[nodiscard]]
			int
			modifiers () const noexcept
			{
				return m_modifiers;
			}

		private:

			/**
			 * @brief STL streams printable object.
			 * @param out A reference to the stream output.
			 * @param obj A reference to the object to print.
			 * @return std::ostream &
			 */
			friend std::ostream & operator<< (std::ostream & out, const ShortcutDoc & obj);

			Input::Key m_key;
			int m_modifiers;
	};

	inline
	std::ostream &
	operator<< (std::ostream & out, const ShortcutDoc & obj)
	{
		if ( Input::isKeyboardModifierPressed(Input::ModKeyShift, obj.modifiers()) )
		{
			out << "SHIFT + ";
		}

		if ( Input::isKeyboardModifierPressed(Input::ModKeyControl, obj.modifiers()) )
		{
			out << "CTRL + ";
		}

		if ( Input::isKeyboardModifierPressed(Input::ModKeyAlt, obj.modifiers()) )
		{
			out << "ALT + ";
		}

		if ( Input::isKeyboardModifierPressed(Input::ModKeySuper, obj.modifiers()) )
		{
			out << "SUPER + ";
		}

		return out << to_cstring(obj.key()) << " : " << obj.description();
	}

	/**
	 * @brief Stringifies the object.
	 * @param obj A reference to the object to print.
	 * @return std::string
	 */
	inline
	std::string
	to_string (const ShortcutDoc & obj) noexcept
	{
		std::stringstream output;

		output << obj;

		return output.str();
	}
}
