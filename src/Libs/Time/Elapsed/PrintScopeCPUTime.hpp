/*
 * src/Libs/Time/Elapsed/PrintScopeCPUTime.hpp
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
#include <ctime>
#include <string>
#include <iostream>

/* Local inclusions. */
#include "Libs/Time/Types.hpp"

namespace EmEn::Libs::Time::Elapsed
{
	/**
	 * @brief Print in the console the duration in CPU time of a specific scope.
	 * @note The internal precision is set to nanoseconds.
	 * @tparam precision_t The desired precision of time to report. Default milliseconds.
	 */
	template< std::uint8_t precision_t = static_cast< std::uint8_t >(Precision::Milliseconds) >
	class PrintScopeCPUTime final
	{
		public:

			/**
			 * @brief Constructs a default scope timer.
			 */
			PrintScopeCPUTime () noexcept = default;

			/**
			 * @brief Starts a nano timer.
			 * @param label A reference to a string for the output [std::move].
			 */
			explicit
			PrintScopeCPUTime (std::string label) noexcept
				: m_label(std::move(label))
			{

			}

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			PrintScopeCPUTime (const PrintScopeCPUTime & copy) noexcept = delete;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			PrintScopeCPUTime (PrintScopeCPUTime && copy) noexcept = delete;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 */
			PrintScopeCPUTime & operator= (const PrintScopeCPUTime & copy) noexcept = delete;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 */
			PrintScopeCPUTime & operator= (PrintScopeCPUTime && copy) noexcept = delete;

			/**
			 * @brief The destructor will print out the result.
			 */
			~PrintScopeCPUTime ()
			{
				auto nanoseconds = 0UL;

				if constexpr ( CLOCKS_PER_SEC == 1000 )
				{
					/* Duration is expressed in milliseconds. */
					nanoseconds = (std::clock() - m_start) * 1000000;
				}

				if constexpr ( CLOCKS_PER_SEC == 1000000 )
				{
					/* Duration is expressed in microseconds. */
					nanoseconds = (std::clock() - m_start) * 1000;
				}

				if constexpr ( CLOCKS_PER_SEC == 1000000000 )
				{
					/* Duration is expressed in nanoseconds. */
					nanoseconds = std::clock() - m_start;
				}

				if constexpr ( precision_t == static_cast< std::uint8_t >(Precision::Seconds) )
				{
					std::cout << m_label << " (CPU) : " << static_cast< double >(nanoseconds) / 1000000000.0 << " s" "\n";
				}

				if constexpr ( precision_t == static_cast< std::uint8_t >(Precision::Milliseconds) )
				{
					std::cout << m_label << " (CPU) : " << static_cast< double >(nanoseconds) / 1000000.0 << " ms" "\n";
				}

				if constexpr ( precision_t == static_cast< std::uint8_t >(Precision::Microseconds) )
				{
					std::cout << m_label << " (CPU) : " << static_cast< double >(nanoseconds) / 1000.0 << " μs" "\n";
				}

				if constexpr ( precision_t == static_cast< std::uint8_t >(Precision::Nanoseconds) )
				{
					std::cout << m_label << " (CPU) : " << nanoseconds << " ns" "\n";
				}
			}

		private:

			std::clock_t m_start{std::clock()};
			std::string m_label{"Time"};
	};

	/**
	 * @brief Print in the console the duration in CPU time of a specific scope.
	 * @note The internal precision is set to nanoseconds.
	 * @tparam precision_t The desired precision of time to report. Default milliseconds.
	 */
	template< std::uint8_t precision_t = static_cast< std::uint8_t >(Precision::Milliseconds) >
	class PrintScopeCPUTimeThreshold final
	{
		public:

			/**
			 * @brief Constructs a default nano scope timer.
			 * @param threshold The limit value to print.
			 */
			explicit
			PrintScopeCPUTimeThreshold (double threshold) noexcept
				: m_threshold(threshold)
			{

			}

			/**
			 * @brief Starts a labeled nano scope timer.
			 * @param label A reference to a string for the output [std::move].
			 * @param threshold The limit value to print.
			 */
			PrintScopeCPUTimeThreshold (std::string label, double threshold) noexcept
			: m_label(std::move(label)), m_threshold(threshold)
			{

			}

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			PrintScopeCPUTimeThreshold (const PrintScopeCPUTimeThreshold & copy) noexcept = delete;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			PrintScopeCPUTimeThreshold (PrintScopeCPUTimeThreshold && copy) noexcept = delete;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 * @return PrintScopeCPUTimeThreshold &
			 */
			PrintScopeCPUTimeThreshold & operator= (const PrintScopeCPUTimeThreshold & copy) noexcept = delete;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 * @return PrintScopeCPUTimeThreshold &
			 */
			PrintScopeCPUTimeThreshold & operator= (PrintScopeCPUTimeThreshold && copy) noexcept = delete;

			/**
			 * @brief The destructor will print out the result.
			 */
			~PrintScopeCPUTimeThreshold ()
			{
				auto nanoseconds = 0UL;

				if constexpr ( CLOCKS_PER_SEC == 1000 )
				{
					/* Duration is expressed in milliseconds. */
					nanoseconds = (std::clock() - m_start) * 1000000;
				}

				if constexpr ( CLOCKS_PER_SEC == 1000000 )
				{
					/* Duration is expressed in microseconds. */
					nanoseconds = (std::clock() - m_start) * 1000;
				}

				if constexpr ( CLOCKS_PER_SEC == 1000000000 )
				{
					/* Duration is expressed in nanoseconds. */
					nanoseconds = std::clock() - m_start;
				}

				if constexpr ( precision_t == static_cast< std::uint8_t >(Precision::Seconds) )
				{
					const auto interval = static_cast< double >(nanoseconds) / 1000000000.0;

					if ( interval < m_threshold )
					{
						return;
					}

					std::cerr << m_label << " (CPU) : " << interval << " s" "\n";
				}

				if constexpr ( precision_t == static_cast< std::uint8_t >(Precision::Milliseconds) )
				{
					const auto interval = static_cast< double >(nanoseconds) / 1000000.0;

					if ( interval < m_threshold )
					{
						return;
					}

					std::cerr << m_label << " (CPU) : " << interval << " ms" "\n";
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
					if ( nanoseconds < static_cast< long >(m_threshold) )
					{
						return;
					}

					std::cerr << m_label << " (CPU) : " << nanoseconds << " ns" "\n";
				}
			}

		private:

			std::clock_t m_start{std::clock()};
			std::string m_label{"CPUTime"};
			double m_threshold;
	};
}
