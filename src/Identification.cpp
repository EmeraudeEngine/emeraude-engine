/*
 * src/Identification.cpp
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

#include "Identification.hpp"

/* STL inclusions. */
#include <sstream>
#include <ranges>

/* Local inclusions. */
#include "Libs/String.hpp"

namespace EmEn
{
	using namespace EmEn::Libs;

	Identification::Identification (const char * name, const Version & version, const char * organization, const char * domain) noexcept
		: m_applicationName(name),
		m_applicationVersion(version),
		m_applicationOrganization(organization),
		m_applicationDomain(domain)
	{
		/* NOTE: Engine identification string. */
		{
			std::stringstream stream;

			stream << LibraryName << " (" << LibraryVersion << "; " << LibraryPlatform << "; " << LibraryCompilationDate << ") LGPLv3 - " << LibraryAuthorName;

			m_engineId = stream.str();
		}

		/* NOTE: Application identification string. */
		{
			std::stringstream stream;

			stream << m_applicationName << " (" << m_applicationVersion << ") - " << m_applicationOrganization;

			m_applicationId = stream.str();
		}

		/* NOTE: Application reverse id. */
		{
			std::stringstream stream;

			for ( const auto & chunk : std::ranges::reverse_view(String::explode(m_applicationDomain, '.')) )
			{
				stream << chunk << '.';
			}

			stream << m_applicationName;

			m_applicationReverseId = String::toLower(stream.str());
		}
	}
}
