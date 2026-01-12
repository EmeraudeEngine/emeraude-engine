/*
 * src/Input/PointerController.hpp
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
	 * @brief The pointer controller class.
	 * @extends EmEn::Input::PointerController This is an input controller.
	 */
	class PointerController final : public ControllerInterface
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"PointerController"};

			/**
			 * @brief Constructs a default pointer.
			 */
			PointerController () noexcept = default;

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
			 * @brief Returns the state of the pointer button.
			 * @param button
			 * @return bool
			 */
			[[nodiscard]]
			bool isButtonPressed (MouseButton button) const noexcept;

			/**
			 * @brief Returns whether any button is pressed.
			 * @return bool
			 */
			[[nodiscard]]
			bool isAnyButtonPressed () const noexcept;

			/**
			 * @brief Returns the state of the pointer button.
			 * @param button
			 * @return bool
			 */
			[[nodiscard]]
			bool isButtonReleased (MouseButton button) const noexcept;

			/**
			 * @brief This function is called by the input manager to update device state.
			 * @note This must be called by the main thread.
			 * @param window A reference to the window.
			 */
			static void readDeviceState (const Window & window) noexcept;

		private:

			static std::array< char, 8 > s_deviceState;

			bool m_disabled{false};
	};
}
