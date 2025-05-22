/*
 * src/Help.hpp
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
#include <string>
#include <vector>

/* Local inclusions for inheritances. */
#include "Libs/NameableTrait.hpp"

/* Local inclusions for usages. */
#include "ArgumentDoc.hpp"
#include "ShortcutDoc.hpp"

namespace EmEn
{
	/**
	 * @brief This class holds help for an application.
	 */
	class Help final : public Libs::NameableTrait
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"Help"};

			/**
			 * @brief Constructs the Help service.
			 * @param name A reference to a string [std::move].
			 */
			explicit
			Help (std::string name) noexcept
				: NameableTrait{std::move(name)}
			{

			}

			/**
			 * @brief Adds a new argument to the help.
			 * @param argumentDoc A reference to a ArgumentDoc instance.
			 * @return void
			 */
			void
			registerArgument (const ArgumentDoc & argumentDoc) noexcept
			{
				m_argumentDocs.emplace_back(argumentDoc);
			}

			/**
			 * @brief Adds a new argument to the help.
			 * @param description A reference to a string.
			 * @param longName A reference to as string for the long name.
			 * @param shortName A char for the short name. Default none.
			 * @param options A reference to a string vector as options for the argument. Default none.
			 * @return void
			 */
			void
			registerArgument (const std::string & description, const std::string & longName, char shortName = 0, const std::vector< std::string > & options = {}) noexcept
			{
				m_argumentDocs.emplace_back(description, longName, shortName, options);
			}

			/**
			 * @brief Adds a new shortcut to the help.
			 * @param shortcutDoc A reference to a ShortcutDoc instance.
			 * @return void
			 */
			void
			registerShortcut (const ShortcutDoc & shortcutDoc) noexcept
			{
				m_shortcutDocs.emplace_back(shortcutDoc);
			}

			/**
			 * @brief Adds a new shortcut to the help.
			 * @param description A reference to a string.
			 * @param key The main key for the shortcut.
			 * @param modifiers The additional modifiers. Default none.
			 * @return void
			 */
			void
			registerShortcut (const std::string & description, EmEn::Input::Key key, int modifiers = 0) noexcept
			{
				m_shortcutDocs.emplace_back(description, key, modifiers);
			}

			/**
			 * @brief Returns the argument documentation list.
			 * @return const std::vector< ArgumentDoc > &
			 */
			[[nodiscard]]
			const std::vector< ArgumentDoc > &
			argumentDocs () const noexcept
			{
				return m_argumentDocs;
			}

			/**
			 * @brief Returns the shortcut documentation list.
			 * @return const std::vector< ShortcutDoc > &
			 */
			[[nodiscard]]
			const std::vector< ShortcutDoc > &
			shortcutDocs () const noexcept
			{
				return m_shortcutDocs;
			}

			/**
			 * @brief Returns the argument documentation as a string.
			 * @return const std::vector< ArgumentDoc > &
			 */
			[[nodiscard]]
			std::string argumentDocsString () const noexcept;

			/**
			 * @brief Returns the shortcut documentation as a string.
			 * @return const std::vector< ShortcutDoc > &
			 */
			[[nodiscard]]
			std::string shortcutDocsString () const noexcept;

			/**
			 * @brief Returns the complete help in a string.
			 * @return std::string
			 */
			[[nodiscard]]
			std::string getHelp () const noexcept;

		private:

			/* Flag names */
			static constexpr auto ServiceInitialized{0UL};

			std::vector< ArgumentDoc > m_argumentDocs;
			std::vector< ShortcutDoc > m_shortcutDocs;
	};
}
