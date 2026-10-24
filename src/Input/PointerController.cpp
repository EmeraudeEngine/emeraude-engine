/*
 * src/Input/PointerController.cpp
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

#include "PointerController.hpp"

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
	std::array< std::atomic< uint8_t >, 8 > PointerController::s_deviceState{};
	std::array< std::atomic< uint8_t >, 8 > PointerController::s_injectedState{};

	bool
	PointerController::isButtonPressed (MouseButton button) const noexcept
	{
		/* NOTE: public API — an out-of-range button is "not pressed", never an out-of-bounds read. */
		if ( m_disabled || static_cast< size_t >(button) >= s_deviceState.size() )
		{
			return false;
		}

		return std::next(s_deviceState.cbegin(), static_cast< std::ptrdiff_t >(button))->load(std::memory_order_relaxed) != 0;
	}

	bool
	PointerController::isAnyButtonPressed () const noexcept
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
	PointerController::isButtonReleased (MouseButton button) const noexcept
	{
		if ( m_disabled || static_cast< size_t >(button) >= s_deviceState.size() )
		{
			return true;
		}

		return std::next(s_deviceState.cbegin(), static_cast< std::ptrdiff_t >(button))->load(std::memory_order_relaxed) == 0;
	}

	void
	PointerController::injectButtonState (int32_t button, bool held) noexcept
	{
		if ( button >= 0 && static_cast< size_t >(button) < s_injectedState.size() )
		{
			std::next(s_injectedState.begin(), button)->store(held ? 1 : 0, std::memory_order_relaxed);
			std::next(s_deviceState.begin(), button)->store(held ? 1 : 0, std::memory_order_relaxed);
		}
	}

	std::string
	PointerController::getRawState () const noexcept
	{
		std::stringstream output;

		output << "Pointer mapping." "\n";

		int32_t button = GLFW_MOUSE_BUTTON_1;

		for ( const auto & state : s_deviceState )
		{
			output << "Button #" << button << ( state.load(std::memory_order_relaxed) != 0 ? " : Pressed" "\n" : " : Released" "\n" );

			++button;
		}

		return output.str();
	}

	void
	PointerController::readDeviceState (const Window & window) noexcept
	{
		/* NOTE: Read in one go into a local buffer, then published button by button. */
		std::array< char, 8 > hardware{};

#ifdef GLFW_EM_CUSTOM_VERSION
		glfwGetMouseButtonState(window.handle(), hardware.data());
#else
		for ( int32_t button = GLFW_MOUSE_BUTTON_1; button < GLFW_MOUSE_BUTTON_LAST + 1; button++ )
		{
			hardware[static_cast< size_t >(button)] = glfwGetMouseButton(window.handle(), button) == GLFW_PRESS ? 1 : 0;
		}
#endif

		/* The buttons held by injection stay held (a console mousePress) whatever the hardware says. */
		for ( size_t button = 0; button < s_deviceState.size(); ++button )
		{
			const auto offset = static_cast< std::ptrdiff_t >(button);
			const auto pressed = *std::next(hardware.cbegin(), offset) != 0;
			const auto injected = std::next(s_injectedState.cbegin(), offset)->load(std::memory_order_relaxed) != 0;

			std::next(s_deviceState.begin(), offset)->store(pressed || injected ? 1 : 0, std::memory_order_relaxed);
		}

		/* NOTE: glfwGetCursorPos() was removed from here, because of high CPU usage. */
	}
}
