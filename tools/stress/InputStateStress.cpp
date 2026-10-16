/*
 * tools/stress/InputStateStress.cpp
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

/* STL inclusions. */
#include <atomic>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <thread>

/* Local inclusions. */
#include "Input/KeyboardController.hpp"

/*
 * The ThreadSanitizer proof of the input state (Ave Robustus II P5, lot 4; built with EMERAUDE_ENABLE_TSAN only).
 * The main thread writes the keyboard state (KeyboardController::changeKeyState(), the GLFW key callback's path, and
 * injectKeyState(), the console's) while a second thread reads it the way the logic thread does (the player's
 * movement: isKeyPressed(), isAnyKeyPressed(), isKeyReleased()). Before lot 4 the state was a plain char array: a data
 * race on every key. The verdict is TSan's: run it with TSAN_OPTIONS=halt_on_error=1 (procedure: projet-alpha
 * docs/unit-testing.md). It needs no window and no device: the paths it drives are the ones without GLFW.
 */

int
main ()
{
	using namespace EmEn::Input;

	/* NOTE: The keys the player reads (W, A, S, D, Space, Left Shift), and a few more. */
	constexpr std::array< int32_t, 8 > Keys{87, 65, 83, 68, 32, 340, 262, 263};
	constexpr auto Duration{std::chrono::seconds{5}};

	std::atomic_bool running{true};
	std::atomic< uint64_t > reads{0};
	std::atomic< uint64_t > pressedSeen{0};

	std::jthread logicThread{[&running, &reads, &pressedSeen, &Keys] () {
		const KeyboardController keyboard;

		while ( running.load(std::memory_order_acquire) )
		{
			for ( const auto key : Keys )
			{
				if ( keyboard.isKeyPressed(key) )
				{
					pressedSeen.fetch_add(1, std::memory_order_relaxed);
				}

				static_cast< void >(keyboard.isKeyReleased(key));
			}

			static_cast< void >(keyboard.isAnyKeyPressed());

			reads.fetch_add(1, std::memory_order_relaxed);
		}
	}};

	uint64_t writes = 0;
	const auto start = std::chrono::steady_clock::now();

	while ( std::chrono::steady_clock::now() - start < Duration )
	{
		for ( const auto key : Keys )
		{
			KeyboardController::changeKeyState(key, (writes & 1U) == 0U);
			KeyboardController::injectKeyState(key, (writes & 2U) == 0U);
		}

		++writes;
	}

	running.store(false, std::memory_order_release);
	logicThread.join();

	std::cout << "InputStateStress: " << writes << " write passes, " << reads.load() << " read passes, " << pressedSeen.load() << " pressed reads." "\n";

	return writes > 0 && reads.load() > 0 && pressedSeen.load() > 0 ? 0 : 1;
}
