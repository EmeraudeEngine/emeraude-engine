/*
 * src/Input/KeyboardController.cpp
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

#include "KeyboardController.hpp"

/* STL inclusions. */
#include <algorithm>
#include <cstddef>
#include <iterator>
#include <sstream>

/* Third-party inclusions. */
#include "GLFW/glfw3.h"

/* Local inclusions. */
#include "Window.hpp"

namespace EmEn::Input
{
	std::array< std::atomic< uint8_t >, 349 > KeyboardController::s_deviceState{};
	std::array< std::atomic< uint8_t >, 349 > KeyboardController::s_injectedState{};

	bool
	KeyboardController::isKeyPressed (int32_t key) const noexcept
	{
		/* NOTE: public API — any int reaches it: out of range (KeyUnknown included) is "not pressed", never an
		 * out-of-bounds read (triad 2026-09-30). */
		if ( m_disabled || key < 0 || static_cast< size_t >(key) >= s_deviceState.size() )
		{
			return false;
		}

		return std::next(s_deviceState.cbegin(), key)->load(std::memory_order_relaxed) != 0;
	}

	bool
	KeyboardController::isAnyKeyPressed () const noexcept
	{
		if ( m_disabled )
		{
			return false;
		}

		return std::ranges::any_of(s_deviceState, [] (const std::atomic< uint8_t > & state) {
			return state.load(std::memory_order_relaxed) != 0;
		});
	}

	bool
	KeyboardController::isKeyReleased (int32_t key) const noexcept
	{
		if ( m_disabled || key < 0 || static_cast< size_t >(key) >= s_deviceState.size() )
		{
			return true;
		}

		return std::next(s_deviceState.cbegin(), key)->load(std::memory_order_relaxed) == 0;
	}

	void
	KeyboardController::changeKeyState (int32_t key, bool pressed) noexcept
	{
		/* NOTE: KeyUnknown (-1) and any other out-of-range code are ignored — an out-of-bounds WRITE otherwise. */
		if ( key >= 0 && static_cast< size_t >(key) < s_deviceState.size() )
		{
			std::next(s_deviceState.begin(), key)->store(pressed ? 1 : 0, std::memory_order_relaxed);
		}
	}

	void
	KeyboardController::injectKeyState (int32_t key, bool held) noexcept
	{
		if ( key >= 0 && static_cast< size_t >(key) < s_injectedState.size() )
		{
			std::next(s_injectedState.begin(), key)->store(held ? 1 : 0, std::memory_order_relaxed);
			std::next(s_deviceState.begin(), key)->store(held ? 1 : 0, std::memory_order_relaxed);
		}
	}

	std::string
	KeyboardController::getRawState () const noexcept
	{
		std::stringstream output;

		output << "Keyboard mapping." "\n";

		int32_t key = 0;

		for ( const auto & state : s_deviceState )
		{
			if ( key >= GLFW_KEY_SPACE )
			{
				output << "Key #" << key << ( state.load(std::memory_order_relaxed) != 0 ? " : Pressed" "\n" : " : Released" "\n" );
			}

			++key;
		}

		return output.str();
	}

	void
	KeyboardController::readDeviceState (const Window & window) noexcept
	{
		/* NOTE: The hardware state is read in one go into a local buffer, then published key by key. GLFW knows the keys
		 * from GLFW_KEY_SPACE on; below it, only an injection can hold a code. */
		std::array< char, 349 > hardware{};

#ifdef GLFW_EM_CUSTOM_VERSION
		glfwGetKeyboardState(window.handle(), hardware.data());
#else
		for ( int32_t key = GLFW_KEY_SPACE; key < GLFW_KEY_LAST + 1; key++ )
		{
			hardware[static_cast< size_t >(key)] = glfwGetKey(window.handle(), key) == GLFW_PRESS ? 1 : 0;
		}
#endif

		/* The keys held by injection stay held (a console keyDown) whatever the hardware says. */
		auto hardwareIt = hardware.cbegin();
		auto injectedIt = s_injectedState.cbegin();

		for ( auto & state : s_deviceState )
		{
			const auto injected = injectedIt->load(std::memory_order_relaxed) != 0;

			state.store(*hardwareIt != 0 || injected ? 1 : 0, std::memory_order_relaxed);

			++hardwareIt;
			++injectedIt;
		}
	}
}
