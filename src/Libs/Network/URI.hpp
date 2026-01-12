/*
 * src/Libs/Network/URI.hpp
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
#include <filesystem>
#include <string>
#include <iostream>

/* Local inclusions for usages. */
#include "Query.hpp"
#include "URIDomain.hpp"

namespace EmEn::Libs::Network
{
	/**
	 * @brief The URI class.
	 * @note [protocol:][//[user-info@]host[:port]][path]
	 */
	class URI
	{
		public:

			static constexpr auto HttpProtocol{"http:"};
			static constexpr auto SecureHttpProtocol{"https:"};

			/**
			 * @brief Constructs a default URI.
			 */
			URI () noexcept = default;

			/**
			 * @brief Constructs a URI from a raw string.
			 * @param rawURI A reference to a string.
			 */
			explicit
			URI (const std::string & rawURI) noexcept
			{
				if ( !this->parseRawString(rawURI) )
				{
					std::cerr << "URI::URI(), empty string !" "\n";
				}
			}

			/**
			 * @brief Assignment operator for std::string.
			 * @param rawURI A reference of a string for URI.
			 * @return URI &
			 */
			URI &
			operator= (const std::string & rawURI) noexcept
			{
				if ( !this->parseRawString(rawURI) )
				{
					std::cerr << "URI::operator=(), empty string !" "\n";
				}

				return *this;
			}

			/**
			 * @brief Sets the scheme of the URI.
			 * @param scheme A reference to a string.
			 * @return void
			 */
			void
			setScheme (const std::string & scheme) noexcept
			{
				/* TODO: Le nom du schéma est une lettre suivie de n'importe quelle
				 * combinaison de lettres, de chiffres, du signe plus (+), du point (.)
				 * ou d'un tiret (-) et se termine par deux points (:). */
				m_scheme = scheme;
			}

			/**
			 * @brief Sets the URI domain.
			 * @param uriDomain A reference to an URIDomain.
			 * @return void
			 */
			void
			setURIDomain (const URIDomain & uriDomain) noexcept
			{
				m_uriDomain = uriDomain;
			}

			/**
			 * @brief Sets the path of the URI.
			 * @param path A reference to a filesystem path.
			 * @return void
			 */
			void
			setPath (const std::filesystem::path & path) noexcept
			{
				m_path = path;
			}

			/**
			 * @brief Sets the query of the URI.
			 * @param query A reference to a query.
			 * @return void
			 */
			void
			setQuery (const Query & query) noexcept
			{
				m_query = query;
			}

			/**
			 * @brief Sets the fragment of the URI.
			 * @param fragment A reference to a string.
			 * @return void
			 */
			void
			setFragment (const std::string & fragment) noexcept
			{
				m_fragment = fragment;
			}

			/**
			 * @brief Returns the scheme part.
			 * @return const std::string &
			 */
			[[nodiscard]]
			const std::string &
			scheme () const noexcept
			{
				return m_scheme;
			}

			/**
			 * @brief Returns the domain part.
			 * @return const URIDomain &
			 */
			[[nodiscard]]
			const URIDomain &
			uriDomain () const noexcept
			{
				return m_uriDomain;
			}

			/**
			 * @brief Returns the path part.
			 * @return const std::filesystem::path &
			 */
			[[nodiscard]]
			const std::filesystem::path &
			path () const noexcept
			{
				return m_path;
			}

			/**
			 * @brief Returns the query part.
			 * @return const Query &
			 */
			[[nodiscard]]
			const Query &
			query () const noexcept
			{
				return m_query;
			}

			/**
			 * @brief Returns the fragment part.
			 * @return const std::string &
			 */
			[[nodiscard]]
			const std::string &
			fragment () const noexcept
			{
				return m_fragment;
			}

			/**
			 * @brief Returns the resource part.
			 * @return std::string
			 */
			[[nodiscard]]
			std::string resource () const noexcept;

			/**
			 * @brief Returns whether the URI is empty.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			empty () const noexcept
			{
				if ( m_scheme.empty() )
				{
					return true;
				}

				if ( m_uriDomain.empty() )
				{
					return true;
				}

				return false;
			}

		private:

			/**
			 * @brief checkSimplePath
			 * @param string
			 * @return bool
			 */
			[[nodiscard]]
			bool checkSimplePath (const std::string & string) noexcept;

			/**
			 * @brief extractScheme
			 * @param string
			 * @return std::string
			 */
			[[nodiscard]]
			std::string extractScheme (const std::string & string) noexcept;

			/**
			 * @brief extractFragment
			 * @param string
			 * @return std::string
			 */
			[[nodiscard]]
			std::string extractFragment (const std::string & string) noexcept;

			/**
			 * @brief extractQuery
			 * @param string
			 * @return std::string
			 */
			[[nodiscard]]
			std::string extractQuery (const std::string & string) noexcept;

			/**
			 * @brief extractURIDomain
			 * @param string
			 * @return std::string
			 */
			[[nodiscard]]
			std::string extractURIDomain (const std::string & string) noexcept;

			/**
			 * @brief parseRawString
			 * @param rawString
			 * @return bool
			 */
			bool parseRawString (std::string rawString) noexcept;

			/**
			 * @brief STL streams printable object.
			 * @param out A reference to the stream output.
			 * @param obj A reference to the object to print.
			 * @return std::ostream &
			 */
			friend std::ostream & operator<< (std::ostream & out, const URI & obj);

			std::string m_scheme;
			URIDomain m_uriDomain;
			std::filesystem::path m_path;
			Query m_query;
			std::string m_fragment;
	};

	/**
	 * @brief Stringifies the object.
	 * @param obj A reference to the object to print.
	 * @return std::string
	 */
	inline
	std::string
	to_string (const URI & obj) noexcept
	{
		std::stringstream output;

		output << obj;

		return output.str();
	}
}
