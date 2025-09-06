/*
 * src/Input/GamepadController.hpp
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

#pragma once

/* STL inclusions. */
#include <cstdint>
#include <array>
#include <string>

/* Third-party libraries.
 * FIXME: Try to hide GLFW in implementation. */
#define GLFW_INCLUDE_NONE
#include "GLFW/glfw3.h"

/* Local inclusions for inheritances. */
#include "ControllerInterface.hpp"

/* Local inclusions for usages. */
#include "Types.hpp"
#include "Tracer.hpp"

/* Forward declarations. */
namespace EmEn
{
	class Window;
}

namespace EmEn::Input
{
	/**
	 * @brief The gamepad controller class.
	 * @extends EmEn::Input::ControllerInterface This is an input controller.
	 */
	class GamepadController final : public ControllerInterface
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"GamepadController"};

			/**
			 * @brief Constructs a gamepad controller.
			 */
			GamepadController () noexcept = default;

			/** @copydoc EmEn::Input::ControllerInterface::disable() */
			void
			disable (bool state) noexcept override
			{
				m_disabled = state;
			}

			/** @copydoc EmEn::Input::ControllerInterface::disabled() */
			[[nodiscard]]
			bool
			disabled () const noexcept override
			{
				return m_disabled;
			}

			/** @copydoc EmEn::Input::ControllerInterface::isConnected() */
			[[nodiscard]]
			bool
			isConnected () const noexcept override
			{
				return m_deviceID > -1 && m_deviceID <= DeviceCount;
			}

			/** @copydoc EmEn::Input::ControllerInterface::getRawState() */
			[[nodiscard]]
			std::string getRawState () const noexcept override;

			/**
			 * @brief Returns the device ID attached from GLFW.
			 * @return int32_t
			 */
			[[nodiscard]]
			int32_t
			deviceID () const noexcept
			{
				return m_deviceID;
			}

			/**
			 * @brief Attaches a device ID to this controller.
			 * @param deviceID The device ID coming from GLFW.
			 * @return void
			 */
			void
			attachDeviceID (int32_t deviceID) noexcept
			{
				if ( deviceID >= DeviceCount )
				{
					TraceError{ClassId} << "Unable to attach a device ID higher than " << DeviceCount;

					return;
				}

				m_deviceID = deviceID;
			}

			/**
			 * @brief Detaches a previous attached device.
			 * @return void
			 */
			void
			detachDevice () noexcept
			{
				m_deviceID = -1;
			}

			/**
			 * @brief Sets the axis threshold to trigger a movement.
			 * @param value The threshold value.
			 * @return void
			 */
			void
			setAxisThreshold (float value) noexcept
			{
				m_threshold = std::abs(value);
			}

			/**
			 * @brief Sets the axis sensitivity.
			 * @param multiplier A positive multiplier.
			 * @return void
			 */
			void
			setAxisSensitivity (float multiplier) noexcept
			{
				m_multiplier = std::abs(multiplier);
			}

			/**
			 * @brief Returns the axis threshold.
			 * @return float
			 */
			[[nodiscard]]
			float
			axisThreshold () const noexcept
			{
				return m_threshold;
			}

			/**
			 * @brief Returns the multiplier applied to the axis sensitivity.
			 * @return float
			 */
			[[nodiscard]]
			float
			axisSensitivity () const noexcept
			{
				return m_multiplier;
			}

			/**
			 * @brief Checks whether a device button is pressed.
			 * @param button The button enum.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isButtonPressed (GamepadButton button) const noexcept
			{
				if ( !this->usable() )
				{
					return false;
				}

				return s_devicesState[m_deviceID].buttons[button] == GLFW_PRESS;
			}

			/**
			 * @brief Checks whether a device button is released.
			 * @param button The button enum.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isButtonReleased (GamepadButton button) const noexcept
			{
				if ( !this->usable() )
				{
					return true;
				}

				return s_devicesState[m_deviceID].buttons[button] == GLFW_PRESS;
			}

			/**
			 * @brief Returns the value of an axis after apply the threshold and the sensitivity multiplier.
			 * @param axe The axis enum.
			 * @return float
			 */
			[[nodiscard]]
			float
			axeValue (GamepadAxis axe) const noexcept
			{
				if ( !this->usable() )
				{
					const auto value = s_devicesState[m_deviceID].axes[axe];

					if ( value > m_threshold || value < -m_threshold )
					{
						return value * m_multiplier;
					}
				}

				return 0.0F;
			}

			/**
			 * @brief This function is called by the input manager to update device state.
			 * @note This must be called by the main thread.
			 * @param deviceID The gamepad ID.
			 * @return void.
			 */
			static void readDeviceState (int32_t deviceID) noexcept;

			/**
			 * @brief Clears the device state.
			 * @param deviceID The gamepad ID.
			 * @return void.
			 */
			static void clearDeviceState (int32_t deviceID) noexcept;

		private:

			/**
			 * @brief Returns whether the device is usable.
			 * @return bool
			 */
			[[nodiscard]]
			bool usable () const noexcept;

			inline static std::array< GLFWgamepadstate, DeviceCount > s_devicesState{};

			int32_t m_deviceID{-1};
			float m_threshold{0.15F};
			float m_multiplier{4.0F};
			bool m_disabled{false};
	};
}
