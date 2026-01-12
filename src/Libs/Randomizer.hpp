/*
 * src/Libs/Randomizer.hpp
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
#include <random>
#include <vector>
#include <array>

namespace EmEn::Libs
{
	/**
	 * @brief The randomizer class.
	 * @tparam number_t The type of integral number. Default int32_t.
	 * @tparam generator_t The type of generator. Default std::mt19937.
	 */
	template< typename number_t = int32_t, typename generator_t = std::mt19937 >
	requires (std::is_arithmetic_v< number_t > && std::uniform_random_bit_generator< generator_t >)
	class Randomizer final
	{
		public:

			/**
			 * @brief Constructs a randomizer fully random.
			 */
			Randomizer ()
			   : m_generator(m_device())
			{

			}

			/**
			 * @brief Constructs a randomizer with a seed to reproduce results.
			 * @param seed A fixed value.
			 */
			explicit Randomizer (typename generator_t::result_type seed) noexcept
				: m_generator(seed)
			{

			}

			/**
			 * @brief Returns a random number between a specified range.
			 * @param min The minimum number.
			 * @param max The maximum number.
			 * @return number_t
			 */
			[[nodiscard]]
			number_t
			value (number_t min, number_t max) noexcept
			{
				if ( max < min )
				{
					std::swap(min, max);
				}

				if constexpr ( std::is_integral_v< number_t > )
				{
					return std::uniform_int_distribution< number_t >{min, max}(m_generator);
				}
				else
				{
					return std::uniform_real_distribution< number_t >{min, max}(m_generator);
				}
			}

			/**
			 * @brief Returns an STL vector of random number between a specified range.
			 * @param count The number of desired numbers.
			 * @param min The minimum number.
			 * @param max The maximum number.
			 * @return std::vector< number_t >
			 */
			[[nodiscard]]
			std::vector< number_t >
			vector (size_t count, number_t min, number_t max)
			{
				std::vector< number_t > range(count);

				if ( max < min )
				{
					std::swap(min, max);
				}

				if constexpr ( std::is_integral_v< number_t > )
				{
					if constexpr (std::is_same_v< number_t, int8_t > || std::is_same_v< number_t, uint8_t > ||
								  std::is_same_v< number_t, char >   || std::is_same_v< number_t, signed char > ||
								  std::is_same_v< number_t, unsigned char > || std::is_same_v< number_t, char8_t >)
					{
						std::uniform_int_distribution< int > distribution{static_cast< int >(min), static_cast< int >(max)};

						for ( auto & value : range )
						{
							value = static_cast<number_t>(distribution(m_generator));
						}
					}
					else
					{
						std::uniform_int_distribution< number_t > distribution{min, max};

						for ( auto & value : range )
						{
							value = distribution(m_generator);
						}
					}
				}
				else
				{
					std::uniform_real_distribution< number_t > distribution{min, max};

					for ( auto & value : range )
					{
						value = distribution(m_generator);
					}
				}

				return range;
			}

			/**
			 * @brief Returns an STL array of random number between a specified range.
			 * @tparam count_t The number of desired numbers.
			 * @param min The minimum number.
			 * @param max The maximum number.
			 * @return number_t
			 */
			template< size_t count_t >
			[[nodiscard]]
			std::array< number_t, count_t >
			array (number_t min, number_t max) noexcept
			{
				std::array< number_t, count_t > range{};

				if ( max < min )
				{
					std::swap(min, max);
				}

				if constexpr ( std::is_integral_v< number_t > )
				{
					std::uniform_int_distribution< number_t > distribution{min, max};

					for ( auto & value : range )
					{
						value = distribution(m_generator);
					}
				}
				else
				{
					std::uniform_real_distribution< number_t > distribution{min, max};

					for ( auto & value : range )
					{
						value = distribution(m_generator);
					}
				}

				return range;
			}

		private:

			std::random_device m_device{};
			generator_t m_generator;
	};
}
