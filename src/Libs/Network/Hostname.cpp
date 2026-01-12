/*
 * src/Libs/Network/Hostname.cpp
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

#include "Hostname.hpp"

#include <ranges>

/* Local inclusions. */
#include "Libs/String.hpp"

namespace EmEn::Libs::Network
{
	Hostname
	Hostname::fromString (const std::string & string) noexcept
	{
		Hostname hostname;

		if ( string.empty() )
		{
			return hostname;
		}

		const auto chunks = String::explode(string, '.');

		auto domainDone = false;

		std::vector< std::string > subdomain;
		std::vector< std::string > domain;

		for ( const auto & chunk : std::ranges::reverse_view(chunks) )
		{
			if ( domainDone )
			{
				subdomain.insert(subdomain.begin(), chunk);

				continue;
			}

			if ( chunk.length() > 3 )
			{
				domainDone = true;
			}

			domain.insert(domain.begin(), chunk);
		}

		if ( !subdomain.empty() )
		{
			hostname.setSubdomain(String::implode(subdomain, '.'));
		}

		if ( !domain.empty() )
		{
			hostname.setDomain(String::implode(domain, '.'));
		}

		return hostname;
	}

	std::string
	Hostname::tld () const noexcept
	{
		if ( const auto position = m_domain.find('.'); position != std::string::npos )
		{
			return m_domain.substr(position + 1);
		}

		return {};
	}
}
