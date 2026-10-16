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
#include <iterator>
#include <span>
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
			const auto value = std::next(std::next(s_devicesState.cbegin(), m_deviceID)->axes.cbegin(), static_cast< std::ptrdiff_t >(axeIndex))->load(std::memory_order_relaxed);

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

		return std::next(std::next(s_devicesState.cbegin(), m_deviceID)->buttons.cbegin(), buttonNum)->load(std::memory_order_relaxed);
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

		return !std::next(std::next(s_devicesState.cbegin(), m_deviceID)->buttons.cbegin(), buttonNum)->load(std::memory_order_relaxed);
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

		return std::next(std::next(s_devicesState.cbegin(), m_deviceID)->hats.cbegin(), hatNum)->load(std::memory_order_relaxed);
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

		const auto & [axes, buttons, hats] = *std::next(s_devicesState.cbegin(), m_deviceID);

		int32_t index = 0;

		for ( const auto & axis : axes )
		{
			output << "Axe #" << index++ << " : " << axis.load(std::memory_order_relaxed) << '\n';
		}

		index = 0;

		for ( const auto & button : buttons )
		{
			output << "Button #" << index++ << ( button.load(std::memory_order_relaxed) ? " : Pressed" "\n" : " : Released" "\n" );
		}

		index = 0;

		for ( const auto & hat : hats )
		{
			output << "Hat #" << index++ << " : " << static_cast< int >(hat.load(std::memory_order_relaxed)) << '\n';
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

		auto & [axes, buttons, hats] = *std::next(s_devicesState.begin(), deviceID);

		/* NOTE: GLFW gives a pointer and a count (null and 0 when the joystick is gone): each list is published up to
		 * the shorter of the two. */
		const auto hardware = [] (const auto * values, int32_t count, size_t capacity) noexcept {
			const auto size = values == nullptr || count <= 0 ? size_t{0} : std::min(static_cast< size_t >(count), capacity);

			return std::span{values, size};
		};

		int32_t count = 0;

		auto axisIt = axes.begin();

		for ( const auto value : hardware(glfwGetJoystickAxes(deviceID, &count), count, axes.size()) )
		{
			(axisIt++)->store(value, std::memory_order_relaxed);
		}

		auto buttonIt = buttons.begin();

		for ( const auto value : hardware(glfwGetJoystickButtons(deviceID, &count), count, buttons.size()) )
		{
			(buttonIt++)->store(value == GLFW_PRESS, std::memory_order_relaxed);
		}

		auto hatIt = hats.begin();

		for ( const auto value : hardware(glfwGetJoystickHats(deviceID, &count), count, hats.size()) )
		{
			(hatIt++)->store(static_cast< JoystickHatDirection >(value), std::memory_order_relaxed);
		}
	}

	void
	JoystickController::clearDeviceState (int32_t deviceID) noexcept
	{
		if ( deviceID < 0 || deviceID >= DeviceCount )
		{
			return;
		}

		auto & [axes, buttons, hats] = *std::next(s_devicesState.begin(), deviceID);

		for ( auto & axis : axes )
		{
			axis.store(0.0F, std::memory_order_relaxed);
		}

		for ( auto & button : buttons )
		{
			button.store(false, std::memory_order_relaxed);
		}

		for ( auto & hat : hats )
		{
			hat.store(Center, std::memory_order_relaxed);
		}
	}
}
