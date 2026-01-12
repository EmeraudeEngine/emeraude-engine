/*
 * src/Input/KeyboardController.hpp
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
	/**
	 * @brief The keyboard controller class.
	 * @extends EmEn::Input::ControllerInterface This is an input controller.
	 */
	class KeyboardController final : public ControllerInterface
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"KeyboardController"};

			/**
			 * @brief Construct a default keyboard controller.
			 */
			KeyboardController () noexcept = default;

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
				return true;
			}

			/** @copydoc EmEn::Input::ControllerInterface::getRawState() */
			[[nodiscard]]
			std::string getRawState () const noexcept override;

			/**
			 * @brief Returns whether a specific key is pressed.
			 * @param key The keycode.
			 * @return bool
			 */
			[[nodiscard]]
			bool isKeyPressed (int32_t key) const noexcept;

			/**
			 * @brief Returns whether any key on keyboard is pressed.
			 * @return bool
			 */
			[[nodiscard]]
			bool isAnyKeyPressed () const noexcept;

			/**
			 * @brief Returns whether a specific key is released.
			 * @param key The keycode.
			 * @return bool
			 */
			[[nodiscard]]
			bool isKeyReleased (int32_t key) const noexcept;

			/**
			 * @brief Changes a key state. This is useful for special key.
			 * @param key The keycode.
			 * @param pressed The new key state.
			 * @return bool
			 */
			static void changeKeyState (int32_t key, bool pressed) noexcept;

			/**
			 * @brief This function is called by the input manager to update device state.
			 * @note This must be called by the main thread.
			 * @param window A reference to the window.
			 */
			static void readDeviceState (const Window & window) noexcept;

			/**
			 * @brief Returns whether a modifier key is pressed.
			 * @param modKey The modifier key being tested.
			 * @param modifiers The current state of modifier keys.
			 * @return bool
			 */
			[[nodiscard]]
			static
			bool
			isModifierKeyPressed (ModKey modKey, int32_t modifiers) noexcept
			{
				return ( modifiers & modKey ) > 0;
			}

		private:

			static std::array< char, 349 > s_deviceState;

			bool m_disabled{false};
	};
}
