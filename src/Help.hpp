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
#include <sstream>

/* Local inclusions for inheritances. */
#include "Libs/NameableTrait.hpp"

/* Local inclusions for usages. */
#include "Input/Types.hpp"

namespace EmEn
{
	/**
	 * @brief Base class for the help service documentation.
	 */
	class AbstractDoc
	{
		public:

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			AbstractDoc (const AbstractDoc & copy) noexcept = default;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			AbstractDoc (AbstractDoc && copy) noexcept = default;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 * @return AbstractDoc &
			 */
			AbstractDoc & operator= (const AbstractDoc & copy) noexcept = default;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 * @return AbstractDoc &
			 */
			AbstractDoc & operator= (AbstractDoc && copy) noexcept = default;

			/**
			 * @brief The virtual destructor.
			 */
			virtual ~AbstractDoc () = default;

			/**
			 * @brief Returns the description.
			 * @return const std::string &
			 */
			[[nodiscard]]
			const std::string &
			description () const noexcept
			{
				return m_description;
			}

		protected:

			/**
			 * @brief Constructs base documentation.
			 * @param description A reference to a string [std::move].
			 */
			explicit
			AbstractDoc (std::string description) noexcept
				: m_description{std::move(description)}
			{

			}

		private:

			std::string m_description;
	};

	/**
	 * @brief Class for argument documentation.
	 * @extends EmEn::AbstractDoc The base documentation class.
	 */
	class ArgumentDoc final : public AbstractDoc
	{
		public:

			/**
			 * @brief Constructs an argumentation documentation.
			 * @param description A reference to a string [std::move].
			 * @param longName A reference to as string for the long name [std::move].
			 * @param shortName A char for the short name. Default none.
			 * @param options A reference to a string vector as options for the argument. Default none.
			 */
			ArgumentDoc (std::string description, std::string longName, char shortName = 0, const std::vector< std::string > & options = {}) noexcept
				: AbstractDoc{std::move(description)},
				m_longName{std::move(longName)},
				m_shortName{shortName},
				m_options{options}
			{

			}

			/**
			 * @brief Returns the argument long name.
			 * @return const std::string &
			 */
			[[nodiscard]]
			const std::string &
			longName () const noexcept
			{
				return m_longName;
			}

			/**
			 * @brief Returns the argument short name.
			 * @return char
			 */
			[[nodiscard]]
			char
			shortName () const noexcept
			{
				return m_shortName;
			}

			/**
			 * @brief Returns the list of options for the arguments. (optional)
			 * @return const std::vector< std::string > &
			 */
			[[nodiscard]]
			const std::vector< std::string > &
			options () const noexcept
			{
				return m_options;
			}

		private:

			/**
			 * @brief STL streams printable object.
			 * @param out A reference to the stream output.
			 * @param obj A reference to the object to print.
			 * @return std::ostream &
			 */
			friend std::ostream & operator<< (std::ostream & out, const ArgumentDoc & obj);

			std::string m_longName;
			char m_shortName;
			std::vector< std::string > m_options;
	};

	/**
 	 * @brief Class for shortcut documentation.
	 * @extends EmEn::AbstractDoc The base documentation class.
	 */
	class ShortcutDoc final : public AbstractDoc
	{
		public:

			/**
			 * @brief Constructs shortcut documentation.
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

	/**
	 * @brief This class holds help for an application.
	 */
	class Help final : public Libs::NameableTrait
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"Help"};

			/**
			 * @brief Constructs a help service.
			 * @param name A reference to a string [std::move].
			 */
			explicit
			Help (std::string name) noexcept
				: NameableTrait{std::move(name)}
			{

			}

			/**
			 * @brief Adds a new argument to the help.
			 * @param argumentDoc A reference to an ArgumentDoc instance.
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

	inline
	std::ostream &
	operator<< (std::ostream & out, const ArgumentDoc & obj)
	{
		const auto shortPresent = obj.shortName() != 0;
		const auto longPresent = !obj.longName().empty();

		if ( shortPresent )
		{
			out << '-' << obj.shortName();
		}
		else
		{
			out << '\t';
		}

		if ( longPresent )
		{
			if ( shortPresent )
			{
				out << ", ";
			}

			out << "--" << obj.longName();
		}

		if ( !obj.options().empty() )
		{
			for ( const auto &option: obj.options() )
			{
				out << " [" << option << "]";
			}
		}

		return out << " : " << obj.description();
	}

	/**
	 * @brief Stringifies the object.
	 * @param obj A reference to the object to print.
	 * @return std::string
	 */
	inline
	std::string
	to_string (const ArgumentDoc & obj) noexcept
	{
		std::stringstream output;

		output << obj;

		return output.str();
	}

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
