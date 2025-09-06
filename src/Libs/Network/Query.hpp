/*
 * src/Libs/Network/Query.hpp
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
#include <map>
#include <string>
#include <sstream>

namespace EmEn::Libs::Network
{
	/**
	 * @brief The URL query class.
	 */
	class Query final
	{
		public:

			/**
			 * @brief Constructs a default query.
			 */
			Query () noexcept = default;

			/**
			 * @brief Constructs a query.
			 * @warning There is no check with this version.
			 * @param variables Query variables.
			 */
			explicit
			Query (const std::map< std::string, std::string > & variables) noexcept
				: m_variables{variables}
			{

			}

			/**
			 * @brief Replaces the query variables.
			 * @param variables A reference to a map of variables.
			 * @return void
			 */
			void
			setVariables (const std::map< std::string, std::string > & variables) noexcept
			{
				m_variables = variables;
			}

			/**
			 * @brief Adds a variable to the existing ones.
			 * @param key A reference to a string for the variable name.
			 * @param value A reference to a string for the variable value.
			 * @return void
			 */
			void
			addVariable (const std::string & key, const std::string & value) noexcept
			{
				m_variables.emplace(key, value);
			}

			/**
			 * @brief Returns the list of query variables.
			 * @return const std::map< std::string, std::string > &
			 */
			[[nodiscard]]
			const std::map< std::string, std::string > &
			variables () const noexcept
			{
				return m_variables;
			}

			/**
			 * @brief Returns whether the query variables are empty.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			empty () const noexcept
			{
				return m_variables.empty();
			}

			/**
			 * @brief Constructs a query from a string.
			 * @param string A reference to string.
			 * @return Query
			 */
			[[nodiscard]]
			static Query fromString (const std::string & string) noexcept;

		private:

			/**
			 * @brief STL streams printable object.
			 * @param out A reference to the stream output.
			 * @param obj A reference to the object to print.
			 * @return std::ostream &
			 */
			friend std::ostream & operator<< (std::ostream & out, const Query & obj);

			std::map< std::string, std::string > m_variables;
	};

	/**
	 * @brief Stringifies the object.
	 * @param obj A reference to the object to print.
	 * @return std::string
	 */
	inline
	std::string
	to_string (const Query & obj) noexcept
	{
		std::stringstream output;

		output << obj;

		return output.str();
	}
}
