/*
 * src/Libs/Utility.hpp
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
#include <cstdlib> // std::rand
#include <cmath> // std::pow, std::round
#include <type_traits>
#include <functional> // std::function
#include <algorithm> // std::any_of
#include <vector>
#include <chrono>

namespace EmEn::Libs::Utility
{
	/**
	 * @brief Execute the function a numerous times.
	 * @param count The number of times.
	 * @param loopScope A reference to a function.
	 * @return void
	 */
	inline
	void
	forCount (size_t count, const std::function< void () > & loopScope)
	{
		while ( count-- != 0 )
		{
			loopScope();
		}
	}

	/**
	 * @brief Execute the function a numerous times.
	 * @param count The number of times.
	 * @param loopScope A reference to a function.
	 * @return void
	 */
	inline
	void
	forCount (size_t count, const std::function< void (size_t) > & loopScope)
	{
		for ( size_t index = 0; index < count; ++index )
		{
			loopScope(index);
		}
	}

	/**
	 * @brief Contains a method version for STL vector using a pair as key/value.
	 * @tparam x_t The key and needle type.
	 * @tparam y_t The value type.
	 * @param haystack A reference to a vector of pairs.
	 * @param needle A reference to the key.
	 * @return bool
	 */
	template< typename x_t, typename y_t >
	[[nodiscard]]
	bool
	contains (const std::vector< std::pair< x_t, y_t > > & haystack, const x_t & needle) noexcept(noexcept(needle == std::declval< x_t >()))
	{
		return std::any_of(haystack.cbegin(), haystack.cend(), [&needle] (const auto & item) {
			return item.first == needle;
		});
	}

	/**
	 * @brief Returns the number between boundaries or on extreme number.
	 * @tparam number_t The type of the number. Default float.
	 * @param value The number to be tested.
	 * @param lowest The lowest number allowed.
	 * @param highest The highest number allowed.
	 * @return number_t
	 */
	template< typename number_t = float >
	[[deprecated("Use STL from C++17 : std::clamp(value, lowest, highest)")]]
	[[nodiscard]]
	number_t
	between (number_t value, number_t lowest, number_t highest) noexcept requires (std::is_arithmetic_v< number_t >)
	{
		if ( value < lowest )
		{
			return lowest;
		}

		if ( value > highest )
		{
			return highest;
		}

		return value;
	}

	/**
	 * @brief Gets the celled result of a division.
	 * @tparam number_t The type of the number. Default uint32_t.
	 * @note This function will return 0 if the divisor is 0.
	 * @param dividend The dividend.
	 * @param divisor The divisor.
	 * @return number_t
	 */
	template< typename number_t = uint32_t >
	[[nodiscard]]
	constexpr
	number_t
	ceilDivision (number_t dividend, number_t divisor) noexcept requires (std::is_integral_v< number_t >)
	{
		if ( divisor == 0 )
		{
			return 0;
		}

		return (dividend / divisor) + (dividend % divisor != 0);
	}

	/**
	 * @brief Returns the ratio between two numbers.
	 * @tparam number_t The type of floating point number. Default float.
	 * @param total The divisor.
	 * @param part The dividend.
	 * @param scale Scale the output number.
	 * @return number_t
	 */
	template< typename number_t >
	[[nodiscard]]
	constexpr
	number_t
	ratio (number_t total, number_t part, number_t scale) noexcept requires (std::is_floating_point_v< number_t >)
	{
		if ( total == 0 )
		{
			return 0;
		}

		return (part / total) * scale;
	}

	/**
	 * @brief Returns the ratio between two numbers.
	 * @tparam number_t The type of integral number. Default int32_t.
	 * @param total The divisor.
	 * @param part The dividend.
	 * @param scale Scale the output number.
	 * @return number_t
	 */
	template< typename number_t = int32_t >
	[[nodiscard]]
	constexpr
	number_t
	ratio (number_t total, number_t part, float scale = 1.0F) noexcept requires (std::is_integral_v< number_t >)
	{
		if ( total == 0 )
		{
			return 0;
		}

		return static_cast< number_t >(static_cast< float >(part) / static_cast< float >(total) * scale);
	}

	/**
	 * @brief Rounds up a number.
	 * @tparam number_t The type of floating point number. Default float.
	 * @param value The number to round.
	 * @param precision The amount of decimal.
	 * @return number_t
	 */
	template< typename number_t = float >
	[[nodiscard]]
	number_t
	round (number_t value, size_t precision = 0) requires (std::is_floating_point_v< number_t >)
	{
		auto scale = static_cast< number_t >(std::pow(10, precision));

		return std::round(value * scale) / scale;
	}

	/**
	 * @brief Returns a random integer between a specified range.
	 * @warning This function uses the rand() function from C, this old function is predictable and not thread-safe. Use EmEn::Libs::Randomizer class instead.
	 * @tparam number_t The type of integer number. Default int32_t.
	 * @param min The minimum number.
	 * @param max The maximum number.
	 * @return number_t
	 */
	template< typename number_t = int32_t >
	[[nodiscard]]
	number_t
	quickRandom (number_t min, number_t max) requires (std::is_integral_v< number_t >)
	{
		if ( min > max )
		{
			std::swap(min, max);
		}

		const auto delta = (1 + max - min);

		if ( delta <= 0 )
		{
			return min;
		}

		return (static_cast< number_t >(std::rand()) % delta) + min; // NOLINT
	}

	/**
	 * @brief Returns a random float between a specified range.
	 * @warning This function uses the rand() function from C, this old function is predictable and not thread-safe. Use EmEn::Libs::Randomizer class instead.
	 * @tparam number_t The type of floating point number. Default float.
	 * @param min The minimum number.
	 * @param max The maximum number.
	 * @return number_t
	 */
	template< typename number_t = float >
	[[nodiscard]]
	number_t
	quickRandom (number_t min, number_t max) requires (std::is_floating_point_v< number_t >)
	{
		if ( max < min )
		{
			std::swap(min, max);
		}

		const auto random = static_cast< number_t >(std::rand()) / static_cast< number_t >(RAND_MAX); // NOLINT
		const auto range = max - min;

		return (random * range) + min;
	}

	/**
	 * @brief Checks if two floating numbers are equal.
	 * @tparam number_t The type of floating point number. Default float.
	 * @param operandA The first number.
	 * @param operandB The second number.
	 * @param epsilon The floating point tolerance value. Default C++ epsilon.
	 * @return bool
	 */
	template< typename number_t = float >
	[[nodiscard]]
	bool
	equal (number_t operandA, number_t operandB, number_t epsilon = std::numeric_limits< number_t >::epsilon()) noexcept requires (std::is_floating_point_v< number_t >)
	{
		return std::abs(operandA - operandB) <= epsilon;
	}

	/**
	 * @brief Checks if two floating numbers are different.
	 * @tparam number_t The type of floating point number. Default float.
	 * @param operandA The first number.
	 * @param operandB The second number.
	 * @param epsilon The floating point tolerance value. Default C++ epsilon.
	 * @return bool
	 */
	template< typename number_t = float >
	[[nodiscard]]
	bool
	different (number_t operandA, number_t operandB, number_t epsilon = std::numeric_limits< number_t >::epsilon()) noexcept requires (std::is_floating_point_v< number_t >)
	{
		return std::abs(operandA - operandB) > epsilon;
	}

	/**
	 * @brief Checks if a floating number is zero.
	 * @tparam number_t The type of floating point number.
	 * @param value The number to test.
	 * @param epsilon The floating point tolerance value. Default C++ epsilon.
	 * @return bool
	 */
	template< typename number_t = float >
	[[nodiscard]]
	bool
	isZero (number_t value, number_t epsilon = std::numeric_limits< number_t >::epsilon()) noexcept requires (std::is_arithmetic_v< number_t >)
	{
		if constexpr ( std::is_integral_v< number_t > )
		{
			return value == 0;
		}

		return std::abs(value) <= epsilon;
	}

	/**
	 * @brief Replaces a number if it's below the tolerated zero.
	 * @tparam number_t The type of floating point number. Default float.
	 * @param value The number to test.
	 * @param replacement The number to replace with if the test failed.
	 * @param epsilon The floating point tolerance value. Default C++ epsilon.
	 * @return number_t
	 */
	template< typename number_t = float >
	[[nodiscard]]
	number_t
	ifZero (number_t value, number_t replacement, number_t epsilon = std::numeric_limits< number_t >::epsilon()) noexcept requires (std::is_floating_point_v< number_t >)
	{
		return std::abs(value) <= epsilon ? replacement : value;
	}

	/**
	 * @brief Checks if a floating number is one.
	 * @tparam number_t The type of floating point number.
	 * @param value The number to test.
	 * @param epsilon The floating point tolerance value. Default C++ epsilon.
	 * @return bool
	 */
	template< typename number_t = float >
	[[nodiscard]]
	bool
	isOne (number_t value, number_t epsilon = std::numeric_limits< number_t >::epsilon()) noexcept requires (std::is_floating_point_v< number_t >)
	{
		return equal(value, static_cast< number_t >(1), epsilon);
	}

	/**
	 * @brief Returns a random seed for the perlin noise generator based on UNIX timestamp.
	 * @return int32_t
	 */
	[[nodiscard]]
	inline
	int32_t
	timeBasedSeed () noexcept
	{
		const auto now_count = std::chrono::system_clock::now().time_since_epoch().count();

		/* NOTE: "Folds" the 64-bit value into 32-bit by combining the high and low parts. */
		const auto unsigned_count = static_cast< uint64_t >(now_count);
		const auto high_bits = static_cast< uint32_t >(unsigned_count >> 32);
		const auto low_bits  = static_cast< uint32_t >(unsigned_count);

		return static_cast< int32_t >(high_bits ^ low_bits);
	}
}
