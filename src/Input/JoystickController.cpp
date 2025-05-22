/*
 * src/Input/JoystickController.cpp
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

#include "JoystickController.hpp"

/* STL inclusions. */
#include <cstddef>
#include <cstdlib>
#include <algorithm>
#include <iostream>
#include <sstream>

/* Third-party libraries. */
#define GLFW_INCLUDE_NONE
#include "GLFW/glfw3.h"

/* Local inclusions. */
#include "Types.hpp"
#include "Tracer.hpp"

namespace EmEn::Input
{
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
	JoystickController::disable (bool state) noexcept
	{
		m_disabled = state;
	}

	bool
	JoystickController::disabled () const noexcept
	{
		return m_disabled;
	}

	bool
	JoystickController::isConnected () const noexcept
	{
		return m_deviceID > -1 && m_deviceID <= DeviceCount;
	}

	int32_t
	JoystickController::deviceID () const noexcept
	{
		return m_deviceID;
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

	void
	JoystickController::setAxisThreshold (float value) noexcept
	{
		m_threshold = std::abs(value);
	}

	void
	JoystickController::setAxisSensitivity (float multiplier) noexcept
	{
		m_multiplier = std::abs(multiplier);
	}

	float
	JoystickController::axisThreshold () const noexcept
	{
		return m_threshold;
	}

	float
	JoystickController::axisSensitivity () const noexcept
	{
		return m_multiplier;
	}

	float
	JoystickController::axeValue (JoystickAxis axeIndex) const noexcept
	{
		if ( !this->usable() )
		{
			auto value = s_devicesState.at(m_deviceID).axes[axeIndex];

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

		if constexpr ( IsDebug )
		{
			if ( static_cast< size_t >(buttonNum) > JoystickMaxButtons )
			{
				std::cerr << "Buttons limit is " << JoystickMaxButtons << " !" "\n";

				return false;
			}
		}

		return s_devicesState.at(m_deviceID).buttons[buttonNum];
	}

	bool
	JoystickController::isButtonReleased (int32_t buttonNum) const noexcept
	{
		if ( !this->usable() )
		{
			return true;
		}

		if constexpr ( IsDebug )
		{
			if ( static_cast< size_t >(buttonNum) > JoystickMaxButtons )
			{
				std::cerr << "Buttons limit is " << JoystickMaxButtons << " !" "\n";

				return true;
			}
		}

		return !s_devicesState.at(m_deviceID).buttons[buttonNum];
	}

	JoystickHatDirection
	JoystickController::hatValue (int32_t hatNum) const noexcept
	{
		if ( !this->usable() )
		{
			return Center;
		}

		if constexpr ( IsDebug )
		{
			if ( static_cast< size_t >(hatNum) > JoystickMaxHats )
			{
				std::cerr << "Hats limit is " << JoystickMaxHats << " !" "\n";

				return Center;
			}
		}

		return s_devicesState.at(m_deviceID).hats[hatNum];
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
			output << "Hat #" << hat << " : " << hats[hat] << '\n';
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
