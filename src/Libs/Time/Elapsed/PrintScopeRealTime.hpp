/*
 * src/Libs/Time/Elapsed/PrintScopeRealTime.hpp
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
#include <chrono>
#include <string>
#include <utility>
#include <iostream>

/* Local inclusions. */
#include "Libs/Time/Types.hpp"

namespace EmEn::Libs::Time::Elapsed
{
	/**
	 * @brief Print in the console the duration in wall clock time of a specific scope.
	 * @note The internal precision is set to nanoseconds.
	 * @tparam precision_t The desired precision of time to report. Default milliseconds.
	 * @tparam clock_t The type of clock used. Default std::chrono::high_resolution_clock.
	 */
	template< std::uint8_t precision_t = static_cast< std::uint8_t >(Precision::Milliseconds), typename clock_t = std::chrono::high_resolution_clock >
	class PrintScopeRealTime final
	{
		public:

			/**
			 * @brief Constructs a default scope timer.
			 */
			PrintScopeRealTime () noexcept = default;

			/**
			 * @brief Starts a nano timer.
			 * @param label A reference to a string for the output [std::move].
			 */
			explicit
			PrintScopeRealTime (std::string label) noexcept
				: m_label(std::move(label))
			{

			}

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			PrintScopeRealTime (const PrintScopeRealTime & copy) noexcept = delete;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			PrintScopeRealTime (PrintScopeRealTime && copy) noexcept = delete;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 */
			PrintScopeRealTime & operator= (const PrintScopeRealTime & copy) noexcept = delete;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 */
			PrintScopeRealTime & operator= (PrintScopeRealTime && copy) noexcept = delete;

			/**
			 * @brief The destructor will print out the result.
			 */

			~PrintScopeRealTime ()
			{
				const auto nanoseconds = std::chrono::duration_cast< std::chrono::nanoseconds >(clock_t::now() - m_start).count();

				if constexpr ( precision_t == static_cast< std::uint8_t >(Precision::Seconds) )
				{
					std::cout << m_label << " : " << static_cast< double >(nanoseconds) / 1000000000.0 << " s" "\n";
				}

				if constexpr ( precision_t == static_cast< std::uint8_t >(Precision::Milliseconds) )
				{
					std::cout << m_label << " : " << static_cast< double >(nanoseconds) / 1000000.0 << " ms" "\n";
				}

				if constexpr ( precision_t == static_cast< std::uint8_t >(Precision::Microseconds) )
				{
					std::cout << m_label << " : " << static_cast< double >(nanoseconds) / 1000.0 << " μs" "\n";
				}

				if constexpr ( precision_t == static_cast< std::uint8_t >(Precision::Nanoseconds) )
				{
					std::cout << m_label << " : " << nanoseconds << " ns" "\n" "\n";
				}
			}

		private:

			std::chrono::time_point< clock_t > m_start{clock_t::now()};
			std::string m_label{"Time"};
	};

	/**
	 * @brief Print in the console the duration in wall clock time of a specific scope.
	 * @note The internal precision is set to nanoseconds.
	 * @tparam precision_t The desired precision of time to report. Default milliseconds.
	 * @tparam clock_t The type of clock used. Default std::chrono::high_resolution_clock.
	 */
	template< std::uint8_t precision_t = static_cast< std::uint8_t >(Precision::Milliseconds), typename clock_t = std::chrono::high_resolution_clock >
	class PrintScopeRealTimeThreshold final
	{
		public:

			/**
			 * @brief Constructs a default scope timer.
			 * @param threshold The limit value to print.
			 */
			explicit
			PrintScopeRealTimeThreshold (double threshold) noexcept
				: m_threshold(threshold)
			{

			}

			/**
			 * @brief tarts a labeled nano scope timer.
			 * @param label A reference to a string for the output [std::move].
			 * @param threshold The limit value to print.
			 */
			PrintScopeRealTimeThreshold (std::string label, double threshold) noexcept
				: m_label(std::move(label)), m_threshold(threshold)
			{

			}

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			PrintScopeRealTimeThreshold (const PrintScopeRealTimeThreshold & copy) noexcept = delete;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			PrintScopeRealTimeThreshold (PrintScopeRealTimeThreshold && copy) noexcept = delete;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 * @return PrintScopeRealTimeThreshold &
			 */
			PrintScopeRealTimeThreshold & operator= (const PrintScopeRealTimeThreshold & copy) noexcept = delete;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 * @return PrintScopeRealTimeThreshold &
			 */
			PrintScopeRealTimeThreshold & operator= (PrintScopeRealTimeThreshold && copy) noexcept = delete;

			/**
			 * @brief The destructor will print out the result.
			 */
			~PrintScopeRealTimeThreshold ()
			{
				const auto nanoseconds = std::chrono::duration_cast< std::chrono::nanoseconds >(clock_t::now() - m_start).count();

				if constexpr ( precision_t == static_cast< std::uint8_t >(Precision::Seconds) )
				{
					const auto interval = static_cast< double >(nanoseconds) / 1000000000.0;

					if ( interval < m_threshold )
					{
						return;
					}

					std::cerr << m_label << " : " << interval << " s" "\n";
				}

				if constexpr ( precision_t == static_cast< std::uint8_t >(Precision::Milliseconds) )
				{
					const auto interval = static_cast< double >(nanoseconds) / 1000000.0;

					if ( interval < m_threshold )
					{
						return;
					}

					std::cerr << m_label << " : " << interval << " ms" "\n";
				}

				if constexpr ( precision_t == static_cast< std::uint8_t >(Precision::Microseconds) )
				{
					const auto interval = static_cast< double >(nanoseconds) / 1000.0;

					if ( interval < m_threshold )
					{
						return;
					}

					std::cerr << m_label << " : " << interval << " μs" "\n";
				}

				if constexpr ( precision_t == static_cast< std::uint8_t >(Precision::Nanoseconds) )
				{
					if ( nanoseconds < m_threshold )
					{
						return;
					}

					std::cerr << m_label << " : " << nanoseconds << " ns" "\n";
				}
			}

		private:

			std::chrono::time_point< clock_t > m_start{clock_t::now()};
			std::string m_label{"RealTime"};
			double m_threshold;
	};
}
