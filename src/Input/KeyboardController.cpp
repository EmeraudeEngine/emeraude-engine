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
 * https://github.com/londnoir/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#include "KeyboardController.hpp"

/* STL inclusions. */
#include <cstddef>
#include <algorithm>
#include <sstream>

/* Third-party libraries */
#define GLFW_INCLUDE_NONE
#include "GLFW/glfw3.h"

/* Local usage. */
#include "Window.hpp"

namespace EmEn::Input
{
	std::array< char, 349 > KeyboardController::s_deviceState{};

	bool
	KeyboardController::isKeyPressed (int32_t key) const noexcept
	{
		if ( m_disabled || key == KeyUnknown )
		{
			return false;
		}

		return s_deviceState[static_cast< size_t >(key)];
	}

	bool
	KeyboardController::isAnyKeyPressed () const noexcept
	{
		if ( m_disabled )
		{
			return false;
		}

		return std::ranges::any_of(s_deviceState, [] (auto state) {
			return state;
		});
	}

	bool
	KeyboardController::isKeyReleased (int32_t key) const noexcept
	{
		if ( m_disabled || key == KeyUnknown )
		{
			return true;
		}

		return !s_deviceState[static_cast< size_t >(key)];
	}

	void
	KeyboardController::changeKeyState (int32_t key, bool pressed) noexcept
	{
		if ( key != KeyUnknown )
		{
			s_deviceState[static_cast< size_t >(key)] = pressed;
		}
	}

	std::string
	KeyboardController::getRawState () const noexcept
	{
		std::stringstream output;

		output << "Keyboard mapping." "\n";

		for ( int32_t key = GLFW_KEY_SPACE; key < GLFW_KEY_LAST + 1; key++ )
		{
			if ( s_deviceState.at(static_cast< size_t >(key)) )
			{
				output << "Key #" << key << " : Pressed" "\n";
			}
			else
			{
				output << "Button #" << key << " : Released" "\n";
			}
		}

		return output.str();
	}

	void
	KeyboardController::readDeviceState (const Window & window) noexcept
	{
#ifdef GLFW_EM_CUSTOM_VERSION
		glfwGetKeyboardState(window.handle(), s_deviceState.data());
#else
		for ( int32_t key = GLFW_KEY_SPACE; key < GLFW_KEY_LAST + 1; key++ )
		{
			s_deviceState[static_cast< size_t >(key)] = glfwGetKey(window.handle(), key) == GLFW_PRESS ? 1 : 0;
		}
#endif
	}
}
