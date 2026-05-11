/*
 * src/Libs/Hash/Types.hpp
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

#pragma once

/* STL inclusions. */
#include <string>

namespace EmEn::Libs::Hash
{
	/**
	 * @brief Hash type supported by the engine.
	 */
	enum HashType
	{
		Undefined,
		CRC32,
		MD5,
		SHA1,
		SHA256
	};

	constexpr auto UndefinedString{"Undefined"};
	constexpr auto CRC32String{"CRC32"};
	constexpr auto MD5String{"MD5"};
	constexpr auto SHA1String{"SHA1"};
	constexpr auto SHA256String{"SHA256"};

	/**
	* @brief Returns a C-String version of the enum value.
	* @param value The enum value.
	* @return const char *
	*/
	[[nodiscard]]
	inline
	const char *
	to_cstring (HashType value) noexcept
	{
		switch ( value )
		{
			case HashType::Undefined :
				return UndefinedString;

			case HashType::CRC32 :
				return CRC32String;

			case HashType::MD5 :
				return MD5String;

			case HashType::SHA1 :
				return SHA1String;

			case HashType::SHA256 :
				return SHA256String;
		}

		return UndefinedString;
	}

	/**
	 * @brief Returns a string version of the enum value.
	 * @param value The enum value.
	 * @return std::string
	 */
	[[nodiscard]]
	inline
	std::string
	to_string (HashType value)
	{
		return {to_cstring(value)};
	}

	[[nodiscard]]
	inline
	HashType
	to_HashType (const std::string & value) noexcept
	{
		if ( value == CRC32String )
		{
			return HashType::CRC32;
		}

		if ( value == MD5String )
		{
			return HashType::MD5;
		}

		if ( value == SHA1String )
		{
			return HashType::SHA1;
		}

		if ( value == SHA256String )
		{
			return HashType::SHA256;
		}

		return HashType::Undefined;;
	}
}
