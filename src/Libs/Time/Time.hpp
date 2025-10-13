/*
 * src/Libs/Time/Time.hpp
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

#pragma once

/* STL inclusions. */
#include <chrono>
#include <cstdint>
#include <ctime>
#include <string>

namespace EmEn::Libs::Time
{
	/** @brief Returns the time point when the program has started. */
	static const std::chrono::time_point< std::chrono::steady_clock > ProgramStaringTime{std::chrono::steady_clock::now()};

	/**
	 * @brief Returns the elapsed time in seconds since the start of the program.
	 * @return int64_t
	 */
	inline
	int64_t
	elapsedSeconds () noexcept
	{
		return std::chrono::duration_cast< std::chrono::seconds >(std::chrono::steady_clock::now() - ProgramStaringTime).count();
	}

	/**
	 * @brief Returns the elapsed time in milliseconds since the start of the program.
	 * @return int64_t
	 */
	inline
	int64_t
	elapsedMilliseconds () noexcept
	{
		return std::chrono::duration_cast< std::chrono::milliseconds >(std::chrono::steady_clock::now() - ProgramStaringTime).count();
	}

	/**
	 * @brief Returns the elapsed time in nanoseconds since the start of the program.
	 * @return int64_t
	 */
	inline
	int64_t
	elapsedMicroseconds () noexcept
	{
		return std::chrono::duration_cast< std::chrono::microseconds >(std::chrono::steady_clock::now() - ProgramStaringTime).count();
	}

	/**
	 * @brief Returns UNIX timestamp as a simple integer.
	 * @return uint32_t
	 */
	inline
	uint32_t
	UNIXTimestamp () noexcept
	{
		return static_cast< unsigned int >(std::time(nullptr));
	}

	/**
	 * @brief Returns UNIX timestamp as a string.
	 * @return std::string
	 */
	inline
	std::string
	UNIXTimestampString () noexcept
	{
		return (std::stringstream{} << UNIXTimestamp()).str();
	}
}
