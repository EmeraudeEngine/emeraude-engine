/*
 * src/Console/Argument.cpp
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

#include "Argument.hpp"

/* STL inclusions. */
#include <cmath>

/* Local inclusions. */
#include "Tracer.hpp"

namespace EmEn::Console
{
	bool
	Argument::asBoolean () const noexcept
	{
		if ( m_type == ArgumentType::Integer )
		{
			const auto value = std::any_cast< int32_t >(m_value);

			return value > 0;
		}

		if ( m_type == ArgumentType::Float )
		{
			const auto value = std::any_cast< float >(m_value);

			return value > 0.0F;
		}

		if ( m_type != ArgumentType::Boolean )
		{
			TraceWarning{ClassId} << "This argument is not a boolean !";

			return false;
		}

		return std::any_cast< bool >(m_value);
	}

	int32_t
	Argument::asInteger () const noexcept
	{
		if ( m_type == ArgumentType::Float )
		{
			const auto value = std::any_cast< float >(m_value);

			return static_cast< int32_t >(std::round(value));
		}

		if ( m_type != ArgumentType::Integer )
		{
			TraceWarning{ClassId} << "This argument is not an integer number !";

			return 0;
		}

		return std::any_cast< int32_t >(m_value);
	}

	float
	Argument::asFloat () const noexcept
	{
		if ( m_type != ArgumentType::Float )
		{
			TraceWarning{ClassId} << "This argument is not a floating point number !";

			return 0.0F;
		}

		return std::any_cast< float >(m_value);
	}

	std::string
	Argument::asString () const noexcept
	{
		if ( m_type != ArgumentType::String )
		{
			TraceWarning{ClassId} << "This argument is not a string !";

			return {};
		}

		return std::any_cast< std::string >(m_value);
	}
}
