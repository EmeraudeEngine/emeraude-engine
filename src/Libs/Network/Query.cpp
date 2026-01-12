/*
 * src/Libs/Network/Query.cpp
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

#include "Query.hpp"

/* STL inclusions. */
#include <algorithm>
#include <vector>

/* Local inclusions. */
#include "Libs/String.hpp"

namespace EmEn::Libs::Network
{
	Query
	Query::fromString (const std::string & string) noexcept
	{
		Query query;

		const auto variables = String::explode(string, '&');

		for ( const auto & variable : variables )
		{
			const auto definition = String::explode(variable, '=');

			if ( definition.size() == 2 )
			{
				query.addVariable(definition[0], definition[1]);
			}
			else
			{
				query.addVariable(definition[0], "");
			}
		}

		return query;
	}

	std::ostream &
	operator<< (std::ostream & out, const Query & obj)
	{
		std::vector< std::string > variables(obj.m_variables.size());

		std::ranges::transform(obj.m_variables, std::back_inserter(variables), [] (const auto & variable) {
			return variable.first + '=' + variable.second;
		});

		return out << String::implode(variables, '&');
	}
}
