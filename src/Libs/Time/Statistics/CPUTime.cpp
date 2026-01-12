/*
 * src/Libs/Time/Statistics/CPUTime.cpp
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

#include "CPUTime.hpp"

namespace EmEn::Libs::Time::Statistics
{
	void
	CPUTime::stop () noexcept
	{
		/* Increment executions count. */
		m_currentExecutionsPerSecond++;

		/* Gets the duration. */
		const auto duration = std::clock() - m_startTime;

		/* Insert duration for average statistics. */
		if constexpr ( CLOCKS_PER_SEC == 1000 )
		{
			/* Duration is expressed in milliseconds. */
			this->insertDuration(duration);
		}

		if constexpr ( CLOCKS_PER_SEC == 1000000 )
		{
			/* Duration is expressed in microseconds. */
			this->insertDuration(duration / 1000);
		}

		if constexpr ( CLOCKS_PER_SEC == 1000000000 )
		{
			/* Duration is expressed in nanoseconds. */
			this->insertDuration(duration / 1000000);
		}

		/* Keep track of time elapsed. */
		m_delta += duration;

		/* Checks if a second passed. */
		if ( m_delta >= CLOCKS_PER_SEC )
		{
			/* Insert EPS for average statistics. */
			this->insertEPS(m_currentExecutionsPerSecond);

			/* Reset statistics variables. */
			m_delta -= CLOCKS_PER_SEC;
			m_currentExecutionsPerSecond = 0;
		}
	}
}
