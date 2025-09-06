/*
 * src/Libs/Network/Hostname.hpp
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

namespace EmEn::Libs::Network
{
	/**
	 * @brief The Hostname class.
	 */
	class Hostname final
	{
		public:

			/**
			 * @brief Constructs a default hostname.
			 */
			Hostname () noexcept = default;

			/**
			 * @brief Constructs a hostname.
			 * @warning There is no check with this version.
			 * @param subdomain The subdomain part of the host [std::move].
			 * @param domain The domain part of the host [std::move].
			 */
			Hostname (std::string subdomain, std::string domain) noexcept
				: m_subdomain(std::move(subdomain)),
				m_domain(std::move(domain))
			{

			}

			/**
			 * @brief Sets the subdomain part of the host.
			 * @param name The subdomain name.
			 * @return void
			 */
			void
			setSubdomain (const std::string & name) noexcept
			{
				m_subdomain = name;
			}

			/**
			 * @brief Returns the subdomain part of the host. Example: [www].example.co.uk
			 * @return const std::string &
			 */
			[[nodiscard]]
			const std::string &
			subdomain () const noexcept
			{
				return m_subdomain;
			}

			/**
			 * @brief Sets the domain part of the host.
			 * @param name The domain name.
			 * @return void
			 */
			void
			setDomain (const std::string & name) noexcept
			{
				m_domain = name;
			}

			/**
			 * @brief Returns the domain part of the host. Example: www.[example.co.uk]
			 * @return const std::string &
			 */
			[[nodiscard]]
			const std::string &
			domain () const noexcept
			{
				return m_domain;
			}

			/**
			 * @brief Returns the name of the host. Example: [www.example.co.uk]
			 * @return std::string
			 */
			[[nodiscard]]
			std::string
			name () const noexcept
			{
				if ( m_subdomain.empty() )
				{
					return m_domain;
				}

				return m_subdomain + '.' + m_domain;
			}

			/**
			 * @brief Returns the TLD part of the host. Example www.example.[co.uk]
			 * @return std::string
			 */
			[[nodiscard]]
			std::string tld () const noexcept;

			/**
			 * @brief Returns whether the hostname is empty.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			empty () const noexcept
			{
				return m_domain.empty();
			}

			/**
			 * @brief Constructs a hostname from a string.
			 * @param string A reference to string.
			 * @return Hostname
			 */
			[[nodiscard]]
			static Hostname fromString (const std::string & string) noexcept;

		private:

			/**
			 * @brief STL streams printable object.
			 * @param out A reference to the stream output.
			 * @param obj A reference to the object to print.
			 * @return std::ostream &
			 */
			friend
			std::ostream &
			operator<< (std::ostream & out, const Hostname & obj)
			{
				return out << obj.name();
			}

			std::string m_subdomain;
			std::string m_domain;
	};

	/**
	 * @brief Stringifies the object.
	 * @param obj A reference to the object to print.
	 * @return std::string
	 */
	inline
	std::string
	to_string (const Hostname & obj) noexcept
	{
		std::stringstream output;

		output << obj;

		return output.str();
	}
}
