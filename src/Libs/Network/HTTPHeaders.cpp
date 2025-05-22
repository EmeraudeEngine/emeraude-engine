/*
 * src/Libs/Network/HTTPHeaders.cpp
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

#include "HTTPHeaders.hpp"

/* Emeraude-Engine configuration. */
#include "emeraude_config.hpp"

/* STL inclusions. */
#include <iostream>

/* Local inclusions. */
#include "Libs/String.hpp"

namespace EmEn::Libs::Network
{
	HTTPHeaders::HTTPHeaders (Version version) noexcept
		: m_version(version)
	{

	}

	void
	HTTPHeaders::setVersion (Version version) noexcept
	{
		m_version = version;
	}

	HTTPHeaders::Version
	HTTPHeaders::version () const noexcept
	{
		return m_version;
	}

	void
	HTTPHeaders::add (const std::string & key, const std::string & value) noexcept
	{
		m_headers.emplace(key, value);
	}

	std::string
	HTTPHeaders::value (const std::string & key) const noexcept
	{
		const auto headerIt = m_headers.find(key);

		if ( headerIt != m_headers.cend() )
		{
			return headerIt->second;
		}

		return {};
	}

	bool
	HTTPHeaders::parse (const std::string & rawHeaders) noexcept
	{
		auto lines = String::explode(rawHeaders, Separator, false);

		if ( empty(lines) )
		{
			std::cerr << __PRETTY_FUNCTION__ << ", empty HTTP header !" "\n";

			return false;
		}

		/* NOTE: Check the first and remove it. */
		if ( this->parseFirstLine(lines[0]) )
		{
			lines.erase(lines.begin());
		}
		else
		{
			std::cerr << __PRETTY_FUNCTION__ << ", unable to identify the HTTP header !" "\n";

			return false;
		}

		/* NOTE: Parse each header. */
		size_t errors = 0;

		for ( const auto & line : lines )
		{
			const auto chunks = String::explode(line, ':', false, 1);

			if ( chunks.size() != 2 )
			{
				std::cerr << __PRETTY_FUNCTION__ << ", unable to parse header line : " << line << "\n";

				errors++;

				continue;
			}

			this->add(String::trim(chunks[0]), String::trim(chunks[1]));
		}

		return errors == 0;
	}

	const char *
	HTTPHeaders::version (Version version) noexcept
	{
		switch ( version )
		{
			case Version::HTTP09 :
				return HTTP09;

			case Version::HTTP10 :
				return HTTP10;

			case Version::HTTP11 :
				return HTTP11;

			case Version::HTTP20 :
				return HTTP20;

			case Version::HTTP30 :
				return HTTP30;
		}

		return HTTP09;
	}

	HTTPHeaders::Version
	HTTPHeaders::parseVersion (const std::string & version) noexcept
	{
		if ( version == HTTP09 )
		{
			return Version::HTTP09;
		}

		if ( version == HTTP10 )
		{
			return Version::HTTP10;
		}

		if ( version == HTTP11 )
		{
			return Version::HTTP11;
		}

		if ( version == HTTP20 )
		{
			return Version::HTTP20;
		}

		if ( version == HTTP30 )
		{
			return Version::HTTP30;
		}

		return Version::HTTP09;
	}
}
