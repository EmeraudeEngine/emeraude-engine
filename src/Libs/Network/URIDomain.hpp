/*
 * src/Libs/Network/URIDomain.hpp
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
#include <cstdint>
#include <array>
#include <map>
#include <sstream>
#include <string>

/* Local inclusions for usages. */
#include "Hostname.hpp"

namespace EmEn::Libs::Network
{
	/**
	 * @brief The URI domain class.
	 * @note [//[user-info@]host[:port]]
	 */
	class URIDomain final
	{
		public:

			static constexpr std::array< const char *, 4 > Bases{
				"//",
				"./",
				"../",
				"/"
			};

			/**
			 * @brief Constructs a default URI domain.
			 */
			URIDomain () noexcept = default;

			/**
			 * @brief Constructs a URI domain.
			 * @param hostname The host part of the URI domain [std::move].
			 * @param port The port used by the URI domain. Default 0.
			 * @param base The base [std::move]. Default "//".
			 * @param username The username used by the URI domain [std::move]. Default none.
			 * @param password The password used by the URI domain [std::move]. Default none.
			 * @param options The user login options used by the URI domain. Default none.
			 */
			explicit
			URIDomain (Hostname hostname, uint32_t port = 0, std::string base = "//", std::string username = {}, std::string password = {}, const std::map< std::string, std::string > & options = {}) noexcept
				: m_base{std::move(base)},
				m_username{std::move(username)},
				m_password{std::move(password)},
				m_options{options},
				m_hostname{std::move(hostname)},
				m_port{port}
			{

			}

			/**
			 * @brief Constructs a URI domain from a raw string.
			 * @param rawString A reference to a string.
			 */
			explicit URIDomain (std::string rawString) noexcept;

			/**
			 * @brief Sets the URI domain base.
			 * @param base A reference to a string.
			 * @return void
			 */
			void
			setBase (const std::string & base) noexcept
			{
				m_base = base;
			}

			/**
			 * @brief Sets the URI domain user credentials.
			 * @param username A reference to a string.
			 * @param password A reference to a string.
			 * @return void
			 */
			void
			setUserinfo (const std::string & username, const std::string & password) noexcept
			{
				m_username = username;

				if ( !password.empty() )
				{
					this->setPassword(password);
				}
			}

			/**
			 * @brief Sets the URI domain username.
			 * @note Separate method from URIDomain::setUserinfo().
			 * @param username A reference to a string.
			 * @return void
			 */
			void
			setUsername (const std::string & username) noexcept
			{
				m_username = username;
			}

			/**
			 * @brief Sets the URI domain user password.
			 * @note Separate method from URIDomain::setUserinfo().
			 * @param password A reference to a string.
			 * @return void
			 */
			void
			setPassword (const std::string & password) noexcept
			{
				m_password = password;
			}

			/**
			 * @brief Replaces the options of the URI domain.
			 * @param options A reference to a map.
			 * @return void
			 */
			void
			setOptions (const std::map< std::string, std::string > & options) noexcept
			{
				m_options = options;
			}

			/**
			 * @brief Adds an option to the URI domain.
			 * @param name A reference to a string.
			 * @param value A reference to a string.
			 * @return void
			 */
			void
			addOption (const std::string & name, const std::string & value) noexcept
			{
				m_options.emplace(name, value);
			}

			/**
			 * @brief Sets the hostname of the URI domain.
			 * @param hostname A reference to a Hostname.
			 * @return void
			 */
			void
			setHostname (const Hostname & hostname) noexcept
			{
				m_hostname = hostname;
			}

			/**
			 * @brief Sets the port to the URI domain.
			 * @param port The port number as an unsigned 32-bit integer.
			 * @return void
			 */
			void
			setPort (uint32_t port) noexcept
			{
				m_port = port;
			}

			/**
			 * @brief Returns the user information part.
			 * @return std::string
			 */
			[[nodiscard]]
			std::string userinfo () const noexcept;

			/**
			 * @brief Returns the username.
			 * @return const std::string &
			 */
			[[nodiscard]]
			const std::string &
			username () const noexcept
			{
				return m_username;
			}

			/**
			 * @brief Returns the password.
			 * @return const std::string &
			 */
			[[nodiscard]]
			const std::string &
			password () const noexcept
			{
				return m_password;
			}

			/**
			 * @brief Returns a list of URI domain options.
			 * @return const std::map< std::string, std::string > &
			 */
			[[nodiscard]]
			const std::map< std::string, std::string > &
			options () const noexcept
			{
				return m_options;
			}

			/**
			 * @brief Returns the URI domain hostname.
			 * @return const Hostname &
			 */
			[[nodiscard]]
			const Hostname &
			hostname () const noexcept
			{
				return m_hostname;
			}

			/**
			 * @brief Returns the URI domain host.
			 * @return std::string
			 */
			[[nodiscard]]
			std::string host () const noexcept;

			/**
			 * @brief Returns the URI domain port.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			port () const noexcept
			{
				return m_port;
			}

			/**
			 * @brief Returns the URI domain base.
			 * @return std::string
			 */
			[[nodiscard]]
			std::string
			base () const noexcept
			{
				return m_base;
			}

			/**
			 * @brief Returns whether the host is empty.
			 * @return bool
			 */
			[[nodiscard]]
			inline
			bool
			empty () const noexcept
			{
				return m_hostname.empty();
			}

		private:

			/**
			 * @brief Extracts the port from a raw string.
			 * @param string A reference to a string.
			 * @return std::string
			 */
			[[nodiscard]]
			std::string extractPort (const std::string & string) noexcept;

			/**
			 * @brief Extracts the base from a raw string.
			 * @param string A reference to a string.
			 * @return std::string
			 */
			[[nodiscard]]
			std::string extractBase (const std::string & string) noexcept;

			/**
			 * @brief Extracts the user information from a raw string.
			 * @param string A reference to a string.
			 * @return std::string
			 */
			[[nodiscard]]
			std::string extractUserInfos (const std::string & string) noexcept;

			/**
			 * @brief Parses the user information.
			 * @param string A reference to a string.
			 * @return void
			 */
			void parseUserInfos (const std::string & string) noexcept;

			/**
			 * @brief STL streams printable object.
			 * @param out A reference to the stream output.
			 * @param obj A reference to the object to print.
			 * @return std::ostream &
			 */
			friend std::ostream & operator<< (std::ostream & out, const URIDomain & obj);

			std::string m_base;
			std::string m_username;
			std::string m_password;
			std::map< std::string, std::string > m_options;
			Hostname m_hostname;
			uint32_t m_port{0};
	};

	/**
	 * @brief Stringifies the object.
	 * @param obj A reference to the object to print.
	 * @return std::string
	 */
	inline
	std::string
	to_string (const URIDomain & obj) noexcept
	{
		std::stringstream output;

		output << obj;

		return output.str();
	}
}
