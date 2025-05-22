/*
 * src/Console/Expression.hpp
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
#include <vector>

/* Local inclusions. */
#include "Argument.hpp"

namespace EmEn::Console
{
	/**
	 * @brief The console expression class.
	 */
	class Expression final
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"Expression"};

			/**
			 * @brief Constructs a console expression.
			 * @param command A reference to a string for the command.
			 */
			explicit Expression (const std::string & command = {}) noexcept;

			/**
			 * @brief Returns whether the expression is valid.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isValid () const noexcept
			{
				/* NOTE: It should be at least one identifier. */
				if ( m_identifiers.empty() )
				{
					return false;
				}

				/* NOTE: If there is no function name found, the command is pointless. */
				if ( m_commandName.empty() )
				{
					return false;
				}

				return true;
			}

			/**
			 * @brief Returns whether the expression has a parameter.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			hasParameter () const noexcept
			{
				return !m_arguments.empty();
			}

			/**
			 * @brief Returns the next identifier.
			 * @return std::string
			 */
			[[nodiscard]]
			std::string nextIdentifier () noexcept;

			/**
			 * @brief Returns a reference to the list of identifiers.
			 * @return const std::vector< std::string > &
			 */
			[[nodiscard]]
			const std::vector< std::string > &
			identifiers () const noexcept
			{
				return m_identifiers;
			}

			/**
			 * @brief Returns the command name.
			 * @return const std::string &
			 */
			[[nodiscard]]
			const std::string &
			commandName () const noexcept
			{
				return m_commandName;
			}

			/**
			 * @brief Returns a reference to the list of arguments.
			 * @return const std::vector< Argument > &
			 */
			[[nodiscard]]
			const std::vector< Argument > &
			arguments () const noexcept
			{
				return m_arguments;
			}

			/**
			 * @brief STL streams printable object.
			 * @param out A reference to the stream output.
			 * @param obj A reference to the object to print.
			 * @return std::ostream &
			 */
			friend std::ostream & operator<< (std::ostream & out, const Expression & obj);

			/**
			 * @brief Stringifies the object.
			 * @param obj A reference to the object to print.
			 * @return std::string
			 */
			friend
			std::string
			to_string (const Expression & obj) noexcept
			{
				std::stringstream output;

				output << obj;

				return output.str();
			}

		private:

			/**
			 * @brief Extracts the command name.
			 * @param buffer A writable reference to a string.
			 * @param where A writable reference to a string.
			 * @return void
			 */
			static void extract (std::string & buffer, std::string & where) noexcept;

			/**
			 * @brief Extracts the command arguments.
			 * @param buffer A writable reference to a string.
			 * @param where A writable reference to a vector of string.
			 * @return void
			 */
			static void extract (std::string & buffer, std::vector< std::string > & where) noexcept;

			std::vector< std::string > m_identifiers;
			std::string m_commandName;
			Arguments m_arguments;
			size_t m_identifierLevel{0};
	};
}
