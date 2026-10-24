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
 * https://github.com/EmeraudeEngine/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#pragma once

/* Project configuration. */
#include "emeraude_export.hpp"

/* STL inclusions. */
#include <array>
#include <atomic>
#include <cstdint>
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
	class EMEN_LEAN_API PointerController final : public ControllerInterface
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
			 * @param button The mouse button.
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
			 * @param button The mouse button.
			 * @return bool
			 */
			[[nodiscard]]
			bool isButtonReleased (MouseButton button) const noexcept;

			/**
			 * @brief Holds or releases a button by INJECTION (the console's mousePress / mouseRelease): the polled state
			 * (isButtonPressed()) sees it, and the held state survives the per-frame copy of the hardware state
			 * (readDeviceState()) until released — the KeyboardController::injectKeyState() model.
			 * @note Main thread (the console commands), like the other writers.
			 * @param button The GLFW mouse button (an out-of-range button is ignored).
			 * @param held Whether the button is held.
			 */
			static void injectButtonState (int32_t button, bool held) noexcept;

			/**
			 * @brief This function is called by the input manager to update device state.
			 * @note This must be called by the main thread.
			 * @param window A reference to the window.
			 */
			static void readDeviceState (const Window & window) noexcept;

		private:

			/* NOTE: Written on the main thread, read by the logic thread: one atomic per button (see KeyboardController). */
			static std::array< std::atomic< uint8_t >, 8 > s_deviceState;
			/** @brief The buttons held by injection, OR-ed into the hardware state after each copy. */
			static std::array< std::atomic< uint8_t >, 8 > s_injectedState;

			bool m_disabled{false};
	};
}
