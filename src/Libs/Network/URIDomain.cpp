/*
 * src/Libs/Network/URIDomain.cpp
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

#include "URIDomain.hpp"

/* Emeraude-Engine configuration. */
#include "emeraude_config.hpp"

/* STL inclusions. */
#include <array>
#include <iostream>
#include <sstream>

/* Local inclusions. */
#include "Libs/String.hpp"

namespace EmEn::Libs::Network
{
	URIDomain::URIDomain (Hostname hostname, uint32_t port, std::string base, std::string user, std::string password, const std::map< std::string, std::string > & options) noexcept
		: m_base(std::move(base)), m_username(std::move(user)), m_password(std::move(password)), m_options(options), m_hostname(std::move(hostname)), m_port(port)
	{

	}

	URIDomain::URIDomain (std::string rawString) noexcept
	{
		if ( rawString.empty() )
		{
			std::cerr << __PRETTY_FUNCTION__ << ", empty string !" "\n";

			return;
		}

		rawString = this->extractPort(rawString);

		if ( rawString.empty() )
		{
			return;
		}

		rawString = this->extractBase(rawString);

		if ( rawString.empty() )
		{
			return;
		}

		m_hostname = Hostname::fromString(this->extractUserInfos(rawString));
	}

	std::string
	URIDomain::extractPort (const std::string & string) noexcept
	{
		const auto chunks = String::explode(string, ':');

		switch ( chunks.size() )
		{
			/* Port information present with user information. */
			case 3 :
				this->setPort(String::toNumber< unsigned int >(chunks[2]));

				return chunks[0] + ':' + chunks[1];

			/* Port information present. */
			case 2 :
				this->setPort(String::toNumber< unsigned int >(chunks[1]));

				/* NOTE: The ':' char was separating the user information. */
				return m_port == 0 ? string : chunks[0];

			case 1 :
				return chunks[0];

			default:
				std::cerr << __PRETTY_FUNCTION__ << ", invalid URI domain, multiple ':' char found !" "\n";

				return string;
		}
	}

	std::string
	URIDomain::extractBase (const std::string & string) noexcept
	{
		for ( const auto & base : URIDomain::Bases )
		{
			if ( !string.starts_with(base) )
			{
				continue;
			}

			this->setBase(base);

			return String::replace(base, "", string);
		}

		m_base.clear();

		return string;
	}

	std::string
	URIDomain::extractUserInfos (const std::string & string) noexcept
	{
		const auto chunks = String::explode(string, '@');

		switch ( chunks.size() )
		{
			/* User information present. */
			case 2 :
				this->parseUserInfos(chunks[0]);

				return chunks[1];

			/* No user information. */
			case 1 :
				return chunks[0];

			default:
				std::cerr << __PRETTY_FUNCTION__ << ", invalid URI domain, multiple '@' char found !" "\n";

				return string;
		}
	}

	void
	URIDomain::parseUserInfos (const std::string & string) noexcept
	{
		/* NOTE: ";AUTH" */
		auto chunks = String::explode(string, ';');

		for ( size_t index = 1; index < chunks.size(); index++ )
		{
			const auto option = String::explode(chunks[index], '=');

			if ( option.size() != 2 )
			{
				std::cerr << __PRETTY_FUNCTION__ << ", invalid URI domain option '" << chunks[index] << "' !" "\n";

				continue;
			}

			this->addOption(option[0], option[1]);
		}

		/* NOTE: "USER:PASSWORD" */
		chunks = String::explode(chunks[0], ':');

		switch ( chunks.size() )
		{
			case 2 :
				this->setPassword(chunks[1]);

				[[fallthrough]];

			case 1 :
				this->setUsername(chunks[0]);
				break;

			default:
				std::cerr << __PRETTY_FUNCTION__ << ", invalid URI domain, multiple ':' char found in user informations !" "\n";

				break;
		}
	}

	std::string
	URIDomain::userinfo () const noexcept
	{
		auto output = m_username;

		if ( !m_password.empty() )
		{
			output += ':' + m_password;
		}

		if ( !m_options.empty() )
		{
			for ( const auto &option: m_options )
			{
				output += ';' + option.first + '=' + option.second;
			}
		}

		return output;
	}

	std::string
	URIDomain::host () const noexcept
	{
		std::stringstream string{};

		string << m_hostname;

		if ( m_port > 0 )
		{
			string << m_port;
		}

		return string.str();
	}

	std::ostream &
	operator<< (std::ostream & out, const URIDomain & obj)
	{
		out << obj.m_base;

		if ( !obj.m_username.empty() )
		{
			out << obj.m_username;

			if ( !obj.m_password.empty() )
			{
				out << ':' << obj.m_password;
			}

			if ( !obj.m_options.empty() )
			{
				for ( const auto &option: obj.m_options )
				{
					out << ';' << option.first << '=' << option.second;
				}
			}

			out << '@';
		}

		out << obj.m_hostname;

		if ( obj.m_port > 0 )
		{
			out << ':' << obj.m_port;
		}

		return out;
	}

	std::string
	to_string (const URIDomain & obj) noexcept
	{
		std::stringstream output;

		output << obj;

		return output.str();
	}
}
