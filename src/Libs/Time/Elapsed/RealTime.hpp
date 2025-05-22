/*
 * src/Libs/Time/Elapsed/RealTime.hpp
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
#include <ctime>

/* Local inclusions. */
#include "Abstract.hpp"

namespace EmEn::Libs::Time::Elapsed
{
	/**
	 * @brief Gets the duration in wall clock time between two point in milliseconds.
	 * @note The internal precision is set to nanoseconds.
	 * @extends EmEn::Libs::Time::Elapsed::Abstract
	 * @tparam clockType The type of clock used. Default std::chrono::high_resolution_clock.
	 */
	template< typename clockType = std::chrono::high_resolution_clock >
	class RealTime final : public Abstract
	{
		public:

			/**
			 * @brief Constructs an elapsed time structure.
			 */
			RealTime () noexcept = default;

			/** @copydoc EmEn::Libs::Time::Elapsed::Abstract::start() */
			void
			start () noexcept override
			{
				m_startTime = clockType::now();
			}

			/** @copydoc EmEn::Libs::Time::Elapsed::Abstract::stop() */
			void
			stop () noexcept override
			{
				this->setDuration(std::chrono::duration_cast< std::chrono::nanoseconds >(clockType::now() - m_startTime).count());
			}

		private:

			std::chrono::time_point< clockType > m_startTime{};
	};
}
