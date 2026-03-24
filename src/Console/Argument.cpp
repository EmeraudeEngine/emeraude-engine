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
		if ( const auto * p = std::get_if< int32_t >(&m_value) )
		{
			return *p > 0;
		}

		if ( const auto * p = std::get_if< float >(&m_value) )
		{
			return *p > 0.0F;
		}

		if ( const auto * p = std::get_if< bool >(&m_value) )
		{
			return *p;
		}

		TraceWarning{ClassId} << "This argument is not a boolean !";

		return false;
	}

	int32_t
	Argument::asInteger () const noexcept
	{
		if ( const auto * p = std::get_if< float >(&m_value) )
		{
			return static_cast< int32_t >(std::round(*p));
		}

		if ( const auto * p = std::get_if< int32_t >(&m_value) )
		{
			return *p;
		}

		TraceWarning{ClassId} << "This argument is not an integer number !";

		return 0;
	}

	float
	Argument::asFloat () const noexcept
	{
		if ( const auto * p = std::get_if< float >(&m_value) )
		{
			return *p;
		}

		if ( const auto * p = std::get_if< int32_t >(&m_value) )
		{
			return static_cast< float >(*p);
		}

		TraceWarning{ClassId} << "This argument is not a floating point number !";

		return 0.0F;
	}

	std::string
	Argument::asString () const noexcept
	{
		if ( const auto * p = std::get_if< std::string >(&m_value) )
		{
			return *p;
		}

		TraceWarning{ClassId} << "This argument is not a string !";

		return {};
	}
}
