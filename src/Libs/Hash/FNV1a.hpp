/*
 * src/Libs/Hash/FNV1a.hpp
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
#include <cstddef>
#include <string_view>

namespace EmEn::Libs::Hash
{
	/**
	 * @brief Hashes a string using the FNV-1a algorithm.
	 * @param string A string view.
	 * @return std::size_t
	 */
	[[nodiscard]]
	constexpr
	std::size_t
	FNV1a (const std::string_view string) noexcept
	{
		constexpr std::size_t OffsetBasis{14695981039346656037ULL};
		constexpr std::size_t Prime{1099511628211ULL};

		std::size_t value = OffsetBasis;

		for ( const char character : string )
		{
			value = (value ^ static_cast< std::size_t >(character)) * Prime;
		}

		return value;
	}
}

constexpr
std::size_t
operator""_hash (const char * string, std::size_t)
{
	return EmEn::Libs::Hash::FNV1a(string);
}
