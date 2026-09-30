/*
 * src/Input/JoystickController.cpp
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

#include "JoystickController.hpp"

/* STL inclusions. */
#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <sstream>

/* Third-party inclusions. */
#include "GLFW/glfw3.h"

/* Local inclusions. */
#include "Tracer.hpp"
#include "Types.hpp"

namespace EmEn::Input
{
	/* Out-of-line definition (single instance in the DLL) — see the declaration in the header. */
	std::array< JoystickState, DeviceCount > JoystickController::s_devicesState{};

	bool
	JoystickController::usable () const noexcept
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

	void
	JoystickController::attachDeviceID (int32_t deviceID) noexcept
	{
		if ( deviceID >= DeviceCount )
		{
			TraceError{ClassId} << "Unable to attach a device ID higher than " << DeviceCount;

			return;
		}

		m_deviceID = deviceID;
	}

	float
	JoystickController::axeValue (JoystickAxis axeIndex) const noexcept
	{
		/* NOTE: it read the axis only when the device was NOT usable — a connected joystick always answered 0, and with
		 * no device `.at(-1)` threw, i.e. std::terminate (triad 2026-09-30). */
		if ( this->usable() && static_cast< size_t >(axeIndex) < JoystickMaxAxis )
		{
			const auto value = s_devicesState[static_cast< size_t >(m_deviceID)].axes[axeIndex];

			if ( value > m_threshold || value < -m_threshold )
			{
				return value * m_multiplier;
			}
		}

		return 0.0F;
	}

	bool
	JoystickController::isButtonPressed (int32_t buttonNum) const noexcept
	{
		if ( !this->usable() )
		{
			return false;
		}

		/* NOTE: public API — checked in Release too, and `>=` (it was `>`, in Debug only; triad 2026-09-30). */
		if ( buttonNum < 0 || buttonNum >= JoystickMaxButtons )
		{
			return false;
		}

		return s_devicesState[static_cast< size_t >(m_deviceID)].buttons[static_cast< size_t >(buttonNum)];
	}

	bool
	JoystickController::isButtonReleased (int32_t buttonNum) const noexcept
	{
		if ( !this->usable() )
		{
			return true;
		}

		/* NOTE: public API — checked in Release too, and `>=` (it was `>`, in Debug only; triad 2026-09-30). */
		if ( buttonNum < 0 || buttonNum >= JoystickMaxButtons )
		{
			return true;
		}

		return !s_devicesState[static_cast< size_t >(m_deviceID)].buttons[static_cast< size_t >(buttonNum)];
	}

	JoystickHatDirection
	JoystickController::hatValue (int32_t hatNum) const noexcept
	{
		if ( !this->usable() )
		{
			return Center;
		}

		/* NOTE: public API — checked in Release too, and `>=` (it was `>`, in Debug only; triad 2026-09-30). */
		if ( hatNum < 0 || hatNum >= JoystickMaxHats )
		{
			return Center;
		}

		return s_devicesState[static_cast< size_t >(m_deviceID)].hats[static_cast< size_t >(hatNum)];
	}

	std::string
	JoystickController::getRawState () const noexcept
	{
		if ( m_deviceID < 0 || m_deviceID >= DeviceCount )
		{
			return "No joystick connected !" "\n";
		}

		std::stringstream output;
		output << "Joystick #" << m_deviceID << " mapping." "\n";

		const auto & [axes, buttons, hats] = s_devicesState[m_deviceID];

		for ( int32_t axe = 0; axe < JoystickMaxAxis; axe++ )
		{
			output << "Axe #" << axe << " : " << axes[axe] << '\n';
		}

		for ( int32_t button = 0; button < JoystickMaxButtons; button++ )
		{
			if ( buttons[button] )
			{
				output << "Button #" << button << " : Pressed" "\n";
			}
			else
			{
				output << "Button #" << button << " : Released" "\n";
			}
		}

		for ( int32_t hat = 0; hat < JoystickMaxHats; hat++ )
		{
			output << "Hat #" << hat << " : " << static_cast< int >(hats[hat]) << '\n';
		}

		return output.str();
	}

	void
	JoystickController::readDeviceState (int32_t deviceID) noexcept
	{
		if ( deviceID < 0 || deviceID >= DeviceCount )
		{
			return;
		}

		auto & [axes, buttons, hats] = s_devicesState[deviceID];

		int32_t count = 0;

		const auto * currentAxes = glfwGetJoystickAxes(deviceID, &count);

		for ( int32_t index = 0; index < std::min(count, JoystickMaxAxis); index++ )
		{
			axes[index] = currentAxes[index];
		}

		const auto * currentButtons = glfwGetJoystickButtons(deviceID, &count);

		for ( int32_t index = 0; index < std::min(count, JoystickMaxButtons); index++ )
		{
			buttons[index] = currentButtons[index] == GLFW_PRESS;
		}

		currentButtons = glfwGetJoystickHats(deviceID, &count);

		for ( int32_t index = 0; index < std::min(count, JoystickMaxHats); index++ )
		{
			hats[index] = static_cast< JoystickHatDirection >(currentButtons[index]);
		}
	}

	void
	JoystickController::clearDeviceState (int32_t deviceID) noexcept
	{
		if ( deviceID < 0 || deviceID >= DeviceCount )
		{
			return;
		}

		auto & [axes, buttons, hats] = s_devicesState[deviceID];

		for ( auto & axis : axes )
		{
			axis = 0.0F;
		}

		for ( auto & button : buttons )
		{
			button = false;
		}

		for ( auto & hat : hats )
		{
			hat = Center;
		}
	}
}
