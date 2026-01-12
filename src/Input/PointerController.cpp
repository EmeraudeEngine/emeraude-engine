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
 * https://github.com/londnoir/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#include "PointerController.hpp"

/* STL inclusions. */
#include <cstddef>
#include <algorithm>
#include <sstream>

/* Third-party libraries */
#define GLFW_INCLUDE_NONE
#include "GLFW/glfw3.h"

/* Local inclusions. */
#include "Window.hpp"

namespace EmEn::Input
{
	std::array< char, 8 > PointerController::s_deviceState{};

	bool
	PointerController::isButtonPressed (MouseButton button) const noexcept
	{
		if ( m_disabled )
		{
			return false;
		}

		return s_deviceState[static_cast< size_t >(button)];
	}

	bool
	PointerController::isAnyButtonPressed () const noexcept
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
	PointerController::isButtonReleased (MouseButton button) const noexcept
	{
		if ( m_disabled )
		{
			return true;
		}

		return !s_deviceState[static_cast< size_t >(button)];
	}

	std::string
	PointerController::getRawState () const noexcept
	{
		std::stringstream output;

		output << "Pointer mapping." "\n";

		for ( int32_t button = GLFW_MOUSE_BUTTON_1; button < GLFW_MOUSE_BUTTON_LAST + 1; button++ )
		{
			if ( s_deviceState.at(static_cast< size_t >(button)) )
			{
				output << "Button #" << button << " : Pressed" "\n";
			}
			else
			{
				output << "Button #" << button << " : Released" "\n";
			}
		}

		return output.str();
	}

	void
	PointerController::readDeviceState (const Window & window) noexcept
	{
#ifdef GLFW_EM_CUSTOM_VERSION
		glfwGetMouseButtonState(window.handle(), s_deviceState.data());
#else
		for ( int32_t button = GLFW_MOUSE_BUTTON_1; button < GLFW_MOUSE_BUTTON_LAST + 1; button++ )
		{
			s_deviceState.at(static_cast< size_t >(button)) = glfwGetMouseButton(window.handle(), button) == GLFW_PRESS;
		}
#endif

		/* NOTE: glfwGetCursorPos() was removed from here, because of high CPU usage. */
	}
}
