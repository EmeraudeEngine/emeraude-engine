/*
 * src/Input/GamepadController.cpp
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

#include "GamepadController.hpp"

/* STL inclusions. */
#include <sstream>

namespace EmEn::Input
{
	bool
	GamepadController::usable () const noexcept
	{
		if ( m_deviceID < 0 )
		{
			return false;
		}

		if ( m_deviceID >= DeviceCount )
		{
			return false;
		}

		if ( m_disabled )
		{
			return false;
		}

		return true;
	}

	std::string
	GamepadController::getRawState () const noexcept
	{
		if ( m_deviceID < 0 || m_deviceID >= DeviceCount )
		{
			return "No gamepad connected !" "\n";
		}

		const auto & [buttons, axes] = s_devicesState.at(m_deviceID);

		std::stringstream output;
		output << "Gamepad #" << m_deviceID << " mapping." "\n";

		for ( auto button = GLFW_GAMEPAD_BUTTON_A; button < GLFW_GAMEPAD_BUTTON_LAST; button++ )
		{
			if ( buttons[button] != 0U )
			{
				output << "Button #" << button << " : Pressed" "\n";
			}
			else
			{
				output << "Button #" << button << " : Released" "\n";
			}
		}

		for ( auto axe = 0U; axe < GLFW_GAMEPAD_AXIS_LAST; axe++ )
		{
			output << "Axe #" << axe << " : " << axes[axe] << "\n";
		}

		return output.str();
	}

	void
	GamepadController::readDeviceState (int32_t deviceID) noexcept
	{
		if ( deviceID < 0 || deviceID >= DeviceCount )
		{
			return;
		}

		if ( glfwGetGamepadState(deviceID, &s_devicesState[deviceID]) == GLFW_FALSE )
		{
			TraceError{ClassId} << "Gamepad device #" << deviceID << " is not connected !";
		}
	}

	void
	GamepadController::clearDeviceState (int32_t deviceID) noexcept
	{
		if ( deviceID < 0 || deviceID >= DeviceCount )
		{
			return;
		}

		auto & [buttons, axes] = s_devicesState[deviceID];

		for ( auto & button : buttons )
		{
			button = 0;
		}

		for ( auto & axe : axes )
		{
			axe = 0.0F;
		}
	}
}
