/*
 * src/Resources/Types.cpp
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

#include "Types.hpp"

/* Emeraude-Engine configuration. */
#include "emeraude_config.hpp"

/* STL inclusions. */
#include <iostream>

namespace EmEn::Resources
{
	const char *
	to_cstring (SourceType enumeration) noexcept
	{
		switch ( enumeration )
		{
			case SourceType::Undefined :
				return UndefinedString;

			case SourceType::LocalData :
				return LocalDataString;

			case SourceType::ExternalData :
				return ExternalDataString;

			case SourceType::DirectData :
				return DirectDataString;
		}

		return UndefinedString;
	}

	SourceType
	to_SourceType (const std::string & string) noexcept
	{
		if ( string == LocalDataString )
		{
			return SourceType::LocalData;
		}

		if ( string == ExternalDataString )
		{
			return SourceType::ExternalData;
		}

		if ( string == DirectDataString )
		{
			return SourceType::DirectData;
		}

		std::cerr << __PRETTY_FUNCTION__ << ", unknown " << string << " source type. Return Undefined by default !" "\n";

		return SourceType::Undefined;
	}

	const char *
	to_cstring (Status value) noexcept
	{
		switch ( value )
		{
			case Status::Unloaded :
				return UnloadedString;

			case Status::Enqueuing :
				return EnqueuingString;

			case Status::ManualEnqueuing :
				return ManualEnqueuingString;

			case Status::Loading :
				return LoadingString;

			case Status::Loaded :
				return LoadedString;

			case Status::Failed :
				return FailedString;
		}

		return nullptr;
	}
}
