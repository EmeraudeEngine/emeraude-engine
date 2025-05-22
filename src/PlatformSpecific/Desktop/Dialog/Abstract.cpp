/*
 * src/PlatformSpecific/Desktop/Dialog/Abstract.cpp
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

#include "Abstract.hpp"

/* STL inclusions. */
#include <sstream>

namespace EmEn::PlatformSpecific::Desktop::Dialog
{
	Abstract::Abstract (std::string title) noexcept
		: m_title(std::move(title))
	{

	}

#ifdef IS_LINUX
	std::vector< std::string >
	Abstract::convertFilterStructureForPFD (const std::vector< std::pair< std::string, std::vector< std::string > > > & extensionFilters) noexcept
	{
		std::vector< std::string > pfdFilterStructure;
		pfdFilterStructure.reserve(extensionFilters.size() * 2);

		for ( const auto & [filterName, extensions] : extensionFilters )
		{
			pfdFilterStructure.emplace_back(filterName);

			std::stringstream extensionString;

			for ( auto it = extensions.cbegin(); it != extensions.cend(); ++it )
			{
				extensionString << "*." << *it;

				if ( std::next(it) != extensions.cend() )
				{
					extensionString << ' ';
				}
			}

			pfdFilterStructure.emplace_back(extensionString.str());
		}

		return pfdFilterStructure;
	}
#endif
}
