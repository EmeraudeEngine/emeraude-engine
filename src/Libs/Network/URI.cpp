/*
 * src/Libs/Network/URI.cpp
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

#include "URI.hpp"

/* Emeraude-Engine configuration. */
#include "emeraude_config.hpp"

/* STL inclusions. */
#include <sstream>

/* Local inclusions. */
#include "Libs/String.hpp"

namespace EmEn::Libs::Network
{
	URI::URI (const std::string & rawString) noexcept
	{
		if ( !this->parseRawString(rawString) )
		{
			std::cerr << __PRETTY_FUNCTION__ << ", empty string !" "\n";
		}
	}

	URI &
	URI::operator= (const std::string & rawString) noexcept
	{
		if ( !this->parseRawString(rawString) )
		{
			std::cerr << __PRETTY_FUNCTION__ << ", empty string !" "\n";
		}

		return *this;
	}

	bool
	URI::parseRawString (std::string rawString) noexcept
	{
		if ( rawString.empty() )
		{
			return false;
		}

		rawString = this->extractFragment(rawString);

		if ( rawString.empty() )
		{
			return false;
		}

		rawString = this->extractQuery(rawString);

		if ( rawString.empty() )
		{
			return false;
		}

		/* NOTE: Pre-check for simple path. */
		if ( this->checkSimplePath(rawString) )
		{
			return false;
		}

		rawString = this->extractScheme(rawString);

		if ( rawString.empty() )
		{
			return false;
		}

		rawString = this->extractURIDomain(rawString);

		if ( rawString.empty() )
		{
			return false;
		}

		m_path = rawString;

		return true;
	}

	[[nodiscard]]
	bool
	URI::checkSimplePath (const std::string & string) noexcept
	{
		for ( const auto & chunk : String::explode(string, '/') )
		{
			if ( chunk.find(':') != std::string::npos )
			{
				return false;
			}
		}

		this->setPath(string);

		return true;
	}

	std::string
	URI::extractScheme (const std::string & string) noexcept
	{
		auto chunks = String::explode(string, ':');

		if ( chunks.size() >= 2 )
		{
			this->setScheme(chunks[0]);

			chunks.erase(chunks.begin());

			return String::implode(chunks, ':');
		}

		return string;
	}

	std::string
	URI::extractFragment (const std::string & string) noexcept
	{
		const auto chunks = String::explode(string, '#');

		if ( chunks.size() >= 2 )
		{
			this->setFragment(chunks[1]);

			if ( chunks.size() > 2 )
			{
				std::cerr << __PRETTY_FUNCTION__ << ", multiple '#' char found in the URL !" "\n";
			}
		}

		return chunks[0];
	}

	std::string
	URI::extractQuery (const std::string & string) noexcept
	{
		const auto chunks = String::explode(string, '?');

		if ( chunks.size() >= 2 )
		{
			this->setQuery(Query::fromString(chunks[1]));

			if ( chunks.size() > 2 )
			{
				std::cerr << __PRETTY_FUNCTION__ << ", multiple '?' char found in the URL !" "\n";
			}
		}

		return chunks[0];
	}

	std::string
	URI::extractURIDomain (const std::string & string) noexcept
	{
		std::string tmpBase{};

		for ( const auto & base : URIDomain::Bases )
		{
			if ( string.starts_with(base) )
			{
				tmpBase = base;

				break;
			}
		}

		std::vector< std::string > chunks{};

		if ( tmpBase.empty() )
		{
			chunks = String::explode(string, '/');

			this->setURIDomain(URIDomain{chunks[0]});
		}
		else
		{
			chunks = String::explode(String::replace(tmpBase, "", string), '/');

			this->setURIDomain(URIDomain{tmpBase + chunks[0]});
		}

		chunks.erase(chunks.begin());

		return String::implode(chunks, '/');
	}

	std::string
	URI::resource () const noexcept
	{
		std::stringstream string{};

		if ( !m_path.empty() )
		{
			string << m_path;
		}

		if ( !m_query.empty() )
		{
			string << '?' << m_query;
		}

		return string.str();
	}

	std::ostream &
	operator<< (std::ostream & out, const URI & obj)
	{
		if ( !obj.m_scheme.empty() )
		{
			out << obj.m_scheme << ':';
		}

		if ( !obj.m_uriDomain.empty() )
		{
			out << obj.m_uriDomain;

			if ( !obj.m_path.empty() )
			{
				out << '/' << obj.m_path;
			}
		}
		else if ( !obj.m_path.empty() )
		{
			out << obj.m_path;
		}

		if ( !obj.m_query.empty() )
		{
			out << '?' << obj.m_query;
		}

		if ( !obj.m_fragment.empty() )
		{
			out << '#' << obj.m_fragment;
		}

		return out;
	}

	std::string
	to_string (const URI & obj) noexcept
	{
		std::stringstream output;

		output << obj;

		return output.str();
	}
}
