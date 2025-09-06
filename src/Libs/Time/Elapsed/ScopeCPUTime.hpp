/*
 * src/Libs/Time/Elapsed/ScopeCPUTime.hpp
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
#include <cstdint>
#include <ctime>

namespace EmEn::Libs::Time::Elapsed
{
	/**
	 * @brief Gets the duration in CPU time of a specific scope.
	 * @note The internal precision is set to nanoseconds.
	 */
	class ScopeCPUTime final
	{
		public:

			/**
			 * @brief Construct a scope timer in nanoseconds.
			 * @param duration A reference where the duration will be accumulated in nanoseconds.
			 */
			explicit
			ScopeCPUTime (uint64_t & duration) noexcept
				: m_duration{duration}
			{

			}

			/** @brief Deleted copy constructor. */
			ScopeCPUTime (const ScopeCPUTime &) = delete;

			/** @brief Deleted move constructor. */
			ScopeCPUTime (ScopeCPUTime &&) = delete;

			/** @brief Deleted assignment operator. */
			ScopeCPUTime & operator= (const ScopeCPUTime &) = delete;

			/** @brief Deleted move assignment operator. */
			ScopeCPUTime & operator= (const ScopeCPUTime &&) = delete;

			/** @brief Destructor. */
			~ScopeCPUTime ()
			{
				if constexpr ( CLOCKS_PER_SEC == 1000 )
				{
					/* Duration is expressed in milliseconds. */
					m_duration += (std::clock() - m_start) * 1000000;
				}

				if constexpr ( CLOCKS_PER_SEC == 1000000 )
				{
					/* Duration is expressed in microseconds. */
					m_duration += (std::clock() - m_start) * 1000;
				}

				if constexpr ( CLOCKS_PER_SEC == 1000000000 )
				{
					/* Duration is expressed in nanoseconds. */
					m_duration += std::clock() - m_start;
				}
			}

		private:

			uint64_t & m_duration;
			std::clock_t m_start{std::clock()};
	};
}
