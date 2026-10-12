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
 * https://github.com/EmeraudeEngine/emeraude-engine
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
	 * @brief How many copies of the published logic state exist (the logic → render hand-off).
	 * @note ⚠️ THREE, never two. The logic thread publishes at a fixed 60 Hz while the render thread
	 * reads the state it latched for the whole frame (Scene::beginRenderFrame()). With two slots the
	 * logic can only alternate, so its SECOND publication inside one frame lands on the slot the frame
	 * is reading: every frame longer than a logic tick mixed two ticks (the camera of the early draws
	 * against the camera of the late ones — objects "sliding" on each other while the camera turns).
	 * Measured on 2026-09-24 before the fix: 41-66 % of the frames on `terrain`, 11-16 % on `sponza`.
	 * Three slots make it a lock-free triple buffer: the render thread owns one, the logic thread
	 * writes another, the third holds the latest publication, and the exchanges keep them disjoint
	 * (Scene::publishStateForRendering(), Scene::beginRenderFrame()).
	 */
	constexpr uint32_t RenderStateSlotCount{3};

	/**
	 * @brief Default ceiling frequency (Hz) of the engine main event loop.
	 * @note 100 Hz => a 0.010 s waitSystemEvents() timeout. This is an idle tick ceiling,
	 * not a fixed period: the loop returns earlier when OS events arrive. Overridable via
	 * the Core constructor's mainLoopFrequencyHz parameter.
	 */
	template< typename number_t = uint32_t >
	requires (std::is_arithmetic_v< number_t >)
	constexpr number_t DefaultMainLoopFrequencyHz{100};

	/**
	 * @brief Safety re-check cadence (Hz) of the on-demand rendering wait.
	 * @note In RenderingMode::OnDemand the rendering thread sleeps until a redraw is
	 * requested (Core::requestRedraw()). This 60 Hz ceiling bounds how long a defensively
	 * assumed missed dirty signal can delay a refresh: the wait re-evaluates at least this
	 * often. It does NOT force a frame on timeout, so the GPU stays idle when nothing changed.
	 * @tparam number_t The type of number. Default uint32_t.
	 */
	template< typename number_t = uint32_t >
	requires (std::is_arithmetic_v< number_t >)
	constexpr number_t OnDemandRenderingSafetyRefreshHz{60};

	/**
	 * @brief Returns how many times the world physics simulation is updated in one second.
	 * @note Fixed-timestep cadence of the logics loop (Core::logicsTask). Distinct from the
	 * engine main event-loop frequency (see the Core constructor's mainLoopFrequencyHz parameter).
	 * @tparam number_t The type of number. Default uint32_t.
	 */
	template< typename number_t = uint32_t >
	requires (std::is_arithmetic_v< number_t >)
	constexpr number_t WorldPhysicsUpdateFrequency{60};

	/**
	 * @brief Returns the duration of one cycle in a second.
	 * @note Typically 1 / 60 = 0.0166...
	 * @tparam number_t The type of float point number. Default float.
	 */
	template< typename number_t = float >
	requires (std::is_arithmetic_v< number_t >)
	constexpr number_t WorldPhysicsUpdateCycleDurationS{static_cast< number_t >(1) / WorldPhysicsUpdateFrequency< number_t >};

	/**
	 * @brief Returns in milliseconds the duration of one cycle.
	 * @note Typically 1000 / 60 = 16.6666...
	 * @tparam number_t The type of float point number. Default float.
	 */
	template< typename number_t = float >
	requires (std::is_arithmetic_v< number_t >)
	constexpr number_t WorldPhysicsUpdateCycleDurationMS{static_cast< number_t >(1000) / WorldPhysicsUpdateFrequency< number_t >};

	/**
	 * @brief Returns in microseconds the duration of one cycle.
	 * @note Typically 1,000,000 / 60 = 16666.6666...
	 * @tparam number_t The type of float point number. Default float.
	 */
	template< typename number_t = float >
	requires (std::is_arithmetic_v< number_t >)
	constexpr number_t WorldPhysicsUpdateCycleDurationUS{static_cast< number_t >(1000000) / WorldPhysicsUpdateFrequency< number_t >};

	/**
	 * @brief Returns the time of a number of logic cycles in milliseconds, exact (rounded down).
	 * @note COUNT the cycles and derive the time from them. Adding WorldPhysicsUpdateCycleDurationMS< uint32_t > (16,
	 * not 16.67) once per cycle runs 4.2 % slow; a float accumulator drifts (engine docs/caution-points.md, tick timers).
	 * 64-bit: no overflow below 3 × 10^14 years of cycles.
	 * @param cycles The number of logic cycles.
	 * @return uint64_t
	 */
	[[nodiscard]]
	constexpr
	uint64_t
	logicCyclesToMilliseconds (uint64_t cycles) noexcept
	{
		return cycles * 1000U / WorldPhysicsUpdateFrequency< uint64_t >;
	}

	/**
	 * @brief Returns the time of a number of logic cycles in microseconds, exact (rounded down).
	 * @note See logicCyclesToMilliseconds(): 1,000,000 / 60 is 16666.67, never accumulate the truncated 16666.
	 * @param cycles The number of logic cycles.
	 * @return uint64_t
	 */
	[[nodiscard]]
	constexpr
	uint64_t
	logicCyclesToMicroseconds (uint64_t cycles) noexcept
	{
		return cycles * 1000000U / WorldPhysicsUpdateFrequency< uint64_t >;
	}

	/**
	 * @brief Returns how many logic cycles a duration in milliseconds lasts, rounded UP (a non-zero duration lasts at
	 * least one cycle, and logicCyclesToMilliseconds() of the result is never below the duration).
	 * @param milliseconds The duration in milliseconds.
	 * @return uint64_t
	 */
	[[nodiscard]]
	constexpr
	uint64_t
	millisecondsToLogicCycles (uint64_t milliseconds) noexcept
	{
		return (milliseconds * WorldPhysicsUpdateFrequency< uint64_t > + 999U) / 1000U;
	}

	static_assert(logicCyclesToMilliseconds(900) == 15000);
	static_assert(logicCyclesToMilliseconds(899) == 14983);
	static_assert(logicCyclesToMicroseconds(3) == 50000);
	static_assert(millisecondsToLogicCycles(0) == 0);
	static_assert(millisecondsToLogicCycles(1) == 1);
	static_assert(millisecondsToLogicCycles(1000) == 60);
	static_assert(millisecondsToLogicCycles(15000) == 900);
	static_assert(millisecondsToLogicCycles(17) == 2);

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
