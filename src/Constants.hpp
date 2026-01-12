/*
 * src/Constants.hpp
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
#include <type_traits>

namespace EmEn
{
	/**
	 * @brief Returns how many times the engine will be updated in one second.
	 * @tparam number_t The type of number. Default uint32_t.
	 */
	template< typename number_t = uint32_t >
	requires (std::is_arithmetic_v< number_t >)
	constexpr number_t EngineUpdateFrequency{60};

	/**
	 * @brief Returns the duration of one cycle in a second.
	 * @note Typically 1 / 60 = 0.0166...
	 * @tparam number_t The type of float point number. Default float.
	 */
	template< typename number_t = float >
	requires (std::is_arithmetic_v< number_t >)
	constexpr number_t EngineUpdateCycleDurationS{static_cast< number_t >(1) / EngineUpdateFrequency< number_t >};

	/**
	 * @brief Returns in milliseconds the duration of one cycle.
	 * @note Typically 1000 / 60 = 16.6666...
	 * @tparam number_t The type of float point number. Default float.
	 */
	template< typename number_t = float >
	requires (std::is_arithmetic_v< number_t >)
	constexpr number_t EngineUpdateCycleDurationMS{static_cast< number_t >(1000) / EngineUpdateFrequency< number_t >};

	/**
	 * @brief Returns in microseconds the duration of one cycle.
	 * @note Typically 1,000,000 / 60 = 16666.6666...
	 * @tparam number_t The type of float point number. Default float.
	 */
	template< typename number_t = float >
	requires (std::is_arithmetic_v< number_t >)
	constexpr number_t EngineUpdateCycleDurationUS{static_cast< number_t >(1000000) / EngineUpdateFrequency< number_t >};

	/**
	 * @brief Constant for scaling to 0.
	 * @tparam number_t The type of float point number. Default float.
	 */
	template< typename number_t = float >
	requires (std::is_floating_point_v< number_t >)
	constexpr number_t Zero{0};

	/**
	 * @brief Constant for scaling to 0.5.
	 * @tparam number_t The type of float point number. Default float.
	 */
	template< typename number_t = float >
	requires (std::is_floating_point_v< number_t >)
	constexpr number_t Half{0.5};

	/**
	 * @brief Constant for scaling to 1.
	 * @tparam number_t The type of float point number. Default float.
	 */
	template< typename number_t = float >
	requires (std::is_floating_point_v< number_t >)
	constexpr number_t One{1};

	/**
	 * @brief Constant for scaling to 2.
	 * @tparam number_t The type of number. Default float.
	 */
	template< typename number_t = float >
	requires (std::is_arithmetic_v< number_t >)
	constexpr number_t Double{2};

	/**
	 * @brief Constant for scaling to 3.
	 * @tparam number_t The type of number. Default float.
	 */
	template< typename number_t = float >
	requires (std::is_arithmetic_v< number_t >)
	constexpr number_t Triple{3};

	/**
	 * @brief Constant for scaling to 4.
	 * @tparam number_t The type of number. Default float.
	 */
	template< typename number_t = float >
	requires (std::is_arithmetic_v< number_t >)
	constexpr number_t Quadruple{4};

	/**
	 * @brief Constant for computer number based on 8*1.
	 * @tparam number_t The number precision. Default uint32_t.
	 */
	template< typename number_t = uint32_t >
	requires (std::is_arithmetic_v< number_t >)
	constexpr number_t ComputerNumber8{8};

	/**
	 * @brief Constant for computer number based on 8*2.
	 * @tparam number_t The number precision. Default uint32_t.
	 */
	template< typename number_t = uint32_t >
	requires (std::is_arithmetic_v< number_t >)
	constexpr number_t ComputerNumber16{16};

	/**
	 * @brief Constant for computer number based on 8*4.
	 * @tparam number_t The number precision. Default uint32_t.
	 */
	template< typename number_t = uint32_t >
	requires (std::is_arithmetic_v< number_t >)
	constexpr number_t ComputerNumber32{32};

	/**
	 * @brief Constant for computer number based on 8*8.
	 * @tparam number_t The number precision. Default uint32_t.
	 */
	template< typename number_t = uint32_t >
	requires (std::is_arithmetic_v< number_t >)
	constexpr number_t ComputerNumber64{64}; 

	/**
	 * @brief Constant for computer number based on 8*16.
	 * @tparam number_t The number precision. Default uint32_t.
	 */
	template< typename number_t = uint32_t >
	requires (std::is_arithmetic_v< number_t >)
	constexpr number_t ComputerNumber128{128};

	/**
	 * @brief Constant for computer number based on 8*32.
	 * @tparam number_t The number precision. Default uint32_t.
	 */
	template< typename number_t = uint32_t >
	requires (std::is_arithmetic_v< number_t >)
	constexpr number_t ComputerNumber256{256};

	/**
	 * @brief Constant for computer number based on 8*64.
	 * @tparam number_t The number precision. Default uint32_t.
	 */
	template< typename number_t = uint32_t >
	requires (std::is_arithmetic_v< number_t >)
	constexpr number_t ComputerNumber512{512};

	/**
	 * @brief Constant for computer number based on 8*128.
	 * @tparam number_t The number precision. Default uint32_t.
	 */
	template< typename number_t = uint32_t >
	requires (std::is_arithmetic_v< number_t >)
	constexpr number_t ComputerNumber1024{1024};
}
