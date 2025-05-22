/*
 * src/Types.cpp
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

namespace EmEn
{
	const char *
	to_cstring (Severity value) noexcept
	{
		switch ( value )
		{
			case Severity::Debug :
				return DebugString;

			case Severity::Info :
				return InfoString;

			case Severity::Success :
				return SuccessString;

			case Severity::Warning :
				return WarningString;

			case Severity::Error :
				return ErrorString;

			case Severity::Fatal :
				return FatalString;

			default:
				return "Unknown";
		}
	}

	const char *
	to_cstring (LogFormat value) noexcept
	{
		switch ( value )
		{
			case LogFormat::Text :
				return TextString;

			case LogFormat::JSON :
				return JSONString;

			case LogFormat::HTML :
				return HTMLString;

			default:
				return "Text";
		}
	}

	LogFormat
	to_LogFormat (const std::string & value) noexcept
	{
		if ( value == TextString )
		{
			return LogFormat::Text;
		}

		if ( value == JSONString )
		{
			return LogFormat::JSON;
		}

		if ( value == HTMLString )
		{
			return LogFormat::HTML;
		}

		return LogFormat::Text;
	}
}
