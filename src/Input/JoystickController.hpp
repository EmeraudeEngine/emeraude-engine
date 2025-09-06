/*
 * src/Input/JoystickController.hpp
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

/* Local inclusions for inheritances. */
#include "ControllerInterface.hpp"

/* Local inclusions for usages. */
#include "Types.hpp"

/* Forward declarations. */
namespace EmEn
{
	class Window;
}

namespace EmEn::Input
{
	/** @brief Structure to copy the joystick state.  */
	struct JoystickState
	{
		std::array< float, 6 > axes{0.0F};
		std::array< bool, JoystickMaxButtons > buttons{false};
		std::array< JoystickHatDirection, JoystickMaxHats >hats{Center};
	};

	/**
	 * @brief The joystick controller class.
	 * @extends EmEn::Input::ControllerInterface This is an input controller.
	 */
	class JoystickController final : public ControllerInterface
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"JoystickController"};

			/**
			 * @brief Constructs a joystick controller.
			 */
			JoystickController () noexcept = default;

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
			 * @brief Returns the device id.
			 * @return int32_t
			 */
			[[nodiscard]]
			int32_t
			deviceID () const noexcept
			{
				return m_deviceID;
			}

			/**
			 * @brief Attaches the device identifier.
			 * @param deviceID The joystick identifier.
			 * @return void
			 */
			void attachDeviceID (int32_t deviceID) noexcept;

			/**
			 * @brief Detaches the device.
			 * @return void
			 */
			void
			detachDevice () noexcept
			{
				this->attachDeviceID(-1);
			}

			/**
			 * @brief Sets the axis threshold before it fires events.
			 * @param value A value.
			 * @return void
			 */
			void
			setAxisThreshold (float value) noexcept
			{
				m_threshold = std::abs(value);
			}

			/**
			 * @brief Sets the axis sensitivity with a multiplier.
			 * @param multiplier A value.
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
			 * @brief Returns the axis sensitivity multiplier.
			 * @return float
			 */
			[[nodiscard]]
			float
			axisSensitivity () const noexcept
			{
				return m_multiplier;
			}

			/**
			 * @brief axeValue
			 * @param axe
			 * @return float
			 */
			[[nodiscard]]
			float axeValue (JoystickAxis axe) const noexcept;

			/**
			 * @brief isButtonPressed
			 * @param buttonNum
			 * @return bool
			 */
			[[nodiscard]]
			bool isButtonPressed (int32_t buttonNum) const noexcept;

			/**
			 * @brief isButtonReleased
			 * @param buttonNum
			 * @return bool
			 */
			[[nodiscard]]
			bool isButtonReleased (int32_t buttonNum) const noexcept;

			/**
			 * @brief hatValue
			 * @param hatNum
			 * @return JoystickHatDirection
			 */
			[[nodiscard]]
			JoystickHatDirection hatValue (int32_t hatNum) const noexcept;

			/**
			 * @brief This function is called by the input manager to update device state.
			 * @note This must be called by the main thread.
			 * @param deviceID The joystick ID.
			 * @return void
			 */
			static void readDeviceState (int32_t deviceID) noexcept;

			/**
			 * @brief Clears the device state.
			 * @param deviceID The joystick ID.
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

			inline static std::array< JoystickState, DeviceCount > s_devicesState{};

			int32_t m_deviceID{-1};
			float m_threshold{0.15F};
			float m_multiplier{4.0F};
			bool m_disabled{false};
	};
}
