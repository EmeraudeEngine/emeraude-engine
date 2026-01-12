/*
 * src/Libs/Time/Elapsed/Abstract.hpp
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
#include <cstdint>

namespace EmEn::Libs::Time::Elapsed
{
	/**
	 * @brief The elapsed time abstract class.
	 */
	class Abstract
	{
		public:

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			Abstract (const Abstract & copy) noexcept = default;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			Abstract (Abstract && copy) noexcept = default;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 * @return AbstractElapsedTime &
			 */
			Abstract & operator= (const Abstract & copy) noexcept = default;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 * @return AbstractElapsedTime &
			 */
			Abstract & operator= (Abstract && copy) noexcept = default;

			/**
			 * @brief Destructs the elapsed time structure.
			 */
			virtual ~Abstract () = default;

			/**
			 * @brief Returns the duration in nanoseconds.
			 * @return uint64_t
			 */
			[[nodiscard]]
			uint64_t
			duration () const noexcept
			{
				return m_duration;
			}

			/**
			 * @brief Returns the duration in seconds.
			 * @return double
			 */
			[[nodiscard]]
			double
			seconds () const noexcept
			{
				return static_cast< double >(m_duration) / 1000000000.0;
			}

			/**
			 * @brief Returns the duration in milliseconds.
			 * @return double
			 */
			[[nodiscard]]
			double
			milliseconds () const noexcept
			{
				return static_cast< double >(m_duration) / 1000000.0;
			}

			/**
			 * @brief Returns the duration in microseconds.
			 * @return double
			 */
			[[nodiscard]]
			double
			microseconds () const noexcept
			{
				return static_cast< double >(m_duration) / 1000.0;
			}

			/**
			 * @brief Starts a measurement.
			 * @return void
			 */
			virtual void start () noexcept = 0;

			/**
			 * @brief Stops the measurement.
			 * @return void
			 */
			virtual void stop () noexcept = 0;

		protected:

			/**
			 * @brief Constructs an elapsed time structure.
			 */
			Abstract () noexcept = default;

			/**
			 * @brief Set duration in nanoseconds.
			 * @param duration The duration in nanoseconds.
			 * @return void
			 */
			void
			setDuration (uint64_t duration) noexcept
			{
				m_duration = duration;
			}

		private:

			uint64_t m_duration{0};
	};
}
