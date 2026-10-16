/*
 * src/Input/GamepadController.cpp
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

#include "GamepadController.hpp"

/* STL inclusions. */
#include <iterator>
#include <sstream>

namespace EmEn::Input
{
	std::array< GamepadState, DeviceCount > GamepadController::s_devicesState{};

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

		const auto & [buttons, axes] = *std::next(s_devicesState.cbegin(), m_deviceID);

		std::stringstream output;
		output << "Gamepad #" << m_deviceID << " mapping." "\n";

		/* NOTE: Every button and axis (the former index loops stopped one short of each list). */
		int32_t button = GLFW_GAMEPAD_BUTTON_A;

		for ( const auto & state : buttons )
		{
			output << "Button #" << button << ( state.load(std::memory_order_relaxed) != 0U ? " : Pressed" "\n" : " : Released" "\n" );

			++button;
		}

		int32_t axe = 0;

		for ( const auto & value : axes )
		{
			output << "Axe #" << axe << " : " << value.load(std::memory_order_relaxed) << "\n";

			++axe;
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

		GLFWgamepadstate state{};

		if ( glfwGetGamepadState(deviceID, &state) == GLFW_FALSE )
		{
			TraceError{ClassId} << "Gamepad device #" << deviceID << " is not connected !";

			return;
		}

		auto & [buttons, axes] = *std::next(s_devicesState.begin(), deviceID);

		/* NOTE: GamepadState mirrors GLFWgamepadstate's two arrays, element for element. */
		static_assert(std::size(GLFWgamepadstate{}.buttons) == std::tuple_size_v< decltype(GamepadState::buttons) >);
		static_assert(std::size(GLFWgamepadstate{}.axes) == std::tuple_size_v< decltype(GamepadState::axes) >);

		const auto * hardwareButton = std::begin(state.buttons);

		for ( auto & button : buttons )
		{
			button.store(*hardwareButton, std::memory_order_relaxed);

			std::advance(hardwareButton, 1);
		}

		const auto * hardwareAxis = std::begin(state.axes);

		for ( auto & axis : axes )
		{
			axis.store(*hardwareAxis, std::memory_order_relaxed);

			std::advance(hardwareAxis, 1);
		}
	}

	void
	GamepadController::clearDeviceState (int32_t deviceID) noexcept
	{
		if ( deviceID < 0 || deviceID >= DeviceCount )
		{
			return;
		}

		auto & [buttons, axes] = *std::next(s_devicesState.begin(), deviceID);

		for ( auto & button : buttons )
		{
			button.store(0, std::memory_order_relaxed);
		}

		for ( auto & axe : axes )
		{
			axe.store(0.0F, std::memory_order_relaxed);
		}
	}
}
