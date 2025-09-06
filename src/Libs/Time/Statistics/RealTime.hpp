/*
 * src/Libs/Time/Statistics/RealTime.hpp
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
#include <cstddef>
#include <cstdint>
#include <chrono>

/* Local inclusions for inheritances. */
#include "Abstract.hpp"

namespace EmEn::Libs::Time::Statistics
{
	/**
	 * @brief A chrono to get the duration in wall clock time between two tops.
	 * @extends EmEn::Libs::Time::Statistics::Abstract The interface for statistics.
	 * @tparam clockType The type of clock used. Default std::chrono::high_resolution_clock.
	 */
	template< typename clockType = std::chrono::high_resolution_clock >
	class RealTime final : public Abstract
	{
		public:

			/**
			 * @brief Constructs a stat counter in wall clock time.
			 * @param range The range of statistics to make an average. Default no averaging.
			 */
			explicit
			RealTime (size_t range = 1) noexcept
				: Abstract(range)
			{

			}

			/** @copydoc EmEn::Libs::Time::Statistics::Abstract::start() */
			void
			start () noexcept override
			{
				m_startTime = clockType::now();
			}

			/** @copydoc EmEn::Libs::Time::Statistics::Abstract::stop() */
			void
			stop () noexcept override
			{
				/* Increment executions count. */
				m_currentExecutionsPerSecond++;

				/* Gets the duration. */
				auto duration = std::chrono::duration_cast< std::chrono::milliseconds >(clockType::now() - m_startTime).count();

				/* Insert duration for average statistics. */
				this->insertDuration(duration);

				/* Keep track of time elapsed. */
				m_delta += duration;

				/* Checks if a second passed. */
				if ( m_delta >= TimeCorrection )
				{
					/* Insert EPS for average statistics. */
					this->insertEPS(m_currentExecutionsPerSecond);

					/* Reset statistics variables. */
					m_delta -= TimeCorrection;
					m_currentExecutionsPerSecond = 0;
				}
			}

		private:

			static constexpr auto TimeCorrection{1000};

			std::chrono::time_point< clockType > m_startTime{};
			uint64_t m_delta{0};
			uint32_t m_currentExecutionsPerSecond{0};
	};
}
