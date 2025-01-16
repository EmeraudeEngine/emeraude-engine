/*
 * src/Libraries/Time/Statistics/Abstract.hpp
 * This file is part of Emeraude-Engine
 *
 * Copyright (C) 2010-2024 - "LondNoir" <londnoir@gmail.com>
 *
 * Emeraude-Engine is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Emeraude-Engine is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Emeraude-Engine; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301  USA
 *
 * Complete project and additional information can be found at :
 * https://bitbucket.org/londnoir/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#pragma once

/* STL inclusions. */
#include <cstddef>
#include <cstdint>
#include <vector>

namespace Libraries::Time::Statistics
{
	/**
	 * @brief The statistics abstract class.
	 * @note This keeps the statistics mechanism independent of how time is taken.
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
			 * @return Abstract &
			 */
			Abstract & operator= (const Abstract & copy) noexcept = default;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 * @return Abstract &
			 */
			Abstract & operator= (Abstract && copy) noexcept = default;

			/**
			 * @brief Destructs the statistics.
			 */
			virtual ~Abstract () = default;

			/**
			 * @brief Returns the last duration in milliseconds.
			 * @return uint64_t
			 */
			[[nodiscard]]
			uint64_t duration () const noexcept;

			/**
			 * @brief Returns the average duration in milliseconds.
			 * @return double
			 */
			[[nodiscard]]
			double averageDuration () const noexcept;

			/**
			 * @brief Returns the last executions count per second registered.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t executionsPerSecond () const noexcept;

			/**
			 * @brief Returns the average executions count per second registered.
			 * @return double
			 */
			[[nodiscard]]
			double averageExecutionsPerSecond () const noexcept;

			/**
			 * @brief Returns the range of samples for averaging.
			 * @return size_t
			 */
			[[nodiscard]]
			size_t range () const noexcept;

			/**
			 * @brief Returns the number of stat taken.
			 * @return size_t
			 */
			[[nodiscard]]
			size_t topCount () const noexcept;

			/**
			 * @brief Prints statistics in console.
			 * @return void
			 */
			void print () const noexcept;

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
			 * @brief Constructs a statistics.
			 * @param range The range of statistics to make an average.
			 */
			explicit Abstract (size_t range) noexcept;

			/**
			 * @brief Increments the index.
			 * @param index A reference to an index.
			 * @return void
			 */
			void incrementIndex (size_t & index) const noexcept;

			/**
			 * @brief Returns the previous index.
			 * @param index The index.
			 * @return size_t
			 */
			[[nodiscard]]
			size_t getPreviousIndex (size_t index) const noexcept;

			/**
			 * @brief Inserts a duration.
			 * @param duration The duration in milliseconds.
			 * @return void
			 */
			void insertDuration (uint64_t duration) noexcept;

			/**
			 * @brief Inserts an execution count per seconds.
			 * @param count The number of execution.
			 * @return void
			 */
			void insertEPS (uint32_t count) noexcept;

		private:

			size_t m_range{1};
			std::vector< uint64_t > m_durations{};
			std::vector< uint32_t > m_executionsPerSeconds{};
			size_t m_topCount{0};
			size_t m_durationIndex{0};
			size_t m_EPSIndex{0};
	};
}
